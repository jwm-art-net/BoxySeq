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

bool        fsbound_set_coords(fsbound*, int x, int y, int w, int h);
void        fsbound_get_coords(fsbound*, int* x, int* y, int* w, int* h);
int         fsbound_get_x(fsbound*);
int         fsbound_get_y(fsbound*);
int         fsbound_get_w(fsbound*);
int         fsbound_get_h(fsbound*);


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif

