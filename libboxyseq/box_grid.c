#include "box_grid.h"


#include "debug.h"
#include "event_port.h"
#include "grid_boundary.h"
#include "midi_out_port.h"
#include "real_time_data.h"


#include <stdlib.h>


#include "include/grid_boundary_data.h"


struct box_grid
{
    evport_manager* portman;

    evport* intersort;

     /*  evport* global_in_port;all events passing through this
                                grid are passed through this port
                                for the purposes of sorting them
                                into an appropriate order for
                                eventual MIDI output.
                            */
    evport* block_port;

    evbuf*     ui_note_on_buf;
    evbuf*     ui_note_off_buf;
    evbuf*     ui_unplace_buf;

    freespace*  fs;
};


/*  NOTE:   the box_grid data structure does not need any seperate
            RT/UI representations. That is, there are no user-modifiable
            contents in the box_grid.

    In other words, the rtdata data structure and accompanying functions
    are not required here.
*/


grid* grid_new(void)
{
    grid* gr = malloc(sizeof(*gr));

    if (!gr)
        goto fail0;

    if (!(gr->portman = evport_manager_new("grid")))
        goto fail1;

    gr->intersort = evport_manager_evport_new(  gr->portman,
                                                "intersort",
                                                RT_EVLIST_SORT_POS);
    if (!gr->intersort)
        goto fail2;

    gr->block_port = evport_manager_evport_new( gr->portman,
                                                "block",
                                                RT_EVLIST_SORT_POS);
    if (!gr->block_port)
        goto fail2;

    if (!(gr->fs = freespace_new()))
        goto fail2;

    gr->ui_note_on_buf = 0;
    gr->ui_note_off_buf = 0;
    gr->ui_unplace_buf = 0;

    return gr;

fail2:
    evport_manager_free(gr->portman);
fail1:
    free(gr);
fail0:
    WARNING("out of memory for new grid\n");
    return 0;
}


void grid_free(grid* gr)
{
    if (!gr)
        return;

    freespace_free(gr->fs);
    evport_manager_free(gr->portman);
    free(gr);
}


evport* grid_get_intersort(grid* gr)
{
    return gr->intersort;
}


freespace*  grid_get_freespace(grid* gr)
{
    return gr->fs;
}


void grid_set_ui_note_on_buf(grid* gr, evbuf* evb)
{
    gr->ui_note_on_buf = evb;
}

void grid_set_ui_note_off_buf(grid* gr, evbuf* evb)
{
    gr->ui_note_off_buf = evb;
}

void grid_set_ui_unplace_buf(grid* gr, evbuf* evb)
{
    gr->ui_unplace_buf = evb;
}


void grid_rt_flush_intersort(grid* gr, bbt_t ph, bbt_t nph,
                                    jack_nframes_t nframes,
                                    double frames_per_tick)
{
    /*  events inside the intersort must only occur within this cycle
        the event within the intersort is processed by pos (nb. not by
        note_dur or box_release).
    */

    event ev;

    evport_read_reset(gr->intersort);

    while(evport_read_and_remove_event(gr->intersort, &ev))
    {
        grbound* rtgrb = rtdata_data(ev.grb->rt);

        #ifndef NDEBUG
        if (ev.pos < ph || ev.pos > nph)
            DWARNING("invalid event position\n");
        /*
        event_flags_to_str(ev.flags, buf);
        DMESSAGE("ph~nph: %6d ~ %6d\t[%s] pos: %d dur:%d rel:%d"
                 "x:%d y:%d w:%d h:%d\n",
                    ph, nph, buf, ev.pos, ev.note_dur, ev.box_release,
                    ev.box.x, ev.box.y, ev.box.w, ev.box.h);
        */
        #endif

        if (EVENT_IS_TYPE( &ev, EV_TYPE_NOTE ))
        {
            EVENT_SET_TYPE( &ev, EV_TYPE_BLOCK );
            EVENT_SET_STATUS_OFF( &ev );

            moport_rt_output_jack_midi_event(rtgrb->midiout, &ev,
                                             ph, nframes,
                                             frames_per_tick);
            ev.pos = ev.box_release;

            if (ev.box_release < nph)
            {
                evport_write_event(gr->intersort, &ev);
                DMESSAGE("block ends this cycle!\n");
                event_dump(&ev);
            }
            else
                evport_write_event(gr->block_port, &ev);

            evbuf_write(gr->ui_note_off_buf, &ev);
        }
        else
        {
            freespace_add(gr->fs,   ev.box.x,   ev.box.y,
                                    ev.box.w,   ev.box.h );

            evbuf_write(gr->ui_unplace_buf, &ev);
        }
    }
}


void grid_rt_process_intersort(grid* gr, bbt_t ph, bbt_t nph,
                                    jack_nframes_t nframes,
                                    double frames_per_tick)
{
    /*  events inside the intersort must only ocurr within this cycle
        the event within the intersort is processed by pos (nb. not by
        note_dur or box_release).
    */

    event ev;

    evport_read_reset(gr->intersort);

    while(evport_read_and_remove_event(gr->intersort, &ev))
    {
        grbound* rtgrb = rtdata_data(ev.grb->rt);

        #ifndef NDEBUG
        if (ev.pos < ph || ev.pos > nph)
            DWARNING("invalid event position\n");
/*
        event_flags_to_str(ev.flags, buf);
        DMESSAGE("ph~nph: %6d ~ %6d\t[%s] pos: %d dur:%d rel:%d\n",
                    ph, nph, buf, ev.pos, ev.note_dur, ev.box_release);
*/
        #endif

        if (EVENT_IS_STATUS_ON( &ev ))
        {
            #ifndef NDEBUG
            if (ph == 0 && nph == 4)
            {
                DWARNING("intersort should not be "
                         "adding events right now!\n");
            }
            #endif

            if (freespace_find( gr->fs,
                                &rtgrb->box,
                                rtgrb->flags,
                                ev.box.w,   ev.box.h,
                                &ev.box.x,  &ev.box.y ))
            {
                if (EVENT_IS_TYPE( &ev, EV_TYPE_NOTE ))
                {
                    /* must set velocity before "pushing for pitch" */
                    if (rtgrb->flags & FSPLACE_TOP_TO_BOTTOM)
                        ev.note_velocity = ev.box.y;
                    else
                        ev.note_velocity = ev.box.y + ev.box.h;

                    ev.note_pitch =
                            moport_rt_push_event_pitch(rtgrb->midiout,
                                                        &ev,
                                                        rtgrb->flags,
                                                        rtgrb->scale_bin,
                                                        rtgrb->scale_key );
                    if (ev.note_pitch == -1)
                    {
                        if ((rtgrb->flags & GRBOUND_BLOCK_ON_NOTE_FAIL))
                        {
                            ev.pos = ev.box_release;
                            EVENT_SET_TYPE( &ev, EV_TYPE_BLOCK );
                            /*  send to block port to maintain event until
                                it expires */
                            evport_write_event(gr->block_port, &ev);
                        }
                    }
                    else
                    {
                        moport_rt_output_jack_midi_event(rtgrb->midiout,
                                                         &ev,
                                                         ph, nframes,
                                                         frames_per_tick);
                    }
                }
                else
                {
                    ev.pos = ev.box_release;
                    evport_write_event(gr->block_port, &ev);
                }

                freespace_remove(gr->fs,    ev.box.x, ev.box.y,
                                            ev.box.w, ev.box.h );
                evbuf_write(gr->ui_note_on_buf, &ev);

                /* check for events which end aswell as begin this cycle */
                if (EVENT_IS_TYPE( &ev, EV_TYPE_NOTE ))
                {
                    if (ev.note_dur < nph)
                    {
                        ev.pos = ev.note_dur;
                        EVENT_SET_STATUS_OFF( &ev );
                        evport_write_event(gr->intersort, &ev);
                        DMESSAGE("note ends this cycle!\n");
                    }
                }
                else
                {
                    if (ev.box_release < nph)
                    {
                        ev.pos = ev.box_release;
                        EVENT_SET_STATUS_OFF( &ev );
                        evport_write_event(gr->intersort, &ev);
                        DMESSAGE("block ends this cycle!\n");
                    }
                }
            }
        }
        else /* EVENT_IS_STATUS_OFF( &ev ) */
        {
            if (EVENT_IS_TYPE( &ev, EV_TYPE_NOTE ))
            {
                EVENT_SET_TYPE( &ev, EV_TYPE_BLOCK );
                EVENT_SET_STATUS_OFF( &ev );

                moport_rt_output_jack_midi_event(rtgrb->midiout, &ev,
                                                 ph, nframes,
                                                 frames_per_tick);
                ev.pos = ev.box_release;

                if (ev.box_release < nph)
                {
                    evport_write_event(gr->intersort, &ev);
                    DMESSAGE("block ends this cycle!\n");
                    event_dump(&ev);
                }
                else
                    evport_write_event(gr->block_port, &ev);

                evbuf_write(gr->ui_note_off_buf, &ev);
            }
            else
            {
                freespace_add(gr->fs,   ev.box.x,   ev.box.y,
                                        ev.box.w,   ev.box.h );

                evbuf_write(gr->ui_unplace_buf, &ev);
            }
        }
    }
}


void grid_rt_process_blocks(grid* gr, bbt_t ph, bbt_t nph)
{
/*
        purpose: process events within block_port,
        these are events which don't emit any
        MIDI messages within their lifetime, but are still
        placed and unplaced as blocks within the grid.

        they might be events which failed to achieve status
        as notes due to lack of free space or a pitch conflict.
        (or, when implemented, they might be events coming into
        the grid via its MIDI input port).

        the usage of this port differs slightly in that
        the events it contains stay in the port for the
        duration of the note (hmmm yeah, ummm...)
*/
    event ev;

    evport_read_reset(gr->block_port);

    while(evport_read_event(gr->block_port, &ev))
    {
        /*#ifndef NDEBUG
        EVENT_IS(&ev, EV_STATUS_OFF | EV_TYPE_BLOCK);
        #endif*/

        if (ev.pos >= ph && ev.pos < nph)
        {
            EVENT_SET_STATUS_OFF( &ev );
            evport_write_event(gr->intersort, &ev);
            evport_and_remove_event(gr->block_port);
        }
    }
}


void grid_rt_flush_blocks_to_intersort(grid* gr)
{
    event ev;
    int count = 0;

    DMESSAGE("flushing blocks to intersort...\n");

    evport_read_reset(gr->block_port);

    while(evport_read_and_remove_event(gr->block_port, &ev))
    {
        ev.pos = 0;
        ev.note_dur = 1;
        ev.box_release = 2;
        EVENT_SET_STATUS_OFF( &ev );
        evport_write_event(gr->intersort, &ev);
        count++;
    }

    DMESSAGE("%d blocks flushed to intersort\n", count);
}


bool grid_rt_add_block_area(grid* gr, int x, int y, int w, int h)
{
    if (x < 0 || y < 0 || w < 1 || h < 1)
        return false;

    bool ret = freespace_block_remove(gr->fs, x, y, w, h);

    if (ret)
    {
        DWARNING("BROKEN HERE!!!\n");
        event ev;
        EVENT_SET_STATUS_ON( &ev );
        EVENT_SET_TYPE( &ev, EV_TYPE_BLOCK );
        ev.box.x = x;
        ev.box.y = y;
        ev.box.w = w;
        ev.box.h = h;
        evport_write_event(gr->block_port, &ev);
    }

    return ret;
}


bool grid_rt_add_block_area_event(grid* gr, event* ev)
{
    return false;
}


void grid_dump_block_events(grid* gr)
{
    event ev;

    DMESSAGE("grid block-events dump...\n");

    evport_read_reset(gr->block_port);

    while(evport_read_event(gr->block_port, &ev))
    {
        event_dump(&ev);
    }
}
