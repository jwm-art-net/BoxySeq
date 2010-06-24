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
               | FSPLACE_TOP_TO_BOTTOM
               | GRBOUND_BLOCK_ON_NOTE_FAIL;

    grb->evinput = 0;
    grb->midiout = 0;

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

    evport* unplace_port;

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

    gr->port =
        evport_manager_evport_new(gr->portman,  RT_EVLIST_SORT_POS);

    if (!gr->port)
        goto fail2;

    gr->unplace_port =
        evport_manager_evport_new(gr->portman,  RT_EVLIST_SORT_REL);

    if (!gr->unplace_port)
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


evport* grid_unplace_port(grid* gr)
{
    return gr->unplace_port;
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
            int pitch;

            ev.note_velocity = (grb->flags & FSPLACE_TOP_TO_BOTTOM)
                                    ? ev.box_y
                                    : ev.box_y + ev.box_height;

            ev.note_pitch =
                    pitch = moport_output(grb->midiout, &ev, grb->flags);

            if (pitch == -1 && (grb->flags & GRBOUND_BLOCK_ON_NOTE_FAIL))
            {
                ++pitch;
                #ifdef GRID_DEBUG
                WARNING("event should have been added"
                        "to blocklist but was not\n");
                #endif
                /*  add event to blockers list or whatever it will be...
                    the event will be placed just like a note, however,
                    it will not send any midi message.
                */
            }

            if (pitch > -1)
            {
                freespace_remove(gr->fs,    ev.box_x,
                                            ev.box_y,
                                            ev.box_width,
                                            ev.box_height );
            }
            else
            {
                #ifdef GRID_DEBUG
                WARNING("event not placed in boundary: "
                        "output pitch conflict (is ok (-: )\n");
                #endif
            }
        }
        else
            WARNING("event not placed in boundary: "
                    "no space available (is ok (-: )\n");
    }
}


void grid_rt_unplace(grid* gr, bbt_t ph, bbt_t nph)
{
    event ev;

/*
    event_init(&ev);

    MESSAGE("ph:%8d  |  nph:%8d\n", ph, nph);

    MESSAGE("unplace port has %d events\n", 
            evport_count(gr->unplace_port));
*/
    evport_read_reset(gr->unplace_port);

    while(evport_read_event(gr->unplace_port, &ev))
    {
/*
        printf("---{");
        event_dump(&ev);
        printf("}---\n");
*/
        if (ph >= ev.box_release)
        {

            freespace_add(gr->fs,   ev.box_x,       ev.box_y,
                                    ev.box_width,   ev.box_height );

           evport_and_remove_event(gr->unplace_port);
        }
        else
            break;
    }
/*
    printf("###|");
    event_dump(&ev);
    printf("|###\n");
*/
}
