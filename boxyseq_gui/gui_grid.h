#ifndef GUI_GRID_H
#define GUI_GRID_H


#include <gtk/gtk.h>


#include "grid_boundary.h"
#include "gui_misc.h"


/*  gui_grid
 *------------
 *  the visual representation of the free space grid and the boundaries
 *  within it, and the dynamic events popping in and out of them.
 *
 */

typedef struct gui_grid_editor gui_grid;


gui_grid*   gui_grid_create(GtkWidget* grid_container, boxyseq*);
void        gui_grid_destroy(gui_grid*);


void    gui_grid_connect_zoom_in_button(gui_grid*, GtkWidget* button);
void    gui_grid_connect_zoom_out_button(gui_grid*, GtkWidget* button);

/*
void    gui_grid_boundary_event_play(gui_grid*);
void    gui_grid_boundary_event_block(gui_grid*);
*/

/*  here, the current boundary for which the event type is modified is
    already selected within the gui_grid. event_toggle toggles mute/unmute
    which translates to play-type or block-type placements. event_ignore
    ignores the events and no placements at all are made.
*/
void    gui_grid_boundary_event_toggle_process(gui_grid*);
void    gui_grid_boundary_event_toggle_play(gui_grid*);

/*  where flag is value from
    a) FREESPACE_PLACEMENT_FLAGS (libboxyseq/freespace_state.h)
        or
    b) GRID_BOUNDARY_FLAGS (libboxyseq/grid_boundary.h)
 */
void    gui_grid_boundary_flags_set(gui_grid*, int flag);
void    gui_grid_boundary_flags_unset(gui_grid*, int flag);
void    gui_grid_boundary_flags_toggle(gui_grid*, int flag);

/* direction could be called as result of pressing arrow key */
void    gui_grid_direction(gui_grid*, int dir); /* see gui_misc.h */

void    gui_grid_order_boundary(gui_grid*, int dir); /* (+/-) */

#endif
