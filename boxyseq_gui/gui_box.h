#ifndef GUI_BOX_MANIP_H
#define GUI_BOX_MANIP_H


#include <gtk/gtk.h>


typedef struct gui_box_data guibox;


typedef enum BOX_ACTION
{
    BOX_ACTION_CREATE =     0x0001,
    BOX_ACTION_DESTROY =    0x0002,
    BOX_ACTION_MOVE =       0x0003,
    BOX_ACTION_RESIZE =     0x0004,

    BOX_ACTION_CANCEL =     0x0006,
    BOX_ACTION_MASK =       0x0007,

    BOX_ACTION_HOVER =      0x0008

} box_action;


typedef gboolean (*gui_expose_event_cb)(GtkWidget*,
                                        GdkEventExpose*,
                                        gpointer);

typedef gboolean (*gui_button_event_cb)(GtkWidget*,
                                        GdkEventButton*,
                                        gpointer);

typedef gboolean (*gui_motion_event_cb)(GtkWidget*,
                                        GdkEventMotion*,
                                        gpointer);


guibox*     gui_box_create(GtkWidget* container, int maxx, int maxy);
void        gui_box_destroy(guibox*);
GtkWidget*  gui_box_drawing_area(guibox*);

void        gui_box_connect_expose_event(guibox*,   gui_expose_event_cb,
                                                    gpointer userdata);
void        gui_box_connect_button_press_event(     guibox*,
                                                    gui_button_event_cb,
                                                    gpointer userdata);
void        gui_box_connect_button_release_event(   guibox*,
                                                    gui_button_event_cb,
                                                    gpointer userdata);
void        gui_box_connect_motion_event(guibox*,   gui_motion_event_cb,
                                                    gpointer userdata);

void        gui_box_connect_zoom_in_button( guibox*, GtkWidget* button);
void        gui_box_connect_zoom_out_button(guibox*, GtkWidget* button);


void        gui_box_motion(guibox*, int ptr_x, int ptr_y);
void        gui_box_press(guibox*, int action,  int bx, int by,
                                                int bw, int bh);
void        gui_box_release(guibox*, int action);

gboolean    gui_box_move(guibox*, int* bx, int* by, int bw, int bh);
gboolean    gui_box_resize(guibox*, int* bx, int* by, int* bw, int* bh);

gboolean    gui_box_ptr_within(guibox*, int bx, int by, int bw, int bh);
double      gui_box_scale_get_drawable_offset(guibox*,  int* draw_offx,
                                                        int* draw_offy);
#endif
