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
			g_free (action->name);
			g_free (action);
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
	GApplication *app;

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
			app=g_application_get_default();
			g_application_quit(app);
		}
		else
		{
			hide_shell(app_data);
		}
	}

	g_object_unref(settings);
}

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

int main(int argc, char* argv[])
{
	gboolean hidden = FALSE;
	GtkApplication *app;
	gint retval;
	app = gtk_application_new ("org.mate.mate-control-center.shell", 0);
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

	g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
	g_signal_connect (app, "window-removed", G_CALLBACK (quit), NULL);
	retval = g_application_run (G_APPLICATION (app), argc, argv);
	return retval;
}
