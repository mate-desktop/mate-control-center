/*
 * pp-monitors.c: the monitors as the panel numbers them
 *
 * Copyright (C) 2026 MATE Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

#include <config.h>

#include <string.h>

#include <glib/gi18n.h>

#include "pp-monitors.h"
#include "pp-panel.h"

static void
monitor_free (gpointer data)
{
	PpMonitor *monitor = data;

	g_free (monitor->id);
	g_free (monitor->connector);
	g_free (monitor->name);
	g_free (monitor);
}

static GPtrArray *
from_panel (void)
{
	GVariant *result = pp_panel_get_monitors (NULL);
	GPtrArray *monitors;
	GVariantIter *iter;
	PpMonitor monitor;
	const char *id, *connector, *name;

	if (result == NULL)
		return NULL;

	monitors = g_ptr_array_new_with_free_func (monitor_free);
	g_variant_get (result, "(a(isss(iiii)b))", &iter);
	while (g_variant_iter_next (iter, "(i&s&s&s(iiii)b)", &monitor.index, &id, &connector, &name,
				    &monitor.geometry.x, &monitor.geometry.y,
				    &monitor.geometry.width, &monitor.geometry.height,
				    &monitor.primary)) {
		PpMonitor *copy = g_new (PpMonitor, 1);

		*copy = monitor;

		copy->id = g_strdup (id);
		copy->connector = g_strdup (connector);
		copy->name = g_strdup (name);
		g_ptr_array_add (monitors, copy);
	}
	g_variant_iter_free (iter);
	g_variant_unref (result);

	if (monitors->len == 0)
		g_clear_pointer (&monitors, g_ptr_array_unref);
	return monitors;
}

static PpMonitor *
from_gdk_monitor (GdkMonitor *gdk_monitor, int index)
{
	PpMonitor *monitor = g_new0 (PpMonitor, 1);
	const char *manufacturer = gdk_monitor_get_manufacturer (gdk_monitor);
	const char *model = gdk_monitor_get_model (gdk_monitor);

	monitor->index = index;
	monitor->id = g_strdup ("");
	monitor->connector = g_strdup ("");
	if (model != NULL && *model != '\0' && manufacturer != NULL && *manufacturer != '\0' &&
	    g_ascii_strncasecmp (model, manufacturer, strlen (manufacturer)) != 0)
		monitor->name = g_strdup_printf ("%s %s", manufacturer, model);
	else
		monitor->name = g_strdup (model != NULL ? model : "");
	monitor->primary = gdk_monitor_is_primary (gdk_monitor);
	gdk_monitor_get_geometry (gdk_monitor, &monitor->geometry);
	return monitor;
}

static GPtrArray *
from_gdk (GdkDisplay *display)
{
	GPtrArray *monitors = g_ptr_array_new_with_free_func (monitor_free);
	GdkMonitor *primary;
	int n, i;

	if (display == NULL)
		return monitors;

	n = gdk_display_get_n_monitors (display);
	primary = gdk_display_get_primary_monitor (display);
	if (primary == NULL && n > 0)
		primary = gdk_display_get_monitor (display, 0);
	if (primary != NULL) {
		g_ptr_array_add (monitors, from_gdk_monitor (primary, 0));
		((PpMonitor *) monitors->pdata[0])->primary = TRUE;
	}
	for (i = 0; i < n; i++) {
		GdkMonitor *gdk_monitor = gdk_display_get_monitor (display, i);

		if (gdk_monitor != primary)
			g_ptr_array_add (monitors, from_gdk_monitor (gdk_monitor, monitors->len));
	}
	return monitors;
}

GPtrArray *
pp_monitors_new (GdkDisplay *display)
{
	GPtrArray *monitors = from_panel ();

	return monitors != NULL ? monitors : from_gdk (display);
}

const PpMonitor *
pp_monitors_lookup (GPtrArray *monitors, int index)
{
	guint i;

	for (i = 0; monitors != NULL && i < monitors->len; i++) {
		const PpMonitor *monitor = monitors->pdata[i];

		if (monitor->index == index)
			return monitor;
	}
	return NULL;
}

char *
pp_monitor_label (const PpMonitor *monitor)
{
	gboolean has_name = monitor->name != NULL && monitor->name[0] != '\0';

	if (monitor->connector != NULL && monitor->connector[0] != '\0') {
		if (has_name)
			return g_strdup_printf (C_("Monitor", "%s (%s)"), monitor->name, monitor->connector);
		return g_strdup (monitor->connector);
	}
	if (has_name)
		return g_strdup_printf (_("Monitor %d: %s"), monitor->index + 1, monitor->name);
	return g_strdup_printf (_("Monitor %d"), monitor->index + 1);
}

void
pp_monitors_assign (GSettings *toplevel, const PpMonitor *monitor)
{
	GSettingsSchema *schema = NULL;

	g_object_get (toplevel, "settings-schema", &schema, NULL);
	if (schema != NULL && g_settings_schema_has_key (schema, "monitor-id") &&
	    monitor->id != NULL && monitor->id[0] != '\0')
		g_settings_set_string (toplevel, "monitor-id", monitor->id);
	if (schema != NULL)
		g_settings_schema_unref (schema);
	g_settings_set_int (toplevel, "monitor", monitor->index);
}
