/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/* mate-wm-manager.c
 * Copyright (C) 2002 Seth Nickell
 * Copyright (C) 1998, 2002 Red Hat, Inc.
 *
 * Written by: Seth Nickell <snickell@stanford.edu>,
 *             Havoc Pennington <hp@redhat.com>
 *             Owen Taylor <otaylor@redhat.com>,
 *             Bradford Hovinen <hovinen@helixcode.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <config.h>
#include "mate-wm-manager.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <sys/types.h>
#include <dirent.h>
#include <string.h>

typedef struct {
        MateDesktopItem   *ditem;
        char              *name; /* human readable, localized */
        char              *identify_name; /* name we expect to be set on the screen */
        char              *exec;
        char              *config_exec;
        char              *config_tryexec;
        char              *module;
        gboolean           session_managed;
        gboolean           is_user;
        gboolean           is_present;
        gboolean           is_config_present;
        MateWindowManager *mate_wm;
} AvailableWindowManager;

static gboolean done_scan = FALSE;
static GList *available_wms;

static void
wm_free (AvailableWindowManager *wm)
{
        g_free (wm->name);
        g_free (wm->exec);
        g_free (wm->config_exec);
        g_free (wm->config_tryexec);
        g_free (wm->module);
        g_free (wm->identify_name);

        g_free (wm);
}

static GList *
list_desktop_files_in_dir (const gchar *directory)
{
        DIR *dir;
        struct dirent *child;
        GList *result = NULL;
        gchar *suffix;

        if ((dir = opendir (directory)) == NULL)
                return NULL;

        while ((child = readdir (dir)) != NULL) {
                /* Ignore files without .desktop suffix, and ignore
                 * .desktop files with no prefix
                 */
                suffix = child->d_name + strlen (child->d_name) - 8;
                /* strlen(".desktop") == 8 */

                if (suffix <= child->d_name ||
                    strcmp (suffix, ".desktop") != 0)
                        continue;

                result = g_list_prepend (result,
                                         g_build_filename (directory, child->d_name, NULL));
        }
        closedir (dir);

        return result;
}

static gint
wm_compare (gconstpointer a, gconstpointer b)
{
        const AvailableWindowManager *wm_a = (const AvailableWindowManager *)a;
        const AvailableWindowManager *wm_b = (const AvailableWindowManager *)b;

        /* mmm, sloooow */

        return g_utf8_collate (mate_desktop_item_get_string (wm_a->ditem, MATE_DESKTOP_ITEM_NAME),
                               mate_desktop_item_get_string (wm_b->ditem, MATE_DESKTOP_ITEM_NAME));
}

static AvailableWindowManager*
wm_load (const char *desktop_file,
         gboolean    is_user)
{
        gchar *path;
        AvailableWindowManager *wm;

        wm = g_new0 (AvailableWindowManager, 1);

        wm->ditem = mate_desktop_item_new_from_file (desktop_file, 0, NULL);

        if (wm->ditem == NULL) {
                g_free (wm);
                return NULL;
        }

        mate_desktop_item_set_entry_type (wm->ditem, MATE_DESKTOP_ITEM_TYPE_APPLICATION);

        wm->exec = g_strdup (mate_desktop_item_get_string (wm->ditem,
                                                            MATE_DESKTOP_ITEM_EXEC));

        wm->name = g_strdup (mate_desktop_item_get_string (wm->ditem,
                                                            MATE_DESKTOP_ITEM_NAME));

        wm->config_exec = g_strdup (mate_desktop_item_get_string (wm->ditem,
                                                                   "ConfigExec"));
        wm->config_tryexec = g_strdup (mate_desktop_item_get_string (wm->ditem,
                                                                      "ConfigTryExec"));
        wm->session_managed = mate_desktop_item_get_boolean (wm->ditem,
                                                              "SessionManaged");

        wm->module = g_strdup (mate_desktop_item_get_string (wm->ditem,
                                                              "X-MATE-WMSettingsModule"));

        wm->identify_name = g_strdup (mate_desktop_item_get_string (wm->ditem,
                                                                     "X-MATE-WMName"));

        wm->is_user = is_user;

        if (mate_desktop_item_get_string (wm->ditem, MATE_DESKTOP_ITEM_EXEC)) {
                const char *tryexec;

                tryexec = mate_desktop_item_get_string (wm->ditem, MATE_DESKTOP_ITEM_TRY_EXEC);

                if (tryexec) {
                        path = g_find_program_in_path (tryexec);
                        wm->is_present = (path != NULL);
                        if (path)
                                g_free (path);
                } else
                        wm->is_present = TRUE;
        } else
                wm->is_present = FALSE;

        if (wm->config_exec) {
                if (wm->config_tryexec) {
                        path = g_find_program_in_path (wm->config_tryexec);
                        wm->is_config_present = (path != NULL);
                        if (path)
                                g_free (path);
                } else {
			path = g_find_program_in_path (wm->config_exec);
                        wm->is_config_present = (path != NULL);
                        if (path)
                                g_free (path);
		}
        } else
                wm->is_config_present = FALSE;

        if (wm->name && wm->exec &&
            (wm->is_user || wm->is_present))
                return wm;
        else {
                wm_free (wm);
                return NULL;
        }
}

static void
scan_wm_directory (const gchar *directory, gboolean is_user)
{
        GList *tmp_list;
        GList *files;

        files = list_desktop_files_in_dir (directory);

        tmp_list = files;
        while (tmp_list) {
                AvailableWindowManager *wm;

                wm = wm_load (tmp_list->data, is_user);

                if (wm != NULL)
                        available_wms = g_list_prepend (available_wms, wm);

                tmp_list = tmp_list->next;
        }

        g_list_foreach (files, (GFunc) g_free, NULL);
        g_list_free (files);
}

void
mate_wm_manager_init (void)
{
        gchar *user_config_dir;

        if (done_scan)
        {
                return;
        }
        done_scan = TRUE;

        /* look up WMs on system config folder */
        scan_wm_directory (MATE_WM_PROPERTY_PATH, FALSE);

        /* look up WMs on user config folder */
        user_config_dir = g_build_filename (g_get_user_config_dir (), "mate", "wm-properties", NULL);
        scan_wm_directory (user_config_dir, TRUE);
        g_free (user_config_dir);

        available_wms = g_list_sort (available_wms, wm_compare);
}

static AvailableWindowManager*
get_current_wm (GdkScreen *screen)
{
        AvailableWindowManager *current_wm;
        const char *name;
        GList *tmp_list;

        g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

        name = gdk_x11_screen_get_window_manager_name (screen);

        current_wm = NULL;

        tmp_list = available_wms;
        while (tmp_list != NULL) {
                AvailableWindowManager *wm = tmp_list->data;

                if (wm->identify_name &&
                    strcmp (wm->identify_name, name) == 0) {
                        current_wm = wm;
                        break;
                }
                tmp_list = tmp_list->next;
        }

        if (current_wm == NULL) {
                /* Try with localized name, sort of crackrock
                 * back compat hack
                 */

                tmp_list = available_wms;
                while (tmp_list != NULL) {
                        AvailableWindowManager *wm = tmp_list->data;

                        if (strcmp (wm->name, name) == 0) {
                                current_wm = wm;
                                break;
                        }
                        tmp_list = tmp_list->next;
                }
        }

        return current_wm;
}

MateWindowManager*
mate_wm_manager_get_current (GdkScreen *screen)
{
        AvailableWindowManager *wm;

        wm = get_current_wm (screen);

        if (wm != NULL && wm->module != NULL)
                /* may still return NULL here */
                return (MateWindowManager*) mate_window_manager_new (wm->ditem);
        else
                return NULL;
}

gboolean
mate_wm_manager_spawn_config_tool_for_current (GdkScreen  *screen,
                                                GError    **error)
{
        AvailableWindowManager *wm;

        wm = get_current_wm (screen);

        if (wm != NULL && wm->config_exec != NULL) {
                return g_spawn_command_line_async (wm->config_exec,
                                                   error);
        } else {
                const char *name;

                name = gdk_x11_screen_get_window_manager_name (screen);

                g_set_error (error,
                             G_SPAWN_ERROR,
                             G_SPAWN_ERROR_FAILED,
                             _("Window manager \"%s\" has not registered a configuration tool\n"),
                             name);
                return FALSE;
        }
}
