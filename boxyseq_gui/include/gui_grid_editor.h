#ifndef INCLUDE_GUI_GRID_EDITOR_DATA_H
#define INCLUDE_GUI_GRID_EDITOR_DATA_H

/*

******************* THIS INCLUDE IS IMPLEMENTATION ONLy. *****************
** DO NOT MAKE THIS DATA STRUCTURE ACCESSIBLE OUTSIDE OF IMPLEMENTATION **
************************* THANKYOU FOR LISTENING *************************

*/


typedef enum GRID_OBJECT
{
    GRID_OBJECT_BOUNDARY =  0x0001,
    GRID_OBJECT_USERBLOCK = 0x0002,

} gridobj;



struct gui_grid_editor
{
    boxyseq*    bs;
    jackdata*   jd;

    evlist*     events;

    guibox* gb;

    guint timeout_id;

    int         action;
    int         object;
    grbound*    action_grb;
    event*      action_ev;

    GtkWidget*      drawing_area;

    cairo_t*    cr;
};


#endif
