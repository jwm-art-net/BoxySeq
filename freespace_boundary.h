#ifndef FREESPACE_BOUNDARY_H
#define FREESPACE_BOUNDARY_H

typedef struct freespace_boundary fsbound;


fsbound*    fsbound_new(void);
void        fsbound_free(fsbound*);
void        fsbound_init(fsbound*);

_Bool       fsbound_set_coords(fsbound*, int x, int y, int w, int h);
void        fsbound_get_coords(fsbound*, int* x, int* y, int* w, int* h);
int         fsbound_get_x(fsbound*);
int         fsbound_get_y(fsbound*);
int         fsbound_get_w(fsbound*);
int         fsbound_get_h(fsbound*);

/*
int         fsbound_flags(fsbound*);
void        fsbound_flags_clear(fsbound*);
void        fsbound_flags_set(fsbound*, int flags);
void        fsbound_flags_unset(fsbound*, int flags);
*/

#endif

/*
void        fsbound_set_row_smart(fsbound*);
void        fsbound_set_col_smart(fsbound*);

void        fsbound_set_left_to_right(fsbound*);
void        fsbound_set_right_to_left(fsbound*);

void        fsbound_set_top_to_bottom(fsbound*);
void        fsbound_set_bottom_to_top(fsbound*);

_Bool       fsbound_is_row_smart(fsbound*);
_Bool       fsbound_is_col_smart(fsbound*);

_Bool       fsbound_is_left_to_right(fsbound*);
_Bool       fsbound_is_right_to_left(fsbound*);

_Bool       fsbound_is_top_to_bottom(fsbound*);
_Bool       fsbound_is_bottom_to_top(fsbound*);
*/
