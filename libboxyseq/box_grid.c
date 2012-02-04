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

    evport* unplace_port;
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

    gr->intersort = evport_manager_evport_new(gr->portman,  "intersort",
                                                        RT_EVLIST_SORT_POS);

    if (!gr->intersort)
        goto fail2;

    gr->block_port = evport_manager_evport_new(gr->portman, "block",
                                                        RT_EVLIST_SORT_POS);

    if (!gr->block_port)
        goto fail2;

    gr->unplace_port = evport_manager_evport_new(gr->portman, "unplace",
                                                        RT_EVLIST_SORT_POS);

    if (!gr->unplace_port)
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


/*evport* grid_global_input_port(grid* gr)
{
    return gr->global_in_port;
}
*/

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


void grid_rt_process_intersort(grid* gr, bbt_t ph, bbt_t nph)
{
    /*  events inside the intersort must only ocurr within this cycle
        the event within the intersort is processed by pos (nb. not by
        note_dur or box_release).
    */

    event ev;

    evport_read_reset(gr->intersort);

    char buf[40];

    while(evport_read_and_remove_event(gr->intersort, &ev))
    {
        grbound* rtgrb = rtdata_data(ev.grb->rt);

        /*  do lots of shit here that all the commented out functions
         *  below did and then some more stuff on top too yeah.
         */
        event_flags_to_str(ev.flags, buf);

        if (ev.pos < ph || ev.pos > nph)
        {
            DWARNING("invalid event position:");
        }

        DMESSAGE("ph: %d ~ nph:%d\tevent %s pos: %d dur:%d rel:%d\n",
                    ph, nph, buf, ev.pos, ev.note_dur, ev.box_release);

        if (EVENT_IS_STATUS_ON( &ev ))
        {
            if (freespace_find( gr->fs,
                                &rtgrb->box,
                                rtgrb->flags,
                                ev.box.w,   ev.box.h,
                                &ev.box.x,  &ev.box.y ))
            {
                if (EVENT_IS_TYPE_NOTE( &ev ))
                {
                    ev.note_pitch =
                        moport_rt_placed_event_pitch(rtgrb->midiout,
                                                        &ev,
                                                        rtgrb->flags,
                                                        rtgrb->scale_bin,
                                                        rtgrb->scale_key );
                    if (ev.note_pitch == -1)
                    {
                        if ((rtgrb->flags & GRBOUND_BLOCK_ON_NOTE_FAIL))
                        {
                            ev.pos = ev.box_release;
                            EVENT_SET_TYPE_BLOCK( &ev );
                            /*  send to block port to maintain event until
                                it expires */
                            evport_write_event(gr->block_port, &ev);
                        }
                        else
                        {
                            if (rtgrb->flags & FSPLACE_TOP_TO_BOTTOM)
                                ev.note_velocity = ev.box.y;
                            else
                                ev.note_velocity = ev.box.y + ev.box.h;
                        }
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

                if (EVENT_IS_TYPE_NOTE( &ev ))
                {
                    if (ev.note_dur < nph)
                    {
                        ev.pos = ev.note_dur;
                        EVENT_SET_STATUS_OFF( &ev );
                        evport_write_event(gr->intersort, &ev);

                        DMESSAGE("ends this cycle!\n");

                    }
                }
                else
                {
                    if (ev.box_release < nph)
                    {
                        ev.pos = ev.box_release;
                        EVENT_SET_STATUS_OFF( &ev );
                        evport_write_event(gr->intersort, &ev);

                        DMESSAGE("ends this cycle!\n");

                    }
                }
            }
        }
        else /* EVENT_IS_STATUS_OFF( &ev ) */
        {
            if (EVENT_IS_TYPE_NOTE( &ev ))
            {
                DMESSAGE("[OFF] event is type note [OFF]\n");

                EVENT_SET_TYPE_BLOCK( &ev );
                EVENT_SET_STATUS_OFF( &ev );

                ev.pos = ev.box_release;

                if (ev.box_release < nph)
                {
                    evport_write_event(gr->intersort, &ev);
                    DMESSAGE("ends this cycle!\n");
                }
                else{DMESSAGE("released in future :-| hopefully!\n");
                    evport_write_event(gr->block_port, &ev);
                    }

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


/*
void grid_rt_process_placement(grid* gr, bbt_t ph, bbt_t nph)
{
    event ev;

    evport_read_reset(gr->global_in_port);

    while(evport_read_and_remove_event(gr->global_in_port, &ev))
    {
        grbound* rtgrb = rtdata_data(ev.grb->rt);

        if (freespace_find(     gr->fs,
                                &rtgrb->box,
                                rtgrb->flags,
                                ev.box.w,   ev.box.h,
                                &ev.box.x,  &ev.box.y ))
        {
            int pitch = ev.note_pitch = -1;

            ev.note_velocity = (rtgrb->flags & FSPLACE_TOP_TO_BOTTOM)
                                    ? ev.box.y
                                    : ev.box.y + ev.box.h;

            if (EVENT_IS_TYPE_NOTE( &ev ))
            {
                ev.note_pitch =
                pitch = moport_rt_placed_event_pitch(rtgrb->midiout,
                                                    &ev,
                                                    rtgrb->flags,
                                                    rtgrb->scale_bin,
                                                    rtgrb->scale_key );
                if (pitch == -1)
                {
                    if (!(rtgrb->flags & GRBOUND_BLOCK_ON_NOTE_FAIL))
                        continue;

                    EVENT_SET_TYPE_BLOCK( &ev );
                }
            }

            //  event status is a bit hit and miss until now...

            EVENT_SET_STATUS_ON( &ev );

            if (EVENT_IS_TYPE_BLOCK( &ev ))
                evport_write_event(gr->block_port, &ev);

            freespace_remove(gr->fs,    ev.box.x,
                                        ev.box.y,
                                        ev.box.w,
                                        ev.box.h  );

            evbuf_write(gr->ui_note_on_buf, &ev);
        }
    }
}
*/

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
        #ifdef GRID_DEBUG
        if (!EVENT_IS_STATUS_ON( &ev ))
            WARNING("event in block port %s has incorrect status\n",
                    evport_name(gr->block_port));
        #endif

/*        if (ev.box_release >= nph)
            break;

        if (ev.box_release >= ph)
*/
/*        if (ev.pos >= nph)
            break;*/

if (!EVENT_IS_TYPE_BLOCK( &ev ))
{
    DMESSAGE("\nnon-block in block-port :-(\n");
}

        if (ev.pos >= ph && ev.pos < nph)
        {
            EVENT_SET_STATUS_OFF( &ev );
            DMESSAGE("set block off pos:%d dur:%d rel:%d\n",
                        ev.pos,ev.note_dur,ev.box_release);
            evport_write_event(gr->intersort, &ev);
            evport_and_remove_event(gr->block_port);
        }
    }
}


int grid_rt_note_off_event(grid* gr, event* ev)
{
    evbuf_write(gr->ui_note_off_buf, ev);
    return evport_write_event(gr->unplace_port, ev);
}


int grid_rt_unplace_event(grid* gr, event* ev)
{
    return evport_write_event(gr->unplace_port, ev);
}


/*
void grid_rt_process_unplacement(grid* gr, bbt_t ph, bbt_t nph)
{
    event ev;

    evport_read_reset(gr->unplace_port);

    bool done = 0;

    while(evport_read_event(gr->unplace_port, &ev))
    {
        if (ev.box_release > nph)
            done = 1;// break;

        if (ev.box_release < nph)
        {
            freespace_add(gr->fs,   ev.box.x,   ev.box.y,
                                    ev.box.w,   ev.box.h );

            evport_and_remove_event(gr->unplace_port);
            evbuf_write(gr->ui_unplace_buf, &ev);

            if (done)
                WARNING("unplace port not sorted!*****************\n");
        }
    }
}
*/

bool grid_rt_add_block_area(grid* gr, int x, int y, int w, int h)
{
    if (x < 0 || y < 0 || w < 1 || h < 1)
        return false;


    return freespace_block_remove(gr->fs, x, y, w, h);
}


void grid_remove_event(grid* gr, event* ev)
{
    evport_write_event(gr->unplace_port, ev);
}


void grid_remove_events(grid* gr)
{
    event ev;

    evport_clear_data(gr->intersort);

    evport_read_reset(gr->unplace_port);

    while(evport_read_and_remove_event(gr->unplace_port, &ev))
    {
        freespace_add(gr->fs,   ev.box.x,   ev.box.y,
                                ev.box.w,   ev.box.h );
    }

    evport_read_reset(gr->block_port);

    /* FIXME:   at some point we'll need to distinguish between blocks
                which have been placed by the user, and blocks which
                *became* blocks because the event could not be played
                as a MIDI event.
    */

    while(evport_read_and_remove_event(gr->block_port, &ev))
    {
        freespace_add(gr->fs,   ev.box.x,   ev.box.y,
                                ev.box.w,   ev.box.h );
    }

    evbuf_reset(gr->ui_note_on_buf);
    evbuf_reset(gr->ui_note_off_buf);
    evbuf_reset(gr->ui_unplace_buf);

    /* signal (primitively) to UI to remove everything... */

    EVENT_SET_TYPE_CLEAR( &ev );
    evbuf_write(gr->ui_unplace_buf, &ev);
}

