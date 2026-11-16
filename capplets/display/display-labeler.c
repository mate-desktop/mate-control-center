/* Labels on each monitor naming it, for the Wayland display capplet.
 *
 * Copyright (C) 2026  mate-desktop.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <config.h>

#include <glib/gi18n.h>

#ifdef HAVE_GTK_LAYER_SHELL
#include <gtk-layer-shell/gtk-layer-shell.h>
#endif

#include "display-labeler.h"

#define EDGE_THICKNESS 2
#define PADDING        12
#define RETRY_MS       200
#define RETRIES        10

struct DisplayLabeler
{
    GPtrArray *windows;
    GArray    *heads;
    gboolean   mirror;
    guint      retry;
    int        retries_left;
    gulong     added_id;
    gulong     removed_id;
};

static void place_labels (DisplayLabeler *labeler);

void
display_labeler_get_color (guint index, guint count, GdkRGBA *color)
{
    double r, g, b;

    if (count == 0)
        count = 1;

    gtk_hsv_to_rgb (2.0 / 3 / count * (index % count), 1.0 / 3, 1.0, &r, &g, &b);
    color->red = r;
    color->green = g;
    color->blue = b;
    color->alpha = 1.0;
}

const char *
display_head_get_label (OutputHead *head)
{
    if (head->display_name && head->display_name[0])
        return head->display_name;

    if (head->description && head->description[0])
        return head->description;

    return head->name;
}

static gboolean
on_label_draw (GtkWidget *widget, cairo_t *cr, gpointer data)
{
    GdkRGBA *color = g_object_get_data (G_OBJECT (widget), "color");
    int width = gtk_widget_get_allocated_width (widget);
    int height = gtk_widget_get_allocated_height (widget);

    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_rectangle (cr, EDGE_THICKNESS / 2.0, EDGE_THICKNESS / 2.0,
                     width - EDGE_THICKNESS, height - EDGE_THICKNESS);
    cairo_set_line_width (cr, EDGE_THICKNESS);
    cairo_stroke (cr);

    gdk_cairo_set_source_rgba (cr, color);
    cairo_rectangle (cr, EDGE_THICKNESS, EDGE_THICKNESS,
                     width - EDGE_THICKNESS * 2, height - EDGE_THICKNESS * 2);
    cairo_fill (cr);

    return FALSE;
}

static GdkMonitor *
find_monitor (OutputHead *head)
{
    GdkDisplay *display = gdk_display_get_default ();
    int i, n = gdk_display_get_n_monitors (display);

    for (i = 0; i < n; i++)
    {
        GdkMonitor *monitor = gdk_display_get_monitor (display, i);
        GdkRectangle geometry;

        gdk_monitor_get_geometry (monitor, &geometry);
        if (geometry.x == head->x && geometry.y == head->y && geometry.width > 0)
            return monitor;
    }

    return NULL;
}

static GtkWidget *
make_label (OutputHead *head, GdkMonitor *monitor, gboolean mirror, guint index, guint count)
{
    GtkWidget *window;
    GtkWidget *label;
    GtkCssProvider *provider;
    GdkRGBA *color = g_new (GdkRGBA, 1);
    char *markup;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
    gtk_window_set_accept_focus (GTK_WINDOW (window), FALSE);
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), TRUE);
    gtk_window_set_title (GTK_WINDOW (window), display_head_get_label (head));
    gtk_widget_set_app_paintable (window, TRUE);
    gtk_container_set_border_width (GTK_CONTAINER (window), PADDING + EDGE_THICKNESS);

    display_labeler_get_color (index, count, color);
    g_object_set_data_full (G_OBJECT (window), "color", color, g_free);
    g_signal_connect (window, "draw", G_CALLBACK (on_label_draw), NULL);

#ifdef HAVE_GTK_LAYER_SHELL
    if (gtk_layer_is_supported ())
    {
        gtk_layer_init_for_window (GTK_WINDOW (window));
        gtk_layer_set_namespace (GTK_WINDOW (window), "mate-display-label");
        gtk_layer_set_layer (GTK_WINDOW (window), GTK_LAYER_SHELL_LAYER_OVERLAY);
        gtk_layer_set_keyboard_mode (GTK_WINDOW (window), GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
        gtk_layer_set_anchor (GTK_WINDOW (window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
        gtk_layer_set_anchor (GTK_WINDOW (window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
        gtk_layer_set_monitor (GTK_WINDOW (window), monitor);
    }
#endif

    if (mirror)
        markup = g_markup_printf_escaped ("<b>%s</b>", _("Mirror Screens"));
    else
        markup = g_markup_printf_escaped ("<b>%s</b>\n<small>%s</small>",
                                          display_head_get_label (head), head->name);

    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), markup);
    g_free (markup);

    provider = gtk_css_provider_new ();
    gtk_css_provider_load_from_data (provider, "label { color: black; }", -1, NULL);
    gtk_style_context_add_provider (gtk_widget_get_style_context (label),
                                    GTK_STYLE_PROVIDER (provider),
                                    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref (provider);

    gtk_container_add (GTK_CONTAINER (window), label);

    gtk_widget_show_all (window);

    return window;
}

static void
destroy_windows (DisplayLabeler *labeler)
{
    g_ptr_array_set_size (labeler->windows, 0);
}

static gboolean
retry_place (gpointer data)
{
    DisplayLabeler *labeler = data;

    labeler->retry = 0;
    place_labels (labeler);

    return G_SOURCE_REMOVE;
}

static void
place_labels (DisplayLabeler *labeler)
{
    guint i, count;
    gboolean missing = FALSE;

    destroy_windows (labeler);

    if (labeler->heads == NULL)
        return;

    count = labeler->heads->len;

    for (i = 0; i < labeler->heads->len; i++)
    {
        OutputHead *head = &g_array_index (labeler->heads, OutputHead, i);
        GdkMonitor *monitor;

        if (!head->enabled)
            continue;

        monitor = find_monitor (head);
        if (monitor == NULL)
            missing = TRUE;
        else
            g_ptr_array_add (labeler->windows, make_label (head, monitor, labeler->mirror, i, count));

        if (labeler->mirror)
            break;
    }

    if (missing && labeler->retries_left > 0 && labeler->retry == 0)
    {
        labeler->retries_left--;
        labeler->retry = g_timeout_add (RETRY_MS, retry_place, labeler);
    }
}

static void
on_monitors_changed (GdkDisplay *display, GdkMonitor *monitor, gpointer data)
{
    DisplayLabeler *labeler = data;

    if (labeler->heads == NULL)
        return;

    labeler->retries_left = RETRIES;
    place_labels (labeler);
}

DisplayLabeler *
display_labeler_new (void)
{
    DisplayLabeler *labeler = g_new0 (DisplayLabeler, 1);
    GdkDisplay *display = gdk_display_get_default ();

    labeler->windows = g_ptr_array_new_with_free_func ((GDestroyNotify) gtk_widget_destroy);
    labeler->added_id = g_signal_connect (display, "monitor-added",
                                          G_CALLBACK (on_monitors_changed), labeler);
    labeler->removed_id = g_signal_connect (display, "monitor-removed",
                                            G_CALLBACK (on_monitors_changed), labeler);

    return labeler;
}

void
display_labeler_show (DisplayLabeler *labeler, GArray *heads, gboolean mirror)
{
    display_heads_free (labeler->heads);
    labeler->heads = display_heads_copy (heads);
    labeler->mirror = mirror;
    labeler->retries_left = RETRIES;

    if (labeler->retry)
    {
        g_source_remove (labeler->retry);
        labeler->retry = 0;
    }

    place_labels (labeler);
}

void
display_labeler_hide (DisplayLabeler *labeler)
{
    if (labeler->retry)
    {
        g_source_remove (labeler->retry);
        labeler->retry = 0;
    }

    display_heads_free (labeler->heads);
    labeler->heads = NULL;
    destroy_windows (labeler);
}

void
display_labeler_free (DisplayLabeler *labeler)
{
    GdkDisplay *display = gdk_display_get_default ();

    if (labeler == NULL)
        return;

    display_labeler_hide (labeler);
    g_signal_handler_disconnect (display, labeler->added_id);
    g_signal_handler_disconnect (display, labeler->removed_id);
    g_ptr_array_free (labeler->windows, TRUE);
    g_free (labeler);
}
