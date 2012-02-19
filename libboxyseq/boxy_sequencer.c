#include "boxy_sequencer.h"

#include "box_grid.h"
#include "debug.h"


#include <stdlib.h>
#include <string.h>
#include <time.h>


#include "include/boxy_sequencer_data.h"


boxyseq* boxyseq_new(int argc, char** argv)
{
    char* tmp;
    boxyseq* bs = malloc(sizeof(*bs));

    if (!bs)
        goto fail0;

    bs->basename = 0;
    tmp = strrchr( argv[0], '/' );

    if (!(bs->basename =        strdup( tmp ? tmp + 1: argv[0])))
        goto fail1;

    if (!(bs->patterns =        pattern_manager_new()))
        goto fail2;

    if (!(bs->grbounds =        grbound_manager_new()))
        goto fail3;

    if (!(bs->moports =         moport_manager_new()))
        goto fail4;

    if (!(bs->ports_pattern =   evport_manager_new("pattern")))
        goto fail5;

    if (!(bs->gr = grid_new()))
        goto fail6;

    bs->ui_note_on_buf = jack_ringbuffer_create(DEFAULT_EVBUF_SIZE
                                                        * sizeof(event));
    if (!bs->ui_note_on_buf)
        goto fail7;

    bs->ui_note_off_buf = jack_ringbuffer_create(DEFAULT_EVBUF_SIZE
                                                        * sizeof(event));
    if (!bs->ui_note_off_buf)
        goto fail8;

    bs->ui_unplace_buf = jack_ringbuffer_create(DEFAULT_EVBUF_SIZE
                                                        * sizeof(event));
    if (!bs->ui_unplace_buf)
        goto fail9;

    bs->ui_input_buf = jack_ringbuffer_create(DEFAULT_EVBUF_SIZE
                                                        * sizeof(event));
    if (!bs->ui_input_buf)
        goto fail10;

    /*
    if (!jack_ringbuffer_mlock(bs->ui_note_on_buf)
     || !jack_ringbuffer_mlock(bs->ui_note_off_buf)
     || !jack_ringbuffer_mlock(bs->ui_unplace_buf)
     || !jack_ringbuffer_mlock(bs->ui_input_buf))
    {
        goto fail11;
    }
    */
    if (!(bs->ui_eventlist = evlist_new()))
        goto fail11;

    grid_set_ui_note_on_buf(bs->gr,     bs->ui_note_on_buf);
    grid_set_ui_note_off_buf(bs->gr,    bs->ui_note_off_buf);
    grid_set_ui_unplace_buf(bs->gr,     bs->ui_unplace_buf);

    bs->rt_quitting = 0;

    return bs;

fail11: jack_ringbuffer_free(bs->ui_input_buf);
fail10: jack_ringbuffer_free(bs->ui_unplace_buf);
fail9:  jack_ringbuffer_free(bs->ui_note_off_buf);
fail8:  jack_ringbuffer_free(bs->ui_note_on_buf);
fail7:  grid_free(bs->gr);
fail6:  evport_manager_free(bs->ports_pattern);
fail5:  moport_manager_free(bs->moports);
fail4:  grbound_manager_free(bs->grbounds);
fail3:  pattern_manager_free(bs->patterns);
fail2:  free(bs->basename);
fail1:  free(bs);
fail0:  WARNING("out of memory allocating boxyseq data\n");
    return 0;
}


void boxyseq_free(boxyseq* bs)
{
    if (!bs)
        return;

    evlist_free(bs->ui_eventlist);

    jack_ringbuffer_free(bs->ui_input_buf);
    jack_ringbuffer_free(bs->ui_unplace_buf);
    jack_ringbuffer_free(bs->ui_note_off_buf);
    jack_ringbuffer_free(bs->ui_note_on_buf);

    grid_free(bs->gr);

    evport_manager_free(bs->ports_pattern);
    moport_manager_free(bs->moports);
    grbound_manager_free(bs->grbounds);
    pattern_manager_free(bs->patterns);

    free(bs->basename);
    free(bs);
}


const char* boxyseq_basename(const boxyseq* bs)
{
    return bs->basename;
}


jackdata* boxyseq_jackdata(boxyseq* bs)
{
    return bs->jd;
}


void boxyseq_set_jackdata(boxyseq* bs, jackdata* jd)
{
    bs->jd = jd;
    moport_manager_jack_client_set(bs->moports, jackdata_client(jd));
}


evport_manager* boxyseq_pattern_port_manager(boxyseq* bs)
{
    return bs->ports_pattern;
}


pattern_manager* boxyseq_pattern_manager(boxyseq*bs)
{
    return bs->patterns;
}


grbound_manager* boxyseq_grbound_manager(boxyseq*bs)
{
    return bs->grbounds;
}


moport_manager* boxyseq_moport_manager(boxyseq*bs)
{
    return bs->moports;
}


jack_ringbuffer_t* boxyseq_ui_note_on_buf(const boxyseq* bs)
{
    return bs->ui_note_on_buf;
}

jack_ringbuffer_t* boxyseq_ui_note_off_buf(const boxyseq* bs)
{
    return bs->ui_note_off_buf;
}

jack_ringbuffer_t* boxyseq_ui_unplace_buf(const boxyseq* bs)
{
    return bs->ui_unplace_buf;
}


void boxyseq_ui_place_static_block( const boxyseq* bs,
                                    int x,      int y,
                                    int width,  int height)
{
    event ev;

    event_init(&ev);

    ev.box.x = x;
    ev.box.y = y;
    ev.box.w = width;
    ev.box.h = height;

    EVENT_SET_TYPE( &ev, EV_TYPE_STATIC );
    EVENT_SET_STATUS_ON( &ev );

/*    if (!evbuf_write(bs->ui_input_buf, &ev))*/
        WARNING("unable to create user block\n");
}


void boxyseq_rt_init_jack_cycle(boxyseq* bs, jack_nframes_t nframes)
{
    moport_manager_rt_init_jack_cycle(bs->moports, nframes);
}


void boxyseq_rt_play(boxyseq* bs,
                     jack_nframes_t nframes,
                     bool repositioned,
                     bbt_t ph, bbt_t nph)
{
    evport* intersort;

    if (bs->rt_quitting)
        return;

    while (jack_ringbuffer_read_space(bs->ui_input_buf) >= sizeof(event))
    {
        event ev;

        jack_ringbuffer_read(bs->ui_input_buf, (char*)&ev, sizeof(ev));

        switch(EVENT_GET_TYPE( &ev ))
        {
        case EV_TYPE_SHUTDOWN:
            DMESSAGE("RT shutdown...\n");
            bs->rt_quitting = 1;
            boxyseq_rt_clear(bs, 0, 0, nframes);
            return;

        case EV_TYPE_STATIC:
            if (!grid_rt_add_block_area(bs->gr, ev.box.x, ev.box.y,
                                                ev.box.w, ev.box.h))
            {
                WARNING("failed to add block-area\n");
            }
            else
            {
                MESSAGE("placed block-area:x:%d, y:%d, w:%d, h:%d\n", 
                        ev.box.x, ev.box.y, ev.box.w, ev.box.h);
            }
            break;

        default:
            WARNING("unknown user event\n");
        }
    }

    intersort = grid_get_intersort(bs->gr);
    moport_manager_rt_pull_ending(bs->moports, ph, nph, intersort);
    evport_manager_rt_clear_all(bs->ports_pattern);
    pattern_manager_rt_play(bs->patterns, repositioned, ph, nph);
    grbound_manager_rt_pull_starting(bs->grbounds, intersort);
    grid_rt_process_blocks(bs->gr, ph, nph);
    grid_rt_process_intersort(bs->gr, ph, nph, nframes,
                        jackdata_rt_transport_frames_per_tick(bs->jd));
}


