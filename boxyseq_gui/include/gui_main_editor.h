#ifndef INCLUDE_GUI_MAIN_DATA_H
#define INCLUDE_GUI_MAIN_DATA_H

/*

******************* THIS INCLUDE IS IMPLEMENTATION ONLy. *****************
** DO NOT MAKE THIS DATA STRUCTURE ACCESSIBLE OUTSIDE OF IMPLEMENTATION **
************************* THANKYOU FOR LISTENING *************************

*/


struct gui_main_editor
{
    GtkWidget*  window;
    GtkWidget*  timelabel;

    GtkWidget*  play_img;
    GtkWidget*  stop_img;
    GtkWidget*  rewind_img;

    GtkWidget*  play_button;
    GtkWidget*  rewind_button;
    GtkWidget*  zoom_in_button;
    GtkWidget*  zoom_out_button;


    gui_grid*   ggr;

    boxyseq*    bs;

    guint   pos_timeout_id;
    guint   bs_timeout_id;

    int     jack_rolling;

};




#endif
