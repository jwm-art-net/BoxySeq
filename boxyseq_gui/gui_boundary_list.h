#ifndef GUI_BOUNDARY_LIST_H
#define GU_BOUNDARY_LIST_H


#include <gtk/gtk.h>


#include "boxy_sequencer.h"

/*  this creates a new child window which presents a list of existing
    boundary boxes and their properties. it is the main interface for
    editing boundary properties.
 */



typedef struct gui_boundary_list_data gui_boundlist;

gui_boundlist*  gui_boundary_list_create(boxyseq*);
void            gui_boundary_list_free(gui_boundlist*);
void            gui_boundary_list_update(gui_boundlist*);

#endif
