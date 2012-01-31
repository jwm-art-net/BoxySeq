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

    evport* global_in_port; /*  all events passing through this
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

    gr->global_in_port =
        evport_manager_evport_new(gr->portman,  RT_EVLIST_SORT_POS);

    if (!gr->global_in_port)
        goto fail2;

    gr->block_port =
        evport_manager_evport_new(gr->portman,  RT_EVLIST_SORT_DUR);

    if (!gr->block_port)
        goto fail2;

    gr->unplace_port =
        evport_manager_evport_new(gr->portman,  RT_EVLIST_SORT_REL);

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


evport* grid_global_input_port(grid* gr)
{
    return gr->global_in_port;
}


freespace*  grid_freespace(grid* gr)
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




void grid_rt_process_placement(grid* gr, bbt_t ph, bbt_t nph)
{
    event ev;

    evport_read_reset(gr->global_in_port);

    while(evport_read_and_remove_event(gr->global_in_port, &ev))
    {
        grbound* rtgrb = rtdata_data(ev.grb->rt);

        if (freespace_find(     gr->fs,
                                rtgrb->bound,
                                rtgrb->flags,
                                ev.box_width,
                                ev.box_height,
                                &ev.box_x,
                                &ev.box_y ))
        {
            int pitch = ev.note_pitch = -1;

            ev.note_velocity = (rtgrb->flags & FSPLACE_TOP_TO_BOTTOM)
                                    ? ev.box_y
                                    : ev.box_y + ev.box_height;

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

            /*  event status is a bit hit and miss until now...
            */
            EVENT_SET_STATUS_ON( &ev );

            if (EVENT_IS_TYPE_BLOCK( &ev ))
                evport_write_event(gr->block_port, &ev);

            freespace_remove(gr->fs,    ev.box_x,
                                        ev.box_y,
                                        ev.box_width,
                                        ev.box_height );

            evbuf_write(gr->ui_note_on_buf, &ev);
        }
    }
}


void grid_rt_process_blocks(grid* gr, bbt_t ph, bbt_t nph)
{
    /*  purpose: process events within block_port,
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

        if (ev.note_dur == -1)
            printf("...\n");

        if (nph < ev.note_dur)
            break;

        if (ev.note_dur >= ph)
        {   /* unplace might fail, but... */
            grid_rt_unplace_event(gr, &ev);
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


void grid_rt_process_unplacement(grid* gr, bbt_t ph, bbt_t nph)
{
    event ev;

    evport_read_reset(gr->unplace_port);

    bool done = 0;

    while(evport_read_event(gr->unplace_port, &ev))
    {
        if (ev.box_release > nph)
            done = 1;/*break;*/

        if (ev.box_release < nph)
        {
            freespace_add(gr->fs,   ev.box_x,       ev.box_y,
                                    ev.box_width,   ev.box_height );

            evport_and_remove_event(gr->unplace_port);
            evbuf_write(gr->ui_unplace_buf, &ev);

            if (done)
                WARNING("unplace port not sorted!*****************\n");
        }

    }
}


void grid_remove_event(grid* gr, event* ev)
{
    evport_write_event(gr->unplace_port, ev);
}


void grid_remove_events(grid* gr)
{
    event ev;

    evport_clear_data(gr->global_in_port);

    evport_read_reset(gr->unplace_port);

    while(evport_read_and_remove_event(gr->unplace_port, &ev))
    {
        freespace_add(gr->fs,   ev.box_x,       ev.box_y,
                                ev.box_width,   ev.box_height );
    }

    evport_read_reset(gr->block_port);

    /* FIXME:   at some point we'll need to distinguish between blocks
                which have been placed by the user, and blocks which
                *became* blocks because the event could not be played
                as a MIDI event.
    */

    while(evport_read_and_remove_event(gr->block_port, &ev))
    {
        freespace_add(gr->fs,   ev.box_x,       ev.box_y,
                                ev.box_width,   ev.box_height );
    }

    evbuf_reset(gr->ui_note_on_buf);
    evbuf_reset(gr->ui_note_off_buf);
    evbuf_reset(gr->ui_unplace_buf);

    /* signal (primitively) to UI to remove everything... */

    EVENT_SET_TYPE_CLEAR( &ev );
    evbuf_write(gr->ui_unplace_buf, &ev);
}

