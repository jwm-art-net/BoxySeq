#ifndef GUI_GRID_H
#define GUI_GRID_H


#include <gtk/gtk.h>


#include "grid_boundary.h"


typedef struct gui_grid_editor gui_grid;


gui_grid*   gui_grid_create(GtkWidget* grid_container, boxyseq*);
void        gui_grid_destroy(gui_grid*);


void    gui_grid_connect_zoom_in_button(gui_grid*, GtkWidget* button);
void    gui_grid_connect_zoom_out_button(gui_grid*, GtkWidget* button);


#endif
