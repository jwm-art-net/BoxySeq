#include "gui_grid.h"


#include "boxy_sequencer.h"
#include "debug.h"
#include "event_list.h"
#include "grid_boundary.h"
#include "jack_process.h"

#include "gui_box.h"

#include <stdlib.h>

#include "include/gui_grid_editor.h"

#define SCALE_RATIO( integer_0_to_10 ) \
    (2.0 + ((double)(integer_0_to_10) / 10.0) * 6.0)


static void gui_grid_boundary(cairo_t* cr, grbound* grb, gui_grid* ggr)
{
    int bx, by, bw, bh;
    gboolean filled = FALSE;

    grbound_fsbound_get(grb, &bx, &by, &bw, &bh);

    cairo_rectangle(cr, bx, by, bw, bh);

    if (ggr->object == GRID_OBJECT_BOUNDARY && ggr->action_grb == grb)
    {
        filled = TRUE;

        switch((ggr->action & BOX_ACTION_MASK))
        {
        case BOX_ACTION_MOVE:
        case BOX_ACTION_RESIZE:
            cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
            cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
            cairo_fill(cr);
            cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
            break;

        default: /* mouse hover */
            cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
            cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
            cairo_fill(cr);
            cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
            break;
        }
    }
    else
    {
        double r, g, b;

        switch(grbound_event_type(grb))
        {
            case GRBOUND_EVENT_PLAY:
                scale_int_as_rgb(grbound_scale_binary(grb), &r, &g, &b);
                cairo_set_source_rgb(cr, r, g, b);
                break;
            case GRBOUND_EVENT_BLOCK:
                cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
                break;
            case GRBOUND_EVENT_IGNORE:
            default:
                cairo_set_source_rgb(cr, 0.5, 0.0, 0.0);
                break;
        }
        cairo_set_line_width(cr, 0.5);
        cairo_stroke (cr);
    }
}


