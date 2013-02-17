#ifndef FREESPACE_STATE_H
#define FREESPACE_STATE_H


#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>


#define FSWIDTH  128 /* YOU CHANGE YOU BREAK! */
#define FSHEIGHT 128

#define MAX_BLOCK_AREAS 32 /* max block areas which can be used */


enum FREESPACE_PLACEMENT_FLAGS
{
    FSPLACE_ROW_SMART =         0x0001,     /* else COL_SMART       */
    FSPLACE_LEFT_TO_RIGHT =     0x0002,     /* else RIGHT_TO_LEFT   */
    FSPLACE_TOP_TO_BOTTOM =     0x0004,     /* else BOTTOM_TO_TOP   */
};


typedef struct freespace_state freespace;


freespace*  freespace_new(void);
void        freespace_free(freespace*);

void        freespace_clear(freespace*);


/*  the freespace state
 *-----------------------
 *  the freespace state is a grid of coordinates where each coordinate
 *  can be thought of as the smallest unit of area. each unit of area
 *  can be in one of two states: used or unused (occupied or free etc).
 *
 *  the default state of the grid is as unused space. the grid is used
 *  for performing fast searches to find an area of free space amongst
 *  areas of used space.
 *
 *  as implied by the limited number of possible states of space in the
 *  grid (ie used or unused) areas of used space cannot overlap each other.
 *
 *  additionally, tracking of the individual areas removed is not the
 *  responsibility of the freespace state.
 *
 *  except: freespace_block_[clear|add|remove]. these functions do provide
 *  some form of tracking for block-areas. block-areas are areas which when
 *  placed might overlap existing areas of used space in the grid, but when
 *  those areas are removed they should not wipe out the block-areas.
 *  likewise, when the block-areas are removed, they should not wipe out
 *  each other or any areas removed that were pre-existing.
 *
 *  the number of block-areas that can be tracked is limited to
 *  MAX_BLOCK_AREAS.
 *
 *  all freespace_* functions without exception should not be shared amongst
 *  threads.
 *
 *  with the exception of freespace_new, freespace_free, and freespace_dump
 *  all the freespace_* functions are real time safe.
 *
 */

/*  freespace_find:     locates an area within the freespace state grid
                        where an area 'width' x 'height' of unused space
                        can be satisfied. the algorithm uses placement
                        options as found in the fsbound, and the area
                        will be found within the boundary defined by
                        fsbound.

    success -           returns true, and sets *resultx and *resulty
                        to coordinates of area found.

    failure -           returns false, and sets *resultx and *resulty
                        both to -1 each.
*/
bool        freespace_find(     freespace*,
                                /* boundary: */
                                int bx,         int by,
                                int bw,         int bh,

                                int placement_flags,
                                /* request: */
                                int width,      int height,
                                int* resultx,   int* resulty    );


/*  freespace_remove:   removes free space. meaning, the area at x, y
                        of width, height is no longer available as free
                        unused space. in 99% of situations the area of
                        freespace to be removed should have been 
                        previously located by freespace_find.
*/
void        freespace_remove(   freespace*,
                                int x,      int y,
                                int width,  int height );

/*  freespace_add:      returns the area at x, y, of width, height to
                        the state of being free unsused space once more
                        available for use by the placement algorithm.
*/
void        freespace_add(      freespace*,
                                int x,      int y,
                                int width,  int height );


/*  freespace_block_clear clears areas affected by freespace_block_*
                        functions and erases the tracking of them.
*/
void        freespace_block_clear(freespace*);


/*  freespace_block_remove is somewhat like freespace_remove but
                        additionally operates on a secondary layer. the area
                        removed will be treated by freespace_find like any
                        other area of used space, but when freespace_add is
                        called on an area which intersects, the common area
                        remains as used space.
*/
bool        freespace_block_remove(freespace*,
                                int x,      int y,
                                int width,  int height );


/*  freespace_block_add is somewhat like freespace_add but is the companion
                        to freespace_block_remove. it operates on a 
                        secondary layer so that only space removed by
                        freespace_block_remove that DOES NOT intersect with
                        space removed by freespace_remove is returned.
 */
void        freespace_block_add(freespace*,
                                int x,      int y,
                                int width,  int height );


/*  freespace_dump:     dumps one of the 4 freespace state bufs as text.
                        the arrays are as follows:
                            0 - buf - the actual freespace state
                            1 - col - used only for column smart searching
                            2 - blk - space used by blocks
                            3 - nat - space not used by blocks
                            
*/
void        freespace_dump(freespace*, int buf);

char*       freespace_placement_to_str(int placement_flags);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif
