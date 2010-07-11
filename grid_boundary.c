#include "grid_boundary.h"

#include "common.h"
#include "debug.h"
#include "event_pool.h"


#include <stdlib.h>


struct grid_boundary
{
    int         flags;
    int         channel;
    int         scale_bin;
    int         scale_key;

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
               | FSPLACE_TOP_TO_BOTTOM
               | GRBOUND_BLOCK_ON_NOTE_FAIL;

    grb->scale_bin = binary_string_to_int("111111111111");
    grb->scale_key = 0;
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


int grbound_scale_key_set(grbound* grb, int scale_key)
{
    return (grb->scale_key = scale_key);
}


int grbound_scale_key(grbound* grb)
{
    return grb->scale_key;
}


int grbound_scale_binary_set(grbound* grb, int scale_bin)
{
    return (grb->scale_bin = scale_bin);
}


int grbound_scale_binary(grbound* grb)
{
    return grb->scale_bin;
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

    evport_read_reset(grb->evinput);

    while (evport_read_event(grb->evinput, &ev))
    {
        ev.misc = grb;
        EVENT_CHANNEL_SET(&ev, grb->channel);
        if (!evport_write_event(port, &ev))
            WARNING("failed to write sort to port\n");
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

    evbuf*     gui_note_on_buf;
    evbuf*     gui_note_off_buf;
    evbuf*     gui_unplace_buf;

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

    gr->gui_note_on_buf = 0;
    gr->gui_note_off_buf = 0;
    gr->gui_unplace_buf = 0;

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


void grid_set_gui_note_on_buf(grid* gr, evbuf* evb)
{
    gr->gui_note_on_buf = evb;
}

void grid_set_gui_note_off_buf(grid* gr, evbuf* evb)
{
    gr->gui_note_off_buf = evb;
}

void grid_set_gui_unplace_buf(grid* gr, evbuf* evb)
{
    gr->gui_unplace_buf = evb;
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
            int pitch = ev.note_pitch = -1;

            ev.note_velocity = (grb->flags & FSPLACE_TOP_TO_BOTTOM)
                                    ? ev.box_y
                                    : ev.box_y + ev.box_height;

            if (EVENT_IS_TYPE_NOTE( &ev ))
            {
                ev.note_pitch =
                    pitch = moport_start_event( grb->midiout, &ev,
                                                grb->flags,
                                                grb->scale_bin,
                                                grb->scale_key );
                if (pitch == -1)
                {
                    if (!(grb->flags & GRBOUND_BLOCK_ON_NOTE_FAIL))
                        continue;

                    EVENT_SET_TYPE_BLOCK( &ev );
                }
            }

            if (EVENT_IS_TYPE_BLOCK( &ev ))
            {
                EVENT_SET_STATUS_ON( &ev );
                evport_write_event(gr->block_port, &ev);
            }

            freespace_remove(gr->fs,    ev.box_x,
                                        ev.box_y,
                                        ev.box_width,
                                        ev.box_height );

            evbuf_write(gr->gui_note_on_buf, &ev);
        }
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


int grid_rt_unplace_event(grid* gr, event* ev)
{
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
            evbuf_write(gr->gui_unplace_buf, &ev);

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
