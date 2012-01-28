#include "gui_boundary_list.h"

#include "include/gui_boundary_list_data.h"
#include "debug.h"

#include <stdlib.h>


gui_boundlist* gui_boundary_list_create(boxyseq* bs)
{

    gui_boundlist* gbl = malloc(sizeof(*gbl));

    if (!gbl)
    {
        WARNING("failed to allocate memory for gui boundary list\n");
        return 0;
    }

    gbl->bs = bs;
    gbl->grbman = boxyseq_grbound_manager(bs);

    gui_boundary_list_update(gbl);

    return gbl;
}


void gui_boundary_list_update(gui_boundlist* gbl)
{
    GtkListStore* store;
    GtkTreeIter iter;

}
