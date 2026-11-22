/*
 * pp-config.c: GSettings backed model of the MATE panel configuration
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
#include <glib/gstdio.h>
#include <gio/gdesktopappinfo.h>

#include "pp-config.h"
#include "pp-panel.h"

#define GENERAL_SCHEMA    "org.mate.panel"
#define TOPLEVEL_SCHEMA   "org.mate.panel.toplevel"
#define BACKGROUND_SCHEMA "org.mate.panel.toplevel.background"
#define OBJECT_SCHEMA     "org.mate.panel.object"
#define TOPLEVEL_PATH     "/org/mate/panel/toplevels/"
#define OBJECT_PATH       "/org/mate/panel/objects/"
#define GENERAL_PATH      "/org/mate/panel/general/"

#define ORIENT_TOP    1
#define ORIENT_RIGHT  2
#define ORIENT_BOTTOM 4
#define ORIENT_LEFT   8

#define MAX_MOVE_STEPS 3
#define PANEL_RESTART_DELAY_MS 1000

static GHashTable *applet_info = NULL;

static GSettingsSchema *
lookup_schema (const char *id)
{
	GSettingsSchemaSource *source = g_settings_schema_source_get_default ();

	if (source == NULL)
		return NULL;

	return g_settings_schema_source_lookup (source, id, TRUE);
}

gboolean
pp_config_available (void)
{
	const char *ids[] = { GENERAL_SCHEMA, TOPLEVEL_SCHEMA, BACKGROUND_SCHEMA, OBJECT_SCHEMA };
	guint i;

	for (i = 0; i < G_N_ELEMENTS (ids); i++) {
		GSettingsSchema *schema = lookup_schema (ids[i]);

		if (schema == NULL)
			return FALSE;
		g_settings_schema_unref (schema);
	}

	return TRUE;
}

gboolean
pp_config_has_pack_keys (void)
{
	GSettingsSchema *schema = lookup_schema (OBJECT_SCHEMA);
	gboolean result;

	result = schema != NULL &&
		 g_settings_schema_has_key (schema, "pack-type") &&
		 g_settings_schema_has_key (schema, "pack-index");
	g_clear_pointer (&schema, g_settings_schema_unref);

	return result;
}

gboolean
pp_config_locked_down (void)
{
	GSettings *general = pp_config_general ();
	gboolean locked_down = g_settings_get_boolean (general, "locked-down");

	g_object_unref (general);
	return locked_down;
}

GSettings *
pp_config_general (void)
{
	return g_settings_new_with_path (GENERAL_SCHEMA, GENERAL_PATH);
}

GSettings *
pp_config_toplevel (const char *toplevel_id)
{
	char *path = g_strdup_printf (TOPLEVEL_PATH "%s/", toplevel_id);
	GSettings *settings = g_settings_new_with_path (TOPLEVEL_SCHEMA, path);

	g_free (path);
	return settings;
}

GSettings *
pp_config_background (const char *toplevel_id)
{
	char *path = g_strdup_printf (TOPLEVEL_PATH "%s/background/", toplevel_id);
	GSettings *settings = g_settings_new_with_path (BACKGROUND_SCHEMA, path);

	g_free (path);
	return settings;
}

GSettings *
pp_config_object (const char *object_id)
{
	char *path = g_strdup_printf (OBJECT_PATH "%s/", object_id);
	GSettings *settings = g_settings_new_with_path (OBJECT_SCHEMA, path);

	g_free (path);
	return settings;
}

static void
reset_settings (GSettings *settings)
{
	GSettingsSchema *schema = NULL;
	char **keys;
	int i;

	g_object_get (settings, "settings-schema", &schema, NULL);
	if (schema == NULL)
		return;

	keys = g_settings_schema_list_keys (schema);
	for (i = 0; keys[i] != NULL; i++)
		g_settings_reset (settings, keys[i]);

	g_strfreev (keys);
	g_settings_schema_unref (schema);
}

static void
reset_object (const char *object_id)
{
	GSettings *settings = pp_config_object (object_id);

	reset_settings (settings);
	g_object_unref (settings);
}

static void
reset_toplevel (const char *toplevel_id)
{
	GSettings *settings = pp_config_toplevel (toplevel_id);
	GSettings *background = pp_config_background (toplevel_id);

	reset_settings (settings);
	reset_settings (background);
	g_object_unref (settings);
	g_object_unref (background);
}

static char **
get_list (const char *key)
{
	GSettings *general = pp_config_general ();
	char **list = g_settings_get_strv (general, key);

	g_object_unref (general);
	return list;
}

static void
set_list (const char *key, GPtrArray *items)
{
	GSettings *general = pp_config_general ();

	g_ptr_array_add (items, NULL);
	g_settings_set_strv (general, key, (const char * const *) items->pdata);
	g_ptr_array_remove_index (items, items->len - 1);
	g_settings_sync ();
	g_object_unref (general);
}

static void
remove_from_list (const char *key, const char *id)
{
	char **list = get_list (key);
	GPtrArray *items = g_ptr_array_new ();
	int i;

	for (i = 0; list[i] != NULL; i++) {
		if (g_strcmp0 (list[i], id) != 0)
			g_ptr_array_add (items, list[i]);
	}

	set_list (key, items);
	g_ptr_array_unref (items);
	g_strfreev (list);
}

static gboolean
list_contains (char **list, const char *id)
{
	return list != NULL && g_strv_contains ((const char * const *) list, id);
}

static GHashTable *
attached_toplevels (void)
{
	GHashTable *set = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	char **objects = get_list ("object-id-list");
	int i;

	for (i = 0; objects[i] != NULL; i++) {
		GSettings *settings = pp_config_object (objects[i]);
		char *attached = g_settings_get_string (settings, "attached-toplevel-id");

		if (attached != NULL && attached[0] != '\0')
			g_hash_table_add (set, attached);
		else
			g_free (attached);

		g_object_unref (settings);
	}

	g_strfreev (objects);
	return set;
}

char **
pp_config_object_ids (void)
{
	return get_list ("object-id-list");
}

char **
pp_config_toplevel_ids (void)
{
	char **all = get_list ("toplevel-id-list");
	GHashTable *attached = attached_toplevels ();
	GPtrArray *result = g_ptr_array_new ();
	int i;

	for (i = 0; all[i] != NULL; i++) {
		if (!g_hash_table_contains (attached, all[i]))
			g_ptr_array_add (result, g_strdup (all[i]));
	}

	g_ptr_array_add (result, NULL);
	g_hash_table_unref (attached);
	g_strfreev (all);

	return (char **) g_ptr_array_free (result, FALSE);
}

static const char *
orientation_name (int orientation)
{
	switch (orientation) {
	case ORIENT_RIGHT:
		return C_("Orientation", "Right");
	case ORIENT_BOTTOM:
		return C_("Orientation", "Bottom");
	case ORIENT_LEFT:
		return C_("Orientation", "Left");
	default:
		return C_("Orientation", "Top");
	}
}

char *
pp_config_toplevel_label (const char *toplevel_id)
{
	GSettings *settings = pp_config_toplevel (toplevel_id);
	char *name = g_settings_get_string (settings, "name");
	int orientation = g_settings_get_enum (settings, "orientation");
	int monitor = g_settings_get_int (settings, "monitor");
	char *label;

	if (name != NULL && name[0] != '\0')
		label = g_strdup (name);
	else
		label = g_strdup_printf (_("%s Panel, Monitor %d"),
					 orientation_name (orientation), monitor + 1);

	g_free (name);
	g_object_unref (settings);
	return label;
}

char *
pp_config_toplevel_create (GdkDisplay *display)
{
	static const int order[] = { ORIENT_TOP, ORIENT_BOTTOM, ORIENT_RIGHT, ORIENT_LEFT };
	char **all = get_list ("toplevel-id-list");
	GPtrArray *items = g_ptr_array_new ();
	GSettings *settings;
	int n_monitors = display != NULL ? MAX (1, gdk_display_get_n_monitors (display)) : 1;
	int *filled = g_new0 (int, n_monitors);
	int monitor = 0;
	int orientation = ORIENT_TOP;
	gboolean found = FALSE;
	char *id = NULL;
	int i;

	for (i = 0; all[i] != NULL; i++) {
		GSettings *existing = pp_config_toplevel (all[i]);
		int m = g_settings_get_int (existing, "monitor");

		if (m >= 0 && m < n_monitors)
			filled[m] |= g_settings_get_enum (existing, "orientation");

		g_object_unref (existing);
		g_ptr_array_add (items, all[i]);
	}

	for (i = 0; id == NULL; i++) {
		id = g_strdup_printf ("toplevel-%d", i);
		if (list_contains (all, id))
			g_clear_pointer (&id, g_free);
	}

	for (i = 0; i < n_monitors && !found; i++) {
		guint k;

		for (k = 0; k < G_N_ELEMENTS (order); k++) {
			if (!(filled[i] & order[k])) {
				monitor = i;
				orientation = order[k];
				found = TRUE;
				break;
			}
		}
	}

	reset_toplevel (id);
	settings = pp_config_toplevel (id);
	g_settings_set_int (settings, "screen", 0);
	if (found) {
		g_settings_set_int (settings, "monitor", monitor);
		g_settings_set_enum (settings, "orientation", orientation);
	}
	g_settings_sync ();
	g_object_unref (settings);

	g_ptr_array_add (items, id);
	set_list ("toplevel-id-list", items);

	g_ptr_array_unref (items);
	g_strfreev (all);
	g_free (filled);

	return id;
}

static void
delete_toplevel_internal (const char *toplevel_id_in, gboolean cleanup)
{
	g_autofree char *toplevel_id = g_strdup (toplevel_id_in);
	char **objects = get_list ("object-id-list");
	GPtrArray *keep = g_ptr_array_new ();
	GPtrArray *removed = g_ptr_array_new ();
	guint i;
	int j;

	for (j = 0; objects[j] != NULL; j++) {
		GSettings *settings = pp_config_object (objects[j]);
		char *parent = g_settings_get_string (settings, "toplevel-id");

		if (g_strcmp0 (parent, toplevel_id) == 0)
			g_ptr_array_add (removed, objects[j]);
		else
			g_ptr_array_add (keep, objects[j]);

		g_free (parent);
		g_object_unref (settings);
	}

	set_list ("object-id-list", keep);

	for (i = 0; i < removed->len; i++) {
		GSettings *settings = pp_config_object (removed->pdata[i]);
		char *attached = g_settings_get_string (settings, "attached-toplevel-id");

		g_object_unref (settings);
		if (attached != NULL && attached[0] != '\0')
			delete_toplevel_internal (attached, cleanup);
		g_free (attached);

		if (cleanup)
			reset_object (removed->pdata[i]);
	}

	remove_from_list ("toplevel-id-list", toplevel_id);
	if (cleanup)
		reset_toplevel (toplevel_id);

	g_settings_sync ();
	g_ptr_array_unref (keep);
	g_ptr_array_unref (removed);
	g_strfreev (objects);
}

gboolean
pp_config_toplevel_delete (const char *toplevel_id)
{
	char **visible = pp_config_toplevel_ids ();
	char *id = g_strdup (toplevel_id);
	gboolean allowed = g_strv_length (visible) > 1 && list_contains (visible, id);

	g_strfreev (visible);
	if (allowed)
		delete_toplevel_internal (id, !pp_panel_is_running ());

	g_free (id);
	return allowed;
}

static gboolean
restart_panel_cb (gpointer user_data)
{
	char *argv[] = { "mate-panel", "--replace", NULL };

	g_spawn_async (NULL, argv, NULL,
		       G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
		       NULL, NULL, NULL, NULL);

	return G_SOURCE_REMOVE;
}

void
pp_config_reset_all (void)
{
	gboolean running = pp_panel_is_running ();
	GPtrArray *empty = g_ptr_array_new ();
	char **objects = get_list ("object-id-list");
	char **toplevels = get_list ("toplevel-id-list");
	int i;

	set_list ("object-id-list", empty);
	set_list ("toplevel-id-list", empty);
	g_ptr_array_unref (empty);

	if (!running) {
		for (i = 0; objects[i] != NULL; i++)
			reset_object (objects[i]);
		for (i = 0; toplevels[i] != NULL; i++)
			reset_toplevel (toplevels[i]);
		g_settings_sync ();
	} else {
		g_timeout_add (PANEL_RESTART_DELAY_MS, restart_panel_cb, NULL);
	}

	g_strfreev (objects);
	g_strfreev (toplevels);
}

static void
load_applet_info (void)
{
	const char * const *dirs = g_get_system_data_dirs ();
	GPtrArray *roots = g_ptr_array_new_with_free_func (g_free);
	guint r;
	int i;

	applet_info = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_key_file_unref);

	for (i = 0; dirs[i] != NULL; i++)
		g_ptr_array_insert (roots, 0, g_build_filename (dirs[i], "mate-panel", "applets", NULL));
	g_ptr_array_add (roots, g_build_filename (g_get_user_data_dir (), "mate-panel", "applets", NULL));

	for (r = 0; r < roots->len; r++) {
		GDir *dir = g_dir_open (roots->pdata[r], 0, NULL);
		const char *file;

		if (dir == NULL)
			continue;

		while ((file = g_dir_read_name (dir)) != NULL) {
			GKeyFile *key_file;
			char *path;
			char *factory = NULL;

			if (!g_str_has_suffix (file, ".mate-panel-applet"))
				continue;

			path = g_build_filename (roots->pdata[r], file, NULL);
			key_file = g_key_file_new ();
			if (g_key_file_load_from_file (key_file, path, G_KEY_FILE_NONE, NULL))
				factory = g_key_file_get_string (key_file, "Applet Factory", "Id", NULL);

			if (factory != NULL) {
				gsize n, k;
				char **groups = g_key_file_get_groups (key_file, &n);

				for (k = 0; k < n; k++) {
					GKeyFile *entry;
					char *name;
					char *icon;

					if (g_strcmp0 (groups[k], "Applet Factory") == 0)
						continue;

					name = g_key_file_get_locale_string (key_file, groups[k], "Name", NULL, NULL);
					icon = g_key_file_get_string (key_file, groups[k], "Icon", NULL);
					entry = g_key_file_new ();
					g_key_file_set_string (entry, "e", "name", name ? name : groups[k]);
					g_key_file_set_string (entry, "e", "icon", icon ? icon : "");
					g_hash_table_replace (applet_info,
							      g_strdup_printf ("%s::%s", factory, groups[k]),
							      entry);
					g_free (name);
					g_free (icon);
				}
				g_strfreev (groups);
				g_free (factory);
			}
			g_key_file_free (key_file);
			g_free (path);
		}
		g_dir_close (dir);
	}

	g_ptr_array_unref (roots);
}

static char *
launchers_dir (void)
{
	return g_build_filename (g_get_user_config_dir (), "mate", "panel2.d", "default", "launchers", NULL);
}

static GDesktopAppInfo *
resolve_launcher (const char *location, char **path_out)
{
	GDesktopAppInfo *info = NULL;
	char *path = NULL;

	*path_out = NULL;
	if (location == NULL || location[0] == '\0')
		return NULL;

	if (g_str_has_prefix (location, "file://"))
		path = g_filename_from_uri (location, NULL, NULL);
	else if (g_path_is_absolute (location))
		path = g_strdup (location);
	else {
		char *dir = launchers_dir ();

		path = g_build_filename (dir, location, NULL);
		g_free (dir);
		if (!g_file_test (path, G_FILE_TEST_EXISTS))
			g_clear_pointer (&path, g_free);
	}

	if (path != NULL)
		info = g_desktop_app_info_new_from_filename (path);
	if (info == NULL && path == NULL)
		info = g_desktop_app_info_new (location);
	if (info != NULL && path == NULL && g_desktop_app_info_get_filename (info) != NULL)
		path = g_strdup (g_desktop_app_info_get_filename (info));

	*path_out = path;
	return info;
}

static void
describe_action (PpObject *object, const char *action)
{
	static const struct {
		const char *action;
		const char *name;
		const char *icon;
	} actions[] = {
		{ "lock",           N_("Lock Screen"),       "system-lock-screen" },
		{ "logout",         N_("Log Out"),           "system-log-out" },
		{ "run",            N_("Run Application"),   "system-run" },
		{ "search",         N_("Search for Files"),  "system-search" },
		{ "force-quit",     N_("Force Quit"),        "process-stop" },
		{ "connect-server", N_("Connect to Server"), "network-server" },
		{ "shutdown",       N_("Shut Down"),         "system-shutdown" }
	};
	guint i;

	for (i = 0; i < G_N_ELEMENTS (actions); i++) {
		if (g_strcmp0 (actions[i].action, action) == 0) {
			object->name = g_strdup (_(actions[i].name));
			object->icon = g_themed_icon_new (actions[i].icon);
			return;
		}
	}

	object->name = g_strdup (_("Action Button"));
	object->icon = g_themed_icon_new ("system-run");
}

static void
describe_object (PpObject *object, GSettings *settings)
{
	char *type = g_settings_get_string (settings, "object-type");

	object->type = type;

	if (g_strcmp0 (type, "applet") == 0) {
		char *iid = g_settings_get_string (settings, "applet-iid");
		GKeyFile *entry;

		if (applet_info == NULL)
			load_applet_info ();

		entry = g_hash_table_lookup (applet_info, iid);
		if (entry != NULL) {
			char *icon = g_key_file_get_string (entry, "e", "icon", NULL);

			object->name = g_key_file_get_string (entry, "e", "name", NULL);
			if (icon != NULL && icon[0] != '\0')
				object->icon = g_icon_new_for_string (icon, NULL);
			g_free (icon);
		} else {
			object->name = g_strdup (iid != NULL && iid[0] != '\0' ? iid : _("Applet"));
		}
		g_free (iid);
	} else if (g_strcmp0 (type, "launcher") == 0) {
		char *location = g_settings_get_string (settings, "launcher-location");
		GDesktopAppInfo *info = resolve_launcher (location, &object->launcher);

		if (info != NULL) {
			object->name = g_strdup (g_app_info_get_name (G_APP_INFO (info)));
			if (g_app_info_get_icon (G_APP_INFO (info)) != NULL)
				object->icon = g_object_ref (g_app_info_get_icon (G_APP_INFO (info)));
			g_object_unref (info);
		}
		if (object->name == NULL)
			object->name = g_strdup (_("Launcher"));
		g_free (location);
	} else if (g_strcmp0 (type, "menu") == 0) {
		object->name = g_strdup (_("Main Menu"));
		object->icon = g_themed_icon_new ("start-here");
	} else if (g_strcmp0 (type, "menu-bar") == 0) {
		object->name = g_strdup (_("Menu Bar"));
		object->icon = g_themed_icon_new ("start-here");
	} else if (g_strcmp0 (type, "separator") == 0) {
		object->name = g_strdup (_("Separator"));
		object->icon = g_themed_icon_new ("view-more-horizontal-symbolic");
	} else if (g_strcmp0 (type, "drawer") == 0) {
		char *tooltip = g_settings_get_string (settings, "tooltip");

		object->name = g_strdup (tooltip != NULL && tooltip[0] != '\0' ? tooltip : _("Drawer"));
		object->icon = g_themed_icon_new ("folder");
		g_free (tooltip);
	} else if (g_strcmp0 (type, "action") == 0) {
		char *action = g_settings_get_string (settings, "action-type");

		describe_action (object, action);
		g_free (action);
	} else {
		object->name = g_strdup (_("Unknown Item"));
	}

	if (object->icon == NULL)
		object->icon = g_themed_icon_new ("application-x-executable");
}

void
pp_object_free (PpObject *object)
{
	if (object == NULL)
		return;

	g_free (object->id);
	g_free (object->name);
	g_free (object->type);
	g_free (object->launcher);
	g_clear_object (&object->icon);
	g_free (object);
}

static int
settings_compare (gconstpointer a, gconstpointer b, gpointer user_data)
{
	const PpObject *x = *(PpObject * const *) a;
	const PpObject *y = *(PpObject * const *) b;
	gboolean pack = GPOINTER_TO_INT (user_data);

	if (x->align != y->align)
		return (int) x->align - (int) y->align;
	if (x->index != y->index)
		return pack && x->align == PP_ALIGN_END ? y->index - x->index : x->index - y->index;
	if (x->position != y->position) {
		if (!pack && x->align == PP_ALIGN_END)
			return y->position - x->position;
		return x->position - y->position;
	}

	return g_strcmp0 (x->id, y->id);
}

static int
panel_order_compare (gconstpointer a, gconstpointer b, gpointer user_data)
{
	const PpObject *x = *(PpObject * const *) a;
	const PpObject *y = *(PpObject * const *) b;
	GHashTable *rank = user_data;
	int rx = GPOINTER_TO_INT (g_hash_table_lookup (rank, x->id));
	int ry = GPOINTER_TO_INT (g_hash_table_lookup (rank, y->id));

	if (rx != ry)
		return (rx == 0 ? G_MAXINT : rx) < (ry == 0 ? G_MAXINT : ry) ? -1 : 1;

	return 0;
}

GPtrArray *
pp_config_objects (const char *toplevel_id)
{
	GPtrArray *result = g_ptr_array_new_with_free_func ((GDestroyNotify) pp_object_free);
	char **objects = get_list ("object-id-list");
	char **panel_order;
	gboolean pack = pp_config_has_pack_keys ();
	int i;

	for (i = 0; objects[i] != NULL; i++) {
		GSettings *settings = pp_config_object (objects[i]);
		char *parent = g_settings_get_string (settings, "toplevel-id");

		if (g_strcmp0 (parent, toplevel_id) == 0) {
			PpObject *object = g_new0 (PpObject, 1);

			object->id = g_strdup (objects[i]);
			object->position = g_settings_get_int (settings, "position");
			object->locked = g_settings_get_boolean (settings, "locked");
			object->align = g_settings_get_boolean (settings, "panel-right-stick") ? PP_ALIGN_END : PP_ALIGN_START;
			if (pack) {
				int pack_type = g_settings_get_enum (settings, "pack-type");

				object->index = g_settings_get_int (settings, "pack-index");
				if (pack_type == PP_ALIGN_CENTER || pack_type == PP_ALIGN_END)
					object->align = pack_type;
			}
			describe_object (object, settings);
			g_ptr_array_add (result, object);
		}

		g_free (parent);
		g_object_unref (settings);
	}

	g_ptr_array_sort_with_data (result, settings_compare, GINT_TO_POINTER (pack));

	panel_order = pp_panel_is_running () ? pp_panel_get_objects (toplevel_id, NULL) : NULL;
	if (panel_order != NULL) {
		GHashTable *rank = g_hash_table_new (g_str_hash, g_str_equal);

		for (i = 0; panel_order[i] != NULL; i++)
			g_hash_table_insert (rank, panel_order[i], GINT_TO_POINTER (i + 1));

		g_ptr_array_sort_with_data (result, panel_order_compare, rank);
		g_hash_table_unref (rank);
		g_strfreev (panel_order);
	}

	g_strfreev (objects);
	return result;
}

static int
index_in (char **ids, const char *id)
{
	int i;

	for (i = 0; ids != NULL && ids[i] != NULL; i++) {
		if (g_strcmp0 (ids[i], id) == 0)
			return i;
	}

	return -1;
}

gboolean
pp_config_object_move (const char *toplevel_id, const char *object_id, int direction, GError **error)
{
	char **before;
	int start;
	int step;
	gboolean moved_any = FALSE;

	before = pp_panel_get_objects (toplevel_id, error);
	if (before == NULL)
		return FALSE;

	start = index_in (before, object_id);
	g_strfreev (before);
	if (start < 0)
		return FALSE;

	for (step = 0; step < MAX_MOVE_STEPS; step++) {
		char **after;
		int now;

		if (!pp_panel_move_object (object_id, direction, error))
			break;

		moved_any = TRUE;
		after = pp_panel_get_objects (toplevel_id, error);
		now = index_in (after, object_id);
		g_strfreev (after);

		if (now != start)
			break;
	}

	return moved_any;
}

void
pp_config_object_set_locked (const char *object_id, gboolean locked)
{
	GSettings *settings = pp_config_object (object_id);

	g_settings_set_boolean (settings, "locked", locked);
	g_settings_sync ();
	g_object_unref (settings);
}

void
pp_config_object_remove (const char *object_id_in)
{
	g_autofree char *object_id = g_strdup (object_id_in);
	gboolean cleanup = !pp_panel_is_running ();
	GSettings *settings = pp_config_object (object_id);
	char *attached = g_settings_get_string (settings, "attached-toplevel-id");

	g_object_unref (settings);
	remove_from_list ("object-id-list", object_id);

	if (attached != NULL && attached[0] != '\0')
		delete_toplevel_internal (attached, cleanup);

	if (cleanup) {
		reset_object (object_id);
		g_settings_sync ();
	}

	g_free (attached);
}

gboolean
pp_config_object_can_edit (PpObject *object)
{
	if (g_strcmp0 (object->type, "drawer") == 0)
		return pp_panel_is_running ();

	return g_strcmp0 (object->type, "launcher") == 0;
}

static char *
writable_launcher_copy (PpObject *object, GError **error)
{
	char *dir = launchers_dir ();
	char *base;
	char *copy;
	GFile *src;
	GFile *dst;
	gboolean ok;
	int n = 0;

	if (g_str_has_prefix (object->launcher, dir) && g_access (object->launcher, W_OK) == 0) {
		g_free (dir);
		return g_strdup (object->launcher);
	}

	g_mkdir_with_parents (dir, 0700);
	base = g_path_get_basename (object->launcher);
	copy = g_build_filename (dir, base, NULL);
	while (g_file_test (copy, G_FILE_TEST_EXISTS)) {
		char *name = g_strdup_printf ("%d-%s", ++n, base);

		g_free (copy);
		copy = g_build_filename (dir, name, NULL);
		g_free (name);
	}

	src = g_file_new_for_path (object->launcher);
	dst = g_file_new_for_path (copy);
	ok = g_file_copy (src, dst, G_FILE_COPY_NONE, NULL, NULL, NULL, error);
	g_object_unref (src);
	g_object_unref (dst);
	g_free (base);
	g_free (dir);

	if (!ok) {
		g_free (copy);
		return NULL;
	}

	{
		GSettings *settings = pp_config_object (object->id);

		g_settings_set_string (settings, "launcher-location", copy);
		g_settings_sync ();
		g_object_unref (settings);
	}

	return copy;
}

gboolean
pp_config_edit_object (PpObject *object, GError **error)
{
	GSubprocess *process;
	char *target;

	if (pp_panel_is_running ()) {
		if (pp_panel_show_properties (object->id, error))
			return TRUE;
		if (error != NULL && *error != NULL)
			return FALSE;
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
				     _("This item has no properties to edit."));
		return FALSE;
	}

	if (g_strcmp0 (object->type, "launcher") != 0 || object->launcher == NULL) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
				     _("This item can only be edited while the panel is running."));
		return FALSE;
	}

	target = writable_launcher_copy (object, error);
	if (target == NULL)
		return FALSE;

	process = g_subprocess_new (G_SUBPROCESS_FLAGS_NONE, error, "mate-desktop-item-edit", target, NULL);
	g_free (target);
	if (process == NULL)
		return FALSE;

	g_object_unref (process);
	return TRUE;
}
