#ifndef FREESPACE_STATE_H
#define FREESPACE_STATE_H


#ifdef __cplusplus
extern "C" {
#endif


#include "freespace_boundary.h"


#include <stdbool.h>


#define FSWIDTH  128 /* YOU CHANGE YOU BREAK! */
#define FSHEIGHT 128



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
                                fsbound*,
                                int placement_flags,
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

/*  freespace_add:      returns the area at x,y of width, height to
                        the state of being free unsused space once more
                        available for use by the placement algorithm.
*/

void        freespace_add(      freespace*,
                                int x,      int y,
                                int width,  int height );


#ifdef FREESPACE_DEBUG
void        freespace_dump(freespace*);
#endif


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif
