#ifndef INCLUDE_GUI_BOX_MANIP_DATA_H
#define INCLUDE_GUI_BOX_MANIP_DATA_H

/*

******************* THIS INCLUDE IS IMPLEMENTATION ONLy. *****************
** DO NOT MAKE THIS DATA STRUCTURE ACCESSIBLE OUTSIDE OF IMPLEMENTATION **
************************* THANKYOU FOR LISTENING *************************

*/


struct gui_box_data
{
    GtkWidget*      scrolled_window;
    GtkWidget*      viewport;
    GtkWidget*      drawing_area;
    GdkDrawable*    drawable;

    GtkWidget*      zoom_in_button;
    GtkWidget*      zoom_out_button;

    int scale;  /* clamp scale to integers to prevent blurred edges */

    int max_x;
    int max_y;

    int ptr_x;
    int ptr_y;

    int ptr_gx;
    int ptr_gy;

    int max_gx;
    int max_gy;

    int ptr_offx;
    int ptr_offy;

    int ptr_orgw;
    int ptr_orgh;
    int ptr_orgx;
    int ptr_orgy;

    int draw_offx;
    int draw_offy;

    int xrszdir;
    int yrszdir;

    gboolean ptr_in_drawable;
    gboolean ptr_in_bounds;
    
};


#endif
