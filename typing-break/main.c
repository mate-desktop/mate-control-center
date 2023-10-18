/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002-2003 Richard Hult <richard@imendio.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <string.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "drw-selection.h"
#include "drwright.h"

#define NOTIFIERAREA_NAME          "org.kde.StatusNotifierWatcher"
#define NOTIFIERAREA_PATH          "/StatusNotifierWatcher"
#define NOTIFIERAREA_INTERFACE     "org.kde.StatusNotifierWatcher"
#define NOTIFIERAREA_PROPERTY      "IsStatusNotifierHostRegistered"

gboolean debug = FALSE;

static gboolean
is_status_notifier_host_available (void)
{
	g_autoptr (GDBusProxy) proxy = NULL;
	g_autoptr (GError) error = NULL;

	proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
	                                       G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS |
	                                       G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
	                                       NULL,
	                                       NOTIFIERAREA_NAME,
	                                       NOTIFIERAREA_PATH,
	                                       NOTIFIERAREA_INTERFACE,
	                                       NULL, &error);

	if (proxy == NULL || error)
	{
		return FALSE;
	}
	g_autoptr (GVariant) variant = g_dbus_proxy_get_cached_property (proxy, NOTIFIERAREA_PROPERTY);
	if (! variant)
	{
		return FALSE;
	}

	return g_variant_get_boolean (variant);
}

int
main (int argc, char *argv[])
{
	DrwSelection *selection;
	gboolean      no_check = FALSE;
        const GOptionEntry options[] = {
          { "debug", 'd', 0, G_OPTION_ARG_NONE, &debug,
            N_("Enable debugging code"), NULL },
          { "no-check", 'n', 0, G_OPTION_ARG_NONE, &no_check,
            N_("Don't check whether the notification area exists"), NULL },
	  { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
        };
        GOptionContext *option_context;
        GError *error = NULL;
        gboolean retval;

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif /* ENABLE_NLS */

        option_context = g_option_context_new (NULL);
#ifdef ENABLE_NLS
        g_option_context_set_translation_domain (option_context, GETTEXT_PACKAGE);
#endif /* ENABLE_NLS */
        g_option_context_add_main_entries (option_context, options, GETTEXT_PACKAGE);
        g_option_context_add_group (option_context, gtk_get_option_group (TRUE));

        retval = g_option_context_parse (option_context, &argc, &argv, &error);
        g_option_context_free (option_context);
        if (!retval) {
                g_print ("%s\n", error->message);
                g_error_free (error);
                exit (1);
        }

	g_set_application_name (_("Typing Monitor"));
	gtk_window_set_default_icon_name ("mate-typing-monitor");

	selection = drw_selection_start ();
	if (!drw_selection_is_master (selection)) {
		g_message ("The typing monitor is already running, exiting.");
		return 0;
	}

	if (!no_check && !is_status_notifier_host_available ()) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (
			NULL, 0,
			GTK_MESSAGE_INFO,
			GTK_BUTTONS_CLOSE,
			_("The typing monitor uses the notification area to display "
			  "information. You don't seem to have a notification area "
			  "on your panel. You can add it by right-clicking on your "
			  "panel and choosing 'Add to panel', selecting 'Notification "
			  "area' and clicking 'Add'."));

		gtk_dialog_run (GTK_DIALOG (dialog));

		gtk_widget_destroy (dialog);
	}

	drwright_new ();

	gtk_main ();

	return 0;
}
