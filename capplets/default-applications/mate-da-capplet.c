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
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include <string.h>
#include <glib/gi18n.h>
#include <stdlib.h>

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
	DA_N_COLUMNS
};

/* for combo box */
enum {
	PIXBUF_COL,
	TEXT_COL,
	ID_COL,
	ICONAME_COL,
	N_COLUMNS
};

static void
close_cb(GtkWidget* window, gint response, gpointer user_data)
{
	if (response == GTK_RESPONSE_HELP)
	{
		capplet_help(GTK_WINDOW(window), "prefs-preferredapps");
	}
	else
	{
		gtk_widget_destroy(window);
		gtk_main_quit();
	}
}

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
			
				/* establecemos el item */
				g_app_info_set_as_default_for_type(item, "x-scheme-handler/http", NULL);
				g_app_info_set_as_default_for_type(item, "x-scheme-handler/https", NULL);

				/* about:config es usado por mozilla firefox y algunos otros con
				 * webtoolkit */
				g_app_info_set_as_default_for_type(item, "x-scheme-handler/about", NULL);
				break;

			case DA_TYPE_EMAIL:
				/* por alguna extraña razon, solo se usa mailto, en vez de mail. */
				g_app_info_set_as_default_for_type(item, "x-scheme-handler/mailto", NULL);
				break;
			
			case DA_TYPE_FILE:
				/* falta agregar más mime-types */
				g_app_info_set_as_default_for_type(item, "inode/directory", NULL);
				break;
			
			case DA_TYPE_TEXT:
				/* falta agregar más mime-types */
				g_app_info_set_as_default_for_type(item, "text/plain", NULL);
				break;

			case DA_TYPE_MEDIA:
				/* por alguna extraña razon, solo se usa mailto, en vez de mail. */
				g_app_info_set_as_default_for_type(item, "audio/x-vorbis+ogg", NULL);
				g_app_info_set_as_default_for_type(item, "audio/x-scpls", NULL);
				g_app_info_set_as_default_for_type(item, "audio/mpeg", NULL);
				g_app_info_set_as_default_for_type(item, "audio/x-wav", NULL);
				g_app_info_set_as_default_for_type(item, "audio/x-mpegurl", NULL);
				g_app_info_set_as_default_for_type(item, "video/webm", NULL);
				break;
				
			case DA_TYPE_VIDEO:
				/* por alguna extraña razon, solo se usa mailto, en vez de mail. */
				g_app_info_set_as_default_for_type(item, "video/mpeg", NULL);
				g_app_info_set_as_default_for_type(item, "video/x-mpeg", NULL);
				g_app_info_set_as_default_for_type(item, "video/msvideo", NULL);
				g_app_info_set_as_default_for_type(item, "video/quicktime", NULL);
				g_app_info_set_as_default_for_type(item, "video/x-avi", NULL);
				g_app_info_set_as_default_for_type(item, "video/x-ogm+ogg", NULL);
				g_app_info_set_as_default_for_type(item, "video/x-matroska", NULL);
				g_app_info_set_as_default_for_type(item, "video/webm", NULL);
				g_app_info_set_as_default_for_type(item, "video/mp4", NULL);
				g_app_info_set_as_default_for_type(item, "video/x-flv", NULL);
				break;

			case DA_TYPE_IMAGE:
				/* por alguna extraña razon, solo se usa mailto, en vez de mail. */
				g_app_info_set_as_default_for_type(item, "image/png", NULL);
				g_app_info_set_as_default_for_type(item, "image/jpeg", NULL);
				g_app_info_set_as_default_for_type(item, "image/gif", NULL);
				g_app_info_set_as_default_for_type(item, "image/bmp", NULL);
				g_app_info_set_as_default_for_type(item, "image/tiff", NULL);
				break;
						
						
			default:;
				break;
		}
	}
}

/* Combo box callbacks */
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
	set_changed(combo, capplet, capplet->video_players, DA_TYPE_VISUAL);
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
refresh_combo_box_icons(GtkIconTheme* theme, GtkComboBox* combo_box, GList* app_list)
{
	GtkTreeIter iter;
	GtkTreeModel* model;
	gboolean valid;
	GdkPixbuf* pixbuf;
	gchar* icon_name;
	
	model = gtk_combo_box_get_model(combo_box);

	valid = gtk_tree_model_get_iter_first(model, &iter);
	
	while (valid)
    {
		gtk_tree_model_get(model, &iter,
				          ICONAME_COL, &icon_name,
				          -1);

		pixbuf = gtk_icon_theme_load_icon(theme, icon_name, 22, 0, NULL);

		gtk_list_store_set(GTK_LIST_STORE(model), &iter,
				PIXBUF_COL, pixbuf,
				-1);

		if (pixbuf)
		{
			g_object_unref(pixbuf);
		}
		
		g_free(icon_name);

		valid = gtk_tree_model_iter_next(model, &iter);
    }
}

static struct {
	const char* name;
	const char* icon;
} icons[] = {
	{"web_browser_image",  "web-browser"},
	{"mail_reader_image",  "emblem-mail"},
	{"media_player_image", "audio-x-generic"}, /* applications-multimedia */
	{"visual_image",       "zoom-best-fit"},
	{"mobility_image",     "preferences-desktop-accessibility"},
	{"messenger_image",    "user-idle"},
	{"filemanager_image",  "file-manager"},
	{"imageviewer_image",  "eog"}, /* no hay otra... */
	{"video_image",        "video-x-generic"},
	{"text_image",         "text-editor"},
	{"terminal_image",     "terminal"},
};

/* Esta funcion se llama cuando se cambia o actualizan los iconos */
static void
theme_changed_cb(GtkIconTheme* theme, MateDACapplet* capplet)
{
	GObject* icon;
	gint i;

	for (i = 0; i < G_N_ELEMENTS(icons); i++)
	{
		icon = gtk_builder_get_object(capplet->builder, icons[i].name);

		GdkPixbuf* pixbuf = gtk_icon_theme_load_icon(theme, icons[i].icon, 32, 0, NULL);

		gtk_image_set_from_pixbuf(GTK_IMAGE(icon), pixbuf);
		
		if (pixbuf)
		{
			g_object_unref(pixbuf);
		}
	}

	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->web_combo_box), capplet->web_browsers);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->mail_combo_box), capplet->mail_readers);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->media_combo_box), capplet->media_players);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->term_combo_box), capplet->terminals);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->visual_combo_box), capplet->visual_ats);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->mobility_combo_box), capplet->mobility_ats);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->file_combo_box), capplet->file_managers);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->text_combo_box), capplet->text_editors);
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
	GdkPixbuf* pixbuf;
	GAppInfo* default_app;
	
	default_app = g_app_info_get_default_for_type(mime, FALSE);

	if (theme == NULL)
	{
		theme = gtk_icon_theme_get_default();
	}

	model = GTK_TREE_MODEL(gtk_list_store_new(4, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING));
	gtk_combo_box_set_model(combo_box, model);

	renderer = gtk_cell_renderer_pixbuf_new();

	/* not all cells have a pixbuf, this prevents the combo box to shrink */
	gtk_cell_renderer_set_fixed_size(renderer, -1, 22);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), renderer, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), renderer,
		"pixbuf", PIXBUF_COL,
		NULL);

	renderer = gtk_cell_renderer_text_new();

	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), renderer,
		"text", TEXT_COL,
		NULL);

	for (entry = app_list; entry != NULL; entry = g_list_next(entry))
	{
		GAppInfo* item = (GAppInfo*) entry->data;
		
		// icon
		GIcon* icon = g_app_info_get_icon(item);
		gchar* icon_name = g_icon_to_string(icon);
		
		if (icon_name == NULL)
		{
			icon_name = g_strdup("binary"); // default icon
		}
		
		pixbuf = gtk_icon_theme_load_icon(theme, icon_name, 22, 0, NULL);

		gtk_list_store_append(GTK_LIST_STORE(model), &iter);
		gtk_list_store_set(GTK_LIST_STORE(model), &iter,
			PIXBUF_COL, pixbuf,
			TEXT_COL, g_app_info_get_display_name(item),
			ID_COL, g_app_info_get_id(item),
			ICONAME_COL, icon_name,
			-1);

		if (pixbuf)
		{
			g_object_unref(pixbuf);
		}
		
		/* set the index */
		if (default_app != NULL && g_app_info_equal(item, default_app))
		{
			gtk_combo_box_set_active(combo_box, index);
		}
		
		g_free(icon_name);
		
		index++;
	}
}

static void
show_dialog(MateDACapplet* capplet, const gchar* start_page)
{
	#define get_widget(name) GTK_WIDGET(gtk_builder_get_object(builder, name))

	GtkBuilder* builder;
	guint builder_result;

	capplet->builder = builder = gtk_builder_new ();

	if (g_file_test(MATECC_UI_DIR "/mate-default-applications-properties.ui", G_FILE_TEST_EXISTS) != FALSE)
	{
		builder_result = gtk_builder_add_from_file(builder, MATECC_UI_DIR "/mate-default-applications-properties.ui", NULL);
	}
	else
	{
		builder_result = gtk_builder_add_from_file(builder, "./mate-default-applications-properties.ui", NULL);
	}

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

	g_signal_connect(capplet->window, "response", G_CALLBACK(close_cb), NULL);

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


	g_signal_connect(capplet->window, "screen-changed", G_CALLBACK(screen_changed_cb), capplet);
	screen_changed_cb(capplet->window, gdk_screen_get_default(), capplet);

	// lists
	capplet->web_browsers = g_app_info_get_all_for_type("x-scheme-handler/http");
	capplet->mail_readers = g_app_info_get_all_for_type("x-scheme-handler/mailto");
	//capplet->terminals = g_app_info_get_all_for_type("inode/directory");
	capplet->media_players = g_app_info_get_all_for_type("audio/x-vorbis+ogg");
	capplet->video_players = g_app_info_get_all_for_type("video/x-ogm+ogg");
	//capplet->visual_ats = g_app_info_get_all_for_type("inode/directory");
	//capplet->mobility_ats = g_app_info_get_all_for_type("inode/directory");
	capplet->text_editors = g_app_info_get_all_for_type("text/plain");
	capplet->image_viewers = g_app_info_get_all_for_type("image/png");
	capplet->file_managers = g_app_info_get_all_for_type("inode/directory");
	
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->web_combo_box), capplet->web_browsers, "x-scheme-handler/http");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->mail_combo_box), capplet->mail_readers, "x-scheme-handler/mailto");
	//fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->term_combo_box), capplet->terminals, "");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->media_combo_box), capplet->media_players, "audio/x-vorbis+ogg");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->video_combo_box), capplet->video_players, "video/x-ogm+ogg");
	//fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->visual_combo_box), capplet->visual_ats, NULL);
	//fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->mobility_combo_box), capplet->mobility_ats, NULL);
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->image_combo_box), capplet->image_viewers, "image/png");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->text_combo_box), capplet->text_editors, "text/plain");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->file_combo_box), capplet->file_managers, "inode/directory");

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


	/* TODO: fix the name icon */
	gtk_window_set_icon_name(GTK_WINDOW (capplet->window), "preferences-desktop-default-applications");

	if (start_page != NULL)
	{
		gchar* page_name;
		GtkWidget* w;

		page_name = g_strconcat (start_page, "_vbox", NULL);

		w = get_widget(page_name);

		if (w != NULL)
		{
			GtkNotebook* nb;
			gint pindex;

			nb = GTK_NOTEBOOK(get_widget("preferred_apps_notebook"));
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

	show_dialog(capplet, start_page);
	g_free(start_page);

	gtk_main();

	return 0;
}
