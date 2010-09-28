#include "gui_box.h"

#include "debug.h"


#include <stdlib.h>


#include "include/gui_box_data.h"


#define SCALE_RATIO( integer_0_to_10 ) \
    (2.0 + ((double)(integer_0_to_10) / 10.0) * 6.0)




void gui_box_motion(guibox* gb, int ptr_x, int ptr_y)
{
    gb->ptr_x = ptr_x - gb->draw_offx;
    gb->ptr_y = ptr_y - gb->draw_offy;
    gb->ptr_gx = (int)(gb->ptr_x / gb->scale);
    gb->ptr_gy = (int)(gb->ptr_y / gb->scale);

    gb->ptr_in_bounds = (gb->ptr_gx >= 0 && gb->ptr_gx < gb->max_gx
                      && gb->ptr_gy >= 0 && gb->ptr_gy < gb->max_gy);
}


void gui_box_press(guibox* gb, int action,  int bx, int by,
                                            int bw, int bh)
{
    switch(action)
    {
    case BOX_ACTION_MOVE:
        gb->ptr_offx = bx - gb->ptr_gx;
        gb->ptr_offy = by - gb->ptr_gy;
        break;

    case BOX_ACTION_RESIZE:
        gb->ptr_offx = gb->ptr_gx;
        gb->ptr_offy = gb->ptr_gy;
        gb->ptr_orgx = bx;
        gb->ptr_orgy = by;
        gb->ptr_orgw = bw;
        gb->ptr_orgh = bh;
        gb->xrszdir = (gb->ptr_gx < bx + bw / 2) ? -1 : 1;
        gb->yrszdir = (gb->ptr_gy < by + bh / 2) ? -1 : 1;
        break;

    default:
        WARNING("unimplimented box press\n");
        break;
    }
}


gboolean gui_box_move(guibox* gb, int* bx, int* by, int bw, int bh)
{
    if (!gb->ptr_in_drawable)
        return FALSE;

    *bx = gb->ptr_gx + gb->ptr_offx;
    *by = gb->ptr_gy + gb->ptr_offy;

    if (*bx < 0)
        *bx = 0;
    else if (*bx + bw > 128)
        *bx = 128 - bw;

    if (*by < 0)
        *by = 0;
    else if (*by + bh > 128)
        *by = 128 - bh;

    return TRUE;
}


gboolean gui_box_resize(guibox* gb, int* bx, int* by, int* bw, int* bh)
{
    int pdx, pdy, obx, oby, obw, obh;

    if (!gb->ptr_in_drawable)
        return FALSE;

    obx = *bx;
    oby = *by;
    obw = *bw;
    obh = *bh;

    pdx = gb->ptr_offx - gb->ptr_gx;
    pdy = gb->ptr_offy - gb->ptr_gy;

    if (gb->xrszdir > 0)
    {
        *bw = gb->ptr_orgw - pdx;

        if (*bw < 2 || *bx + *bw > 128)
            *bw = obw;
    }
    else
    {
        *bw = gb->ptr_orgw + pdx;
        *bx = gb->ptr_orgx - pdx;

        if (*bw < 2 || *bx < 0)
        {
            *bx = obx;
            *bw = obw;
        }
    }

    if (gb->yrszdir > 0)
    {
        *bh = gb->ptr_orgh - pdy;

        if (*bh < 2 || *by + *bh > 128)
            *bh = obh;
    }
    else
    {
        *bh = gb->ptr_orgh + pdy;
        *by = gb->ptr_orgy - pdy;

        if (*bh < 2 || *by < 0)
        {
            *by = oby;
            *bh = obh;
        }
    }

    return TRUE;
}


gboolean gui_box_ptr_within(guibox* gb, int bx, int by, int bw, int bh)
{
    return  (gb->ptr_gx >= bx && gb->ptr_gx < bx + bw
             && gb->ptr_gy >= by && gb->ptr_gy < by + bh);
}


double  gui_box_scale_get_drawable_offset(guibox* gb,   int* draw_offx,
                                                        int* draw_offy)
{
    *draw_offx = gb->draw_offx;
    *draw_offy = gb->draw_offy;
    return gb->scale;
}


static gboolean gui_box_enter_leave_event( GtkWidget* widget,
                                            GdkEvent *gdkevent,
                                            gpointer data)
{
    guibox* gb = (guibox*)data;

    if (gdkevent->type == GDK_ENTER_NOTIFY)
        gb->ptr_in_drawable = TRUE;
    else
        gb->ptr_in_drawable = gb->ptr_in_bounds = FALSE;

    return TRUE;
}


static void gui_box_window_w_changed(GtkAdjustment* adj, gpointer data)
{
    guibox* gb = (guibox*)data;
    int offx = (gtk_adjustment_get_page_size(adj) - gb->max_x) / 2;
    gb->draw_offx = (offx > 0) ? offx : 0;
}


static void gui_box_window_h_changed(GtkAdjustment* adj, gpointer data)
{
    guibox* gb = (guibox*)data;
    int offy = (gtk_adjustment_get_page_size(adj) - gb->max_y) / 2;
    gb->draw_offy = (offy > 0) ? offy : 0;
}


void gui_box_destroy(guibox* gb)
{
    if (!gb)
        return;

    free(gb);
}


