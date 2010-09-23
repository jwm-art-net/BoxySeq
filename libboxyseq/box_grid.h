#ifndef BOX_GRID_H
#define BOX_GRID_H


#ifdef __cplusplus
extern "C" {
#endif


#include "boxyseq_types.h"
#include "event_buffer.h"
#include "event_port.h"
#include "freespace_state.h"


grid*       grid_new(void);
void        grid_free(grid*);

evport*     grid_global_input_port(grid*);
freespace*  grid_freespace(grid*);

void        grid_set_ui_note_on_buf(grid*, evbuf*);
void        grid_set_ui_note_off_buf(grid*, evbuf*);
void        grid_set_ui_unplace_buf(grid*, evbuf*);

void        grid_rt_place(grid*, bbt_t ph, bbt_t nph);
void        grid_rt_block(grid*, bbt_t ph, bbt_t nph);
void        grid_rt_unplace(grid*, bbt_t ph, bbt_t nph);


int         grid_rt_note_off_event(grid*, event*);
int         grid_rt_unplace_event(grid*, event*);


void        grid_remove_event(grid*, event*);
void        grid_remove_events(grid*);


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif
