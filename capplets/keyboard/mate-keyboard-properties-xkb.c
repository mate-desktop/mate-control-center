/* -*- mode: c; style: linux -*- */

/* mate-keyboard-properties-xkb.c
 * Copyright (C) 2003-2007 Sergey V. Udaltsov
 *
 * Written by: Sergey V. Udaltsov <svu@gnome.org>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <gdk/gdkx.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

#include "capplet-util.h"

#include "mate-keyboard-properties-xkb.h"

#include <libmatekbd/matekbd-desktop-config.h>

#define XKB_GENERAL_SCHEMA "org.mate.peripherals-keyboard-xkb.general"
#define XKB_KBD_SCHEMA "org.mate.peripherals-keyboard-xkb.kbd"

XklEngine *engine;
XklConfigRegistry *config_registry;

MatekbdKeyboardConfig initial_config;
MatekbdDesktopConfig desktop_config;

GSettings *xkb_general_settings;
GSettings *xkb_kbd_settings;

char *
xci_desc_to_utf8 (XklConfigItem * ci)
{
	char *sd = g_strstrip (ci->description);
	return sd[0] == 0 ? g_strdup (ci->name) : g_strdup (sd);
}

static void
set_model_text (GtkWidget * picker, gchar * value)
{
	XklConfigItem *ci = xkl_config_item_new ();
	char *model = NULL;

	if (value != NULL && value[0] != '\0') {
		model = g_strdup(value);
	}

	if (model == NULL) {
		model = g_strdup(initial_config.model);
		if (model == NULL)
			model = g_strdup("");
	}

	g_snprintf (ci->name, sizeof (ci->name), "%s", model);

	if (xkl_config_registry_find_model (config_registry, ci)) {
		char *d;

		d = xci_desc_to_utf8 (ci);
		gtk_button_set_label (GTK_BUTTON (picker), d);
		g_free (d);
	} else {
		gtk_button_set_label (GTK_BUTTON (picker), _("Unknown"));
	}
	g_object_unref (G_OBJECT (ci));
	g_free (model);
}

static void
model_key_changed (GSettings * settings, gchar * key, GtkBuilder * dialog)
{
	set_model_text (WID ("xkb_model_pick"),
			g_settings_get_string (settings, key));

	enable_disable_restoring (dialog);
}

static void
setup_model_entry (GtkBuilder * dialog)
{
	gchar *value;

	value = g_settings_get_string (xkb_kbd_settings, "model");
	set_model_text (WID ("xkb_model_pick"), value);
	if (value != NULL)
		g_free (value);

	g_signal_connect (xkb_kbd_settings,
					  "changed::model",
					  G_CALLBACK (model_key_changed),
					  dialog);
}

static void
cleanup_xkb_tabs (GtkBuilder * dialog)
{
	matekbd_desktop_config_term (&desktop_config);
	matekbd_keyboard_config_term (&initial_config);
	g_object_unref (G_OBJECT (config_registry));
	config_registry = NULL;
	g_object_unref (G_OBJECT (engine));
	engine = NULL;
	g_object_unref (G_OBJECT (xkb_kbd_settings));
	xkb_kbd_settings = NULL;
	g_object_unref (G_OBJECT (xkb_general_settings));
	xkb_general_settings = NULL;
}

static void
reset_to_defaults (GtkWidget * button, GtkBuilder * dialog)
{
	MatekbdKeyboardConfig empty_kbd_config;

	matekbd_keyboard_config_init (&empty_kbd_config, engine);
	matekbd_keyboard_config_save_to_gsettings (&empty_kbd_config);
	matekbd_keyboard_config_term (&empty_kbd_config);

	g_settings_reset (xkb_general_settings, "default-group");

	/* all the rest is g-s-d's business */
}

static void
chk_separate_group_per_window_toggled (GSettings * settings,
				       gchar * key,
				       GtkBuilder * dialog)
{
	gtk_widget_set_sensitive (WID ("chk_new_windows_inherit_layout"),
				  g_settings_get_boolean (settings, key));
}

static void
chk_new_windows_inherit_layout_toggled (GtkWidget *
					chk_new_windows_inherit_layout,
					GtkBuilder * dialog)
{
	xkb_save_default_group (gtk_toggle_button_get_active
				(GTK_TOGGLE_BUTTON
				 (chk_new_windows_inherit_layout)) ? -1 :
				0);
}

void
setup_xkb_tabs (GtkBuilder * dialog)
{
	GtkWidget *chk_new_windows_inherit_layout =
	    WID ("chk_new_windows_inherit_layout");

	xkb_general_settings = g_settings_new (XKB_GENERAL_SCHEMA);
	xkb_kbd_settings = g_settings_new (XKB_KBD_SCHEMA);

	engine = xkl_engine_get_instance (GDK_DISPLAY_XDISPLAY(gdk_display_get_default()));
	config_registry = xkl_config_registry_get_instance (engine);

	matekbd_desktop_config_init (&desktop_config, engine);
	matekbd_desktop_config_load_from_gsettings (&desktop_config);

	xkl_config_registry_load (config_registry, desktop_config.load_extra_items);

	matekbd_keyboard_config_init (&initial_config, engine);
	matekbd_keyboard_config_load_from_x_initial (&initial_config, NULL);

	setup_model_entry (dialog);

	g_settings_bind (xkb_general_settings,
					 "group-per-window",
					 WID ("chk_separate_group_per_window"),
					 "active",
					 G_SETTINGS_BIND_DEFAULT);

	g_signal_connect (xkb_general_settings,
					 "changed::group-per-window",
					 G_CALLBACK (chk_separate_group_per_window_toggled),
					 dialog);

#ifdef HAVE_X11_EXTENSIONS_XKB_H
	if (strcmp (xkl_engine_get_backend_name (engine), "XKB"))
#endif
		gtk_widget_hide (WID ("xkb_layouts_print"));

	xkb_layouts_prepare_selected_tree (dialog);
	xkb_layouts_fill_selected_tree (dialog);

	gtk_widget_set_sensitive (chk_new_windows_inherit_layout,
				  gtk_toggle_button_get_active
				  (GTK_TOGGLE_BUTTON
				   (WID
				    ("chk_separate_group_per_window"))));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (chk_new_windows_inherit_layout),
				      xkb_get_default_group () < 0);

	xkb_layouts_register_buttons_handlers (dialog);
	g_signal_connect (G_OBJECT (WID ("xkb_reset_to_defaults")),
			  "clicked", G_CALLBACK (reset_to_defaults),
			  dialog);

	g_signal_connect (G_OBJECT (chk_new_windows_inherit_layout),
			  "toggled", (GCallback)
			  chk_new_windows_inherit_layout_toggled, dialog);

	g_signal_connect_swapped (G_OBJECT (WID ("xkb_layout_options")),
				  "clicked",
				  G_CALLBACK (xkb_options_popup_dialog),
				  dialog);

	g_signal_connect_swapped (G_OBJECT (WID ("xkb_model_pick")),
				  "clicked", G_CALLBACK (choose_model),
				  dialog);

	xkb_layouts_register_gsettings_listener (dialog);
	xkb_options_register_gsettings_listener (dialog);

	g_signal_connect (G_OBJECT (WID ("keyboard_dialog")),
			  "destroy", G_CALLBACK (cleanup_xkb_tabs),
			  dialog);

	enable_disable_restoring (dialog);
}

void
enable_disable_restoring (GtkBuilder * dialog)
{
	MatekbdKeyboardConfig gswic;
	gboolean enable;

	matekbd_keyboard_config_init (&gswic, engine);
	matekbd_keyboard_config_load_from_gsettings (&gswic, NULL);

	enable = !matekbd_keyboard_config_equals (&gswic, &initial_config);

	matekbd_keyboard_config_term (&gswic);
	gtk_widget_set_sensitive (WID ("xkb_reset_to_defaults"), enable);
}

void
xkb_save_gslist_as_strv (gchar *schema, gchar *key, GSList *list)
{
	GSettings *settings;
	GArray *array;
	GSList *l;
	array = g_array_new (TRUE, TRUE, sizeof (gchar *));
	for (l = list; l; l = l->next) {
		array = g_array_append_val (array, l->data);
	}
	settings = g_settings_new (schema);
	g_settings_set_strv (settings, key, (const gchar **) array->data);
	g_array_free (array, TRUE);
	g_object_unref (settings);
}

void
xkb_layouts_set_selected_list(GSList *list)
{
	xkb_save_gslist_as_strv (XKB_KBD_SCHEMA, "layouts", list);
}

void
xkb_options_set_selected_list(GSList *list)
{
	xkb_save_gslist_as_strv (XKB_KBD_SCHEMA, "options", list);
}
