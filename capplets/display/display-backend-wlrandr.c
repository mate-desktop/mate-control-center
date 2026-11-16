/* The display layout through mate-settings-daemon's wlrandr plugin (D-Bus),
 * for Wayland compositors without the MATE output protocol.
 *
 * Copyright (C) 2023, 2026  mate-desktop.org
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <config.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gio/gio.h>

#include "display-backend.h"

#define MSD_WLRANDR_DBUS_PATH     "/org/mate/SettingsDaemon/WLRANDR"
#define MSD_WLRANDR_DBUS_NAME     "org.mate.SettingsDaemon.WLRANDR"

#define HEAD_INFO_FORMAT  "(ssbiiidbb(iiii)a(iiii))"
#define HEAD_SET_TYPE     "a(sbiiiiidib)"
#define HEAD_SET_FORMAT   "(sbiiiiidib)"
#define MODE_FORMAT       "(iiii)"

typedef enum
{
    HEAD_INFO_FIELD_NAME,
    HEAD_INFO_FIELD_DESCRIPTION,
    HEAD_INFO_FIELD_ENABLED,
    HEAD_INFO_FIELD_X,
    HEAD_INFO_FIELD_Y,
    HEAD_INFO_FIELD_TRANSFORM,
    HEAD_INFO_FIELD_SCALE,
    HEAD_INFO_FIELD_ADAPTIVE_SYNC_SUPPORTED,
    HEAD_INFO_FIELD_ADAPTIVE_SYNC,
    HEAD_INFO_FIELD_CURRENT_MODE,
    HEAD_INFO_FIELD_MODES,
} HeadInfoField;

typedef struct
{
    DisplayBackend   base;
    GDBusConnection *connection;
    GDBusProxy      *proxy;
    guint            signal_id;
} WlrandrBackend;

typedef struct
{
    DisplayBackendAppliedFunc done;
    gpointer                  data;
} ApplyCall;

static GArray *
parse_heads (GVariant *configuration)
{
    GVariantIter iter;
    GArray *heads = g_array_new (FALSE, FALSE, sizeof (OutputHead));

    g_variant_iter_init (&iter, configuration);

    while (TRUE)
    {
        GVariant *head_var;
        const char *name;
        const char *description;
        gboolean enabled;
        gboolean adaptive_sync_supported;
        gboolean adaptive_sync;
        int x, y, transform;
        double scale;
        int cur_w, cur_h, cur_rate, cur_pref;
        g_autoptr (GVariant) modes_var = NULL;
        OutputHead head;
        GVariantIter modes_iter;

        head_var = g_variant_iter_next_value (&iter);
        if (head_var == NULL)
            break;

        g_variant_get_child (head_var, HEAD_INFO_FIELD_NAME, "&s", &name);
        g_variant_get_child (head_var, HEAD_INFO_FIELD_DESCRIPTION, "&s", &description);
        g_variant_get_child (head_var, HEAD_INFO_FIELD_ENABLED, "b", &enabled);
        g_variant_get_child (head_var, HEAD_INFO_FIELD_X, "i", &x);
        g_variant_get_child (head_var, HEAD_INFO_FIELD_Y, "i", &y);
        g_variant_get_child (head_var, HEAD_INFO_FIELD_TRANSFORM, "i", &transform);
        g_variant_get_child (head_var, HEAD_INFO_FIELD_SCALE, "d", &scale);
        g_variant_get_child (head_var, HEAD_INFO_FIELD_ADAPTIVE_SYNC_SUPPORTED,
                             "b", &adaptive_sync_supported);
        g_variant_get_child (head_var, HEAD_INFO_FIELD_ADAPTIVE_SYNC, "b", &adaptive_sync);
        g_variant_get_child (head_var, HEAD_INFO_FIELD_CURRENT_MODE,
                             MODE_FORMAT, &cur_w, &cur_h, &cur_rate, &cur_pref);
        g_variant_get_child (head_var, HEAD_INFO_FIELD_MODES, "@a" MODE_FORMAT, &modes_var);

        memset (&head, 0, sizeof (OutputHead));
        head.name = g_strdup (name);
        head.description = g_strdup (description);
        head.enabled = enabled;
        head.x = x;
        head.y = y;
        head.transform = transform & 0x3;
        head.scale = scale;
        head.adaptive_sync_supported = adaptive_sync_supported ? 1 : 0;
        head.adaptive_sync = adaptive_sync;
        head.width = cur_w;
        head.height = cur_h;
        head.rate = cur_rate;
        head.preferred = cur_pref;
        head.modes = g_array_new (FALSE, FALSE, sizeof (Mode));

        if (cur_w > 0 && cur_h > 0)
        {
            Mode m;

            m.width = cur_w;
            m.height = cur_h;
            m.rate = cur_rate;
            m.preferred = cur_pref;
            g_array_append_val (head.modes, m);
        }

        g_variant_iter_init (&modes_iter, modes_var);
        while (TRUE)
        {
            int w, h, rate;
            int pref;
            Mode mode;

            if (!g_variant_iter_next (&modes_iter, MODE_FORMAT, &w, &h, &rate, &pref))
                break;

            if (display_head_has_mode (&head, w, h, rate))
                continue;

            mode.width = w;
            mode.height = h;
            mode.rate = rate;
            mode.preferred = pref;
            g_array_append_val (head.modes, mode);
        }

        g_variant_unref (head_var);

        g_array_append_val (heads, head);
    }

    return heads;
}

static void
on_configuration_received (GObject *source_object, GAsyncResult *res, gpointer data)
{
    WlrandrBackend *self = data;
    GError *error = NULL;
    g_autoptr (GVariant) result = NULL;
    g_autoptr (GVariant) configuration = NULL;

    result = g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object), res, &error);
    if (result == NULL)
    {
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            self->base.changed (NULL, error->message, self->base.changed_data);
        g_error_free (error);
        return;
    }

    g_variant_get (result, "(@a" HEAD_INFO_FORMAT ")", &configuration);
    self->base.changed (parse_heads (configuration), NULL, self->base.changed_data);
}

static void
wlrandr_refresh (DisplayBackend *backend)
{
    WlrandrBackend *self = (WlrandrBackend *) backend;

    g_dbus_proxy_call (self->proxy, "GetConfiguration", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                       on_configuration_received, self);
}

static void
on_set_configuration_returned (GObject *source_object, GAsyncResult *res, gpointer data)
{
    ApplyCall *call = data;
    GError *error = NULL;
    g_autoptr (GVariant) result = NULL;

    result = g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object), res, &error);
    if (result == NULL && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_clear_error (&error);
    call->done (error ? error->message : NULL, call->data);
    g_clear_error (&error);
    g_free (call);
}

static void
wlrandr_apply (DisplayBackend            *backend,
               GArray                    *heads,
               gboolean                   temporary,
               DisplayBackendAppliedFunc  done,
               gpointer                   data)
{
    WlrandrBackend *self = (WlrandrBackend *) backend;
    GVariantBuilder builder;
    ApplyCall *call;
    guint i;

    g_variant_builder_init (&builder, G_VARIANT_TYPE (HEAD_SET_TYPE));

    for (i = 0; i < heads->len; i++)
    {
        OutputHead *head = &g_array_index (heads, OutputHead, i);
        int width = 0, height = 0;

        if (head->enabled)
            display_head_get_size (head, &width, &height);

        g_variant_builder_add (&builder, HEAD_SET_FORMAT,
                               head->name, head->enabled, width, height, head->rate,
                               head->x, head->y, head->scale, head->transform,
                               head->adaptive_sync);
    }

    call = g_new0 (ApplyCall, 1);
    call->done = done;
    call->data = data;
    g_dbus_proxy_call (self->proxy, "SetConfiguration",
                       g_variant_new ("(@a" HEAD_SET_FORMAT ")", g_variant_builder_end (&builder)),
                       G_DBUS_CALL_FLAGS_NONE, -1, NULL, on_set_configuration_returned, call);
}

static void
on_configuration_changed (GDBusConnection *connection,
                          const gchar     *sender_name,
                          const gchar     *object_path,
                          const gchar     *interface_name,
                          const gchar     *signal_name,
                          GVariant        *parameters,
                          gpointer         user_data)
{
    wlrandr_refresh (user_data);
}

static void
wlrandr_free (DisplayBackend *backend)
{
    WlrandrBackend *self = (WlrandrBackend *) backend;

    if (self->signal_id != 0)
        g_dbus_connection_signal_unsubscribe (self->connection, self->signal_id);
    g_clear_object (&self->proxy);
    g_clear_object (&self->connection);
    g_free (self);
}

DisplayBackend *
display_backend_wlrandr_new (DisplayBackendChangedFunc changed, gpointer data, GError **error)
{
    WlrandrBackend *self;
    GDBusConnection *connection;
    GDBusProxy *proxy;

    connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, error);
    if (connection == NULL)
        return NULL;

    proxy = g_dbus_proxy_new_sync (connection, G_DBUS_PROXY_FLAGS_NONE, NULL,
                                   "org.mate.SettingsDaemon", MSD_WLRANDR_DBUS_PATH,
                                   MSD_WLRANDR_DBUS_NAME, NULL, error);
    if (proxy == NULL)
    {
        g_object_unref (connection);
        return NULL;
    }

    self = g_new0 (WlrandrBackend, 1);
    self->base.name = "mate-settings-daemon";
    self->base.features = 0;
    self->base.changed = changed;
    self->base.changed_data = data;
    self->base.refresh = wlrandr_refresh;
    self->base.apply = wlrandr_apply;
    self->base.free = wlrandr_free;
    self->connection = connection;
    self->proxy = proxy;
    self->signal_id = g_dbus_connection_signal_subscribe (connection, NULL, MSD_WLRANDR_DBUS_NAME,
                                                          "ConfigurationChanged",
                                                          MSD_WLRANDR_DBUS_PATH, NULL,
                                                          G_DBUS_SIGNAL_FLAGS_NONE,
                                                          on_configuration_changed, self, NULL);
    return &self->base;
}