void boxyseq_rt_clear(boxyseq* bs, bbt_t ph, bbt_t nph,
                                    jack_nframes_t nframes)
{
    event ev;
    size_t sz;

    DMESSAGE("clearing... ph:%d nph:%d\n", ph, nph);

    evport* intersort = grid_get_intersort(bs->gr);
    evport_manager_rt_clear_all(bs->ports_pattern);
    moport_manager_rt_pull_playing_and_empty(bs->moports, 0, 4, intersort);
    grbound_manager_rt_empty_incoming(bs->grbounds);
    grid_rt_flush_blocks_to_intersort(bs->gr);
    grid_rt_flush_intersort(bs->gr, 0, 4, nframes,
                            jackdata_rt_transport_frames_per_tick(bs->jd));

    EVENT_SET_TYPE(&ev, EV_TYPE_CLEAR);
    sz = jack_ringbuffer_write(bs->ui_unplace_buf, (char*)&ev, sizeof(ev));

    if (sz != sizeof(ev))
    {
        DWARNING("failed to queue clear-event\n");
    }
}


void boxyseq_shutdown(boxyseq* bs)
{
    struct timespec ts;
    event ev;
    size_t sz;

    EVENT_SET_TYPE( &ev, EV_TYPE_SHUTDOWN );
    sz = jack_ringbuffer_write(bs->ui_input_buf, (char*)&ev, sizeof(ev));

    if (sz != sizeof(ev))
    {
        WARNING("failed to queue shutdown-event\n");
    }

    /*  wait for RT thread to discover what we've done...
        ...by waiting for a tenth of a second.
    */
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000;
    nanosleep(&ts, 0);
}


int boxyseq_ui_collect_events(boxyseq* bs)
{
    int ret = 0;
    int dump = 0;

    while (jack_ringbuffer_read_space(bs->ui_unplace_buf) >= sizeof(event))
    {
        lnode* ln;
        event evin;

        jack_ringbuffer_read(bs->ui_unplace_buf, (char*)&evin,
                                                  sizeof(evin));

        if (EVENT_IS_TYPE( &evin, EV_TYPE_CLEAR ))
        {
            DWARNING("\nui eventlist delete events..\n\n");
            dump = 1;
            continue;
        }

  //      if (dump)
            event_dump(&evin);

        ln = evlist_head(bs->ui_eventlist);

        while (ln)
        {
            event* ev = lnode_data(ln);
            lnode* nln = lnode_next(ln);

            if (ev->box.x == evin.box.x
             && ev->box.y == evin.box.y
             && ev->box.w == evin.box.w
             && ev->box.h == evin.box.h)
            {
                evlist_unlink_free(bs->ui_eventlist, ln);
                nln = 0;
            }

            ln = nln;
        }
        ret = 1;
    }

    if (dump)
        DMESSAGE("------ read ui note off buf\n");

    while (jack_ringbuffer_read_space(bs->ui_note_off_buf) >= sizeof(event))
    {
        lnode* ln;
        event evin;

        jack_ringbuffer_read(bs->ui_note_off_buf, (char*)&evin,
                                                   sizeof(evin));
        ln = evlist_head(bs->ui_eventlist);

//        if (dump)
            event_dump(&evin);

        while (ln)
        {
            event* ev = lnode_data(ln);

            if (ev->box.x == evin.box.x
             && ev->box.y == evin.box.y
             && ev->box.w == evin.box.w
             && ev->box.h == evin.box.h)
            {
                EVENT_SET_STATUS_OFF( ev );
            }

            ln = lnode_next(ln);
        }
        ret = 1;
    }

    if (dump)
        DMESSAGE("------ read ui note on buf\n");

    while (jack_ringbuffer_read_space(bs->ui_note_on_buf) >= sizeof(event))
    {
        event evin;

        jack_ringbuffer_read(bs->ui_note_on_buf, (char*)&evin,
                                                  sizeof(evin));

//        if (dump)
            event_dump(&evin);

        evlist_add_event_copy(bs->ui_eventlist, &evin);
        ret = 1;
    }

    return ret;
}


evlist* boxyseq_ui_event_list(boxyseq* bs)
{
    return bs->ui_eventlist;
}
