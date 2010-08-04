#include "gui_pattern.h"

#include "debug.h"


#include <gtk/gtk.h>


#include <stdlib.h>


struct gui_pattern_editor
{
    GtkWidget*  window;
    GtkWidget*  drawing_area;

    pattern*    pat;
};


static void gui_pattern_free(GtkWidget* widget, gui_pattern* gp)
{
    if (!gp)
    {
        WARNING("can't free nothing\n");
        return;
    }

    free(gp);
}

void gui_pattern_show(gui_pattern** gp_ptr, pattern* pat)
{
    gui_pattern* gp;

    if (*gp_ptr)
        return;

    gp = malloc(sizeof(*gp));

    if (!gp)
        goto fail0;

    gp->pat = pat;
    *gp_ptr = gp;

    gp->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(gp->window), "Pattern Editor");
    gtk_window_set_resizable(GTK_WINDOW(gp->window), TRUE);

    g_signal_connect(GTK_OBJECT(gp->window), "destroy",
                     G_CALLBACK(gui_pattern_free), gp);

    g_signal_connect(GTK_OBJECT(gp->window), "destroy",
                     G_CALLBACK(gtk_widget_destroyed), gp_ptr);

    gtk_widget_show_all(gp->window);

    *gp_ptr = gp;
    return;

fail0:
    WARNING("failed to allocate memory for gui pattern editor\n");
    return;
}

