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

    if (!(bs->ui_note_on_buf =  evbuf_new(DEFAULT_EVBUF_SIZE)))
        goto fail7;

    if (!(bs->ui_note_off_buf = evbuf_new(DEFAULT_EVBUF_SIZE)))
        goto fail8;

    if (!(bs->ui_unplace_buf =  evbuf_new(DEFAULT_EVBUF_SIZE)))
        goto fail9;

    if (!(bs->ui_input_buf =    evbuf_new(DEFAULT_EVBUF_SIZE)))
        goto fail10;

    if (!(bs->ui_eventlist = evlist_new()))
        goto fail11;

    grid_set_ui_note_on_buf(bs->gr,     bs->ui_note_on_buf);
    grid_set_ui_note_off_buf(bs->gr,    bs->ui_note_off_buf);
    grid_set_ui_unplace_buf(bs->gr,     bs->ui_unplace_buf);

    bs->rt_quitting = 0;

    return bs;

fail11: evbuf_free(bs->ui_input_buf);
fail10: evbuf_free(bs->ui_unplace_buf);
fail9:  evbuf_free(bs->ui_note_off_buf);
fail8:  evbuf_free(bs->ui_note_on_buf);
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

    evbuf_free(bs->ui_input_buf);
    evbuf_free(bs->ui_unplace_buf);
    evbuf_free(bs->ui_note_off_buf);
    evbuf_free(bs->ui_note_on_buf);

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


evbuf* boxyseq_ui_note_on_buf(const boxyseq* bs)
{
    return bs->ui_note_on_buf;
}

evbuf* boxyseq_ui_note_off_buf(const boxyseq* bs)
{
    return bs->ui_note_off_buf;
}

evbuf* boxyseq_ui_unplace_buf(const boxyseq* bs)
{
    return bs->ui_unplace_buf;
}


void boxyseq_rt_init_jack_cycle(boxyseq* bs, jack_nframes_t nframes)
{
    moport_manager_rt_init_jack_cycle(bs->moports, nframes);
}


void boxyseq_rt_play(boxyseq* bs,
                     jack_nframes_t nframes,
                     _Bool repositioned,
                     bbt_t ph, bbt_t nph)
{
    event ev;
    evport* grid_port;

    if (bs->rt_quitting)
        return;

    while(evbuf_read(bs->ui_input_buf, &ev))
    {
        if (EVENT_IS_TYPE_SHUTDOWN( &ev ))
        {
            bs->rt_quitting = 1;
            boxyseq_rt_clear(bs, nframes);
            return;
        }
    }

    moport_manager_rt_play_old(bs->moports, ph, nph, bs->gr);

    grid_rt_unplace(bs->gr, ph, nph);

    evport_manager_rt_clear_all(bs->ports_pattern);
    pattern_manager_rt_play(bs->patterns, repositioned, ph, nph);

    grid_port = grid_global_input_port(bs->gr);

    grbound_manager_rt_sort(bs->grbounds, grid_port);

    grid_rt_place(bs->gr, ph, nph);

    moport_manager_rt_play_new_and_output(  bs->moports,
                                            ph,
                                            nph,
                                            nframes,
                            #ifdef NO_REAL_TIME
                            0
                            #else
                            jackdata_rt_transport_frames_per_tick(bs->jd)
                            #endif
                                                                );

    grid_rt_block(bs->gr, ph, nph);
}


void boxyseq_rt_clear(boxyseq* bs, jack_nframes_t nframes)
{
    moport_manager_rt_empty(bs->moports, bs->gr, nframes);
    grid_remove_events(bs->gr);
}


void boxyseq_shutdown(boxyseq* bs)
{
    struct timespec ts;
    event ev;

    EVENT_SET_TYPE_SHUTDOWN( &ev );

    if (!evbuf_write(bs->ui_input_buf, &ev))
        WARNING("failed to queue shutdown event\n");

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
    event evin;

    while(evbuf_read(bs->ui_unplace_buf, &evin))
    {
        lnode* ln = evlist_head(bs->ui_eventlist);

        if (EVENT_IS_TYPE_CLEAR( &evin ))
        {
            evlist_select_all(bs->ui_eventlist, 1);
            evlist_delete(bs->ui_eventlist, 1);
            ln = 0;
        }

        while (ln)
        {
            event* ev = lnode_data(ln);
            lnode* nln = lnode_next(ln);

            if (ev->box_x == evin.box_x
             && ev->box_y == evin.box_y
             && ev->box_width  == evin.box_width
             && ev->box_height == evin.box_height)
            {
                evlist_unlink_free(bs->ui_eventlist, ln);
                nln = 0;
            }

            ln = nln;
        }
        ret = 1;
    }

    while(evbuf_read(bs->ui_note_off_buf, &evin))
    {
        lnode* ln = evlist_head(bs->ui_eventlist);

        while (ln)
        {
            event* ev = lnode_data(ln);

            if (ev->box_x == evin.box_x
             && ev->box_y == evin.box_y
             && ev->box_width  == evin.box_width
             && ev->box_height == evin.box_height)
            {
                EVENT_SET_STATUS_OFF( ev );
            }

            ln = lnode_next(ln);
        }
        ret = 1;
    }

    while(evbuf_read(bs->ui_note_on_buf, &evin))
    {
        evlist_add_event_copy(bs->ui_eventlist, &evin);
        ret = 1;
    }

    return ret;
}


evlist* boxyseq_ui_event_list(boxyseq* bs)
{
    return bs->ui_eventlist;
}
