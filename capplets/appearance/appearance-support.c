/*
 * Copyright (C) 2012 Stefano Karapetsas
 * Authors: Stefano Karapetsas <stefano@karapetsas.com>
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "appearance.h"
#include "wm-common.h"

#include <glib.h>
#include <gio/gio.h>

static gboolean
is_program_in_path (const char *program)
{
    char *tmp = g_find_program_in_path (program);
    if (tmp != NULL)
    {
        g_free (tmp);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static gboolean
metacity_is_running()
{
    gboolean is_running = FALSE;
    gchar *current_wm = NULL;

    current_wm = wm_common_get_current_window_manager ();

    is_running = (g_strcmp0(current_wm, WM_COMMON_METACITY) == 0) ||
                 (g_strcmp0(current_wm, WM_COMMON_COMPIZ) == 0);

    g_free (current_wm);

    return is_running;
}

static void
metacity_theme_apply(const gchar *theme)
{
    /* set theme, we use gconf and gsettings binaries to avoid schemas and versions issues */
    if (is_program_in_path ("gconftool-2"))
    {
        gchar *gconf_cmd = g_strdup_printf("gconftool-2 --set --type string /apps/metacity/general/theme '%s'", theme);
        g_spawn_command_line_async (gconf_cmd, NULL);
        g_free (gconf_cmd);
    }

    if (is_program_in_path ("gsettings"))
    {
        gchar *gsettings_cmd = g_strdup_printf("gsettings set org.gnome.desktop.wm.preferences theme '%s'", theme);
        g_spawn_command_line_async (gsettings_cmd, NULL);
        g_free (gsettings_cmd);
    }
}

static void
marco_theme_changed(GSettings *settings, gchar *key, AppearanceData* data)
{
    gchar *theme = NULL;
    if (metacity_is_running ())
    {
        theme = g_settings_get_string (settings, key);
        metacity_theme_apply (theme);
        g_free (theme);
    }
}

void
support_init(AppearanceData* data)
{
    /* needed for wm_common_get_current_window_manager() */
    wm_common_update_window ();
    /* GSettings signal */
    g_signal_connect (data->marco_settings, "changed::" MARCO_THEME_KEY,
                      G_CALLBACK (marco_theme_changed), data);
    /* apply theme at start */
    if (metacity_is_running ())
        marco_theme_changed (data->marco_settings, MARCO_THEME_KEY, data);
}

void
support_shutdown(AppearanceData* data)
{
    /* nothing to do */
}
