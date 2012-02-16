#ifndef BOX_GRID_H
#define BOX_GRID_H


#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <jack/jack.h>

#include "boxyseq_types.h"
#include "common.h"
#include "event_buffer.h"
#include "event_port.h"
#include "freespace_state.h"


grid*       grid_new(void);
void        grid_free(grid*);

/* grid intersort: port for collecting all events occurring this cycle */
evport*     grid_get_intersort(grid*);

freespace*  grid_get_freespace(grid*);

/*  buffers read by user-interface for representation of events */
void        grid_set_ui_note_on_buf(grid*, evbuf*);
void        grid_set_ui_note_off_buf(grid*, evbuf*);
void        grid_set_ui_unplace_buf(grid*, evbuf*);

/*void        grid_rt_process_intersort(grid*, bbt_t ph, bbt_t nph);*/

void        grid_rt_process_intersort(grid*,    bbt_t ph,
                                                bbt_t nph,
                                                jack_nframes_t nframes,
                                                double frames_per_tick);

void        grid_rt_flush_intersort(    grid* gr,   bbt_t ph,
                                                    bbt_t nph,
                                                    jack_nframes_t nframes,
                                                    double frames_per_tick);

/*  grid_rt_process_blocks
 *--------------------------
 *  process block events stored in the block port
 *      * check if block duration expired
 *      * if expired, move event from block port to unplace port.
 */
void        grid_rt_process_blocks(grid*, bbt_t ph, bbt_t nph);
void        grid_rt_flush_blocks_to_intersort(grid*);

/*  grid_rt_add_block_area
 *--------------------------
 *  a block-area is an area in the freespace grid which disrupts placement.
 *  the location of a block-area is chosen by the user not the placement
 *  algorithm.
 *  returns 1 on sucess.
 */
bool        grid_rt_add_block_area(grid*, int x, int y, int w, int h);
bool        grid_rt_add_block_area_event(grid*, event*);

void        grid_dump_block_events(grid*);



#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif
