/*
 * test-panel-monitors.c: the capplet numbers and names monitors as the panel
 * does, and its Monitor selector moves the selected panel.
 *
 * Copyright (C) 2026 MATE Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * It changes the panel settings, so it only runs in a test session that sets
 * PP_TEST_MONITORS=N, and is skipped without a display. It expects N
 * monitors, with PP_TEST_PANEL=1 the panel to be running, and with
 * PP_TEST_CONNECTORS=1 each monitor to have a connector name.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "pp-config.h"
#include "pp-monitors.h"
#include "pp-panel.h"
#include "pp-window.h"

#define CHECK(cond, ...) G_STMT_START {                                 \
	if (!(cond)) {                                                  \
		fprintf (stderr, "%s:%d: check failed: %s: ", __FILE__, \
			 __LINE__, #cond);                              \
		fprintf (stderr, __VA_ARGS__);                          \
		fprintf (stderr, "\n");                                 \
		exit (1);                                               \
	}                                                               \
} G_STMT_END

static GtkWidget *
find (GtkWidget *widget, const char *name)
{
	GList *children, *l;
	GtkWidget *found = NULL;

	if (g_strcmp0 (gtk_widget_get_name (widget), name) == 0)
		return widget;
	if (!GTK_IS_CONTAINER (widget))
		return NULL;

	children = gtk_container_get_children (GTK_CONTAINER (widget));
	for (l = children; l != NULL && found == NULL; l = l->next)
		found = find (l->data, name);
	g_list_free (children);
	return found;
}

static void
spin (int ms)
{
	gint64 end = g_get_monotonic_time () + ms * 1000;

	while (g_get_monotonic_time () < end)
		g_main_context_iteration (NULL, FALSE);
}

static gboolean
wait_for_monitor (GSettings *toplevel, int monitor)
{
	gint64 end = g_get_monotonic_time () + 5 * G_USEC_PER_SEC;

	while (g_settings_get_int (toplevel, "monitor") != monitor && g_get_monotonic_time () < end)
		g_main_context_iteration (NULL, FALSE);
	return g_settings_get_int (toplevel, "monitor") == monitor;
}

static gboolean
wait_for_active (GtkWidget *combo, int active)
{
	gint64 end = g_get_monotonic_time () + 5 * G_USEC_PER_SEC;

	while (gtk_combo_box_get_active (GTK_COMBO_BOX (combo)) != active && g_get_monotonic_time () < end)
		g_main_context_iteration (NULL, FALSE);
	return gtk_combo_box_get_active (GTK_COMBO_BOX (combo)) == active;
}

static int
count_rows (GtkWidget *combo)
{
	return gtk_tree_model_iter_n_children (gtk_combo_box_get_model (GTK_COMBO_BOX (combo)), NULL);
}

static char *
active_text (GtkWidget *combo)
{
	return gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (combo));
}

int
main (int argc, char **argv)
{
	const char *wanted = g_getenv ("PP_TEST_MONITORS");
	gboolean with_panel = g_strcmp0 (g_getenv ("PP_TEST_PANEL"), "1") == 0;
	gboolean connectors = g_strcmp0 (g_getenv ("PP_TEST_CONNECTORS"), "1") == 0;
	GPtrArray *monitors;
	GtkWidget *window, *panels, *monitor_combo;
	GSettings *toplevel;
	const char *toplevel_id;
	const PpMonitor *target;
	char *label, *text;
	guint i;

	if (wanted == NULL || !gtk_init_check (&argc, &argv) || !pp_config_available ())
		return 77;

	CHECK (pp_panel_is_running () == with_panel, "the panel is%s running",
	       with_panel ? " not" : "");

	monitors = pp_monitors_new (gdk_display_get_default ());
	CHECK ((int) monitors->len == atoi (wanted), "%u monitors, not %s", monitors->len, wanted);
	for (i = 0; i < monitors->len; i++) {
		const PpMonitor *monitor = monitors->pdata[i];

		CHECK (monitor->index == (int) i, "monitor %u has index %d", i, monitor->index);
		CHECK (monitor->geometry.width > 0 && monitor->geometry.height > 0,
		       "monitor %u has no size", i);
		CHECK (monitor->primary == (i == 0), "monitor %u: primary %d", i, monitor->primary);
		if (connectors)
			CHECK (monitor->connector[0] != '\0', "monitor %u has no connector", i);
		label = pp_monitor_label (monitor);
		if (monitor->connector[0] != '\0')
			CHECK (strstr (label, monitor->connector) != NULL, "label '%s' lacks %s",
			       label, monitor->connector);
		g_free (label);
	}

	window = pp_window_new (NULL);
	gtk_widget_show_all (window);
	spin (300);

	panels = find (window, "panel-combo");
	monitor_combo = find (window, "monitor-combo");
	CHECK (panels != NULL && monitor_combo != NULL, "no panel or monitor selector");
	CHECK (count_rows (monitor_combo) >= (int) monitors->len, "%d monitors listed, %u known",
	       count_rows (monitor_combo), monitors->len);
	CHECK (gtk_widget_get_sensitive (monitor_combo) == (monitors->len > 1),
	       "the monitor selector is%s sensitive with %u monitors",
	       gtk_widget_get_sensitive (monitor_combo) ? "" : " not", monitors->len);

	toplevel_id = gtk_combo_box_get_active_id (GTK_COMBO_BOX (panels));
	CHECK (toplevel_id != NULL, "no panel selected");
	toplevel = pp_config_toplevel (toplevel_id);

	if (monitors->len > 1) {
		int from = g_settings_get_int (toplevel, "monitor");

		target = pp_monitors_lookup (monitors, from == 0 ? 1 : 0);
		gtk_combo_box_set_active (GTK_COMBO_BOX (monitor_combo), target->index);
		CHECK (wait_for_monitor (toplevel, target->index),
		       "the panel's monitor is %d, not %d",
		       g_settings_get_int (toplevel, "monitor"), target->index);
		if (target->id[0] != '\0') {
			char *id = g_settings_get_string (toplevel, "monitor-id");

			CHECK (g_strcmp0 (id, target->id) == 0, "monitor-id is '%s', not '%s'", id, target->id);
			g_free (id);
		}

		label = pp_monitor_label (target);
		text = active_text (panels);
		CHECK (text != NULL && strstr (text, label) != NULL, "the panel is listed as '%s', not on %s",
		       text, label);
		g_free (text);
		g_free (label);

		g_settings_set_int (toplevel, "monitor", from);
		CHECK (wait_for_active (monitor_combo, from),
		       "the selector did not follow a move made elsewhere");
	}

	g_object_unref (toplevel);
	gtk_widget_destroy (window);
	g_ptr_array_unref (monitors);
	printf ("ok\n");
	return 0;
}
