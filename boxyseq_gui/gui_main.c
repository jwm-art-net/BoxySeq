#include "gui_main.h"

#include "boxy_sequencer.h"
#include "debug.h"
#include "event_buffer.h"
#include "gui_grid.h"
#include "gui_pattern.h"



#include <stdlib.h>
#include <time.h>


static boxyseq* _bs = 0;

static GtkWidget*   window =        0;
static GtkWidget*   timelabel =     0;
static GtkWidget*   play_button =   0;
static GtkWidget*   rew_button =    0;
static GtkWidget*   rew_img =       0;
static GtkWidget*   play_img =      0;
static GtkWidget*   stop_img =      0;

static GtkWidget*   test_button =   0;


static guint    pos_timeout_id = 0;
static guint    bs_timeout_id = 0;

static int      gui_jack_rolling = 0;


static gui_pattern* gui_pattern_edit = 0;
static gui_grid*    gui_grid_edit = 0;


static void gui_quit(void)
{
    gtk_main_quit();
}


static gboolean idle_update_position(jackdata* jd)
{
    jack_position_t pos;
    jack_transport_state_t jstate = jackdata_transport_state(jd, &pos);

    if (jstate == JackTransportStopped && gui_jack_rolling)
    {
        gtk_button_set_image(GTK_BUTTON(play_button), play_img);
        gui_jack_rolling = 0;
    }
    else if (jstate == JackTransportRolling && !gui_jack_rolling)
    {
        gtk_button_set_image(GTK_BUTTON(play_button), stop_img);
        gui_jack_rolling = 1;
    }

    char buf[40] = "BBT ----:--.----";

    if (pos.valid & JackPositionBBT)
        snprintf(buf, 40, "BBT %4d:%2d.%04d",
                            pos.bar, pos.beat, pos.tick);

    gtk_label_set_text(GTK_LABEL(timelabel), buf);

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


static void gui_do_pattern_show(void)
{
    gui_pattern_show(&gui_pattern_edit, 0);
}


static void gui_do_grid_show(void)
{
    gui_grid_show(&gui_grid_edit, 0, _bs);
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

    _bs = bs;
    jackdata* jd = boxyseq_jackdata(bs);

    /* main window */

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show(vbox);

    g_signal_connect(   GTK_OBJECT(window),
                        "destroy",
                        G_CALLBACK(gui_quit),
                        NULL                );

    /* toolbar stuff */

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* transport buttons */

    rew_img = gtk_image_new_from_stock( GTK_STOCK_MEDIA_PREVIOUS,
                                        GTK_ICON_SIZE_SMALL_TOOLBAR );
    play_img = gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY,
                                        GTK_ICON_SIZE_SMALL_TOOLBAR );
    stop_img = gtk_image_new_from_stock(GTK_STOCK_MEDIA_STOP,
                                        GTK_ICON_SIZE_SMALL_TOOLBAR );

    /*  convert play_img and stop_img from floating references which
        are deleted when no longer in use, to a normal reference.
        this prevents either of the images being deleted when we
        swap the play/stop button images. unref'd after gtk_main.
    */
    g_object_ref_sink(play_img);
    g_object_ref_sink(stop_img);

    rew_button = gtk_button_new();
    gtk_button_set_focus_on_click(GTK_BUTTON(rew_button), FALSE);
    gtk_button_set_relief(GTK_BUTTON(rew_button), GTK_RELIEF_NONE);
    gtk_button_set_image(GTK_BUTTON(rew_button), rew_img);
    gtk_box_pack_start(GTK_BOX(hbox), rew_button, FALSE, FALSE, 0);
    gtk_widget_show(rew_button);
    g_signal_connect(   GTK_OBJECT(rew_button),
                        "clicked",
                        G_CALLBACK(gui_transport_rewind),
                        jd                        );

    play_button = gtk_button_new();
    gtk_button_set_focus_on_click(GTK_BUTTON(play_button), FALSE);
    gtk_button_set_relief(GTK_BUTTON(play_button), GTK_RELIEF_NONE);
    gtk_button_set_image(GTK_BUTTON(play_button), play_img);
    gtk_box_pack_start(GTK_BOX(hbox), play_button, FALSE, FALSE, 0);
    gtk_widget_show(play_button);
    g_signal_connect(   GTK_OBJECT(play_button),
                        "clicked",
                        G_CALLBACK(gui_transport_play),
                        jd                        );


    test_button = gtk_button_new_with_label("grid");
    gtk_button_set_focus_on_click(GTK_BUTTON(test_button), FALSE);
    gtk_button_set_relief(GTK_BUTTON(test_button), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(hbox), test_button, FALSE, FALSE, 0);
    gtk_widget_show(test_button);
    g_signal_connect(   GTK_OBJECT(test_button),
                        "clicked",
                        G_CALLBACK(gui_do_grid_show),
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
    timelabel = tmp = gtk_label_new("--:--.----");
    gtk_misc_set_padding(GTK_MISC(tmp), 4, 0);
    gtk_widget_modify_fg(tmp, GTK_STATE_NORMAL, &colfg);
    gtk_widget_show(tmp);
    gtk_container_add(GTK_CONTAINER(evbox), tmp);

    tmp = gtk_hseparator_new();
    gtk_widget_show(tmp);
    gtk_box_pack_start(GTK_BOX(vbox), tmp, FALSE, FALSE, 0);

    pos_timeout_id = g_timeout_add(100,
                                (GtkFunction)idle_update_position,
                                jd);

    bs_timeout_id = g_timeout_add(10,
                                    (GtkFunction)gui_boxyseq_update,
                                    bs);

    gtk_widget_show(window);


    gtk_main();

    /* free the play & stop images */
    g_object_unref(stop_img);
    g_object_unref(play_img);

    return TRUE;
}

