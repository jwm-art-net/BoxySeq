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


void grbound_event_play(grbound* grb)
{
    grb->flags |= GRBOUND_EVENT_PLAY;
    grb->flags &= ~(GRBOUND_EVENT_BLOCK | GRBOUND_EVENT_IGNORE);
}

void grbound_event_block(grbound* grb)
{
    grb->flags |= GRBOUND_EVENT_BLOCK;
    grb->flags &= ~(GRBOUND_EVENT_PLAY | GRBOUND_EVENT_IGNORE);
}

void grbound_event_ignore(grbound* grb)
{
    grb->flags |= GRBOUND_EVENT_IGNORE;
    grb->flags &= ~(GRBOUND_EVENT_PLAY | GRBOUND_EVENT_BLOCK);
}

int grbound_event_type(grbound* grb)
{
    return grb->flags & GRBOUND_EVENT_TYPE_MASK;
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


void grbound_fsbound_set(grbound* grb, int x, int y, int w, int h)
{
    fsbound* fsb = grbound_fsbound(grb);

    if (!fsbound_set_coords(fsb, x, y, w, h))
    {
        WARNING("boundary x:%d y:%d w:%d h:%d out of bounds\n",
                x, y, w, h);
        WARNING("default size will be used\n");
    }
}


void grbound_fsbound_get(grbound* grb, int* x, int* y, int* w, int* h)
{
    if (x) *x = grb->bound->x;
    if (y) *y = grb->bound->y;
    if (w) *w = grb->bound->w;
    if (h) *h = grb->bound->h;
}


void grbound_set_input_port(grbound* grb, evport* port)
{
    grb->evinput = port;
}


void grbound_update_rt_data(const grbound* grb)
{
    rtdata_update(grb->rt);
}


void grbound_rt_sort(grbound* grb, evport* port)
{
    event ev;
    grbound* rtgrb = rtdata_data(grb->rt);

    if (!rtgrb)
    {
        WARNING("no grbound RT data\n");
        return;
    }

    evport_read_reset(rtgrb->evinput);

    switch (grb->flags & GRBOUND_EVENT_TYPE_MASK)
    {
    case GRBOUND_EVENT_PLAY:
        while (evport_read_event(rtgrb->evinput, &ev))
        {
            ev.grb = grb;
            EVENT_CHANNEL_SET(&ev, rtgrb->channel);
            if (!evport_write_event(port, &ev))
                WARNING("failed to write sort to port\n");
        }
        return;

    case GRBOUND_EVENT_BLOCK:
        while (evport_read_event(rtgrb->evinput, &ev))
        {
            ev.grb = grb;
            EVENT_SET_TYPE_BLOCK( &ev );
            EVENT_CHANNEL_SET(&ev, rtgrb->channel);
            if (!evport_write_event(port, &ev))
                WARNING("failed to write sort to port\n");
        }
        return;

    case GRBOUND_EVENT_IGNORE:
    default:
        while(evport_read_event(rtgrb->evinput, &ev));
            /* do nothing */
    }
}


/*  STATIC/PRIVATE implementation:
*/

static grbound* grbound_private_new(bool with_rtdata)
{
    grbound* grb = malloc(sizeof(*grb));

    if (!grb)
        goto fail0;

    if (!(grb->bound = fsbound_new()))
        goto fail1;

    if (with_rtdata)
    {
        grb->rt = rtdata_new(grb,   grbound_rtdata_get_cb,
                                    grbound_rtdata_free_cb );
        if (!grb->rt)
            goto fail2;
    }
    else
        grb->rt = 0;

    fsbound_init(grb->bound);

    grb->flags = FSPLACE_ROW_SMART
               | FSPLACE_LEFT_TO_RIGHT
               | FSPLACE_TOP_TO_BOTTOM
               | GRBOUND_BLOCK_ON_NOTE_FAIL
               | GRBOUND_EVENT_PLAY;

    grb->channel = 0;
    grb->scale_bin = binary_string_to_int("111111111111");
    grb->scale_key = 0;

    grb->evinput = 0;
    grb->midiout = 0;

    #ifdef GRID_DEBUG
    MESSAGE("grbound created:%p\n",grb);
    #endif

    return grb;

fail2:  fsbound_free(grb->bound);
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
    dest->evinput =     grb->evinput;
    dest->midiout =     grb->midiout;

    fsbound_copy(dest->bound, grb->bound);

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

