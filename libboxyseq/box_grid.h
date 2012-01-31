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

/*  grid_rt_process_placement
 *-----------------------------
 *  process events queued for placement,
 *      * search for freespace
 *      * determine if note or block
 *      * if note, send to midi out port,  else send to block port
 *      * remove freespace
 *      * send event to user-interface note-on buffer
 *                  (regardless of categorization)
 */
void        grid_rt_process_placement(grid*, bbt_t ph, bbt_t nph);

/*  grid_rt_process_blocks
 *--------------------------
 *  process block events stored in the block port
 *      * check if block duration expired
 *      * if expired, move event from block port to unplace port.
 */
void        grid_rt_process_blocks(grid*, bbt_t ph, bbt_t nph);


/*  grid_rt_process_unplacement
 *-------------------------------
 *  process events stored in unplace port
 *      * check if event has released
 *      * if released, return freespace, remove from unplace port
 *        to ui unplace port.
 */
void        grid_rt_process_unplacement(grid*, bbt_t ph, bbt_t nph);



int         grid_rt_note_off_event(grid*, event*);
int         grid_rt_unplace_event(grid*, event*);


void        grid_remove_event(grid*, event*);
void        grid_remove_events(grid*);


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif
