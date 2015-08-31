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
#include "stdio.h"


static void
show_handlebar (AppearanceData *data, gboolean show)
{
  GtkWidget *handlebox = appearance_capplet_get_widget (data, "toolbar_handlebox");
  GtkWidget *toolbar = appearance_capplet_get_widget (data, "toolbar_toolbar");
  GtkWidget *align = appearance_capplet_get_widget (data, "toolbar_align");

  g_object_ref (handlebox);
  g_object_ref (toolbar);

  if (gtk_bin_get_child (GTK_BIN (align)))
    gtk_container_remove (GTK_CONTAINER (align), gtk_bin_get_child (GTK_BIN (align)));

  if (gtk_bin_get_child (GTK_BIN (handlebox)))
    gtk_container_remove (GTK_CONTAINER (handlebox), gtk_bin_get_child (GTK_BIN (handlebox)));

  if (show) {
    gtk_container_add (GTK_CONTAINER (align), handlebox);
    gtk_container_add (GTK_CONTAINER (handlebox), toolbar);
    g_object_unref (handlebox);
  } else {
    gtk_container_add (GTK_CONTAINER (align), toolbar);
  }

  g_object_unref (toolbar);
}

#if !GTK_CHECK_VERSION (3, 10, 0)
static void
set_toolbar_style (AppearanceData *data, const char *value)
{
  static const GtkToolbarStyle gtk_toolbar_styles[] =
    { GTK_TOOLBAR_BOTH, GTK_TOOLBAR_BOTH_HORIZ, GTK_TOOLBAR_ICONS, GTK_TOOLBAR_TEXT };

  int enum_val = gtk_combo_box_get_active((GtkComboBox *)
			 appearance_capplet_get_widget (data, "toolbar_style_select"));

  gtk_toolbar_set_style (GTK_TOOLBAR (appearance_capplet_get_widget (data, "toolbar_toolbar")),
			 gtk_toolbar_styles[enum_val]);
}
#endif

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
    GtkImageMenuItem *item = GTK_IMAGE_MENU_ITEM (appearance_capplet_get_widget (data, *name));
    GtkWidget *image;

    if (value) {
      image = g_object_get_data (G_OBJECT (item), "image");
      if (image) {
	gtk_image_menu_item_set_image (item, image);
	g_object_unref (image);
      }
    } else {
      image = gtk_image_menu_item_get_image (item);
      g_object_set_data (G_OBJECT (item), "image", image);
      g_object_ref (image);
      gtk_image_menu_item_set_image (item, NULL);
    }
  }
}

/** GConf Callbacks and Conversions **/

#if !GTK_CHECK_VERSION (3, 10, 0)
static gboolean
toolbar_to_widget (GValue *value, GVariant *variant, gpointer user_data)
{
  const gchar *val = g_variant_get_string(variant, NULL);
  gint i = 0;

  if (g_strcmp0(val, "both-horiz") == 0 || g_strcmp0(val, "both_horiz") == 0)
    i = 1;
  else if (g_strcmp0(val, "icons") == 0)
    i = 2;
  else if (g_strcmp0(val, "text") == 0)
    i = 3;

  g_value_set_int(value, i);

  return TRUE;
}

static GVariant *
toolbar_from_widget (const GValue *value,
			  const GVariantType *expected_type,
			  gpointer user_data)
{
  static const char *gtk_toolbar_styles_str[] = {
    "both", "both-horiz", "icons", "text" };

  gint index = g_value_get_int(value);
  return g_variant_new_string(gtk_toolbar_styles_str[index]);
}

static void
toolbar_style_cb (GSettings *settings,
		     gchar *key,
		     AppearanceData      *data)
{
  set_toolbar_style (data, g_settings_get_string (settings, key));
}
#endif

static void
menus_have_icons_cb (GSettings *settings,
		     gchar *key,
		     AppearanceData      *data)
{
  set_have_icons (data, g_settings_get_boolean (settings, key));
}

static void
toolbar_detachable_cb (GSettings *settings,
		     gchar *key,
		     AppearanceData *data)
{
  show_handlebar (data, g_settings_get_boolean (settings, key));
}

/** GUI Callbacks **/

static gint
button_press_block_cb (GtkWidget *toolbar,
		       GdkEvent  *event,
		       gpointer   data)
{
  return TRUE;
}

/** Public Functions **/

void
ui_init (AppearanceData *data)
{
  GtkWidget* widget;

#if GTK_CHECK_VERSION (3, 10, 0)
  GtkWidget* container = appearance_capplet_get_widget(data, "vbox24");

  // Remove menu accels and toolbar style toggles for new GTK versions
  gtk_container_remove((GtkContainer *) container,
			 appearance_capplet_get_widget(data, "menu_accel_toggle"));
  gtk_container_remove((GtkContainer *) container,
			 appearance_capplet_get_widget(data, "hbox11"));
#endif

  widget = appearance_capplet_get_widget(data, "menu_icons_toggle");
	g_settings_bind (data->interface_settings,
			 MENU_ICONS_KEY,
			 G_OBJECT (widget),
			 "active",
			 G_SETTINGS_BIND_DEFAULT);
  g_signal_connect (data->interface_settings, "changed::" MENU_ICONS_KEY,
                      G_CALLBACK (menus_have_icons_cb), data);

  set_have_icons (data,
    g_settings_get_boolean (data->interface_settings,
			   MENU_ICONS_KEY));

#if !GTK_CHECK_VERSION (3, 10, 0)
  widget = appearance_capplet_get_widget(data, "menu_accel_toggle");
	g_settings_bind (data->interface_settings,
			 ACCEL_CHANGE_KEY,
			 G_OBJECT (widget),
			 "active",
			 G_SETTINGS_BIND_DEFAULT);

  widget = appearance_capplet_get_widget(data, "toolbar_style_select");
	g_settings_bind_with_mapping (data->interface_settings,
			 TOOLBAR_STYLE_KEY,
			 G_OBJECT (widget),
			 "active",
			 G_SETTINGS_BIND_DEFAULT,
			 toolbar_to_widget,
			 toolbar_from_widget,
			 data,
			 NULL);

  g_signal_connect (data->interface_settings, "changed::" TOOLBAR_STYLE_KEY,
		    (GCallback) toolbar_style_cb, data);

  char* toolbar_style;

  toolbar_style = g_settings_get_string
    (data->interface_settings,
     TOOLBAR_STYLE_KEY);
  set_toolbar_style (data, toolbar_style);
  g_free (toolbar_style);
#endif

  g_signal_connect (appearance_capplet_get_widget (data, "toolbar_handlebox"),
		    "button_press_event",
		    (GCallback) button_press_block_cb, NULL);

  show_handlebar (data,
    g_settings_get_boolean (data->interface_settings,
			   TOOLBAR_DETACHABLE_KEY));

  /* no ui for detachable toolbars */
  g_signal_connect (data->interface_settings,
			   "changed::" TOOLBAR_DETACHABLE_KEY, (GCallback) toolbar_detachable_cb, data);
}
