#ifndef GRID_BOUNDARY_H
#define GRID_BOUNDARY_H

#include "boxyseq.h"
#include "event_port.h"
#include "freespace_boundary.h"
#include "freespace_state.h"
#include "midi_out_port.h"


enum GRID_BOUNDARY_FLAGS
{
    /* note:    these are used in addition to FREESPACE_BOUNDARY_FLAGS,
                and consequently, must not conflict with the values
                defined by FREESPACE_BOUNDARY_FLAGS.
    */
    GRBOUND_PITCH_STRICT_POS =      0x0010,
    GRBOUND_BLOCK_ON_NOTE_FAIL =    0x0020
};


typedef struct grid_boundary grbound;


grbound*    grbound_new(void);
void        grbound_free(grbound*);

int         grbound_flags(grbound*);
void        grbound_flags_clear(grbound*);
void        grbound_flags_set(grbound*, int flags);
void        grbound_flags_unset(grbound*, int flags);

moport*     grbound_midi_out_port(grbound*);
void        grbound_midi_out_port_set(grbound*, moport*);

int         grbound_channel(grbound*);
void        grbound_channel_set(grbound*, int);

fsbound*    grbound_fsbound(grbound*);

void        grbound_set_input_port(grbound*, evport*);

void        grbound_set_midi_channel(grbound*, int);
int         grbound_get_midi_channel(grbound*);


/*  although the grbound has it's own input port, we need to place the
    data coming in from that port into another port which contains the
    events for all the ports for all the patterns for all the grbounds!

    this essential for the events to be processed in the correct order.
*/

void        grbound_rt_sort(grbound*, evport* output);



grid*       grid_new(void);
void        grid_free(grid*);

evport*     grid_input_port(grid*);
freespace*  grid_freespace(grid*);

void        grid_rt_place(grid*, bbt_t ph, bbt_t nph);
void        grid_rt_block(grid*, bbt_t ph, bbt_t nph);
void        grid_rt_unplace(grid*, bbt_t ph, bbt_t nph);

void        grid_rt_unplace_event(grid*, event*);

#endif
