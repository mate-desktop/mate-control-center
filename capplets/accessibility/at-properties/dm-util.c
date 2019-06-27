/* vi: set sw=4 ts=4 wrap ai: */
/*
 * dm-util.c: This file is part of mate-control-center.
 *
 * Copyright (C) 2019 Wu Xiaotian <yetist@gmail.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * */

#include <gio/gio.h>
#include "dm-util.h"

static GDBusProxy *get_sys_proxy (void)
{
    GError     *error = NULL;
    GDBusProxy *proxy = NULL;

    proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                           G_DBUS_PROXY_FLAGS_NONE,
                                           NULL,
                                           "org.freedesktop.DBus",
                                           "/org/freedesktop/DBus",
                                           "org.freedesktop.DBus",
                                           NULL,
                                           &error);
    if (proxy == NULL) {
        g_warning ("Couldn't connect to system bus: %s", error->message);
        g_error_free (error);
    }
    return proxy;
}

static gboolean dm_is_running (void)
{
    GDBusProxy *proxy;
    GError *error = NULL;
    GVariant *ret;
    gboolean running = FALSE;

    proxy = get_sys_proxy ();
    if (proxy == NULL)
        return FALSE;

    ret = g_dbus_proxy_call_sync (proxy,
                                  "NameHasOwner",
                                  g_variant_new ("(s)", "org.freedesktop.DisplayManager"),
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &error);
    if (ret == NULL) {
        g_warning ("Couldn't call dbus method: %s", error->message);
        g_error_free (error);
    } else {
        g_variant_get (ret, "(b)", &running);
        g_variant_unref (ret);
    }

    if (proxy)
        g_object_unref (proxy);

    return running;
}

static gint
dm_get_pid (GError **err)
{
    GDBusProxy *proxy;
    GError *error = NULL;
    GVariant *ret;
    guint32 pid = 0;

    proxy = get_sys_proxy ();
    if (proxy == NULL)
        return FALSE;

    ret = g_dbus_proxy_call_sync (proxy,
                                  "GetConnectionUnixProcessID",
                                  g_variant_new ("(s)", "org.freedesktop.DisplayManager"),
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &error);
    if (ret == NULL) {
        g_propagate_error (err, error);
    } else {
        g_variant_get (ret, "(u)", &pid);
        g_variant_unref (ret);
    }

    if (proxy)
        g_object_unref (proxy);

    return pid;
}

static gchar* get_cmdline_from_pid (gint pid)
{
    gchar path[255];
    gchar *text = NULL;
    gchar *cmdline = NULL;
    GError *error = NULL;

    g_snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    if (!g_file_get_contents (path, &text, NULL, &error)) {
        g_warning ("get cmdline error: %s\n", error->message);
        g_error_free (error);
        return NULL;
    } else {
        cmdline = g_path_get_basename (text);
        g_free (text);
    }
    return cmdline;
}

DMType dm_get_type(void)
{
    gint pid;
    gchar *cmdline;

    DMType dmtype = DM_TYPE_UNKNOWN;

    if (!dm_is_running()) {
        goto ret;
    }

    pid = dm_get_pid (NULL);
    if (pid <= 1) {
        goto ret;
    }

    cmdline = get_cmdline_from_pid (pid);
    if (cmdline == NULL) {
        goto ret;
    }

    if (g_strcmp0(cmdline, "lightdm") == 0) {
        dmtype = DM_TYPE_LIGHTDM;
    } else if (g_strcmp0(cmdline, "mdm") == 0) {
        dmtype = DM_TYPE_MDM;
    } else if (g_strcmp0(cmdline, "gdm") == 0) {
        dmtype = DM_TYPE_GDM;
    }
    g_free (cmdline);
ret:
    return dmtype;
}
