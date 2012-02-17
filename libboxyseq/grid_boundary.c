#include "grid_boundary.h"

#include "common.h"
#include "debug.h"
#include "freespace_state.h"
#include "real_time_data.h"


#include <stdlib.h>


#include "include/grid_boundary_data.h"
#include "include/freespace_boundary_data.h"


static grbound* grbound_private_new(bool with_rtdata);
static grbound* grbound_private_dup(const void* grb, bool with_rtdata);

static void*    grbound_rtdata_get_cb(const void* grb);
static void     grbound_rtdata_free_cb(void* grb);


grbound* grbound_new(void)
{
    return grbound_private_new(1);
}


grbound* grbound_dup(const grbound* grb)
{
    return grbound_private_dup(grb, 1);
}


void grbound_free(grbound* grb)
{
    if (!grb)
        return;

    rtdata_free(grb->rt);
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


void grbound_flags_toggle(grbound* grb, int flags)
{
    grb->flags ^= flags;
}

void grbound_event_process(grbound* grb)
{
    grb->flags |= GRBOUND_EVENT_PROCESS;
}

void grbound_event_ignore(grbound* grb)
{
    grb->flags &= ~GRBOUND_EVENT_PROCESS;
}

void grbound_event_process_and_play(grbound* grb)
{
    grb->flags |= GRBOUND_EVENT_PROCESS;
    grb->flags |= GRBOUND_EVENT_PLAY;
}

void grbound_event_process_and_block(grbound* grb)
{
    grb->flags |= GRBOUND_EVENT_PROCESS;
    grb->flags &= ~GRBOUND_EVENT_PLAY;
}

int grbound_event_type(grbound* grb)
{
    return grb->flags & GRBOUND_EVENT_TYPE_MASK;
}


int grbound_event_toggle_process(grbound* grb)
{
    return (grb->flags ^= GRBOUND_EVENT_PROCESS) & GRBOUND_EVENT_PROCESS;
}


int grbound_event_toggle_play(grbound* grb)
{
    return (grb->flags ^= GRBOUND_EVENT_PLAY) & GRBOUND_EVENT_PLAY;
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


void grbound_rgb_float_get(grbound* grb, float* r, float* g, float* b)
{
    *r = grb->box.r / 255.0f;
    *g = grb->box.g / 255.0f;
    *b = grb->box.b / 255.0f;
}


void grbound_rgb_float_set(grbound* grb, float r, float g, float b)
{
    grb->box.r = (int)(r * 255);
    grb->box.g = (int)(g * 255);
    grb->box.b = (int)(b * 255);
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


bool grbound_fsbound_set(grbound* grb, int x, int y, int w, int h)
{
    return box_set_coords(&grb->box, x, y, w, h);
}


void grbound_fsbound_get(grbound* grb, int* x, int* y, int* w, int* h)
{
    if (x) *x = grb->box.x;
    if (y) *y = grb->box.y;
    if (w) *w = grb->box.w;
    if (h) *h = grb->box.h;
}


void grbound_set_input_port(grbound* grb, evport* port)
{
    grb->evinput = port;
}


void grbound_update_rt_data(const grbound* grb)
{
    rtdata_update(grb->rt);
}


void grbound_rt_pull_starting(grbound* grb, evport* grid_intersort)
{
    event ev;
    grbound* rtgrb = rtdata_data(grb->rt);

    if (!rtgrb)
    {
        WARNING("no grbound RT data\n");
        return;
    }

    evport_read_reset(rtgrb->evinput);

    if (grb->flags & GRBOUND_EVENT_PROCESS)
    {
        if (grb->flags & GRBOUND_EVENT_PLAY)
        {
            while (evport_read_event(rtgrb->evinput, &ev))
            {
                ev.grb = grb;

                if ((!ev.box.r && !ev.box.g && !ev.box.b)
                 || (grb->flags & GRBOUND_OVERRIDE_NOTE_CH))
                {
                    ev.box.r = grb->box.r;
                    ev.box.g = grb->box.g;
                    ev.box.b = grb->box.b;
                }

                EVENT_SET_STATUS_ON( &ev );

                if (grb->flags & GRBOUND_OVERRIDE_NOTE_CH)
                    EVENT_SET_CHANNEL( &ev, rtgrb->channel );

                if (!evport_write_event(grid_intersort, &ev))
                    WARNING("failed to write to grid intersort\n");
            }
        }
        else
        {
            while (evport_read_event(rtgrb->evinput, &ev))
            {
                ev.grb = grb;

                if ((!ev.box.r && !ev.box.g && !ev.box.b)
                 || (grb->flags & GRBOUND_OVERRIDE_NOTE_CH))
                {
                    ev.box.r = grb->box.r;
                    ev.box.g = grb->box.g;
                    ev.box.b = grb->box.b;
                }

                EVENT_SET_STATUS_ON( &ev );
                EVENT_SET_TYPE( &ev, EV_TYPE_BLOCK );

                if (!evport_write_event(grid_intersort, &ev))
                    WARNING("failed to write to grid intersort\n");
            }
        }
    }
    else
    {
        while(evport_read_event(rtgrb->evinput, &ev))
            /* do nothing */;
    }

    while(evport_read_event(rtgrb->evinput, &ev))
    {
        DWARNING("incoming not emptied!\n");
        event_dump(&ev);
    }

}


void grbound_rt_empty_incoming(grbound* grb)
{
    event ev;
    grbound* rtgrb;
    int count = 0;

    if (!grb->rt)
    {
        DWARNING("bollocks\n");
        return;
    }

    rtgrb = rtdata_data(grb->rt);

    if (!rtgrb)
    {
        WARNING("no grbound RT data\n");
        return;
    }

    evport_read_reset(rtgrb->evinput);

    DMESSAGE("emptying incoming...\n");

    while(evport_read_and_remove_event(rtgrb->evinput, &ev))
    {
        event_dump(&ev);
        count++;
    }


    DMESSAGE("emptied %d events from incoming\n", count);
}

#ifndef NDEBUG
void grbound_rt_check_incoming(grbound*grb, bbt_t ph, bbt_t nph)
{
    event ev;
    grbound* rtgrb;

    DMESSAGE("checking incoming...\n");

    rtgrb = rtdata_data(grb->rt);

    evport_read_reset(rtgrb->evinput);

    while(evport_read_event(rtgrb->evinput, &ev))
    {
        if (ev.pos < ph || ev.pos > nph)
        {
            DWARNING("out of place event! now ph:%d nph:%d\n", ph, nph);
            event_dump(&ev);
        }
    }
}
#endif

/*  STATIC/PRIVATE implementation:
*/

static grbound* grbound_private_new(bool with_rtdata)
{
    grbound* grb = malloc(sizeof(*grb));

    if (!grb)
        goto fail0;

    if (with_rtdata)
    {
        grb->rt = rtdata_new(grb,   grbound_rtdata_get_cb,
                                    grbound_rtdata_free_cb );
        if (!grb->rt)
            goto fail1;
    }
    else
        grb->rt = 0;

    box_init_max_dim(&grb->box);

    grb->flags =  GRBOUND_BLOCK_ON_NOTE_FAIL
                | GRBOUND_EVENT_PROCESS
                | GRBOUND_EVENT_PLAY;

    if (rand() % 2)
        grb->flags |= FSPLACE_ROW_SMART;

    if (rand() % 2)
        grb->flags |= FSPLACE_LEFT_TO_RIGHT;

    if (rand() % 2)
        grb->flags |= FSPLACE_TOP_TO_BOTTOM;

    grb->channel = 0;
    grb->scale_bin = binary_string_to_int("111111111111");
    grb->scale_key = 0;

    random_rgb(&grb->box.r, &grb->box.g, &grb->box.b);

    grb->evinput = 0;
    grb->midiout = 0;

    #ifdef GRID_DEBUG
    MESSAGE("grbound created:%p\n",grb);
    #endif

    return grb;

fail1:  free(grb);
fail0:  WARNING("out of memory for grid boundary\n");
    return 0;
}


static grbound* grbound_private_dup(const void* data, bool with_rtdata)
{
    const grbound* grb = data;
    grbound* dest = grbound_private_new(with_rtdata);

    if (!dest)
        return 0;

    dest->flags =       grb->flags;
    dest->channel =     grb->channel;
    dest->scale_bin =   grb->scale_bin;
    dest->scale_key =   grb->scale_key;

    box_copy(&dest->box, &grb->box);

    dest->evinput =     grb->evinput;
    dest->midiout =     grb->midiout;

    return dest;
}


static void* grbound_rtdata_get_cb(const void* grb)
{
    return grbound_private_dup(grb, 0);
}


static void grbound_rtdata_free_cb(void* grb)
{
    grbound_free(grb);
}


void grbound_dump_events(grbound* grb)
{
    DMESSAGE("grbound:%p dump\n", grb);
    DMESSAGE("incoming\n");
    evport_dump(grb->evinput);
}

