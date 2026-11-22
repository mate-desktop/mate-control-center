/*
 * pp-panel.c: client for the org.mate.Panel.Shell D-Bus interface
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

#include "pp-panel.h"

#define SHELL_PATH  "/org/mate/panel/Shell"
#define SHELL_IFACE "org.mate.Panel.Shell"
#define CALL_TIMEOUT_MS 3000

static GDBusConnection *
get_connection (GError **error)
{
	return g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, error);
}

static GVariant *
call_shell (const char          *method,
	    GVariant            *parameters,
	    const GVariantType  *reply_type,
	    GError             **error)
{
	GDBusConnection *connection = get_connection (error);
	GVariant *result;

	if (connection == NULL) {
		if (parameters != NULL)
			g_variant_unref (g_variant_ref_sink (parameters));
		return NULL;
	}

	result = g_dbus_connection_call_sync (connection, PP_PANEL_BUS_NAME, SHELL_PATH, SHELL_IFACE,
					      method, parameters, reply_type,
					      G_DBUS_CALL_FLAGS_NO_AUTO_START,
					      CALL_TIMEOUT_MS, NULL, error);
	g_object_unref (connection);

	if (result == NULL && error != NULL && *error != NULL &&
	    (g_error_matches (*error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD) ||
	     g_error_matches (*error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_OBJECT) ||
	     g_error_matches (*error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_INTERFACE))) {
		g_clear_error (error);
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
				     _("The running panel does not support this feature. "
				       "Please update MATE Panel."));
	} else if (result == NULL && error != NULL && *error != NULL &&
		   g_error_matches (*error, G_DBUS_ERROR, G_DBUS_ERROR_ACCESS_DENIED)) {
		g_clear_error (error);
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
				     _("The panels have been locked down by your administrator."));
	} else if (result == NULL && error != NULL && *error != NULL &&
		   (g_error_matches (*error, G_DBUS_ERROR, G_DBUS_ERROR_SERVICE_UNKNOWN) ||
		    g_error_matches (*error, G_DBUS_ERROR, G_DBUS_ERROR_NAME_HAS_NO_OWNER))) {
		g_clear_error (error);
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_CONNECTED,
				     _("The panel is not running."));
	}

	return result;
}

gboolean
pp_panel_is_running (void)
{
	GDBusConnection *connection = get_connection (NULL);
	GVariant *result;
	gboolean running = FALSE;

	if (connection == NULL)
		return FALSE;

	result = g_dbus_connection_call_sync (connection, "org.freedesktop.DBus", "/org/freedesktop/DBus",
					      "org.freedesktop.DBus", "NameHasOwner",
					      g_variant_new ("(s)", PP_PANEL_BUS_NAME),
					      G_VARIANT_TYPE ("(b)"), G_DBUS_CALL_FLAGS_NONE,
					      CALL_TIMEOUT_MS, NULL, NULL);
	if (result != NULL) {
		g_variant_get (result, "(b)", &running);
		g_variant_unref (result);
	}

	g_object_unref (connection);
	return running;
}

char **
pp_panel_get_objects (const char *toplevel_id, GError **error)
{
	GVariant *result;
	char **ids = NULL;

	result = call_shell ("GetObjects", g_variant_new ("(s)", toplevel_id),
			     G_VARIANT_TYPE ("(as)"), error);
	if (result == NULL)
		return NULL;

	g_variant_get (result, "(^as)", &ids);
	g_variant_unref (result);

	return ids;
}

gboolean
pp_panel_move_object (const char *object_id, int direction, GError **error)
{
	GVariant *result;
	gboolean moved = FALSE;

	result = call_shell ("MoveObject", g_variant_new ("(si)", object_id, direction),
			     G_VARIANT_TYPE ("(b)"), error);
	if (result == NULL)
		return FALSE;

	g_variant_get (result, "(b)", &moved);
	g_variant_unref (result);

	return moved;
}

gboolean
pp_panel_add_to_panel (const char *toplevel_id, GError **error)
{
	GVariant *result;

	result = call_shell ("AddToPanel", g_variant_new ("(s)", toplevel_id), NULL, error);
	if (result == NULL)
		return FALSE;

	g_variant_unref (result);
	return TRUE;
}

gboolean
pp_panel_show_properties (const char *object_id, GError **error)
{
	GVariant *result;
	gboolean shown = FALSE;

	result = call_shell ("ShowProperties", g_variant_new ("(s)", object_id),
			     G_VARIANT_TYPE ("(b)"), error);
	if (result == NULL)
		return FALSE;

	g_variant_get (result, "(b)", &shown);
	g_variant_unref (result);

	return shown;
}
