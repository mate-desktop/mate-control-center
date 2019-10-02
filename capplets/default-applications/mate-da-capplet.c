/*
 *  Authors: Luca Cavalli <loopback@slackit.org>
 *
 *  Copyright 2005-2006 Luca Cavalli
 *  Copyright 2008 Thomas Wood <thos@gnome.org>
 *  Copyright 2010 Perberos <perberos@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of version 2 of the GNU General Public License
 *  as published by the Free Software Foundation
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include <string.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <cairo-gobject.h>

#include "mate-da-capplet.h"
#include "capplet-util.h"


enum {
	DA_TYPE_WEB_BROWSER,
	DA_TYPE_EMAIL,
	DA_TYPE_TERMINAL,
	DA_TYPE_MEDIA,
	DA_TYPE_VIDEO,
	DA_TYPE_VISUAL,
	DA_TYPE_MOBILITY,
	DA_TYPE_IMAGE,
	DA_TYPE_TEXT,
	DA_TYPE_FILE,
	DA_TYPE_DOCUMENT,
	DA_TYPE_WORD,
	DA_TYPE_SPREADSHEET,
	DA_TYPE_CALCULATOR,
	DA_TYPE_MESSENGER,
	DA_N_COLUMNS
};

/* For combo box */
enum {
	SURFACE_COL,
	TEXT_COL,
	ID_COL,
	ICONAME_COL,
	N_COLUMNS
};

static void
set_changed(GtkComboBox* combo, MateDACapplet* capplet, GList* list, gint type)
{
	guint index;
	GAppInfo* item;

	index = gtk_combo_box_get_active(combo);

	if (index < g_list_length(list))
	{
		item = (GAppInfo*) g_list_nth_data(list, index);

		switch (type)
		{
			case DA_TYPE_WEB_BROWSER:
				g_app_info_set_as_default_for_type(item, "x-scheme-handler/http", NULL);
				g_app_info_set_as_default_for_type(item, "x-scheme-handler/https", NULL);
				g_app_info_set_as_default_for_type(item, "text/html", NULL);
				/* about:config is used by firefox and others */
				g_app_info_set_as_default_for_type(item, "x-scheme-handler/about", NULL);
				break;

			case DA_TYPE_EMAIL:
				g_app_info_set_as_default_for_type(item, "x-scheme-handler/mailto", NULL);
				g_app_info_set_as_default_for_type(item, "application/x-extension-eml", NULL);
				g_app_info_set_as_default_for_type(item, "message/rfc822", NULL);
				break;

			case DA_TYPE_FILE:
				g_app_info_set_as_default_for_type(item, "inode/directory", NULL);
				break;

			case DA_TYPE_TEXT:
				g_app_info_set_as_default_for_type(item, "text/plain", NULL);
				break;

			case DA_TYPE_MEDIA:
				g_app_info_set_as_default_for_type(item, "audio/mpeg", NULL);
				g_app_info_set_as_default_for_type(item, "audio/x-mpegurl", NULL);
				g_app_info_set_as_default_for_type(item, "audio/x-scpls", NULL);
				g_app_info_set_as_default_for_type(item, "audio/x-vorbis+ogg", NULL);
				g_app_info_set_as_default_for_type(item, "audio/x-wav", NULL);
				break;

			case DA_TYPE_VIDEO:
				g_app_info_set_as_default_for_type(item, "video/mp4", NULL);
				g_app_info_set_as_default_for_type(item, "video/mpeg", NULL);
				g_app_info_set_as_default_for_type(item, "video/mp2t", NULL);
				g_app_info_set_as_default_for_type(item, "video/msvideo", NULL);
				g_app_info_set_as_default_for_type(item, "video/quicktime", NULL);
				g_app_info_set_as_default_for_type(item, "video/webm", NULL);
				g_app_info_set_as_default_for_type(item, "video/x-avi", NULL);
				g_app_info_set_as_default_for_type(item, "video/x-flv", NULL);
				g_app_info_set_as_default_for_type(item, "video/x-matroska", NULL);
				g_app_info_set_as_default_for_type(item, "video/x-mpeg", NULL);
				g_app_info_set_as_default_for_type(item, "video/x-ogm+ogg", NULL);
				break;

			case DA_TYPE_IMAGE:
				g_app_info_set_as_default_for_type(item, "image/bmp", NULL);
				g_app_info_set_as_default_for_type(item, "image/gif", NULL);
				g_app_info_set_as_default_for_type(item, "image/jpeg", NULL);
				g_app_info_set_as_default_for_type(item, "image/png", NULL);
				g_app_info_set_as_default_for_type(item, "image/tiff", NULL);
				break;

			case DA_TYPE_DOCUMENT:
				g_app_info_set_as_default_for_type(item, "application/pdf", NULL);
				break;

			case DA_TYPE_WORD:
				g_app_info_set_as_default_for_type(item, "application/vnd.oasis.opendocument.text", NULL);
				g_app_info_set_as_default_for_type(item, "application/rtf", NULL);
				g_app_info_set_as_default_for_type(item, "application/msword", NULL);
				g_app_info_set_as_default_for_type(item, "application/vnd.openxmlformats-officedocument.wordprocessingml.document", NULL);
				break;

			case DA_TYPE_SPREADSHEET:
				g_app_info_set_as_default_for_type(item, "application/vnd.oasis.opendocument.spreadsheet", NULL);
				g_app_info_set_as_default_for_type(item, "application/vnd.ms-excel", NULL);
				g_app_info_set_as_default_for_type(item, "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet", NULL);
				break;

			case DA_TYPE_TERMINAL:
				g_settings_set_string (capplet->terminal_settings, TERMINAL_KEY, g_app_info_get_executable (item));
				break;

			case DA_TYPE_VISUAL:
				g_settings_set_string (capplet->visual_settings, VISUAL_KEY, g_app_info_get_executable (item));
				break;

			case DA_TYPE_MOBILITY:
				g_settings_set_string (capplet->mobility_settings, MOBILITY_KEY, g_app_info_get_executable (item));
				break;

			case DA_TYPE_CALCULATOR:
				g_settings_set_string (capplet->calculator_settings, CALCULATOR_KEY, g_app_info_get_executable (item));
				break;

			case DA_TYPE_MESSENGER:
				g_app_info_set_as_default_for_type(item, "x-scheme-handler/icq", NULL);
				g_app_info_set_as_default_for_type(item, "x-scheme-handler/irc", NULL);
				g_app_info_set_as_default_for_type(item, "x-scheme-handler/ircs", NULL);
				g_app_info_set_as_default_for_type(item, "x-scheme-handler/sip", NULL);
				g_app_info_set_as_default_for_type(item, "x-scheme-handler/xmpp", NULL);
				g_settings_set_string (capplet->messenger_settings, MESSENGER_KEY, g_app_info_get_executable (item));

			default:
				break;
		}
	}
}

static void
close_cb(GtkWidget* window, gint response, MateDACapplet* capplet)
{
	if (response == GTK_RESPONSE_HELP)
	{
		capplet_help(GTK_WINDOW(window), "prefs-preferredapps");
	}
	else
	{
		set_changed(GTK_COMBO_BOX(capplet->web_combo_box), capplet, capplet->web_browsers, DA_TYPE_WEB_BROWSER);
		set_changed(GTK_COMBO_BOX(capplet->mail_combo_box), capplet, capplet->mail_readers, DA_TYPE_EMAIL);
		set_changed(GTK_COMBO_BOX(capplet->file_combo_box), capplet, capplet->file_managers, DA_TYPE_FILE);
		set_changed(GTK_COMBO_BOX(capplet->text_combo_box), capplet, capplet->text_editors, DA_TYPE_TEXT);
		set_changed(GTK_COMBO_BOX(capplet->media_combo_box), capplet, capplet->media_players, DA_TYPE_MEDIA);
		set_changed(GTK_COMBO_BOX(capplet->video_combo_box), capplet, capplet->video_players, DA_TYPE_VIDEO);
		set_changed(GTK_COMBO_BOX(capplet->term_combo_box), capplet, capplet->terminals, DA_TYPE_TERMINAL);
		set_changed(GTK_COMBO_BOX(capplet->visual_combo_box), capplet, capplet->visual_ats, DA_TYPE_VISUAL);
		set_changed(GTK_COMBO_BOX(capplet->mobility_combo_box), capplet, capplet->mobility_ats, DA_TYPE_MOBILITY);
		set_changed(GTK_COMBO_BOX(capplet->image_combo_box), capplet, capplet->image_viewers, DA_TYPE_IMAGE);
		set_changed(GTK_COMBO_BOX(capplet->document_combo_box), capplet, capplet->document_viewers, DA_TYPE_DOCUMENT);
		set_changed(GTK_COMBO_BOX(capplet->word_combo_box), capplet, capplet->word_editors, DA_TYPE_WORD);
		set_changed(GTK_COMBO_BOX(capplet->spreadsheet_combo_box), capplet, capplet->spreadsheet_editors, DA_TYPE_SPREADSHEET);
		set_changed(GTK_COMBO_BOX(capplet->calculator_combo_box), capplet, capplet->calculators, DA_TYPE_CALCULATOR);
		set_changed(GTK_COMBO_BOX(capplet->messenger_combo_box), capplet, capplet->messengers, DA_TYPE_MESSENGER);

		gtk_widget_destroy(window);
		gtk_main_quit();
	}
}

/* Combo boxes callbacks */
static void
web_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->web_browsers, DA_TYPE_WEB_BROWSER);
}

static void
mail_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->mail_readers, DA_TYPE_EMAIL);
}

static void
file_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->file_managers, DA_TYPE_FILE);
}

static void
text_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->text_editors, DA_TYPE_TEXT);
}

static void
media_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->media_players, DA_TYPE_MEDIA);
}

static void
video_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->video_players, DA_TYPE_VIDEO);
}

static void
terminal_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->terminals, DA_TYPE_TERMINAL);
}

static void
visual_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->visual_ats, DA_TYPE_VISUAL);
}

static void
mobility_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->mobility_ats, DA_TYPE_MOBILITY);
}

static void
image_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->image_viewers, DA_TYPE_IMAGE);
}

static void
document_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->document_viewers, DA_TYPE_DOCUMENT);
}

static void
word_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->word_editors, DA_TYPE_WORD);
}

static void
spreadsheet_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->spreadsheet_editors, DA_TYPE_SPREADSHEET);
}

static void
calculator_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->calculators, DA_TYPE_CALCULATOR);
}

static void
messenger_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
        set_changed(combo, capplet, capplet->messengers, DA_TYPE_MESSENGER);
}

static void
refresh_combo_box_icons(GtkIconTheme* theme, GtkComboBox* combo_box, GList* app_list)
{
	GtkTreeIter iter;
	GtkTreeModel* model;
	gboolean valid;
	cairo_surface_t *surface;
	gchar* icon_name;
	gint scale_factor;

	model = gtk_combo_box_get_model(combo_box);

	if (model == NULL)
		return;

	valid = gtk_tree_model_get_iter_first(model, &iter);
	scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (combo_box));

	while (valid)
	{
		gtk_tree_model_get (model, &iter,
		                    ICONAME_COL, &icon_name,
		                    -1);

		surface = gtk_icon_theme_load_surface (theme, icon_name,
		                                       22, scale_factor,
		                                       NULL,
		                                       GTK_ICON_LOOKUP_FORCE_SIZE,
		                                       NULL);

		gtk_list_store_set (GTK_LIST_STORE(model), &iter,
		                    SURFACE_COL, surface,
		                    -1);

		if (surface)
			cairo_surface_destroy (surface);

		g_free (icon_name);

		valid = gtk_tree_model_iter_next(model, &iter);
	}
}

static struct {
	const char* name;
	const char* icon;
} icons[] = {
	{"web_browser_image",  "web-browser"},
	{"mail_reader_image",  "emblem-mail"},
	{"media_player_image", "audio-x-generic"},
	{"visual_image",       "zoom-best-fit"},
	{"mobility_image",     "preferences-desktop-accessibility"},
	{"messenger_image",    "instant-messaging"},
	{"filemanager_image",  "file-manager"},
	{"imageviewer_image",  "image-x-generic"},
	{"video_image",        "video-x-generic"},
	{"text_image",         "text-editor"},
	{"terminal_image",     "terminal"},
	{"document_image",     "application-pdf"},
	{"word_image",         "x-office-document"},
	{"spreadsheet_image",  "x-office-spreadsheet"},
	{"calculator_image",   "accessories-calculator"},
};

/* Callback for icon theme change */
static void
theme_changed_cb(GtkIconTheme* theme, MateDACapplet* capplet)
{
	GObject* icon;
	gint i;
	gint scale_factor;
	cairo_surface_t *surface;

	scale_factor = gtk_widget_get_scale_factor (capplet->window);

	for (i = 0; i < G_N_ELEMENTS(icons); i++)
	{
		icon = gtk_builder_get_object(capplet->builder, icons[i].name);

		surface = gtk_icon_theme_load_surface (theme, icons[i].icon,
		                                       32, scale_factor,
		                                       NULL,
		                                       GTK_ICON_LOOKUP_FORCE_SIZE,
		                                       NULL);

		gtk_image_set_from_surface (GTK_IMAGE(icon), surface);

		if (surface)
			cairo_surface_destroy (surface);
	}

	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->web_combo_box), capplet->web_browsers);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->mail_combo_box), capplet->mail_readers);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->media_combo_box), capplet->media_players);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->video_combo_box), capplet->video_players);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->term_combo_box), capplet->terminals);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->visual_combo_box), capplet->visual_ats);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->mobility_combo_box), capplet->mobility_ats);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->file_combo_box), capplet->file_managers);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->text_combo_box), capplet->text_editors);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->document_combo_box), capplet->document_viewers);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->word_combo_box), capplet->word_editors);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->spreadsheet_combo_box), capplet->spreadsheet_editors);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->calculator_combo_box), capplet->calculators);
        refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->messenger_combo_box), capplet->messengers);
}

static void
screen_changed_cb(GtkWidget* widget, GdkScreen* screen, MateDACapplet* capplet)
{
	GtkIconTheme* theme;

	theme = gtk_icon_theme_get_for_screen (screen);

	if (capplet->icon_theme != NULL)
	{
		g_signal_handlers_disconnect_by_func (capplet->icon_theme, theme_changed_cb, capplet);
	}

	g_signal_connect (theme, "changed", G_CALLBACK (theme_changed_cb), capplet);
	theme_changed_cb (theme, capplet);

	capplet->icon_theme = theme;
}

static void
fill_combo_box(GtkIconTheme* theme, GtkComboBox* combo_box, GList* app_list, gchar* mime)
{
	guint index = 0;
	GList* entry;
	GtkTreeModel* model;
	GtkCellRenderer* renderer;
	GtkTreeIter iter;
	cairo_surface_t *surface;
	GAppInfo* default_app;
	gint scale_factor;

	default_app = NULL;
	if (g_strcmp0(mime, "terminal") == 0)
	{
		GSettings *terminal_settings = g_settings_new (TERMINAL_SCHEMA);
		gchar *default_terminal = g_settings_get_string (terminal_settings, TERMINAL_KEY);
		for (entry = app_list; entry != NULL; entry = g_list_next(entry))
		{
			GAppInfo* item = (GAppInfo*) entry->data;
			if (g_strcmp0 (g_app_info_get_executable (item), default_terminal) == 0)
			{
				default_app = item;
			}
		}
		g_free (default_terminal);
		g_object_unref (terminal_settings);
	}
	else if (g_strcmp0(mime, "visual") == 0)
	{
		GSettings *visual_settings = g_settings_new (VISUAL_SCHEMA);
		gchar *default_visual = g_settings_get_string (visual_settings, VISUAL_KEY);
		for (entry = app_list; entry != NULL; entry = g_list_next(entry))
		{
			GAppInfo* item = (GAppInfo*) entry->data;
			if (g_strcmp0 (g_app_info_get_executable (item), default_visual) == 0)
			{
				default_app = item;
			}
		}
		g_free (default_visual);
		g_object_unref (visual_settings);
	}
	else if (g_strcmp0(mime, "mobility") == 0)
	{
		GSettings *mobility_settings = g_settings_new (MOBILITY_SCHEMA);
		gchar *default_mobility = g_settings_get_string (mobility_settings, MOBILITY_KEY);
		for (entry = app_list; entry != NULL; entry = g_list_next(entry))
		{
			GAppInfo* item = (GAppInfo*) entry->data;
			if (g_strcmp0 (g_app_info_get_executable (item), default_mobility) == 0)
			{
				default_app = item;
			}
		}
		g_free (default_mobility);
		g_object_unref (mobility_settings);
	}
	else if (g_strcmp0(mime, "calculator") == 0)
	{
		GSettings *calculator_settings = g_settings_new (CALCULATOR_SCHEMA);
		gchar *default_calculator = g_settings_get_string (calculator_settings, CALCULATOR_KEY);
		for (entry = app_list; entry != NULL; entry = g_list_next(entry))
		{
			GAppInfo* item = (GAppInfo*) entry->data;
			if (g_strcmp0 (g_app_info_get_executable (item), default_calculator) == 0)
			{
				default_app = item;
			}
		}
		g_free (default_calculator);
		g_object_unref (calculator_settings);
	}
	else if (g_strcmp0(mime, "messenger") == 0)
	{
		GSettings *messenger_settings = g_settings_new (MESSENGER_SCHEMA);
		gchar *default_messenger = g_settings_get_string (messenger_settings, MESSENGER_KEY);
		for (entry = app_list; entry != NULL; entry = g_list_next(entry))
		{
			GAppInfo* item = (GAppInfo*) entry->data;
			if (g_strcmp0 (g_app_info_get_executable (item), default_messenger) == 0)
			{
				default_app = item;
			}
		}
		g_free (default_messenger);
		g_object_unref (messenger_settings);
	}
	else
	{
		default_app = g_app_info_get_default_for_type (mime, FALSE);
	}

	if (theme == NULL)
	{
		theme = gtk_icon_theme_get_default();
	}

	model = GTK_TREE_MODEL (gtk_list_store_new (4,
	                                            CAIRO_GOBJECT_TYPE_SURFACE,
	                                            G_TYPE_STRING,
	                                            G_TYPE_STRING,
	                                            G_TYPE_STRING));
	gtk_combo_box_set_model(combo_box, model);

	renderer = gtk_cell_renderer_pixbuf_new();

	/* Not all cells have an icon, this prevents the combo box to shrink */
	gtk_cell_renderer_set_fixed_size(renderer, 22, 22);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), renderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), renderer,
	                                "surface", SURFACE_COL,
	                                NULL);

	renderer = gtk_cell_renderer_text_new();

	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), renderer,
		"text", TEXT_COL,
		NULL);

	scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (combo_box));

	for (entry = app_list; entry != NULL; entry = g_list_next(entry))
	{
		GAppInfo* item = (GAppInfo*) entry->data;

		/* Icon */
		GIcon* icon = g_app_info_get_icon(item);
		gchar* icon_name;

		if (icon != NULL) {
			icon_name = g_icon_to_string (icon);
			if (icon_name == NULL) {
				/* Default icon */
				icon_name = g_strdup ("binary");
			}
		} else {
			icon_name = g_strdup ("binary");
		}

		surface = gtk_icon_theme_load_surface (theme, icon_name,
		                                       22, scale_factor,
		                                       NULL,
		                                       GTK_ICON_LOOKUP_FORCE_SIZE,
		                                       NULL);

		gtk_list_store_append(GTK_LIST_STORE(model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		                    SURFACE_COL, surface,
		                    TEXT_COL, g_app_info_get_display_name(item),
		                    ID_COL, g_app_info_get_id(item),
		                    ICONAME_COL, icon_name,
		                    -1);

		if (surface)
			cairo_surface_destroy (surface);

		/* Set the index for the default app */
		if (default_app != NULL && g_app_info_equal(item, default_app))
		{
			gtk_combo_box_set_active(combo_box, index);
		}

		g_free(icon_name);

		index++;
	}
}

static GList*
fill_list_from_desktop_file (GList* app_list, const gchar *desktop_id)
{
	GList* list = app_list;
	GDesktopAppInfo* appinfo;

	appinfo = g_desktop_app_info_new_from_filename (desktop_id);
	if (appinfo != NULL) {
		list = g_list_prepend (list, appinfo);
	}
	return list;
}

static gint
compare_apps (gconstpointer a, gconstpointer b)
{
	GAppInfo *app_info_1, *app_info_2;
	gchar *app_dpy_name_1, *app_dpy_name_2;
	gint ret;

	app_info_1 = G_APP_INFO (a);
	app_info_2 = G_APP_INFO (b);
	app_dpy_name_1 = g_utf8_casefold (g_app_info_get_display_name (app_info_1), -1);
	app_dpy_name_2 = g_utf8_casefold (g_app_info_get_display_name (app_info_2), -1);
	ret = g_strcmp0 (app_dpy_name_1, app_dpy_name_2);
	g_free (app_dpy_name_1);
	g_free (app_dpy_name_2);
	return ret;
}

static void
show_dialog(MateDACapplet* capplet, const gchar* start_page)
{
	#define get_widget(name) GTK_WIDGET(gtk_builder_get_object(builder, name))

	GtkBuilder* builder;
	guint builder_result;

	capplet->builder = builder = gtk_builder_new ();

	builder_result = gtk_builder_add_from_resource (builder, "/org/mate/mcc/da/mate-default-applications-properties.ui", NULL);

	if (builder_result == 0)
	{
		GtkWidget* dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Could not load the main interface"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), _("Please make sure that the applet is properly installed"));
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

		gtk_dialog_run(GTK_DIALOG(dialog));

		gtk_widget_destroy(dialog);
		exit(EXIT_FAILURE);
	}

	capplet->window = get_widget("preferred_apps_dialog");

	g_signal_connect(capplet->window, "response", G_CALLBACK(close_cb), capplet);

	capplet->web_combo_box = get_widget("web_browser_combobox");
	capplet->mail_combo_box = get_widget("mail_reader_combobox");
	capplet->term_combo_box = get_widget("terminal_combobox");
	capplet->media_combo_box = get_widget("media_player_combobox");
	capplet->video_combo_box = get_widget("video_combobox");
	capplet->visual_combo_box = get_widget("visual_combobox");
	capplet->mobility_combo_box = get_widget("mobility_combobox");
	capplet->text_combo_box = get_widget("text_combobox");
	capplet->file_combo_box = get_widget("filemanager_combobox");
	capplet->image_combo_box = get_widget("image_combobox");
	capplet->document_combo_box = get_widget("document_combobox");
	capplet->word_combo_box = get_widget("word_combobox");
	capplet->spreadsheet_combo_box = get_widget("spreadsheet_combobox");
	capplet->calculator_combo_box = get_widget("calculator_combobox");
        capplet->messenger_combo_box = get_widget("messenger_combobox");

	capplet->visual_startup_checkbutton = get_widget("visual_start_checkbutton");
	capplet->mobility_startup_checkbutton = get_widget("mobility_start_checkbutton");

	g_signal_connect(capplet->window, "screen-changed", G_CALLBACK(screen_changed_cb), capplet);
	screen_changed_cb(capplet->window, gdk_screen_get_default(), capplet);

	/* Lists of default applications */
	capplet->web_browsers = g_app_info_get_all_for_type("x-scheme-handler/http");
	capplet->mail_readers = g_app_info_get_all_for_type("x-scheme-handler/mailto");
	capplet->media_players = g_app_info_get_all_for_type("audio/x-vorbis+ogg");
	capplet->video_players = g_app_info_get_all_for_type("video/x-ogm+ogg");
	capplet->text_editors = g_app_info_get_all_for_type("text/plain");
	capplet->image_viewers = g_app_info_get_all_for_type("image/png");
	capplet->file_managers = g_app_info_get_all_for_type("inode/directory");
	capplet->document_viewers = g_app_info_get_all_for_type("application/pdf");
	capplet->word_editors = g_app_info_get_all_for_type("application/msword");
	capplet->spreadsheet_editors = g_app_info_get_all_for_type("application/vnd.ms-excel");

	capplet->visual_ats = NULL;
        const gchar *const *sys_config_dirs = g_get_system_config_dirs();
	for (const gchar* c = *sys_config_dirs; c; c=*++sys_config_dirs)
        {
		gchar* path = g_strconcat (c, "/autostart/orca-autostart.desktop", NULL);
		capplet->visual_ats = fill_list_from_desktop_file (capplet->visual_ats, path);
		g_free (path);
	}
	capplet->visual_ats = g_list_reverse (capplet->visual_ats);

	capplet->mobility_ats = NULL;
	capplet->mobility_ats = fill_list_from_desktop_file (capplet->mobility_ats, APPLICATIONSDIR "/dasher.desktop");
	capplet->mobility_ats = fill_list_from_desktop_file (capplet->mobility_ats, APPLICATIONSDIR "/gok.desktop");
	capplet->mobility_ats = fill_list_from_desktop_file (capplet->mobility_ats, APPLICATIONSDIR "/onboard.desktop");
	capplet->mobility_ats = g_list_reverse (capplet->mobility_ats);

	/* Terminal havent mime types, so check in .desktop files for
	   Categories=TerminalEmulator */
	GList *entry;
	GList *all_apps;
	capplet->terminals = NULL;
	all_apps = g_app_info_get_all();
	for (entry = all_apps; entry != NULL; entry = g_list_next(entry))
	{
		GDesktopAppInfo* item = (GDesktopAppInfo*) entry->data;
		if (g_desktop_app_info_get_categories (item) != NULL &&
			g_strrstr (g_desktop_app_info_get_categories (item), "TerminalEmulator"))
		{
			capplet->terminals = g_list_prepend (capplet->terminals, item);
		}
	}
	capplet->terminals = g_list_reverse (capplet->terminals);

	/* Calculator havent mime types, so check in .desktop files for
	   Categories=Calculator */
	capplet->calculators = NULL;
	all_apps = g_app_info_get_all();
	for (entry = all_apps; entry != NULL; entry = g_list_next(entry))
	{
		GDesktopAppInfo* item = (GDesktopAppInfo*) entry->data;
		if (g_desktop_app_info_get_categories (item) != NULL &&
			g_strrstr (g_desktop_app_info_get_categories (item), "Calculator"))
		{
			capplet->calculators = g_list_prepend (capplet->calculators, item);
		}
	}
	capplet->calculators = g_list_reverse (capplet->calculators);

	/* Messenger havent mime types, so check in .desktop files for
	   Categories=InstantMessaging */
	capplet->messengers = g_app_info_get_all_for_type ("x-scheme-handler/irc");
	all_apps = g_app_info_get_all();
	for (entry = all_apps; entry != NULL; entry = g_list_next(entry))
	{
		if (capplet->messengers && g_list_index (capplet->messengers, entry) != -1)
			continue;

		GDesktopAppInfo* item = (GDesktopAppInfo*) entry->data;
		if (g_desktop_app_info_get_categories (item) != NULL &&
			g_strrstr (g_desktop_app_info_get_categories (item), "InstantMessaging"))
		{
			capplet->messengers = g_list_prepend (capplet->messengers, item);
		}
	}
	capplet->messengers = g_list_sort (capplet->messengers, compare_apps);

	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->web_combo_box), capplet->web_browsers, "x-scheme-handler/http");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->mail_combo_box), capplet->mail_readers, "x-scheme-handler/mailto");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->term_combo_box), capplet->terminals, "terminal");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->media_combo_box), capplet->media_players, "audio/x-vorbis+ogg");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->video_combo_box), capplet->video_players, "video/x-ogm+ogg");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->image_combo_box), capplet->image_viewers, "image/png");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->text_combo_box), capplet->text_editors, "text/plain");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->file_combo_box), capplet->file_managers, "inode/directory");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->visual_combo_box), capplet->visual_ats, "visual");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->mobility_combo_box), capplet->mobility_ats, "mobility");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->document_combo_box), capplet->document_viewers, "application/pdf");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->word_combo_box), capplet->word_editors, "application/vnd.oasis.opendocument.text");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->spreadsheet_combo_box), capplet->spreadsheet_editors, "application/vnd.oasis.opendocument.spreadsheet");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->calculator_combo_box), capplet->calculators, "calculator");
        fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->messenger_combo_box), capplet->messengers, "messenger");

	g_signal_connect(capplet->web_combo_box, "changed", G_CALLBACK(web_combo_changed_cb), capplet);
	g_signal_connect(capplet->mail_combo_box, "changed", G_CALLBACK(mail_combo_changed_cb), capplet);
	g_signal_connect(capplet->term_combo_box, "changed", G_CALLBACK(terminal_combo_changed_cb), capplet);
	g_signal_connect(capplet->media_combo_box, "changed", G_CALLBACK(media_combo_changed_cb), capplet);
	g_signal_connect(capplet->video_combo_box, "changed", G_CALLBACK(video_combo_changed_cb), capplet);
	g_signal_connect(capplet->visual_combo_box, "changed", G_CALLBACK(visual_combo_changed_cb), capplet);
	g_signal_connect(capplet->mobility_combo_box, "changed", G_CALLBACK(mobility_combo_changed_cb), capplet);
	g_signal_connect(capplet->image_combo_box, "changed", G_CALLBACK(image_combo_changed_cb), capplet);
	g_signal_connect(capplet->text_combo_box, "changed", G_CALLBACK(text_combo_changed_cb), capplet);
	g_signal_connect(capplet->file_combo_box, "changed", G_CALLBACK(file_combo_changed_cb), capplet);
	g_signal_connect(capplet->document_combo_box, "changed", G_CALLBACK(document_combo_changed_cb), capplet);
	g_signal_connect(capplet->word_combo_box, "changed", G_CALLBACK(word_combo_changed_cb), capplet);
	g_signal_connect(capplet->spreadsheet_combo_box, "changed", G_CALLBACK(spreadsheet_combo_changed_cb), capplet);
	g_signal_connect(capplet->calculator_combo_box, "changed", G_CALLBACK(calculator_combo_changed_cb), capplet);
        g_signal_connect(capplet->messenger_combo_box, "changed", G_CALLBACK(messenger_combo_changed_cb), capplet);

	g_settings_bind (capplet->mobility_settings, MOBILITY_STARTUP_KEY, capplet->mobility_startup_checkbutton, "active", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (capplet->visual_startup_settings, VISUAL_STARTUP_KEY, capplet->visual_startup_checkbutton, "active", G_SETTINGS_BIND_DEFAULT);

	gtk_window_set_icon_name(GTK_WINDOW (capplet->window), "preferences-desktop-default-applications");

        GtkNotebook* nb = GTK_NOTEBOOK(get_widget("preferred_apps_notebook"));
        gtk_widget_add_events (GTK_WIDGET (nb), GDK_SCROLL_MASK);
        g_signal_connect (GTK_WIDGET (nb), "scroll-event",
                          G_CALLBACK (capplet_dialog_page_scroll_event_cb),
                          GTK_WINDOW (capplet->window));

	if (start_page != NULL)
	{
		gchar* page_name;
		GtkWidget* w;

		page_name = g_strconcat (start_page, "_vbox", NULL);

		w = get_widget(page_name);

		if (w != NULL)
		{
			gint pindex;

			pindex = gtk_notebook_page_num(nb, w);

			if (pindex != -1)
			{
				gtk_notebook_set_current_page(nb, pindex);
			}
		}

		g_free(page_name);
	}

	gtk_widget_show(capplet->window);

	#undef get_widget
}

int
main(int argc, char** argv)
{
	gchar* start_page = NULL;

	GOptionEntry option_entries[] = {
		{
			"show-page",
			'p',
			G_OPTION_FLAG_IN_MAIN,
			G_OPTION_ARG_STRING,
			&start_page,
			/* TRANSLATORS: don't translate the terms in brackets */
			N_("Specify the name of the page to show (internet|multimedia|system|a11y)"),
			N_("page")
		},
		{NULL}
	};

	GOptionContext* context = g_option_context_new(_("- MATE Default Applications"));
	g_option_context_add_main_entries(context, option_entries, GETTEXT_PACKAGE);

	capplet_init(context, &argc, &argv);

	MateDACapplet* capplet = g_new0(MateDACapplet, 1);

	capplet->terminal_settings = g_settings_new (TERMINAL_SCHEMA);
	capplet->mobility_settings = g_settings_new (MOBILITY_SCHEMA);
	capplet->visual_settings = g_settings_new (VISUAL_SCHEMA);
	capplet->visual_startup_settings = g_settings_new (VISUAL_STARTUP_SCHEMA);
	capplet->calculator_settings = g_settings_new (CALCULATOR_SCHEMA);
	capplet->messenger_settings = g_settings_new (MESSENGER_SCHEMA);

	show_dialog(capplet, start_page);
	g_free(start_page);

	gtk_main();

	g_object_unref (capplet->terminal_settings);
	g_object_unref (capplet->mobility_settings);
	g_object_unref (capplet->visual_settings);
	g_object_unref (capplet->visual_startup_settings);
	g_object_unref (capplet->calculator_settings);
	g_object_unref (capplet->messenger_settings);

	return 0;
}
