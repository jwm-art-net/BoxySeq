#include "gui_grid.h"


#include "boxy_sequencer.h"
#include "debug.h"
#include "event_list.h"
#include "grid_boundary.h"
#include "jack_process.h"


#include <gtk/gtk.h>


#include <stdlib.h>


struct gui_grid_editor
{
    grid*       gr;
    boxyseq*    bs;
    jackdata*   jd;

    evlist*      events;

    double scale;

    int maxsz;

    guint timeout_id;


    GtkWidget*   window;
    GtkWidget*   drawing_area;

    gboolean ptr_in_drawable;
    gboolean ptr_in_grid;

    int ptr_x;
    int ptr_y;

    int ptr_grid_x;
    int ptr_grid_y;
};


static void gui_grid_note_on(cairo_t* cr, int x, int y, int w, int h)
{
    cairo_rectangle(cr, x, y, w, h);

    cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, 1);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.0, 0.5, 0.0, 1);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke (cr);

}

static void gui_grid_note_off(cairo_t* cr, int x, int y, int w, int h)
{
    cairo_rectangle(cr, x, y, w, h);

    cairo_set_source_rgba(cr, 0.0, 0.5, 0.0, 1);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.0, 0.2, 0.0, 1);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke (cr);
}


static void gui_grid_block(cairo_t* cr, int x, int y, int w, int h)
{
    cairo_rectangle(cr, x, y, w, h);

    cairo_set_source_rgba(cr, 0.0, 0.0, 1.0, 1);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.5, 1);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke (cr);
}


static void gui_grid_block_on(cairo_t* cr, int x, int y, int w, int h)
{
    cairo_rectangle(cr, x, y, w, h);

    cairo_set_source_rgba(cr, 0.0, 1.0, 1.0, 1);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.0, 0.5, 0.5, 1);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke (cr);
}


static void gui_grid_block_off(cairo_t* cr, int x, int y, int w, int h)
{
    cairo_rectangle(cr, x, y, w, h);

    cairo_set_source_rgba(cr, 0.0, 0.0, 1.0, 1);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.5, 1);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke (cr);
}


static gboolean gui_grid_timed_updater(gpointer data)
{
    gui_grid* ggr = (gui_grid*)data;
    gtk_widget_queue_draw(ggr->window);
    gtk_widget_queue_draw(ggr->drawing_area);

    return TRUE;
}


static gboolean gui_grid_expose_event(  GtkWidget *widget,
                                        GdkEventExpose *gdkevent,
                                        gpointer data)
{
    gui_grid* ggr = (gui_grid*)data;

    cairo_t* cr = gdk_cairo_create(ggr->drawing_area->window);

    event* ev;
    lnode* ln;

    cairo_pattern_t *pat;

    pat = cairo_pattern_create_linear (0.0, 0.0,  ggr->maxsz, ggr->maxsz);
    cairo_pattern_add_color_stop_rgba (pat, 0, 0.0, 0.0, 0.0, 1);
    cairo_pattern_add_color_stop_rgba (pat, 1, 0.2, 0.2, 0.2, 1);
    cairo_rectangle (cr, 0, 0, ggr->maxsz, ggr->maxsz);
    cairo_set_source (cr, pat);
    cairo_fill (cr);
    cairo_pattern_destroy (pat);

    ln = evlist_head(boxyseq_ui_event_list(ggr->bs));

    int bx, by, bw, bh;

    while(ln)
    {
        ev = (event*)lnode_data(ln);

        bx = (int)(ev->box_x * ggr->scale);
        by = (int)(ev->box_y * ggr->scale);
        bw = (int)(ev->box_width * ggr->scale);
        bh = (int)(ev->box_height * ggr->scale);

        if (EVENT_IS_TYPE_BLOCK( ev ))
        {
            if (EVENT_IS_STATUS_ON( ev ))
                gui_grid_block_on(cr, bx, by, bw, bh);
            else
                gui_grid_block_on(cr, bx, by, bw, bh);
        }
        else
        {
            if (EVENT_IS_STATUS_ON( ev ))
                gui_grid_note_on(cr, bx, by, bw, bh);
            else
                gui_grid_note_off(cr, bx, by, bw, bh);
        }

        ln = lnode_next(ln);
    }

    if (ggr->ptr_in_grid)
    {
        cairo_rectangle(cr, ggr->ptr_grid_x * ggr->scale,
                            ggr->ptr_grid_y * ggr->scale,
                            ggr->scale, ggr->scale);

        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.25);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.25);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke (cr);
    }

    cairo_destroy(cr);

    return TRUE;
}

static gboolean gui_grid_button_press_event(GtkWidget* widget,
                                            GdkEvent *gdkevent,
                                            gpointer data)
{
    return TRUE;
}