static void gui_box_zoom_in(GtkWidget* widget, gpointer data)
{
    guibox* gb = (guibox*)data;

    if (gb->scale_factor == 10)
        return;

    if (gb->scale_factor == 0)
        gtk_widget_set_sensitive(gb->zoom_out_button, TRUE);

    gb->scale_factor++;
    gb->scale = SCALE_RATIO(gb->scale_factor);
    gb->max_x = (int)(gb->max_gx * gb->scale);
    gb->max_y = (int)(gb->max_gy * gb->scale);

    gtk_widget_set_size_request(gb->drawing_area,
                                gb->max_x + 1,
                                gb->max_y + 1);

    if (gb->scale_factor == 10)
        gtk_widget_set_sensitive(gb->zoom_in_button, FALSE);
}


static void gui_box_zoom_out(GtkWidget* widget, gpointer data)
{
    guibox* gb = (guibox*)data;

    if (gb->scale_factor == 0)
        return;

    if (gb->scale_factor == 10)
        gtk_widget_set_sensitive(gb->zoom_in_button, TRUE);

    gb->scale_factor--;
    gb->scale = SCALE_RATIO(gb->scale_factor);
    gb->max_x = (int)(gb->max_gx * gb->scale);
    gb->max_y = (int)(gb->max_gy * gb->scale);

    gtk_widget_set_size_request(gb->drawing_area,
                                gb->max_x + 1,
                                gb->max_y + 1);
    if (gb->scale_factor == 0)
        gtk_widget_set_sensitive(gb->zoom_out_button, FALSE);
}


guibox* gui_box_create(GtkWidget* grid_container, int maxx, int maxy)
{
    guibox* gb;
    GtkWidget* tmp;
    GtkAdjustment* adj;

    gb = malloc(sizeof(*gb));

    if (!gb)
    {
        WARNING("failed to allocate memory for gui box\n");
        return 0;
    }

    gb->max_gx = maxx;
    gb->max_gy = maxy;

    gb->scale_factor = 1;

    gb->scale = SCALE_RATIO(gb->scale_factor);

    gb->max_x = (int)(gb->max_gx * gb->scale);
    gb->max_y = (int)(gb->max_gy * gb->scale);

    gb->draw_offx = 0;
    gb->draw_offy = 0;

    tmp = gtk_drawing_area_new();
    gtk_widget_set_size_request(tmp, gb->max_x + 1, gb->max_y + 1);

    gtk_widget_set_events (tmp, GDK_BUTTON_PRESS_MASK
                              | GDK_BUTTON_RELEASE_MASK
                              | GDK_POINTER_MOTION_MASK
                              | GDK_ENTER_NOTIFY_MASK
                              | GDK_LEAVE_NOTIFY_MASK
                              | GDK_KEY_PRESS_MASK
                              | GDK_EXPOSURE_MASK);


    g_signal_connect(GTK_OBJECT(tmp), "enter_notify_event",
                     G_CALLBACK(gui_box_enter_leave_event), gb);

    g_signal_connect(GTK_OBJECT(tmp), "leave_notify_event",
                     G_CALLBACK(gui_box_enter_leave_event), gb);


    gtk_widget_show(tmp);
    gb->drawing_area = tmp;

    tmp = gtk_viewport_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(tmp), gb->drawing_area);
    gtk_widget_set_size_request(tmp, 400, 400);
    gtk_widget_show(tmp);
    gb->viewport = tmp;

    tmp = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(tmp), gb->viewport);
    gtk_widget_show(tmp);
    gtk_container_add(GTK_CONTAINER(grid_container), tmp);
    gb->scrolled_window = tmp;

    adj = gtk_scrolled_window_get_hadjustment(
                            GTK_SCROLLED_WINDOW(gb->scrolled_window));

    g_signal_connect(GTK_OBJECT(adj), "changed",
                     G_CALLBACK(gui_box_window_w_changed), gb);

    adj = gtk_scrolled_window_get_vadjustment(
                            GTK_SCROLLED_WINDOW(gb->scrolled_window));

    g_signal_connect(GTK_OBJECT(adj), "changed",
                     G_CALLBACK(gui_box_window_h_changed), gb);

    return gb;
}


GtkWidget* gui_box_drawing_area(guibox* gb)
{
    return gb->drawing_area;
}


void gui_box_connect_expose_event(guibox* gb,   gui_expose_event_cb evcb,
                                                gpointer userdata)
{
    g_signal_connect(GTK_OBJECT(gb->drawing_area), "expose_event",
                                        G_CALLBACK(evcb), userdata);
}

void gui_box_connect_button_press_event(guibox* gb,
                                        gui_button_event_cb evcb,
                                        gpointer userdata)
{
    g_signal_connect(GTK_OBJECT(gb->drawing_area), "button_press_event",
                                        G_CALLBACK(evcb), userdata);
}


void gui_box_connect_button_release_event(  guibox* gb,
                                            gui_button_event_cb evcb,
                                            gpointer userdata)
{
    g_signal_connect(GTK_OBJECT(gb->drawing_area), "button_release_event",
                                        G_CALLBACK(evcb), userdata);
}


void gui_box_connect_motion_event(guibox* gb,   gui_motion_event_cb evcb,
                                                gpointer userdata)
{
    g_signal_connect(GTK_OBJECT(gb->drawing_area), "motion_notify_event",
                                        G_CALLBACK(evcb), userdata);
}


void gui_box_connect_zoom_in_button(guibox* gb, GtkWidget* button)
{
    gb->zoom_in_button = button;
    g_signal_connect(   GTK_OBJECT(button),
                        "clicked",
                        G_CALLBACK(gui_box_zoom_in),
                        gb);
}


void gui_box_connect_zoom_out_button(guibox* gb, GtkWidget* button)
{
    gb->zoom_out_button = button;
    g_signal_connect(   GTK_OBJECT(button),
                        "clicked",
                        G_CALLBACK(gui_box_zoom_out),
                        gb);
}