static gboolean gui_grid_timed_updater(gpointer data)
{
    gui_grid* ggr = (gui_grid*)data;
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

    int draw_offx;
    int draw_offy;
    double scale;

    cr = gdk_cairo_create(ggr->drawing_area->window);

    cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    cairo_paint(cr);

    scale = gui_box_scale_get_drawable_offset(ggr->gb,  &draw_offx,
                                                        &draw_offy);
    cairo_translate(cr, draw_offx, draw_offy);
    cairo_scale(cr, scale, scale);

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

        if (EVENT_IS_TYPE_BLOCK( ev ))
        {
            cairo_rectangle(cr, bx, by, bw, bh);
            cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
            cairo_fill(cr);
        }
        else
        {
            double r, g, b;

            scale_int_as_rgb(grbound_scale_binary(ev->grb), &r, &g, &b);

            if (EVENT_IS_STATUS_OFF( ev ))
            {
                r *= 0.5;
                g *= 0.5;
                b *= 0.5;
            }

            cairo_rectangle(cr, bx, by, bw, bh);
            cairo_set_source_rgb(cr, r, g, b);
            cairo_fill(cr);
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


static void gui_grid_button_press_boundary(gui_grid* ggr, int button)
{
    int bx, by, bw, bh;

    if (!ggr->action_grb)
        return;

    grbound_fsbound_get(ggr->action_grb, &bx, &by, &bw, &bh);

    switch(button)
    {
    case 1: ggr->action = BOX_ACTION_MOVE;      break;
    case 3: ggr->action = BOX_ACTION_RESIZE;    break;
    default:ggr->action = 0;                    break;
    }

    if (ggr->action)
        gui_box_press(ggr->gb, ggr->action, bx, by, bw, bh);
}


static void gui_grid_button_release_boundary(gui_grid* ggr, int button)
{
    switch(ggr->action & BOX_ACTION_MASK)
    {
    case BOX_ACTION_MOVE:
    case BOX_ACTION_RESIZE:
        ggr->action = BOX_ACTION_HOVER;
        grbound_update_rt_data(ggr->action_grb);
        break;

    default:
        break;
    }
}


static void gui_grid_motion_boundary(gui_grid* ggr)
{
    grbound_manager* grbman;
    grbound* grb;
    int bx, by, bw, bh;
    grbman = boxyseq_grbound_manager(ggr->bs);
    grb = grbound_manager_grbound_first(grbman);

    switch(ggr->action & BOX_ACTION_MASK)
    {
    case BOX_ACTION_MOVE:
        grbound_fsbound_get(ggr->action_grb, 0, 0, &bw, &bh);
        if (gui_box_move(ggr->gb, &bx, &by, bw, bh))
            grbound_fsbound_set(ggr->action_grb, bx, by, -1, -1);
        break;

    case BOX_ACTION_RESIZE:
        grbound_fsbound_get(ggr->action_grb, &bx, &by, &bw, &bh);
        if (gui_box_resize(ggr->gb, &bx, &by, &bw, &bh))
            grbound_fsbound_set(ggr->action_grb, bx, by, bw, bh);
        break;

    default:
        while(grb)
        {
            ggr->action_grb = 0;
            grbound_fsbound_get(grb, &bx, &by, &bw, &bh);

            if (gui_box_ptr_within(ggr->gb, bx, by, bw, bh))
            {
                ggr->action = BOX_ACTION_HOVER;
                ggr->action_grb = grb;
                break;
            }
            grb = grbound_manager_grbound_next(grbman);
        }

        if (!ggr->action_grb)
            ggr->action = 0;

        break;
    }
}


static void gui_grid_motion_userblock(gui_grid* ggr)
{
}


static gboolean gui_grid_button_press_event(GtkWidget* widget,
                                            GdkEventButton *gdkevent,
                                            gpointer data)
{
    gui_grid* ggr = (gui_grid*)data;

    switch((ggr->object))
    {
    case GRID_OBJECT_BOUNDARY:
        gui_grid_button_press_boundary(ggr, gdkevent->button);
        break;
    default:
        WARNING("unknown grid object type\n");
        break;
    }

    return TRUE;
}

static gboolean gui_grid_button_release_event(  GtkWidget* widget,
                                                GdkEventButton *gdkevent,
                                                gpointer data)
{
    gui_grid* ggr = (gui_grid*)data;

    switch((ggr->object))
    {
    case GRID_OBJECT_BOUNDARY:
        gui_grid_button_release_boundary(ggr, gdkevent->button);
        break;
    default:
        WARNING("unknown grid object type\n");
        break;
    }

    return TRUE;
}


static gboolean gui_grid_motion_event(  GtkWidget* widget,
                                        GdkEventMotion *gdkevent,
                                        gpointer data)
{
    gui_grid* ggr = (gui_grid*)data;
    gui_box_motion(ggr->gb, (int)gdkevent->x, (int)gdkevent->y);

    switch(ggr->object)
    {
    case GRID_OBJECT_BOUNDARY:
        gui_grid_motion_boundary(ggr);
        break;

    case GRID_OBJECT_USERBLOCK:
        gui_grid_motion_userblock(ggr);
        break;

    default:
        WARNING("unhandled grid object for motion event\n");
    }

    return TRUE;
}




void gui_grid_destroy(gui_grid* ggr)
{
    if (!ggr)
        return;

    if (ggr->timeout_id)
        g_source_remove(ggr->timeout_id);

    gui_box_destroy(ggr->gb);

    free(ggr);
}


gui_grid* gui_grid_create(GtkWidget* grid_container, boxyseq* bs)
{
    gui_grid* ggr;
    GtkWidget* tmp;
    GtkAdjustment* adj;

    ggr = malloc(sizeof(*ggr));

    if (!ggr)
    {
        WARNING("failed to allocate memory for gui grid editor\n");
        return 0;
    }

    ggr->bs = bs;
    ggr->jd = boxyseq_jackdata(bs);
    ggr->action = 0;
    ggr->action_grb = 0;
    ggr->object = GRID_OBJECT_BOUNDARY;

    ggr->gb = gui_box_create(grid_container, 128, 128);
    gui_box_connect_expose_event(ggr->gb, gui_grid_expose_event, ggr);
    gui_box_connect_button_press_event(ggr->gb,
                                       gui_grid_button_press_event, ggr);
    gui_box_connect_button_release_event(ggr->gb,
                                         gui_grid_button_release_event,
                                         ggr );
    gui_box_connect_motion_event(ggr->gb, gui_grid_motion_event, ggr);
    ggr->drawing_area = gui_box_drawing_area(ggr->gb);

    ggr->timeout_id = g_timeout_add(33,
                                    (GtkFunction)gui_grid_timed_updater,
                                    ggr);
    return ggr;
}


void gui_grid_connect_zoom_in_button(gui_grid* ggr, GtkWidget* button)
{
    gui_box_connect_zoom_in_button(ggr->gb, button);
}


void gui_grid_connect_zoom_out_button(gui_grid* ggr, GtkWidget* button)
{
    gui_box_connect_zoom_out_button(ggr->gb, button);
}


void gui_grid_boundary_event_play(gui_grid* ggr)
{
    if (!ggr->action_grb)
        return;

    grbound_event_play(ggr->action_grb);
    grbound_update_rt_data(ggr->action_grb);
}


void gui_grid_boundary_event_block(gui_grid* ggr)
{
    if (!ggr->action_grb)
        return;

    grbound_event_block(ggr->action_grb);
    grbound_update_rt_data(ggr->action_grb);
}


void gui_grid_boundary_event_ignore(gui_grid* ggr)
{
    if (!ggr->action_grb)
        return;

    grbound_event_ignore(ggr->action_grb);
    grbound_update_rt_data(ggr->action_grb);
}


void gui_grid_direction(gui_grid* ggr, int dir)
{
    int bx, by, bw, bh;

    if (!ggr->action_grb)
        return;

    grbound_fsbound_get(ggr->action_grb, &bx, &by, &bw, &bh);

    switch(dir)
    {
    case LEFT:  bx--; break;
    case RIGHT: bx++; break;
    case UP:    by--; break;
    case DOWN:  by++; break;

    default:    return;
    }

    grbound_fsbound_set(ggr->action_grb, bx, by, bw, bh);
    grbound_update_rt_data(ggr->action_grb);
}

void gui_grid_order_boundary(gui_grid* ggr, int dir)
{
    if (!ggr->action_grb)
        return;

    grbound_manager* grbman;
    grbman = boxyseq_grbound_manager(ggr->bs);
    grbound_manager_grbound_order(grbman, ggr->action_grb, dir);
    grbound_manager_update_rt_data(grbman);
}
