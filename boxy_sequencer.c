#include "boxy_sequencer.h"

#include "debug.h"
#include "event_port.h"
#include "midi_out_port.h"
#include "pattern.h"


#include <stdlib.h>
#include <string.h>
#include <time.h>


struct boxy_sequencer
{
    char* basename;

    pattern*    pattern_slot[MAX_PATTERN_SLOTS];
    grbound*    grbound_slot[MAX_GRBOUND_SLOTS];
    moport*     moport_slot[MAX_MOPORT_SLOTS];

    evport_manager* ports_pattern;
    evport_manager* ports_midi_out;

    grid*       gr;

    evbuf*      ui_note_on_buf;
    evbuf*      ui_note_off_buf;
    evbuf*      ui_unplace_buf;

    evbuf*      ui_input_buf;

    evlist*     ui_eventlist; /* stores collect events from buffers */

    jack_client_t*  client;
    jackdata*       jd;

    _Bool quitting;
};


boxyseq* boxyseq_new(int argc, char** argv)
{
    boxyseq* bs = malloc(sizeof(*bs));

    if (!bs)
        goto fail0;

    bs->basename = 0;
    char* tmp = strrchr( argv[0], '/' );
    if (!(bs->basename = strdup( tmp ? tmp + 1: argv[0])))
        goto fail1;

    int i;

    for (i = 0; i < MAX_PATTERN_SLOTS; ++i)
        bs->pattern_slot[i] = 0;

    for (i = 0; i < MAX_GRBOUND_SLOTS; ++i)
        bs->grbound_slot[i] = 0;

    for (i = 0; i < MAX_MOPORT_SLOTS; ++i)
        bs->moport_slot[i] = 0;

    if (!(bs->ports_pattern =   evport_manager_new("pattern")))
        goto fail2;

    if (!(bs->ports_midi_out =  evport_manager_new("midi_out")))
        goto fail3;

    if (!(bs->gr = grid_new()))
        goto fail4;

    if (!(bs->ui_note_on_buf =  evbuf_new(DEFAULT_EVBUF_SIZE)))
        goto fail5;

    if (!(bs->ui_note_off_buf = evbuf_new(DEFAULT_EVBUF_SIZE)))
        goto fail6;

    if (!(bs->ui_unplace_buf =  evbuf_new(DEFAULT_EVBUF_SIZE)))
        goto fail7;

    if (!(bs->ui_input_buf =    evbuf_new(DEFAULT_EVBUF_SIZE)))
        goto fail8;

    if (!(bs->ui_eventlist = evlist_new()))
        goto fail9;

    grid_set_ui_note_on_buf(bs->gr,     bs->ui_note_on_buf);
    grid_set_ui_note_off_buf(bs->gr,    bs->ui_note_off_buf);
    grid_set_ui_unplace_buf(bs->gr,     bs->ui_unplace_buf);

    bs->quitting = 0;

    return bs;


fail9:  evbuf_free(bs->ui_input_buf);
fail8:  evbuf_free(bs->ui_unplace_buf);
fail7:  evbuf_free(bs->ui_note_off_buf);
fail6:  evbuf_free(bs->ui_note_on_buf);
fail5:  grid_free(bs->gr);
fail4:  evport_manager_free(bs->ports_midi_out);
fail3:  evport_manager_free(bs->ports_pattern);
fail2:  free(bs->basename);
fail1:  free(bs);
fail0:

    WARNING("out of memory allocating boxyseq data\n");
    return 0;
}


void boxyseq_free(boxyseq* bs)
{
    int i;

    if (!bs)
        return;

    evlist_free(bs->ui_eventlist);

    evbuf_free(bs->ui_input_buf);
    evbuf_free(bs->ui_unplace_buf);
    evbuf_free(bs->ui_note_off_buf);
    evbuf_free(bs->ui_note_on_buf);

    grid_free(bs->gr);

    evport_manager_free(bs->ports_midi_out);
    evport_manager_free(bs->ports_pattern);

    for (i = 0; i < MAX_GRBOUND_SLOTS; ++i)
        grbound_free(bs->grbound_slot[i]);

    for (i = 0; i < MAX_PATTERN_SLOTS; ++i)
        pattern_free(bs->pattern_slot[i]);

    for (i = 0; i < MAX_MOPORT_SLOTS; ++i)
        moport_free(bs->moport_slot[i]);

    free(bs->basename);
    free(bs);
}


const char* boxyseq_basename(const boxyseq* bs)
{
    return bs->basename;
}


void boxyseq_set_jackdata(boxyseq* bs, jackdata* jd)
{
    bs->jd = jd;
    bs->client = jackdata_client(jd);
}


jackdata* boxyseq_jackdata(boxyseq* bs)
{
    return bs->jd;
}


evport_manager* boxyseq_pattern_ports(boxyseq* bs)
{
    return bs->ports_pattern;
}



int boxyseq_pattern_new(boxyseq* bs,
                        float beats_per_bar,
                        float beat_type     )
{
    int slot = 0;

    for (slot = 0; slot < MAX_PATTERN_SLOTS; ++slot)
        if (!bs->pattern_slot[slot])
            break;

    if (slot == MAX_PATTERN_SLOTS)
    {
        WARNING("out of pattern slots\n");
        return -1;
    }

    pattern* pat;

    if (!(bs->pattern_slot[slot] = pat = pattern_new()))
        return -1;

    pattern_set_meter(pat, beats_per_bar, beat_type);
    pattern_set_loop_length(
                pat, pattern_duration_bbt_to_ticks(pat, 1, 0, 0  ));

    return slot;
}


void boxyseq_pattern_free(boxyseq* bs, int slot)
{
    if (slot < 0 || slot >= MAX_PATTERN_SLOTS)
    {
        WARNING("slot value %d is out of range\n", slot);
        return;
    }

    pattern_free(bs->pattern_slot[slot]);
    bs->pattern_slot[slot] = 0;
}


pattern* boxyseq_pattern_dup(boxyseq* bs, int dest_slot, int src_slot)
{
    if (dest_slot == src_slot)
        return bs->pattern_slot[src_slot];

    if (!bs->pattern_slot[src_slot])
        return 0;

    pattern_free(bs->pattern_slot[dest_slot]);
    bs->pattern_slot[dest_slot] = pattern_dup(bs->pattern_slot[src_slot]);

    return bs->pattern_slot[dest_slot];
}


pattern* boxyseq_pattern(boxyseq* bs, int slot)
{
    if (slot < 0 || slot >= MAX_PATTERN_SLOTS)
    {
        WARNING("out of range pattern slot: %d\n", slot);
        return 0;
    }

    return bs->pattern_slot[slot];
}


int boxyseq_grbound_new(boxyseq* bs, int x, int y, int w, int h)
{
    int slot = 0;
    for (slot = 0; slot < MAX_GRBOUND_SLOTS; ++slot)
        if (!bs->grbound_slot[slot])
            break;

    if (slot == MAX_GRBOUND_SLOTS)
    {
        WARNING("out of grbound slots\n");
        return -1;
    }

    grbound* grb;

    if (!(grb = grbound_new()))
        return -1;

    fsbound* fsb = grbound_fsbound(grb);

    if (!fsbound_set_coords(fsb, x, y, w, h))
    {
        WARNING("boundary x:%d y:%d w:%d h:%d out of bounds\n",
                x, y, w, h);
        WARNING("default size will be used\n");
    }

    bs->grbound_slot[slot] = grb;

    return slot;
}


void boxyseq_grbound_free(boxyseq* bs, int slot)
{
    grbound_free(bs->grbound_slot[slot]);
    bs->grbound_slot[slot] = 0;
}


grbound* boxyseq_grbound(boxyseq* bs, int slot)
{
    return bs->grbound_slot[slot];
}


int boxyseq_moport_new(boxyseq* bs)
{
    int slot = 0;

    for (slot = 0; slot < MAX_MOPORT_SLOTS; ++slot)
        if (!bs->moport_slot[slot])
            break;

    if (slot == MAX_MOPORT_SLOTS)
    {
        WARNING("out of midi out port slots\n");
        return -1;
    }

    bs->moport_slot[slot] = moport_new(bs->client, bs->ports_midi_out);

    if (!bs->moport_slot[slot])
        return -1;

    return slot;

}


void boxyseq_moport_free(boxyseq* bs, int slot)
{
    if (slot < 0 || slot >= MAX_MOPORT_SLOTS)
    {
        WARNING("slot value %d is out of range\n", slot);
        return;
    }

    moport_free(bs->moport_slot[slot]);
    bs->moport_slot[slot] = 0;
}


moport* boxyseq_moport(boxyseq* bs, int slot)
{
    return bs->moport_slot[slot];
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
    int i;

    for (i = 0; i < MAX_MOPORT_SLOTS; ++i)
        if (bs->moport_slot[i])
            moport_rt_init_jack_cycle(bs->moport_slot[i], nframes);
}


void boxyseq_rt_play(boxyseq* bs,
                     jack_nframes_t nframes,
                     _Bool repositioned,
                     bbt_t ph, bbt_t nph)
{
    int i;
    event ev;
    evport* grid_port;

    if (bs->quitting)
        return;

    while(evbuf_read(bs->ui_input_buf, &ev))
    {
        if (EVENT_IS_TYPE_SHUTDOWN( &ev ))
        {
            bs->quitting = 1;
            boxyseq_rt_clear(bs, nframes);
            return;
        }
    }

    for (i = 0; i < MAX_MOPORT_SLOTS; ++i)
    {
        if (bs->moport_slot[i])
            moport_rt_play_old(bs->moport_slot[i], ph, nph, bs->gr);
    }

    grid_rt_unplace(bs->gr, ph, nph);

    evport_manager_all_evport_clear_data(bs->ports_pattern);

    for (i = 0; i < MAX_PATTERN_SLOTS; ++i)
    {
        if (bs->pattern_slot[i])
            pattern_rt_play(bs->pattern_slot[i], repositioned, ph, nph);
    }

    grid_port = grid_global_input_port(bs->gr);

    for (i = 0; i < MAX_GRBOUND_SLOTS; ++i)
    {
        if (bs->grbound_slot[i])
            grbound_rt_sort(bs->grbound_slot[i], grid_port);
    }

    grid_rt_place(bs->gr, ph, nph);

    for (i = 0; i < MAX_MOPORT_SLOTS; ++i)
    {
        if (bs->moport_slot[i])
        {
            moport_rt_play_new(bs->moport_slot[i], ph, nph);

            moport_rt_output_jack_midi
                (
                    bs->moport_slot[i],
                    nframes,
                    #ifdef NO_REAL_TIME
                    0
                    #else
                    jackdata_rt_transport_frames_per_tick(bs->jd)
                    #endif
                );
        }
    }

    grid_rt_block(bs->gr, ph, nph);
}


void boxyseq_rt_clear(boxyseq* bs, jack_nframes_t nframes)
{
    int i = 0;

    for (i = 0; i < MAX_MOPORT_SLOTS; ++i)
        if (bs->moport_slot[i])
            moport_empty(bs->moport_slot[i], bs->gr, nframes);

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
