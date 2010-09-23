#include "gui_grid.h"


#include "boxy_sequencer.h"
#include "debug.h"
#include "event_list.h"
#include "grid_boundary.h"
#include "jack_process.h"


#include <gtk/gtk.h>


#include <stdlib.h>


#include "include/gui_grid_editor.h"




static void gui_grid_boundary(cairo_t* cr, grbound* grb, gui_grid* ggr)
{
    int bx, by, bw, bh;
    gboolean filled = FALSE;

    grbound_fsbound_get(grb, &bx, &by, &bw, &bh);

    cairo_rectangle(cr, bx, by, bw, bh);

    if (grb == ggr->action_grb)
    {
        filled = TRUE;

        switch(ggr->action)
        {
        case ACT_GRB_HOVER:
            cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
            cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
            cairo_fill(cr);
            cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
            break;

        case ACT_GRB_MOVE:
        case ACT_GRB_RESIZE:
            cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
            cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
            cairo_fill(cr);
            cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
            break;
        }
    }
    else
    {
        cairo_set_source_rgba(cr, 0.0, 0.0, 1.0, 0.5);
        cairo_set_line_width(cr, 2);
        cairo_stroke (cr);
    }
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

    grbound_manager* grbman = 0;
    grbound* grb = 0;
    event* ev = 0;
    lnode* ln = 0;

    cairo_t*            cr;
    cairo_pattern_t*    pat;

    int bx, by, bw, bh;

    cr = gdk_cairo_create(ggr->drawing_area->window);

    cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    cairo_paint(cr);

    cairo_translate(cr, ggr->draw_offx, ggr->draw_offy);
    cairo_scale(cr, ggr->scale, ggr->scale);

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_rectangle(cr, 0, 0, 128, 128);
    cairo_fill(cr);

    ln = evlist_head(boxyseq_ui_event_list(ggr->bs));

    while(ln)
    {
        ev = (event*)lnode_data(ln);

        bx = ev->box_x;
        by = ev->box_y;
        bw = ev->box_width;
        bh = ev->box_height;

        double r, g, b;
        scale_int_as_rgb(grbound_scale_binary(ev->grb), &r, &g, &b);

        if (EVENT_IS_TYPE_BLOCK( ev ))
        {
            cairo_rectangle(cr, bx, by, bw, bh);
            cairo_set_source_rgb(cr, 1.0 - r, 1.0 - g, 1.0 - b);
            cairo_fill(cr);
        }
        else
        {
            if (EVENT_IS_STATUS_ON( ev ))
            {
                cairo_rectangle(cr, bx, by, bw, bh);
                cairo_set_source_rgb(cr, r, g, b);
                cairo_fill(cr);
            }
            else
            {
                cairo_rectangle(cr, bx, by, bw, bh);
                cairo_set_source_rgb(cr, 0.5 * r, 0.5 * g, 0.5 * b);
                cairo_fill(cr);
            }
        }

        ln = lnode_next(ln);
    }

    grbman = boxyseq_grbound_manager(ggr->bs);
    grb = grbound_manager_grbound_first(grbman);

    while(grb)
    {
        gui_grid_boundary(cr, grb, ggr);
        grb = grbound_manager_grbound_next(grbman);
    }

    cairo_destroy(cr);

    return TRUE;
}


static gboolean gui_grid_button_press_event(GtkWidget* widget,
                                            GdkEventButton *gdkevent,
                                            gpointer data)
{
    grbound* grb;
    gui_grid* ggr = (gui_grid*)data;

    if (ggr->action == ACT_GRB_HOVER && ggr->action_grb)
    {
        int bx, by, bw, bh;

        grbound_fsbound_get(ggr->action_grb, &bx, &by, &bw, &bh);

        ggr->action = ACT_GRB_MOVE;

        switch(gdkevent->button)
        {
            case 1:
                ggr->action = ACT_GRB_MOVE;
                ggr->ptr_offx = bx - ggr->ptr_gx;
                ggr->ptr_offy = by - ggr->ptr_gy;
                break;

            case 3:
                ggr->action = ACT_GRB_RESIZE;
                ggr->ptr_offx = ggr->ptr_gx;
                ggr->ptr_offy = ggr->ptr_gy;

                ggr->xrszdir = (ggr->ptr_gx < bx + bw / 2) ? -1 : 1;
                ggr->yrszdir = (ggr->ptr_gy < by + bh / 2) ? -1 : 1;
                break;

            default:
                ggr->action = ACT_GRB;
                break;
        }
    }

    return TRUE;
}

