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

    evpool*     pool;
};


grbound* grbound_new(void)
{
    grbound* grb = malloc(sizeof(*grb));

    if (!grb)
        goto fail;

    if (!(grb->bound = fsbound_new()))
        goto fail;

    grb->pool = evpool_new(DEFAULT_EVPOOL_SIZE);

    if (!grb->pool)
        goto fail;

    fsbound_init(grb->bound);

    grb->flags = FSPLACE_ROW_SMART
               | FSPLACE_LEFT_TO_RIGHT
               | FSPLACE_TOP_TO_BOTTOM
               | GRBOUND_BLOCK_ON_NOTE_FAIL;

    grb->evinput = 0;

    #ifdef GRBOUND_DEBUG
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

    evpool_free(grb->pool);
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


int grbound_channel(grbound* grb)
{
    return grb->channel;
}


int grbound_channel_set(grbound* grb, int ch)
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
        evport_write_event(port, &ev)->misc = grb;
}


struct grid
{
    event   note_on[128];
    event   note_off[128];

    evport_manager* portman;
    evport* port;  /* assigned from ports_global */

    freespace*  fs;

};


grid* grid_new(void)
{
    int i;

    grid* gr = malloc(sizeof(*gr));
    if (!gr)
        goto fail0;

    if (!(gr->portman = evport_manager_new("grid")))
        goto fail1;

    gr->port = evport_manager_evport_new(gr->portman);

    if (!gr->port)
        goto fail2;

    if (!(gr->fs = freespace_new()))
        goto fail2;

    for (i = 0; i < 128; ++i)
    {
        event_init(&gr->note_on[i]);
        event_init(&gr->note_off[i]);
    }

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


evport* grid_input_port(grid* gr)
{
    return gr->port;
}


freespace*  grid_freespace(grid* gr)
{
    return gr->fs;
}


void grid_rt_place(grid* gr)
{
    event ev;

    evport_read_reset(gr->port);

    while(evport_read_and_remove_event(gr->port, &ev))
    {
        grbound* grb = (grbound*)ev.misc;

        #ifdef GRBOUND_DEBUG
        MESSAGE("evport event.misc = grbound:%p\n", grb);
        event_dump(&ev);
        #endif

        if (freespace_find(     gr->fs,
                                grb->bound,
                                grb->flags,
                                ev.box_width,
                                ev.box_height,
                                &ev.box_x,
                                &ev.box_y ))
        {
            _Bool place = 1;
            int pitch, startpitch, endpitch, pitchdir;

            ev.note_velocity = (grb->flags & FSPLACE_TOP_TO_BOTTOM)
                                    ? ev.box_y
                                    : ev.box_y + ev.box_width;

            if (grb->flags & GRBOUND_PITCH_STRICT_POS)
            {
                if (gr->note_on[ev.box_x].flags & EV_NOTE_ON)
                {
                    if (grb->flags & GRBOUND_BLOCK_ON_NOTE_FAIL)
                        ev.flags &= ~EV_TYPE_NOTE;
                    else
                        place = 0;
                }

                ev.note_pitch = ev.box_x;
            }
            else
            {
                if (grb->flags & FSPLACE_LEFT_TO_RIGHT)
                {
                    startpitch = pitch = ev.box_x;
                    endpitch = pitch + ev.box_width;
                    pitchdir = 1;
                }
                else
                {
                    startpitch = pitch = ev.box_x + ev.box_width;
                    endpitch = ev.box_x;
                    pitchdir = -1;
                }

                while(gr->note_on[pitch].flags & EV_NOTE_ON)
                {
                    pitch += pitchdir;
                    if (pitch == endpitch)
                        break;
                }

                if (pitch == endpitch)
                {
                    if (grb->flags & GRBOUND_BLOCK_ON_NOTE_FAIL)
                        ev.flags &= ~EV_TYPE_NOTE;
                    else
                        place = 0;
                }
                ev.note_pitch = pitch;
            }

            if (place)
            {
                event_copy(&gr->note_on[ev.note_pitch], &ev);

                freespace_remove(gr->fs,    ev.box_x,
                                            ev.box_y,
                                            ev.box_width,
                                            ev.box_height );
            }
            else
            {
                #ifdef GRID_DEBUG
                WARNING("event not placed in boundary\n");
                #endif
            }
        }
        else
            WARNING("failed to place event\n");
    }
}

/*
void grbound_rt_place(evport* port, freespace* fs)
{
    evport_read_reset(port);
    event ev;

    while(evport_read_event(port, &ev))
    {
        grbound* grb = ev.misc;

        event_dump(&ev);

        if (freespace_find(     fs,
                                grb->bound,
                                grb->flags,
                                ev.box_width,
                                ev.box_height,
                                &ev.box_x,
                                &ev.box_y ))
        {
            _Bool place = 1;
            int pitch, startpitch, endpitch, pitchdir;
            event* event_on;

            ev.note_velocity = (grb->flags & FSPLACE_TOP_TO_BOTTOM)
                                    ? ev.box_y
                                    : ev.box_y + ev.box_width;

            if (!(event_on = evpool_event_alloc(grb->pool)))
            {
                #ifdef GRID_DEBUG
                WARNING("event dropped: event pool out of memory\n");
                #endif
                continue;
            }

            if (grb->flags & GRBOUND_PITCH_STRICT_POS)
            {
                if (grb->event_on[ev.box_x])
                {
                    if (grb->flags & GRBOUND_BLOCK_ON_NOTE_FAIL)
                        ev.flags &= ~EV_TYPE_NOTE;
                    else
                        place = 0;
                }

                ev.note_pitch = ev.box_x;
            }
            else
            {
                if (grb->flags & FSPLACE_LEFT_TO_RIGHT)
                {
                    startpitch = pitch = ev.box_x;
                    endpitch = pitch + ev.box_width;
                    pitchdir = 1;
                }
                else
                {
                    startpitch = pitch = ev.box_x + ev.box_width;
                    endpitch = ev.box_x;
                    pitchdir = -1;
                }

                while(grb->event_on[pitch])
                {
                    pitch += pitchdir;
                    if (pitch == endpitch)
                        break;
                }

                if (pitch == endpitch)
                {
                    if (grb->flags & GRBOUND_BLOCK_ON_NOTE_FAIL)
                        ev.flags &= ~EV_TYPE_NOTE;
                    else
                        place = 0;
                }
                ev.note_pitch = pitch;
            }

            if (place)
            {
                event_copy(event_on, &ev);
                grb->event_on[event_on->note_pitch] = event_on;
                freespace_remove(fs, event_on->box_x,
                                     event_on->box_y,
                                     event_on->box_width,
                                     event_on->box_height );
            }
            else
            {
                #ifdef GRID_DEBUG
                WARNING("event not placed in boundary\n");
                #endif
                evpool_event_free(grb->pool, event_on);
            }
        }
        else
            WARNING("failed to place event\n");
    }
}
*/