static gboolean gui_grid_button_release_event(  GtkWidget* widget,
                                                GdkEvent *gdkevent,
                                                gpointer data)
{
    return TRUE;
}


static gboolean gui_grid_motion_event(  GtkWidget* widget,
                                        GdkEventMotion *gdkevent,
                                        gpointer data)
{
    gui_grid* ggr = (gui_grid*)data;

    ggr->ptr_x = (int)gdkevent->x;
    ggr->ptr_y = (int)gdkevent->y;

    ggr->ptr_grid_x = (int)(ggr->ptr_x / ggr->scale);
    ggr->ptr_grid_y = (int)(ggr->ptr_y / ggr->scale);

    if (ggr->ptr_grid_x < 128 && ggr->ptr_grid_y < 128)
        ggr->ptr_in_grid = TRUE;
    else
        ggr->ptr_in_grid = FALSE;

    return TRUE;
}


static gboolean gui_grid_enter_leave_event( GtkWidget* widget,
                                            GdkEvent *gdkevent,
                                            gpointer data)
{
    gui_grid* ggr = (gui_grid*)data;

    if (gdkevent->type == GDK_ENTER_NOTIFY)
        ggr->ptr_in_drawable = TRUE;
    else
        ggr->ptr_in_drawable = ggr->ptr_in_grid = FALSE;

    return TRUE;
}


static void gui_grid_free(GtkWidget* widget, gui_grid* ggr)
{
    if (!ggr)
    {
        WARNING("can't free nothing\n");
        return;
    }

    if (ggr->timeout_id)
        g_source_remove(ggr->timeout_id);

    free(ggr);
}

void gui_grid_show(gui_grid** ggr_ptr, grid* gr, boxyseq* bs)
{
    gui_grid* ggr;

    GtkWidget* tmp;
    GtkWidget* vbox;

    if (*ggr_ptr)
        return;

    ggr= malloc(sizeof(*ggr));

    if (!ggr)
        goto fail0;

    ggr->gr = gr;
    ggr->bs = bs;
    ggr->jd = boxyseq_jackdata(bs);
    ggr->scale = 4.0;

    ggr->maxsz = (int)(128 * ggr->scale);

    ggr->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(ggr->window), "grid Editor");
    gtk_window_set_resizable(GTK_WINDOW(ggr->window), TRUE);

    g_signal_connect(GTK_OBJECT(ggr->window), "destroy",
                     G_CALLBACK(gui_grid_free), ggr);

    g_signal_connect(GTK_OBJECT(ggr->window), "destroy",
                     G_CALLBACK(gtk_widget_destroyed), ggr_ptr);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(ggr->window), vbox);
    gtk_widget_show(vbox);

    tmp = gtk_drawing_area_new();
    gtk_widget_set_size_request(tmp, ggr->maxsz, ggr->maxsz);
    gtk_widget_set_events (tmp, GDK_BUTTON_PRESS_MASK
                              | GDK_BUTTON_RELEASE_MASK
                              | GDK_POINTER_MOTION_MASK
                              | GDK_ENTER_NOTIFY_MASK
                              | GDK_LEAVE_NOTIFY_MASK
                              | GDK_EXPOSURE_MASK);

    g_signal_connect(GTK_OBJECT(tmp), "expose_event",
                    G_CALLBACK(gui_grid_expose_event), ggr);

    g_signal_connect(GTK_OBJECT(tmp), "button_press_event",
                     G_CALLBACK(gui_grid_button_press_event), ggr);

    g_signal_connect(GTK_OBJECT(tmp), "button_release_event",
                     G_CALLBACK(gui_grid_button_release_event), ggr);

    g_signal_connect(GTK_OBJECT(tmp), "motion_notify_event",
                     G_CALLBACK(gui_grid_motion_event), ggr);

    g_signal_connect(GTK_OBJECT(tmp), "enter_notify_event",
                     G_CALLBACK(gui_grid_enter_leave_event), ggr);

    g_signal_connect(GTK_OBJECT(tmp), "leave_notify_event",
                     G_CALLBACK(gui_grid_enter_leave_event), ggr);

    gtk_box_pack_start(GTK_BOX(vbox), tmp, TRUE, TRUE, 0);
    gtk_widget_show(tmp);

    ggr->drawing_area = tmp;

    gtk_widget_show_all(ggr->window);

    ggr->timeout_id = g_timeout_add(33,
                                    (GtkFunction)gui_grid_timed_updater,
                                    ggr);

    *ggr_ptr = ggr;
    return;

fail0:
    WARNING("failed to allocate memory for gui grid editor\n");
    return;
}