static gboolean gui_grid_button_release_event(  GtkWidget* widget,
                                                GdkEventButton *gdkevent,
                                                gpointer data)
{
    gui_grid* ggr = (gui_grid*)data;

    switch(ggr->action)
    {
    case ACT_GRB_MOVE:
    case ACT_GRB_RESIZE:

        ggr->action = ACT_GRB_HOVER;
        grbound_update_rt_data(ggr->action_grb);

        break;

    default:

        break;
    }

    return TRUE;
}


static gboolean gui_grid_motion_event(  GtkWidget* widget,
                                        GdkEventMotion *gdkevent,
                                        gpointer data)
{
    gui_grid* ggr = (gui_grid*)data;
    grbound_manager* grbman;
    grbound* grb;
    int bx, by, bw, bh;

    ggr->ptr_x = (int)gdkevent->x - ggr->draw_offx;
    ggr->ptr_y = (int)gdkevent->y - ggr->draw_offx;

    ggr->ptr_gx = (int)(ggr->ptr_x / ggr->scale);
    ggr->ptr_gy = (int)(ggr->ptr_y / ggr->scale);

    ggr->ptr_in_grid = (ggr->ptr_gx > 0 && ggr->ptr_gx < 128
                     && ggr->ptr_gy > 0 && ggr->ptr_gy < 128);

    grbman = boxyseq_grbound_manager(ggr->bs);
    grb = grbound_manager_grbound_first(grbman);

    switch(ggr->action)
    {
    case ACT_GRB:
    case ACT_GRB_HOVER:

        while(grb)
        {
            ggr->action_grb = 0;
            grbound_fsbound_get(grb, &bx, &by, &bw, &bh);

            if (ggr->ptr_gx >= bx && ggr->ptr_gx <= bx + bw
             && ggr->ptr_gy >= by && ggr->ptr_gy <= by + bh)
            {
                ggr->action = ACT_GRB_HOVER;
                ggr->action_grb = grb;
                break;
            }
            grb = grbound_manager_grbound_next(grbman);
        }

        if (!ggr->action_grb)
            ggr->action = ACT_GRB;

        break;

    case ACT_GRB_MOVE:

        if (ggr->ptr_in_grid)
        {
            bx = ggr->ptr_gx + ggr->ptr_offx;
            by = ggr->ptr_gy + ggr->ptr_offy;

            grbound_fsbound_get(ggr->action_grb, 0, 0, &bw, &bh);

            if (bx < 0)
                bx = 0;
            else if (bx + bw > 128)
                bx = 128 - bw;

            if (by < 0)
                by = 0;
            else if (by + bh > 128)
                by = 128 - bh;

            grbound_fsbound_set(ggr->action_grb, bx, by, -1, -1);
        }

        break;

    case ACT_GRB_RESIZE:

        if (ggr->ptr_in_grid)
        {
            int pdx, pdy;
            int obx, oby, obw, obh;

            grbound_fsbound_get(ggr->action_grb, &bx, &by, &bw, &bh);

            obx = bx;
            oby = by;
            obw = bw;
            obh = bh;

            pdx = ggr->ptr_offx - ggr->ptr_gx;
            pdy = ggr->ptr_offy - ggr->ptr_gy;

            if (ggr->xrszdir > 0)
            {
                bw -= pdx;

                if (bw < 2 || bx + bw > 128)
                    bw = obw;
            }
            else
            {
                bw += pdx;
                bx -= pdx;

                if (bw < 2 || bx < 0)
                {
                    bx = obx;
                    bw = obw;
                }
            }

            if (ggr->yrszdir > 0)
            {
                bh -= pdy;

                if (bh < 2 || by + bh > 128)
                    bh = obh;
            }
            else
            {
                bh += pdy;
                by -= pdy;

                if (bh < 2 || by < 0)
                {
                    by = oby;
                    bh = obh;
                }
            }

            ggr->ptr_offx = ggr->ptr_gx;
            ggr->ptr_offy = ggr->ptr_gy;

            grbound_fsbound_set(ggr->action_grb, bx, by, bw, bh);
        }

        break;
    }

    return TRUE;
}


