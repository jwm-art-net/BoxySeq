#include "gui_main.h"

#include "boxy_sequencer.h"
#include "debug.h"
#include "event_buffer.h"
#include "pattern.h"


#include <stdlib.h>
#include <time.h>

static GtkWidget*   window =        0;
static GtkWidget*   timelabel =     0;
static GtkWidget*   play_button =   0;
static GtkWidget*   rew_button =    0;
static GtkWidget*   rew_img =       0;
static GtkWidget*   play_img =      0;
static GtkWidget*   stop_img =      0;
static GtkWidget*   drawing_area =  0;

static guint    pos_idle_id = 0;
static int      gui_jack_rolling = 0;



static void gui_quit(void)
{
    gtk_main_quit();
}


struct bsdata
{
    jackdata*   jd;

    evbuf*      note_on_buf;
    evbuf*      note_off_buf;
    evbuf*      unplace_buf;

    boxyseq*    bs;
    plist*      evlist;
};


void bsgui_box(cairo_t* cr, int x, int y, int w, int h)
{
    cairo_rectangle(cr, x, y, w, h);

    cairo_set_source_rgba(cr, 1, 0, 0, 0.5);
    cairo_fill_preserve (cr);
    cairo_set_source_rgba(cr, 0.5, 0, 0, 0.5);
    cairo_set_line_width (cr, 1.0);
    cairo_stroke (cr);

}

static gboolean timed_updater(GtkWidget *widget)
{
    gtk_widget_queue_draw(window);
    gtk_widget_queue_draw(drawing_area);
    return TRUE;
}


static gboolean on_expose_event(GtkWidget *widget, GdkEventExpose *gdkevent, gpointer data)
{
    struct bsdata* bsd = (struct bsdata*)data;
    jack_position_t pos;
    jack_transport_state_t jstate;

    jstate = jackdata_transport_state(bsd->jd, &pos);

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

    cairo_t* cr = gdk_cairo_create(drawing_area->window);

    event evin;
    event* ev;
    lnode* ln;
    lnode* nln;

    while(evbuf_read(bsd->unplace_buf, &evin))
    {
        ln = plist_head(bsd->evlist);

        if (evin.box_x == -1
         && evin.box_y == -1
         && evin.box_width  == -1
         && evin.box_height == -1)
        {
            plist_select_all(bsd->evlist, 1);
            plist_delete(bsd->evlist, 1);
            ln = 0;
        }

        while (ln)
        {
            ev = lnode_data(ln);
            nln = lnode_next(ln);

            if (ev->box_x == evin.box_x
             && ev->box_y == evin.box_y
             && ev->box_width  == evin.box_width
             && ev->box_height == evin.box_height)
            {
                plist_unlink_free(bsd->evlist, ln);
                nln = 0;
            }

            ln = nln;
        }
    }

    ln = plist_head(bsd->evlist);

    while(ln)
    {
        ev = (event*)lnode_data(ln);

        bsgui_box(cr,   ev->box_x * 4,      ev->box_y * 4,
                        ev->box_width * 4,  ev->box_height * 4);

        ln = lnode_next(ln);
    }

    while(evbuf_read(bsd->note_on_buf, &evin))
        plist_add_event_copy(bsd->evlist, &evin);

    ln = plist_head(bsd->evlist);

    while(ln)
    {
        ev = (event*)lnode_data(ln);

        bsgui_box(cr,   ev->box_x * 4,      ev->box_y * 4,
                        ev->box_width * 4,  ev->box_height * 4);

        ln = lnode_next(ln);
    }

    cairo_destroy(cr);

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


int gui_init(int* argc, char*** argv, boxyseq* bs, jackdata* jd)
{
    struct bsdata* bsd = malloc(sizeof(*bsd));

    if (!bsd)
        return FALSE;

    bsd->bs = bs;

    bsd->note_on_buf =  boxyseq_gui_note_on_buf(bs);
    bsd->note_off_buf = boxyseq_gui_note_off_buf(bs);
    bsd->unplace_buf =  boxyseq_gui_unplace_buf(bs);

    bsd->jd = jd;

    bsd->evlist = plist_new();

    GtkWidget* tmp;
    GtkWidget* vbox;
    GtkWidget* hbox;
    GtkWidget* evbox;

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


    tmp = gtk_drawing_area_new();
    gtk_widget_set_size_request(tmp, 800, 600);

    gtk_box_pack_start(GTK_BOX(vbox), tmp, TRUE, TRUE, 0);
    gtk_widget_show(tmp);

    drawing_area = tmp;

    g_signal_connect(GTK_OBJECT(tmp), "expose_event",
                       G_CALLBACK(on_expose_event), bsd);


/*
    gtk_widget_set_events (tmp, GDK_BUTTON_PRESS_MASK
                           | GDK_BUTTON_RELEASE_MASK
                           | GDK_POINTER_MOTION_MASK
                           | GDK_ENTER_NOTIFY_MASK
                           | GDK_LEAVE_NOTIFY_MASK
                           | GDK_EXPOSURE_MASK);
    gtk_widget_set_size_request(tmp, 
                        (img->user_width >= MIN_WINDOW_WIDTH)
                        ? img->user_width : MIN_WINDOW_WIDTH,
                        img->user_height);
    gtk_box_pack_start(GTK_BOX(vbox), tmp, TRUE, TRUE, 0);
    gtk_widget_show(tmp);
    g_signal_connect(GTK_OBJECT(tmp), "button_press_event",
                        G_CALLBACK(button_press_event), NULL);
    g_signal_connect(GTK_OBJECT(tmp), "button_release_event",
                        G_CALLBACK(button_release_event), NULL);
    g_signal_connect(GTK_OBJECT(tmp), "expose_event",
                       G_CALLBACK(expose_event), img);
    g_signal_connect(GTK_OBJECT(tmp), "motion_notify_event",
                       G_CALLBACK(motion_event), NULL);
    g_signal_connect(GTK_OBJECT(tmp), "enter_notify_event",
                       G_CALLBACK(notify_enter_leave_event), NULL);
    g_signal_connect(GTK_OBJECT(tmp), "leave_notify_event",
                       G_CALLBACK(notify_enter_leave_event), NULL);


    img->drawing_area = drawing_area = tmp;
*/



    gtk_widget_show(window);

    /* add a callback to update the buttons and the BBT counter */
    pos_idle_id = g_timeout_add(10, (GtkFunction)timed_updater, bsd);

    gtk_main();

    /* free the play & stop images */
    g_object_unref(stop_img);
    g_object_unref(play_img);

    free(bsd);

    return TRUE;
}

