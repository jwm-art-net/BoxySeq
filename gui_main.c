#include "gui_main.h"

#include "debug.h"
#include "jack_transport.h"



#include "freespace_state.h"
static freespace* fs = 0;



#include <time.h>

static GtkWidget*   window =        0;
static GtkWidget*   timelabel =     0;
static GtkWidget*   play_button =   0;
static GtkWidget*   rew_button =    0;
static GtkWidget*   rew_img =       0;
static GtkWidget*   play_img =      0;
static GtkWidget*   stop_img =      0;

static guint    pos_idle_id = 0;
static int      gui_jack_rolling = 0;



static void gui_quit(void)
{
    gtk_main_quit();
}


static gboolean idle_update_position(jtransp* tr)
{
    /* use nanosleep to prevent eating the cpu for breakfast... */
    struct timespec req = { .tv_sec = 0, .tv_nsec = 50000000 };
    struct timespec rem = { 0, 0 };

    /* when the GUI is doing *a lot* more work, that value may
        need reducing... on a side note:
        the priority setting which can be used with g_idle_add_full
        is not enough to remove the requirement of nanosleep in
        this case.
    */
    nanosleep(&req, &rem);

    jack_position_t pos;
    jack_transport_state_t jstate = jtransp_state(tr, &pos);

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
        snprintf(buf, 40,   "BBT %4d:%2d.%04d",
                            pos.bar, pos.beat, pos.tick);

    gtk_label_set_text(GTK_LABEL(timelabel), buf);


static int p = 0;
    ++p;
    if (p == 1)
    {
        freespace_dump(fs);
        printf("\n");
        p = 0;
    }


    return TRUE;
}


static gboolean gui_transport_rewind(   GtkWidget *widget,
                                        gpointer data       )
{
    jtransp* tr = (jtransp*)data;
    jtransp_rewind(tr);
    return TRUE;
}


static gboolean gui_transport_play(     GtkWidget *widget,
                                        gpointer data       )
{
    jtransp* tr = (jtransp*)data;

    switch(jtransp_state(tr, NULL))
    {
    case JackTransportStopped:
        jtransp_play(tr);
        break;
    case JackTransportRolling:
        jtransp_stop(tr);
        break;
    default:
        return TRUE;
    }

    return TRUE;
}


int gui_init(int* argc, char*** argv, boxyseq* bs, jmidi* jm)
{

    fs = grid_freespace(boxyseq_grid(bs));

    GtkWidget* tmp;
    GtkWidget* vbox;
    GtkWidget* hbox;
    GtkWidget* evbox;

    jtransp* tr = jmidi_jtransp(jm);

    if (!gtk_init_check(argc, argv))
    {
        WARNING("Failed to initilize GUI!\n");
        return FALSE;
    }

    /* main window */

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_widget_realize(window);

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
                        tr                        );

    play_button = gtk_button_new();
    gtk_button_set_focus_on_click(GTK_BUTTON(play_button), FALSE);
    gtk_button_set_relief(GTK_BUTTON(play_button), GTK_RELIEF_NONE);
    gtk_button_set_image(GTK_BUTTON(play_button), play_img);
    gtk_box_pack_start(GTK_BOX(hbox), play_button, FALSE, FALSE, 0);
    gtk_widget_show(play_button);
    g_signal_connect(   GTK_OBJECT(play_button),
                        "clicked",
                        G_CALLBACK(gui_transport_play),
                        tr                        );

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

    /* add a callback to update the buttons and the BBT counter */
    pos_idle_id = g_idle_add((GtkFunction)idle_update_position, tr);

    gtk_widget_show(window);
    gtk_main();

    /* free the play & stop images */
    g_object_unref(stop_img);
    g_object_unref(play_img);

    return TRUE;
}
