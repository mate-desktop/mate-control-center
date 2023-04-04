/*
 * Copyright (C) 2007 The GNOME Foundation
 * Written by Jonathan Blandford <jrb@gnome.org>
 *            Jens Granseuer <jensgr@gmx.net>
 * All Rights Reserved
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "appearance.h"
#include "appearance-ui.h"

#include <stdio.h>

static void
set_have_icons (AppearanceData *data, gboolean value)
{
    static const char *menu_item_names[] = {
        "menu_item_1",
        "menu_item_2",
        "menu_item_3",
        "menu_item_4",
        "menu_item_5",
        "cut",
        "copy",
        "paste",
        NULL
    };

    const char **name;

    for (name = menu_item_names; *name != NULL; name++) {
        MateImageMenuItem *item = MATE_IMAGE_MENU_ITEM (appearance_capplet_get_widget (data, *name));
        GtkWidget *image;

        if (value) {
            image = g_object_get_data (G_OBJECT (item), "image");
            if (image) {
                mate_image_menu_item_set_image (item, image);
                g_object_unref (image);
            }
        }
        else
        {
            image = mate_image_menu_item_get_image (item);
            g_object_set_data (G_OBJECT (item), "image", image);
            g_object_ref (image);
            mate_image_menu_item_set_image (item, NULL);
        }
    }
}

static void
menus_have_icons_cb (GSettings *settings,
                     gchar *key,
                     AppearanceData      *data)
{
    set_have_icons (data, g_settings_get_boolean (settings, key));
}

/** Public Functions **/

void
ui_init (AppearanceData *data)
{
    /* FIXME maybe just remove that stuff from .ui file */
    GtkWidget* container = appearance_capplet_get_widget(data, "vbox24");

    /* Remove menu accels and toolbar style toggles for new GTK versions */
    gtk_container_remove((GtkContainer *) container,
                         appearance_capplet_get_widget(data, "menu_accel_toggle"));
    gtk_container_remove((GtkContainer *) container,
                         appearance_capplet_get_widget(data, "hbox11"));

    g_settings_bind (data->interface_settings,
                     MENU_ICONS_KEY,
                     gtk_builder_get_object (data->ui, "menu_icons_toggle"),
                     "active",
                     G_SETTINGS_BIND_DEFAULT);
    g_signal_connect (data->interface_settings, "changed::" MENU_ICONS_KEY,
                      G_CALLBACK (menus_have_icons_cb), data);

    set_have_icons (data,
                    g_settings_get_boolean (data->interface_settings,
                                            MENU_ICONS_KEY));

    g_settings_bind (data->interface_settings,
                     BUTTON_ICONS_KEY,
                     gtk_builder_get_object (data->ui, "button_icons_toggle"),
                     "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (data->filechooser_settings,
                     FILECHOOSER_HIDDEN_ITEMS_KEY,
                     gtk_builder_get_object (data->ui, "filechooser_hidden_items_toggle"),
                     "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (data->filechooser_settings,
                     FILECHOOSER_SIZE_COL_KEY,
                     gtk_builder_get_object (data->ui, "filechooser_size_column_toggle"),
                     "active",
                     G_SETTINGS_BIND_DEFAULT);

#if GTK_CHECK_VERSION(3, 24, 12)
    g_settings_bind (data->filechooser_settings,
                     FILECHOOSER_TYPE_COL_KEY,
                     gtk_builder_get_object (data->ui, "filechooser_type_column_toggle"),
                     "active",
                     G_SETTINGS_BIND_DEFAULT);
#endif /* GTK_CHECK_VERSION(3, 24, 12) */

    g_settings_bind (data->filechooser_settings,
                     FILECHOOSER_DIR_FIRST_KEY,
                     gtk_builder_get_object (data->ui, "filechooser_sort_directories_first_toggle"),
                     "active",
                     G_SETTINGS_BIND_DEFAULT);
}
