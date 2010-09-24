#ifndef GRID_BOUNDARY_H
#define GRID_BOUNDARY_H


#ifdef __cplusplus
extern "C" {
#endif


#include "boxyseq_types.h"
#include "event_port.h"
#include "freespace_boundary.h"


enum GRID_BOUNDARY_FLAGS
{
    /* note:    these are used in addition to FREESPACE_BOUNDARY_FLAGS,
                and consequently, must not conflict with the values
                defined by FREESPACE_BOUNDARY_FLAGS.
    */
    GRBOUND_PITCH_STRICT_POS =      0x0010,
    GRBOUND_BLOCK_ON_NOTE_FAIL =    0x0020,

    GRBOUND_EVENT_PLAY =        0x0100, /* normal handling of events */
    GRBOUND_EVENT_BLOCK =       0x0200, /* handle events as blocks */
    GRBOUND_EVENT_IGNORE =      0x0400, /* ignore all events */
    GRBOUND_EVENT_TYPE_MASK =   0x0f00
};


grbound*    grbound_new(void);
grbound*    grbound_dup(const grbound*);
void        grbound_free(grbound*);

int         grbound_flags(grbound*);
void        grbound_flags_clear(grbound*);
void        grbound_flags_set(grbound*, int flags);
void        grbound_flags_unset(grbound*, int flags);

void        grbound_event_play(grbound*);
void        grbound_event_block(grbound*);
void        grbound_event_ignore(grbound*);

int         grbound_event_type(grbound*);

int         grbound_scale_key_set(grbound*, int scale_key);
int         grbound_scale_key(grbound*);
int         grbound_scale_binary_set(grbound*, int scale_bin);
int         grbound_scale_binary(grbound*);

/*
int         grbound_midi_out_port(grbound*);
void        grbound_midi_out_port_set(grbound*, int);
*/

moport*     grbound_midi_out_port(grbound*);
void        grbound_midi_out_port_set(grbound*, moport*);

int         grbound_channel(grbound*);
void        grbound_channel_set(grbound*, int);

fsbound*    grbound_fsbound(grbound*);


void        grbound_fsbound_set(grbound*, int x, int y, int w, int h);
void        grbound_fsbound_get(grbound*, int* x, int* y, int* w, int* h);

void        grbound_set_input_port(grbound*, evport*);

/*  although the grbound has it's own input port, we need to place the
    data coming in from that port into another port which contains the
    events for all the ports for all the patterns for all the grbounds!

    this essential for the events to be processed in the correct order.
*/
void        grbound_update_rt_data(const grbound*);

void        grbound_rt_sort(grbound*, evport* output);


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif
