#ifndef GUI_GRID_H
#define GUI_GRID_H


#include <gtk/gtk.h>


#include "grid_boundary.h"
#include "gui_misc.h"

typedef struct gui_grid_editor gui_grid;


gui_grid*   gui_grid_create(GtkWidget* grid_container, boxyseq*);
void        gui_grid_destroy(gui_grid*);


void    gui_grid_connect_zoom_in_button(gui_grid*, GtkWidget* button);
void    gui_grid_connect_zoom_out_button(gui_grid*, GtkWidget* button);

void    gui_grid_boundary_play(gui_grid*);
void    gui_grid_boundary_block(gui_grid*);
void    gui_grid_boundary_ignore(gui_grid*);

/* direction could be called as result of pressing arrow key */
void    gui_grid_direction(gui_grid*, int dir); /* see gui_misc.h */

void    gui_grid_order_boundary(gui_grid*, int dir); /* (+/-) */

#endif