static gboolean gui_grid_enter_leave_event( GtkWidget* widget,
                                            GdkEvent *gdkevent,
                                            gpointer data)
{
    gui_grid* ggr = (gui_grid*)data;

    if (gdkevent->type == GDK_ENTER_NOTIFY)
    {
        ggr->ptr_in_drawable = TRUE;
    }
    else
    {
        ggr->ptr_in_drawable = ggr->ptr_in_grid = FALSE;
    }

    return TRUE;
}


static void gui_grid_window_h_changed(GtkAdjustment* adj, gpointer data)
{
    gui_grid* ggr = (gui_grid*)data;
    int offx = (gtk_adjustment_get_page_size(adj) - ggr->maxsz) / 2;

    ggr->draw_offx = (offx > 0) ? offx : 0;
}

static void gui_grid_window_v_changed(GtkAdjustment* adj, gpointer data)
{
    gui_grid* ggr = (gui_grid*)data;

    int offy = (gtk_adjustment_get_page_size(adj) - ggr->maxsz) / 2;

    ggr->draw_offy = (offy > 0) ? offy : 0;
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


static gboolean gui_grid_zoom_in(   GtkWidget* widget,
                                    gpointer data       )
{
    gui_grid* ggr = (gui_grid*)data;

    if (ggr->scale > 7.0)
        return TRUE;

    if (ggr->scale < 2.0)
        gtk_widget_set_sensitive(ggr->zoom_out_button, TRUE);

    ggr->scale *= 2.0;
    ggr->maxsz = (int)(128 * ggr->scale);

    gtk_widget_set_size_request(ggr->drawing_area,
                                ggr->maxsz + 1,
                                ggr->maxsz + 1);

    if (ggr->scale > 7.0)
        gtk_widget_set_sensitive(ggr->zoom_in_button, FALSE);

    return TRUE;
}


static gboolean gui_grid_zoom_out(   GtkWidget* widget,
                                    gpointer data       )
{
    gui_grid* ggr = (gui_grid*)data;

    if (ggr->scale < 2.0)
        return TRUE;

    if (ggr->scale > 7.0)
        gtk_widget_set_sensitive(ggr->zoom_in_button, TRUE);

    ggr->scale *= 0.5;
    ggr->maxsz = (int)(128 * ggr->scale);

    gtk_widget_set_size_request(ggr->drawing_area,
                                ggr->maxsz + 1,
                                ggr->maxsz + 1);

    if (ggr->scale < 2.0)
        gtk_widget_set_sensitive(ggr->zoom_out_button, FALSE);

    return TRUE;
}


void gui_grid_show(gui_grid** ggr_ptr, grid* gr, boxyseq* bs)
{
    gui_grid* ggr;

    GtkWidget* tmp;
    GtkWidget* main;
    GtkWidget* toolbar;

    GtkAdjustment* adj;

    if (*ggr_ptr)
        return;

    ggr= malloc(sizeof(*ggr));

    if (!ggr)
        goto fail0;

    ggr->gr = gr;
    ggr->bs = bs;
    ggr->jd = boxyseq_jackdata(bs);
    ggr->scale = 2.0;

    ggr->maxsz = (int)(128 * ggr->scale);
    ggr->draw_offx = 0;
    ggr->draw_offy = 0;

    ggr->action = ACT_GRB;
    ggr->action_grb = 0;

    ggr->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(ggr->window), "grid Editor");
    gtk_window_set_resizable(GTK_WINDOW(ggr->window), TRUE);

    g_signal_connect(GTK_OBJECT(ggr->window), "destroy",
                     G_CALLBACK(gui_grid_free), ggr);

    g_signal_connect(GTK_OBJECT(ggr->window), "destroy",
                     G_CALLBACK(gtk_widget_destroyed), ggr_ptr);

    main = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(ggr->window), main);
    gtk_widget_show(main);

    toolbar = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(toolbar);
    gtk_container_set_border_width(GTK_CONTAINER(toolbar), 4);
    gtk_box_pack_start(GTK_BOX(main), toolbar, FALSE, TRUE, 0);

    tmp = gtk_button_new();
    gtk_button_set_focus_on_click(GTK_BUTTON(tmp), FALSE);
    gtk_button_set_relief(GTK_BUTTON(tmp), GTK_RELIEF_NONE);
    gtk_button_set_image(GTK_BUTTON(tmp), 
                gtk_image_new_from_stock(GTK_STOCK_ZOOM_OUT,
                                        GTK_ICON_SIZE_SMALL_TOOLBAR));
    gtk_box_pack_start(GTK_BOX(toolbar), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    g_signal_connect(   GTK_OBJECT(tmp),
                        "clicked",
                        G_CALLBACK(gui_grid_zoom_out),
                        ggr);
    ggr->zoom_out_button = tmp;

    tmp = gtk_button_new();
    gtk_button_set_focus_on_click(GTK_BUTTON(tmp), FALSE);
    gtk_button_set_relief(GTK_BUTTON(tmp), GTK_RELIEF_NONE);
    gtk_button_set_image(GTK_BUTTON(tmp), 
                gtk_image_new_from_stock(GTK_STOCK_ZOOM_IN,
                                        GTK_ICON_SIZE_SMALL_TOOLBAR));
    gtk_box_pack_start(GTK_BOX(toolbar), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    g_signal_connect(   GTK_OBJECT(tmp),
                        "clicked",
                        G_CALLBACK(gui_grid_zoom_in),
                        ggr);
    ggr->zoom_in_button = tmp;

    tmp = gtk_drawing_area_new();
    gtk_widget_set_size_request(tmp, ggr->maxsz + 1, ggr->maxsz + 1);
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

    gtk_widget_show(tmp);
    ggr->drawing_area = tmp;

    tmp = gtk_viewport_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(tmp), ggr->drawing_area);
    gtk_widget_set_size_request(tmp, 200, 200);
    gtk_widget_show(tmp);
    ggr->viewport = tmp;

    tmp = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(tmp), ggr->viewport);
    gtk_box_pack_start(GTK_BOX(main), tmp, TRUE, TRUE, 0);
    gtk_widget_show(tmp);
    gtk_widget_show(ggr->window);
    ggr->scrolled_window = tmp;

    adj = gtk_scrolled_window_get_hadjustment(
                            GTK_SCROLLED_WINDOW(ggr->scrolled_window));

    g_signal_connect(GTK_OBJECT(adj), "changed",
                     G_CALLBACK(gui_grid_window_h_changed), ggr);

    adj = gtk_scrolled_window_get_vadjustment(
                            GTK_SCROLLED_WINDOW(ggr->scrolled_window));

    g_signal_connect(GTK_OBJECT(adj), "changed",
                     G_CALLBACK(gui_grid_window_v_changed), ggr);

    ggr->timeout_id = g_timeout_add(33,
                                    (GtkFunction)gui_grid_timed_updater,
                                    ggr);

    *ggr_ptr = ggr;
    return;

fail0:
    WARNING("failed to allocate memory for gui grid editor\n");
    return;
}

