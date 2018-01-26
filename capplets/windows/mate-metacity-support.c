/* mate-metacity-support.c
 * Copyright (C) 2014 Stefano Karapetsas
 *
 * Written by: Stefano Karapetsas <stefano@karapetsas.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#define METACITY_SCHEMA "org.gnome.metacity"
#define COMPOSITING_KEY "compositing-manager"

void
mate_metacity_config_tool ()
{
    GSettings *settings;
    GtkDialog *dialog;
    GtkWidget *vbox;
    GtkWidget *widget;
    gchar *str;

    settings = g_settings_new (METACITY_SCHEMA);

    dialog = GTK_DIALOG (gtk_dialog_new_with_buttons(_("Metacity Preferences"),
                                                     NULL,
                                                     GTK_DIALOG_MODAL,
                                                     "gtk-close",
                                                     GTK_RESPONSE_CLOSE,
                                                     NULL));
    gtk_window_set_icon_name (GTK_WINDOW (dialog), "preferences-system-windows");
    gtk_window_set_default_size (GTK_WINDOW (dialog), 350, 150);

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);

    str = g_strdup_printf ("<b>%s</b>", _("Compositing Manager"));
    widget = gtk_label_new (str);
    g_free (str);
    gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
    gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
    gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

    widget = gtk_check_button_new_with_label (_("Enable software _compositing window manager"));
    gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
    g_settings_bind (settings, COMPOSITING_KEY, widget, "active", G_SETTINGS_BIND_DEFAULT);

    gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (dialog)), vbox, TRUE, TRUE, 0);

    g_signal_connect (dialog, "response", G_CALLBACK (gtk_main_quit), dialog);
    gtk_widget_show_all (GTK_WIDGET (dialog));
    gtk_main ();

    g_object_unref (settings);
}
