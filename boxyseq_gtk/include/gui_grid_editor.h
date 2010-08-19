#ifndef INCLUDE_GUI_GRID_EDITOR_DATA_H
#define INCLUDE_GUI_GRID_EDITOR_DATA_H

/*

******************* THIS INCLUDE IS IMPLEMENTATION ONLy. *****************
** DO NOT MAKE THIS DATA STRUCTURE ACCESSIBLE OUTSIDE OF IMPLEMENTATION **
************************* THANKYOU FOR LISTENING *************************

*/


typedef enum GRID_ACTION
{
    ACT_GRB = 1,
    ACT_GRB_HOVER,
    ACT_GRB_MOVE,
    ACT_GRB_RESIZE

} gridact;



struct gui_grid_editor
{
    grid*       gr;
    boxyseq*    bs;
    jackdata*   jd;

    evlist*     events;

    double scale;

    int maxsz;

    guint timeout_id;

    gridact     action;
    grbound*    action_grb;

    GtkWidget*      window;
    GtkWidget*      drawing_area;
    GdkDrawable*    drawable;

    GdkGC*      gc;
    cairo_t*    cr;

    gboolean ptr_in_drawable;
    gboolean ptr_in_grid;

    int ptr_x;
    int ptr_y;

    int ptr_gx;
    int ptr_gy;

    int ptr_offx;
    int ptr_offy;

    int xrszdir;
    int yrszdir;
};


#endif
