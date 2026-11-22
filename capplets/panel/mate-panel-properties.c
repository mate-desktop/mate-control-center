/*
 * mate-panel-properties.c: entry point of the panel properties capplet
 *
 * Copyright (C) 2026 MATE Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

#include <config.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "capplet-util.h"
#include "pp-config.h"
#include "pp-window.h"

#if !GLIB_CHECK_VERSION (2, 74, 0)
#define G_APPLICATION_DEFAULT_FLAGS G_APPLICATION_FLAGS_NONE
#endif

static void
activate_cb (GApplication *application, gpointer user_data)
{
	GList *windows = gtk_application_get_windows (GTK_APPLICATION (application));

	if (windows != NULL) {
		gtk_window_present (GTK_WINDOW (windows->data));
		return;
	}

	if (!pp_config_available ()) {
		GtkWidget *dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
							    "%s", _("MATE Panel is not installed"));

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s",
							  _("The panel settings schemas could not be found."));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return;
	}

	gtk_widget_show_all (pp_window_new (GTK_APPLICATION (application)));
}

int
main (int argc, char **argv)
{
	GtkApplication *application;
	int status;

	capplet_init (NULL, &argc, &argv);

	g_set_application_name (_("Panel Properties"));
	gtk_window_set_default_icon_name ("mate-panel");

	application = gtk_application_new ("org.mate.PanelProperties", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect (application, "activate", G_CALLBACK (activate_cb), NULL);
	status = g_application_run (G_APPLICATION (application), argc, argv);
	g_object_unref (application);

	return status;
}
