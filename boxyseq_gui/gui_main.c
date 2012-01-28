#include "gui_main.h"

#include "boxy_sequencer.h"
#include "debug.h"
#include "event_buffer.h"
#include "gui_grid.h"
#include "grid_boundary.h"

#include "include/gui_main_editor.h"


#include <stdlib.h>
#include <time.h>


#include <gdk/gdkkeysyms.h>


static gui_main* _gui;


static void gui_quit(void)
{
    gtk_main_quit();
}


static gboolean idle_update_position(jackdata* jd)
{
    jack_position_t pos;
    jack_transport_state_t jstate = jackdata_transport_state(jd, &pos);

    if (jstate == JackTransportStopped && _gui->jack_rolling)
    {
        gtk_button_set_image(GTK_BUTTON(_gui->play_button),
                                        _gui->play_img);
        _gui->jack_rolling = 0;
    }
    else if (jstate == JackTransportRolling && !_gui->jack_rolling)
    {
        gtk_button_set_image(GTK_BUTTON(_gui->play_button),
                                        _gui->stop_img);
        _gui->jack_rolling = 1;
    }

    char buf[40] = "BBT ----:--.----";

    if (pos.valid & JackPositionBBT)
        snprintf(buf, 40, "BBT %4d:%2d.%04d",
                            pos.bar, pos.beat, pos.tick);

    gtk_label_set_text(GTK_LABEL(_gui->timelabel), buf);

    return TRUE;
}


static gboolean gui_boxyseq_update(boxyseq* bs)
{
    boxyseq_ui_collect_events(bs);
    return TRUE;
}


static gboolean gui_transport_rewind(   GtkWidget *widget,
                                        gpointer data       )
{
    jackdata* jd = (jackdata*)data;
    jackdata_transport_rewind(jd);
    return TRUE;
}


static gboolean gui_transport_play(     GtkWidget *widget,
                                        gpointer data       )
{
    jackdata* jd = (jackdata*)data;

    switch(jackdata_transport_state(jd, NULL))
    {
    case JackTransportStopped:
        jackdata_transport_play(jd);
        break;
    case JackTransportRolling:
        jackdata_transport_stop(jd);
        break;
    default:
        return TRUE;
    }

    return TRUE;
}


static gboolean gui_key_press_event(GtkWidget *widget,
                                    GdkEventKey *keyevent,
                                    gpointer data)
{
    switch (keyevent->keyval)
    {
    case GDK_KEY_r:
        gui_grid_boundary_flags_toggle(_gui->ggr, FSPLACE_ROW_SMART);
        break;

    case GDK_KEY_h:
        gui_grid_boundary_flags_toggle(_gui->ggr, FSPLACE_LEFT_TO_RIGHT);
        break;

    case GDK_KEY_v:
        gui_grid_boundary_flags_toggle(_gui->ggr, FSPLACE_TOP_TO_BOTTOM);
        break;

    case GDK_KEY_t:
        gui_grid_boundary_flags_set(_gui->ggr, FSPLACE_TOP_TO_BOTTOM);
        break;

    case GDK_KEY_b:
        gui_grid_boundary_flags_unset(_gui->ggr, FSPLACE_TOP_TO_BOTTOM);
        break;

    case GDK_KEY_m:
        gui_grid_boundary_event_toggle_play(_gui->ggr);
        break;

    case GDK_KEY_x:
        gui_grid_boundary_event_toggle_process(_gui->ggr);
        break;

    case GDK_Up:
    case GDK_KP_Up:
        gui_grid_direction(_gui->ggr, UP);
        break;

    case GDK_Down:
    case GDK_KP_Down:
        gui_grid_direction(_gui->ggr, DOWN);
        break;

    case GDK_Left:
    case GDK_KP_Left:
        gui_grid_direction(_gui->ggr, LEFT);
        break;

    case GDK_Right:
    case GDK_KP_Right:
        gui_grid_direction(_gui->ggr, RIGHT);
        break;

    case GDK_KP_Add:
        gui_grid_order_boundary(_gui->ggr, 1);
        break;

    case GDK_KP_Subtract:
        gui_grid_order_boundary(_gui->ggr, -1);
        break;
    }
    
    return TRUE;
}


int gui_init(int* argc, char*** argv, boxyseq* bs)
{

    GtkWidget* tmp;
    GtkWidget* vbox;
    GtkWidget* hbox;
    GtkWidget* evbox;

    if (!gtk_init_check(argc, argv))
    {
        WARNING("Failed to initilize GUI!\n");
        return FALSE;
    }

    _gui = malloc(sizeof(*_gui));

    if (!_gui)
    {
        WARNING("Failed to create main gui\n");
        return FALSE;
    }

    _gui->bs = bs;
    jackdata* jd = boxyseq_jackdata(bs);


    _gui->jack_rolling = 0;

    /* main window */

    _gui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_resizable(GTK_WINDOW(_gui->window), TRUE);

    g_signal_connect(GTK_OBJECT(_gui->window), "key_press_event",
                       G_CALLBACK(gui_key_press_event), NULL);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(_gui->window), vbox);
    gtk_widget_show(vbox);

    g_signal_connect(   GTK_OBJECT(_gui->window),
                        "destroy",
                        G_CALLBACK(gui_quit),
                        NULL                );

    /* toolbar stuff */

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* transport buttons */

    _gui->rewind_img =
        gtk_image_new_from_stock(   GTK_STOCK_MEDIA_PREVIOUS,
                                    GTK_ICON_SIZE_SMALL_TOOLBAR );
    _gui->play_img =
        gtk_image_new_from_stock(   GTK_STOCK_MEDIA_PLAY,
                                    GTK_ICON_SIZE_SMALL_TOOLBAR );
    _gui->stop_img =
        gtk_image_new_from_stock(   GTK_STOCK_MEDIA_STOP,
                                    GTK_ICON_SIZE_SMALL_TOOLBAR );

    /*  convert play_img and stop_img from floating references which
        are deleted when no longer in use, to a normal reference.
        this prevents either of the images being deleted when we
        swap the play/stop button images. unref'd after gtk_main.
    */
    g_object_ref_sink(_gui->play_img);
    g_object_ref_sink(_gui->stop_img);

    _gui->rewind_button = gtk_button_new();
    gtk_button_set_focus_on_click(  GTK_BUTTON(_gui->rewind_button),
                                                        FALSE);
    gtk_button_set_relief(          GTK_BUTTON(_gui->rewind_button),
                                                        GTK_RELIEF_NONE);
    gtk_button_set_image(           GTK_BUTTON(_gui->rewind_button),
                                                        _gui->rewind_img);
    gtk_box_pack_start(GTK_BOX(hbox), _gui->rewind_button,
                                                        FALSE, FALSE, 0);
    gtk_widget_show(_gui->rewind_button);
    g_signal_connect(   GTK_OBJECT(_gui->rewind_button),
                        "clicked",
                        G_CALLBACK(gui_transport_rewind),
                        jd                        );

    _gui->play_button = gtk_button_new();
    gtk_button_set_focus_on_click(  GTK_BUTTON(_gui->play_button),
                                                        FALSE);
    gtk_button_set_relief(          GTK_BUTTON(_gui->play_button),
                                                        GTK_RELIEF_NONE);
    gtk_button_set_image(           GTK_BUTTON(_gui->play_button),
                                                        _gui->play_img);
    gtk_box_pack_start(GTK_BOX(hbox), _gui->play_button, FALSE, FALSE, 0);
    gtk_widget_show(_gui->play_button);
    g_signal_connect(   GTK_OBJECT(_gui->play_button),
                        "clicked",
                        G_CALLBACK(gui_transport_play),
                        jd                        );

    tmp = gtk_vseparator_new();
    gtk_widget_show(tmp);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);

    GdkColor colbg = {0,0,0,0};
    evbox = gtk_event_box_new();
    gtk_widget_modify_bg(evbox, GTK_STATE_NORMAL, &colbg);
    gtk_widget_show(evbox);
    gtk_box_pack_start(GTK_BOX(hbox), evbox, TRUE, FALSE, 0);

    GdkColor colfg = {0,0,0xffff,0};
    _gui->timelabel = tmp = gtk_label_new("--:--.----");
    gtk_misc_set_padding(GTK_MISC(tmp), 4, 0);
    gtk_widget_modify_fg(tmp, GTK_STATE_NORMAL, &colfg);
    gtk_widget_show(tmp);
    gtk_container_add(GTK_CONTAINER(evbox), tmp);

    tmp = gtk_button_new();
    gtk_button_set_focus_on_click(GTK_BUTTON(tmp), FALSE);
    gtk_button_set_relief(GTK_BUTTON(tmp), GTK_RELIEF_NONE);
    gtk_button_set_image(GTK_BUTTON(tmp), 
                gtk_image_new_from_stock(GTK_STOCK_ZOOM_OUT,
                                        GTK_ICON_SIZE_SMALL_TOOLBAR));
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    _gui->zoom_out_button = tmp;

    tmp = gtk_button_new();
    gtk_button_set_focus_on_click(GTK_BUTTON(tmp), FALSE);
    gtk_button_set_relief(GTK_BUTTON(tmp), GTK_RELIEF_NONE);
    gtk_button_set_image(GTK_BUTTON(tmp), 
                gtk_image_new_from_stock(GTK_STOCK_ZOOM_IN,
                                        GTK_ICON_SIZE_SMALL_TOOLBAR));
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    _gui->zoom_in_button = tmp;

    tmp = gtk_hseparator_new();
    gtk_widget_show(tmp);
    gtk_box_pack_start(GTK_BOX(vbox), tmp, FALSE, FALSE, 0);

    _gui->pos_timeout_id = g_timeout_add(100,
                                (GtkFunction)idle_update_position,
                                jd);

    _gui->bs_timeout_id = g_timeout_add(10,
                                    (GtkFunction)gui_boxyseq_update,
                                    bs);

    _gui->ggr = gui_grid_create(vbox, bs);

    gui_grid_connect_zoom_in_button(_gui->ggr, _gui->zoom_in_button);
    gui_grid_connect_zoom_out_button(_gui->ggr, _gui->zoom_out_button);

    gtk_widget_show(_gui->window);

    gtk_main();

    gui_grid_destroy(_gui->ggr);

    /* free the play & stop images */
    g_object_unref(_gui->stop_img);
    g_object_unref(_gui->play_img);

    return TRUE;
}

