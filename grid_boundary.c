#include "grid_boundary.h"


#include "debug.h"
#include "event_pool.h"


#include <stdlib.h>


struct grid_boundary
{
    int         flags;
    int         channel;

    evport*     evinput;
    fsbound*    bound;

    moport*     midiout;

};


grbound* grbound_new(void)
{
    grbound* grb = malloc(sizeof(*grb));

    if (!grb)
        goto fail;

    if (!(grb->bound = fsbound_new()))
        goto fail;

    fsbound_init(grb->bound);

    grb->flags = FSPLACE_ROW_SMART
               | FSPLACE_LEFT_TO_RIGHT
               | FSPLACE_TOP_TO_BOTTOM;
/*
               | GRBOUND_BLOCK_ON_NOTE_FAIL;
*/

    grb->evinput = 0;
    grb->midiout = 0;

    #ifdef GRID_DEBUG
    MESSAGE("grbound created:%p\n",grb);
    #endif

    return grb;

fail:
    fsbound_free(grb->bound);
    free(grb);
    WARNING("out of memory for grid boundary\n");
    return 0;
}


void grbound_free(grbound* grb)
{
    if (!grb)
        return;

    fsbound_free(grb->bound);
    free(grb);
}


int grbound_flags(grbound* grb)
{
    return grb->flags;
}


void grbound_flags_clear(grbound* grb)
{
    grb->flags = 0;
}


void grbound_flags_set(grbound* grb, int flags)
{
    grb->flags |= flags;
}


void grbound_flags_unset(grbound* grb, int flags)
{
    grb->flags &= ~flags;
}


moport* grbound_midi_out_port(grbound* grb)
{
    return grb->midiout;
}


void grbound_midi_out_port_set(grbound* grb, moport* mo)
{
    grb->midiout = mo;
}


int grbound_channel(grbound* grb)
{
    return grb->channel;
}


void grbound_channel_set(grbound* grb, int ch)
{
    grb->channel = ch;
}


fsbound* grbound_fsbound(grbound* grb)
{
    return grb->bound;
}


void grbound_set_input_port(grbound* grb, evport* port)
{
    grb->evinput = port;
}


void grbound_rt_sort(grbound* grb, evport* port)
{
    event ev;
    event* out;

    evport_read_reset(grb->evinput);

    while (evport_read_event(grb->evinput, &ev))
    {
        out = evport_write_event(port, &ev);
        out->misc = grb;
        event_channel_set(out, grb->channel);
    }
}


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

    freespace*  fs;
};


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


void grid_rt_place(grid* gr, bbt_t ph, bbt_t nph)
{
    event ev;

    evport_read_reset(gr->global_in_port);

    while(evport_read_and_remove_event(gr->global_in_port, &ev))
    {
        grbound* grb = (grbound*)ev.misc;

        if (freespace_find(     gr->fs,
                                grb->bound,
                                grb->flags,
                                ev.box_width,
                                ev.box_height,
                                &ev.box_x,
                                &ev.box_y ))
        {
            int pitch;

            ev.note_velocity = (grb->flags & FSPLACE_TOP_TO_BOTTOM)
                                    ? ev.box_y
                                    : ev.box_y + ev.box_height;

            ev.note_pitch =
                    pitch = moport_start_event( grb->midiout, &ev,
                                                grb->flags );

            if (pitch == -1)
            {
                if (!(grb->flags & GRBOUND_BLOCK_ON_NOTE_FAIL))
                    continue;

                ev.flags = EV_TYPE_BLOCK | EV_STATUS_PLAY;

                evport_write_event(gr->block_port, &ev);
            }

            freespace_remove(gr->fs,    ev.box_x,
                                        ev.box_y,
                                        ev.box_width,
                                        ev.box_height );
        #ifndef GRID_DEBUG
        }
        #else /* GRID_DEBUG */
            if (!moport_event_in_start(grb->midiout, &ev))
                WARNING("event not in midiport start\n");
        }
        else
        {
            WARNING("event (%d x %d) NOT placed in boundary: "
                    "no space available (is ok (-: )\n",
                    ev.box_width, ev.box_height );
            #ifdef NO_REAL_TIME
            freespace_dump(gr->fs);
            #endif
        }
        #endif /* GRID_DEBUG */

    }
}


void grid_rt_block(grid* gr, bbt_t ph, bbt_t nph)
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

    event* out;

    evport_read_reset(gr->block_port);

    while(evport_read_event(gr->block_port, &ev))
    {
        #ifdef GRID_DEBUG
        int status = ev.flags & EV_STATUSMASK;
        if (status != EV_STATUS_PLAY)
            WARNING("event in block port %s has incorrect status\n",
                    evport_name(gr->block_port));
        #endif

        if (nph < ev.note_dur)
            break;

        if (ev.note_dur >= ph)
        {
            out = grid_rt_unplace_event(gr, &ev);
            evport_and_remove_event(gr->block_port);
        }
    }
}


event* grid_rt_unplace_event(grid* gr, event* ev)
{
/*
    if (ev->box_release == 0)
    {WARNING("!!!!!!!!!!!\n");}
    {
        freespace_add(gr->fs,   ev->box_x,      ev->box_y,
                                ev->box_width,  ev->box_height );
        return 0;
    }
*/
    return evport_write_event(gr->unplace_port, ev);
}


void grid_rt_unplace(grid* gr, bbt_t ph, bbt_t nph)
{
    event ev;

    evport_read_reset(gr->unplace_port);

    _Bool done = 0;

    while(evport_read_event(gr->unplace_port, &ev))
    {
        if (ev.box_release > nph)
            done = 1;/*break;*/

        if (ev.box_release < nph)
        {
            freespace_add(gr->fs,   ev.box_x,       ev.box_y,
                                    ev.box_width,   ev.box_height );

            evport_and_remove_event(gr->unplace_port);
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

    evport_read_reset(gr->unplace_port);

    while(evport_read_event(gr->unplace_port, &ev))
    {
        freespace_add(gr->fs,   ev.box_x,       ev.box_y,
                                ev.box_width,   ev.box_height );
    }
}
