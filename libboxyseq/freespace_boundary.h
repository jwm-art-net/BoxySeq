#ifndef FREESPACE_BOUNDARY_H
#define FREESPACE_BOUNDARY_H


#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>


typedef struct freespace_boundary fsbound;


fsbound*    fsbound_new(void);
fsbound*    fsbound_dup(const fsbound*);
void        fsbound_copy(fsbound* dest, const fsbound* src);
void        fsbound_free(fsbound*);
void        fsbound_init(fsbound*);


/*  fsbound_set_coords
 *----------------------
 *  sets the coordinates of a boundary
 *  fails on invalid values of x, y, w, h,
 *  including invalid values of x + w, y + h
 *  on failure, does not change any values.
 */
bool        fsbound_set_coords(fsbound*, int x, int y, int w, int h);

/*  fsbound_get_coords
 *----------------------
 *  fills the pointers with values of coordinates.
 *  pointers must be valid.
 */
void        fsbound_get_coords(fsbound*, int* x, int* y, int* w, int* h);

int         fsbound_get_x(fsbound*);
int         fsbound_get_y(fsbound*);
int         fsbound_get_w(fsbound*);
int         fsbound_get_h(fsbound*);


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif

