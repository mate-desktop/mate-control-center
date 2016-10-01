/*
 * This file is part of the Control Center.
 *
 * Copyright (c) 2006 Novell, Inc.
 *
 * The Control Center is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * The Control Center is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * the Control Center; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#if !GTK_CHECK_VERSION(3,0,0)
#include <unique/unique.h>
#endif
#include <libslab/slab.h>

void handle_static_action_clicked(Tile* tile, TileEvent* event, gpointer data);
static GSList* get_actions_list();

#define CONTROL_CENTER_SCHEMA "org.mate.control-center"
#define CONTROL_CENTER_ACTIONS_LIST_KEY "cc-actions-list"
#define CONTROL_CENTER_ACTIONS_SEPARATOR ";"
#define EXIT_SHELL_ON_STATIC_ACTION "cc-exit-shell-on-static-action"

static GSList* get_actions_list(void)
{
	GSettings *settings;
	GSList* l;
	GSList* key_list = NULL;
	GSList* actions_list = NULL;
	AppAction* action;
	gchar **array;
	gint i;

	settings = g_settings_new (CONTROL_CENTER_SCHEMA);
	array = g_settings_get_strv (settings, CONTROL_CENTER_ACTIONS_LIST_KEY);
	if (array != NULL) {
		for (i = 0; array[i]; i++) {
			key_list = g_slist_append (key_list, g_strdup (array[i]));
		}
	}
	g_strfreev (array);
	g_object_unref (settings);

	if (!key_list)
	{
		g_warning(_("%s key is empty\n"), CONTROL_CENTER_ACTIONS_LIST_KEY);
		return NULL;
	}

	for (l = key_list; l != NULL; l = l->next)
	{
		gchar* entry = (gchar*) l->data;
		gchar** temp;

		action = g_new(AppAction, 1);
		temp = g_strsplit(entry, CONTROL_CENTER_ACTIONS_SEPARATOR, 2);
		action->name = g_strdup(temp[0]);

		if ((action->item = load_desktop_item_from_unknown(temp[1])) == NULL)
		{
			g_warning("get_actions_list() - PROBLEM - Can't load %s\n", temp[1]);
		}
		else
		{
			actions_list = g_slist_prepend(actions_list, action);
		}

		g_strfreev(temp);
		g_free(entry);
	}

	g_slist_free(key_list);

	return g_slist_reverse(actions_list);
}

void handle_static_action_clicked(Tile* tile, TileEvent* event, gpointer data)
{
	AppShellData* app_data = (AppShellData*) data;
	MateDesktopItem* item = (MateDesktopItem*) g_object_get_data(G_OBJECT(tile), APP_ACTION_KEY);
	GSettings *settings;

	if (event->type == TILE_EVENT_ACTIVATED_DOUBLE_CLICK)
	{
		return;
	}

	open_desktop_item_exec(item);

	settings = g_settings_new (CONTROL_CENTER_SCHEMA);

	if (g_settings_get_boolean(settings, EXIT_SHELL_ON_STATIC_ACTION))
	{
		if (app_data->exit_on_close)
		{
			gtk_main_quit();
		}
		else
		{
			hide_shell(app_data);
		}
	}

	g_object_unref(settings);
}

#if GTK_CHECK_VERSION(3,0,0)
static void
activate (GtkApplication *app)
{
	GList *list;
	GSList* actions;
	gboolean hidden = FALSE;

	list = gtk_application_get_windows (app);

	AppShellData* app_data = appshelldata_new("matecc.menu", GTK_ICON_SIZE_DND, FALSE, TRUE, 0);

	generate_categories(app_data);

	actions = get_actions_list();
	layout_shell(app_data, _("Filter"), _("Groups"), _("Common Tasks"), actions, handle_static_action_clicked);

	if (list)
	{
		gtk_window_present (GTK_WINDOW (list->data));
	}
	else
	{
		create_main_window(app_data, "MyControlCenter", _("Control Center"), "preferences-desktop", 975, 600, hidden);
		gtk_application_add_window (app, GTK_WINDOW(app_data->main_app));
    }
}

static void
quit (GApplication *app)
{
	g_application_quit(app);
}
#else
static UniqueResponse message_received_cb(UniqueApp* app, UniqueCommand command, UniqueMessageData* message, guint time, gpointer user_data)
{
	UniqueResponse res;
	AppShellData* app_data = user_data;

	switch (command)
	{
		case UNIQUE_ACTIVATE:
			/* move the main window to the screen that sent us the command */
			gtk_window_set_screen(GTK_WINDOW(app_data->main_app), unique_message_data_get_screen(message));

			if (!app_data->main_app_window_shown_once)
			{
				show_shell(app_data);
			}

			gtk_window_present_with_time(GTK_WINDOW(app_data->main_app), time);

			gtk_widget_grab_focus(SLAB_SECTION(app_data->filter_section)->contents);

			res = UNIQUE_RESPONSE_OK;

			break;
		default:
			res = UNIQUE_RESPONSE_PASSTHROUGH;
			break;
	}

	return res;
}
#endif
int main(int argc, char* argv[])
{
	gboolean hidden = FALSE;
#if GTK_CHECK_VERSION(3,0,0)
	GtkApplication *app;
	gint retval;
	app = gtk_application_new ("org.mate.mate-control-center.shell", 0);
#else
	UniqueApp* unique_app;
	AppShellData* app_data;
	GSList* actions;
#endif
	GError* error;
	GOptionEntry options[] = {
		{"hide", 0, 0, G_OPTION_ARG_NONE, &hidden, N_("Hide on start (useful to preload the shell)"), NULL},
		{NULL}
	};

#ifdef ENABLE_NLS
		bindtextdomain(GETTEXT_PACKAGE, MATELOCALEDIR);
		bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
		textdomain(GETTEXT_PACKAGE);
#endif

	error = NULL;

	if (!gtk_init_with_args(&argc, &argv, NULL, options, GETTEXT_PACKAGE, &error))
	{
		g_printerr("%s\n", error->message);
		g_error_free(error);
		return 1;
	}
#if GTK_CHECK_VERSION(3,0,0)
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    g_signal_connect (app, "window-removed", G_CALLBACK (quit), NULL);
    retval = g_application_run (G_APPLICATION (app), argc, argv);
    return retval;
#else
	unique_app = unique_app_new("org.mate.mate-control-center.shell", NULL);

	if (unique_app_is_running(unique_app))
	{
		int retval = 0;

		if (!hidden)
		{
			UniqueResponse response;
			response = unique_app_send_message(unique_app, UNIQUE_ACTIVATE, NULL);
			retval = (response != UNIQUE_RESPONSE_OK);
		}

		g_object_unref(unique_app);
		return retval;
	}

	app_data = appshelldata_new("matecc.menu", GTK_ICON_SIZE_DND, FALSE, TRUE, 0);
	generate_categories(app_data);

	actions = get_actions_list();
	layout_shell(app_data, _("Filter"), _("Groups"), _("Common Tasks"), actions, handle_static_action_clicked);

	create_main_window(app_data, "MyControlCenter", _("Control Center"), "preferences-desktop", 975, 600, hidden);

	unique_app_watch_window(unique_app, GTK_WINDOW(app_data->main_app));
	g_signal_connect(unique_app, "message-received", G_CALLBACK(message_received_cb), app_data);

    gtk_main();
	g_object_unref(unique_app);

	return 0;
#endif
}
