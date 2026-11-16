/* The display layout through wlr-output-management and the MATE output
 * protocol, straight from the compositor.
 *
 * Copyright (C) 2026  mate-desktop.org
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
#include <unistd.h>

#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkwayland.h>

#include "display-backend.h"
#include "wlr-output-management-unstable-v1-client-protocol.h"
#include "mate-output-v1-client-protocol.h"

#define WLR_OUTPUT_MANAGER_VERSION 4
#define MATE_OUTPUT_MANAGER_VERSION 1

typedef struct ProtocolBackend ProtocolBackend;
typedef struct ProtocolHead ProtocolHead;

typedef struct
{
    ProtocolHead               *head;
    struct zwlr_output_mode_v1 *proxy;
    Mode                        mode;
} ProtocolMode;

struct ProtocolHead
{
    ProtocolBackend              *backend;
    struct zwlr_output_head_v1   *proxy;
    struct mate_output_head_v1   *mate;
    char                         *name;
    char                         *description;
    gboolean                      enabled;
    int                           x;
    int                           y;
    int                           transform;
    double                        scale;
    gboolean                      adaptive_sync;
    ProtocolMode                 *current;
    GPtrArray                    *modes;
    char                         *id;
    char                         *display_name;
    gboolean                      internal;
    gboolean                      primary;
    char                         *mirror_of;
};

struct ProtocolBackend
{
    DisplayBackend                  base;
    struct wl_display              *display;
    struct wl_registry             *registry;
    guint32                         wlr_name;
    guint32                         wlr_version;
    guint32                         mate_name;
    struct zwlr_output_manager_v1  *manager;
    struct mate_output_manager_v1  *mate;
    GList                          *heads;
    guint32                         serial;
    gboolean                        done;
    guint                           emit_id;
};

typedef struct
{
    struct zwlr_output_configuration_v1 *configuration;
    struct mate_output_configuration_v1 *mate;
    DisplayBackendAppliedFunc            done;
    gpointer                             data;
} ProtocolApply;

static gboolean
emit_changed (gpointer data)
{
    ProtocolBackend *self = data;
    GArray *heads = g_array_new (FALSE, FALSE, sizeof (OutputHead));
    GList *l;

    self->emit_id = 0;

    for (l = self->heads; l != NULL; l = l->next)
    {
        ProtocolHead *ph = l->data;
        OutputHead head;
        guint i;

        memset (&head, 0, sizeof (OutputHead));
        head.name = g_strdup (ph->name);
        head.description = g_strdup (ph->description ? ph->description : ph->name);
        head.enabled = ph->enabled;
        head.x = ph->x;
        head.y = ph->y;
        head.transform = ph->transform & 0x3;
        head.scale = ph->scale > 0.0 ? ph->scale : 1.0;
        head.adaptive_sync_supported = ph->adaptive_sync ? 1 : -1;
        head.adaptive_sync = ph->adaptive_sync;
        head.modes = g_array_new (FALSE, FALSE, sizeof (Mode));
        head.id = g_strdup (ph->id);
        head.display_name = g_strdup (ph->display_name);
        head.internal = ph->internal;
        head.primary = ph->primary;
        head.mirror_of = ph->mirror_of && *ph->mirror_of ? g_strdup (ph->mirror_of) : NULL;

        if (ph->enabled && ph->current)
        {
            head.width = ph->current->mode.width;
            head.height = ph->current->mode.height;
            head.rate = ph->current->mode.rate;
            head.preferred = ph->current->mode.preferred;
            g_array_append_val (head.modes, ph->current->mode);
        }

        for (i = 0; i < ph->modes->len; i++)
        {
            ProtocolMode *pm = g_ptr_array_index (ph->modes, i);

            if (pm->mode.width <= 0 || pm->mode.height <= 0)
                continue;
            if (display_head_has_mode (&head, pm->mode.width, pm->mode.height, pm->mode.rate))
                continue;
            g_array_append_val (head.modes, pm->mode);
        }

        g_array_append_val (heads, head);
    }

    self->base.changed (heads, NULL, self->base.changed_data);
    return G_SOURCE_REMOVE;
}

static void
schedule_changed (ProtocolBackend *self)
{
    if (self->done && self->emit_id == 0)
        self->emit_id = g_idle_add (emit_changed, self);
}

static void
mode_destroy (ProtocolMode *pm)
{
    if (pm->head->current == pm)
        pm->head->current = NULL;
    if (zwlr_output_mode_v1_get_version (pm->proxy) >= ZWLR_OUTPUT_MODE_V1_RELEASE_SINCE_VERSION)
        zwlr_output_mode_v1_release (pm->proxy);
    else
        zwlr_output_mode_v1_destroy (pm->proxy);
    g_free (pm);
}

static void
mode_size (void *data, struct zwlr_output_mode_v1 *mode, int32_t width, int32_t height)
{
    ProtocolMode *pm = data;

    pm->mode.width = width;
    pm->mode.height = height;
}

static void
mode_refresh (void *data, struct zwlr_output_mode_v1 *mode, int32_t refresh)
{
    ProtocolMode *pm = data;

    pm->mode.rate = refresh;
}

static void
mode_preferred (void *data, struct zwlr_output_mode_v1 *mode)
{
    ProtocolMode *pm = data;

    pm->mode.preferred = TRUE;
}

static void
mode_finished (void *data, struct zwlr_output_mode_v1 *mode)
{
    ProtocolMode *pm = data;

    g_ptr_array_remove (pm->head->modes, pm);
}

static const struct zwlr_output_mode_v1_listener mode_listener = {
    .size = mode_size,
    .refresh = mode_refresh,
    .preferred = mode_preferred,
    .finished = mode_finished,
};

static void
head_free (ProtocolHead *ph)
{
    g_ptr_array_free (ph->modes, TRUE);
    if (ph->mate)
        mate_output_head_v1_destroy (ph->mate);
    if (zwlr_output_head_v1_get_version (ph->proxy) >= ZWLR_OUTPUT_HEAD_V1_RELEASE_SINCE_VERSION)
        zwlr_output_head_v1_release (ph->proxy);
    else
        zwlr_output_head_v1_destroy (ph->proxy);
    g_free (ph->name);
    g_free (ph->description);
    g_free (ph->id);
    g_free (ph->display_name);
    g_free (ph->mirror_of);
    g_free (ph);
}

static void
head_name (void *data, struct zwlr_output_head_v1 *head, const char *name)
{
    ProtocolHead *ph = data;

    g_free (ph->name);
    ph->name = g_strdup (name);
}

static void
head_description (void *data, struct zwlr_output_head_v1 *head, const char *description)
{
    ProtocolHead *ph = data;

    g_free (ph->description);
    ph->description = g_strdup (description);
}

static void
head_physical_size (void *data, struct zwlr_output_head_v1 *head, int32_t width, int32_t height)
{
}

static void
head_mode (void *data, struct zwlr_output_head_v1 *head, struct zwlr_output_mode_v1 *mode)
{
    ProtocolHead *ph = data;
    ProtocolMode *pm = g_new0 (ProtocolMode, 1);

    pm->head = ph;
    pm->proxy = mode;
    zwlr_output_mode_v1_add_listener (mode, &mode_listener, pm);
    g_ptr_array_add (ph->modes, pm);
}

static void
head_enabled (void *data, struct zwlr_output_head_v1 *head, int32_t enabled)
{
    ProtocolHead *ph = data;

    ph->enabled = enabled != 0;
    if (!ph->enabled)
        ph->current = NULL;
}

static void
head_current_mode (void *data, struct zwlr_output_head_v1 *head, struct zwlr_output_mode_v1 *mode)
{
    ProtocolHead *ph = data;
    guint i;

    ph->current = NULL;
    for (i = 0; i < ph->modes->len; i++)
    {
        ProtocolMode *pm = g_ptr_array_index (ph->modes, i);

        if (pm->proxy == mode)
            ph->current = pm;
    }
}

static void
head_position (void *data, struct zwlr_output_head_v1 *head, int32_t x, int32_t y)
{
    ProtocolHead *ph = data;

    ph->x = x;
    ph->y = y;
}

static void
head_transform (void *data, struct zwlr_output_head_v1 *head, int32_t transform)
{
    ProtocolHead *ph = data;

    ph->transform = transform;
}

static void
head_scale (void *data, struct zwlr_output_head_v1 *head, wl_fixed_t scale)
{
    ProtocolHead *ph = data;

    ph->scale = wl_fixed_to_double (scale);
}

static void
head_finished (void *data, struct zwlr_output_head_v1 *head)
{
    ProtocolHead *ph = data;
    ProtocolBackend *self = ph->backend;

    self->heads = g_list_remove (self->heads, ph);
    head_free (ph);
}

static void
head_string_ignored (void *data, struct zwlr_output_head_v1 *head, const char *value)
{
}

static void
head_adaptive_sync (void *data, struct zwlr_output_head_v1 *head, uint32_t state)
{
    ProtocolHead *ph = data;

    ph->adaptive_sync = state == ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
}

static const struct zwlr_output_head_v1_listener head_listener = {
    .name = head_name,
    .description = head_description,
    .physical_size = head_physical_size,
    .mode = head_mode,
    .enabled = head_enabled,
    .current_mode = head_current_mode,
    .position = head_position,
    .transform = head_transform,
    .scale = head_scale,
    .finished = head_finished,
    .make = head_string_ignored,
    .model = head_string_ignored,
    .serial_number = head_string_ignored,
    .adaptive_sync = head_adaptive_sync,
};

static void
mate_head_id (void *data, struct mate_output_head_v1 *head, const char *id)
{
    ProtocolHead *ph = data;

    g_free (ph->id);
    ph->id = g_strdup (id);
}

static void
mate_head_edid_identity (void *data, struct mate_output_head_v1 *head, const char *vendor,
                         uint32_t product, uint32_t serial, const char *serial_string)
{
}

static void
mate_head_edid (void *data, struct mate_output_head_v1 *head, int32_t fd, uint32_t size)
{
    close (fd);
}

static void
mate_head_display_name (void *data, struct mate_output_head_v1 *head, const char *name)
{
    ProtocolHead *ph = data;

    g_free (ph->display_name);
    ph->display_name = g_strdup (name);
}

static void
mate_head_internal (void *data, struct mate_output_head_v1 *head, uint32_t internal)
{
    ProtocolHead *ph = data;

    ph->internal = internal != 0;
}

static void
mate_head_primary (void *data, struct mate_output_head_v1 *head, uint32_t primary)
{
    ProtocolHead *ph = data;

    ph->primary = primary != 0;
}

static void
mate_head_mirror_of (void *data, struct mate_output_head_v1 *head, const char *id)
{
    ProtocolHead *ph = data;

    g_free (ph->mirror_of);
    ph->mirror_of = g_strdup (id);
}

static void
mate_head_done (void *data, struct mate_output_head_v1 *head)
{
    ProtocolHead *ph = data;

    schedule_changed (ph->backend);
}

static void
mate_head_finished (void *data, struct mate_output_head_v1 *head)
{
}

static const struct mate_output_head_v1_listener mate_head_listener = {
    .id = mate_head_id,
    .edid_identity = mate_head_edid_identity,
    .edid = mate_head_edid,
    .display_name = mate_head_display_name,
    .internal = mate_head_internal,
    .primary = mate_head_primary,
    .mirror_of = mate_head_mirror_of,
    .done = mate_head_done,
    .finished = mate_head_finished,
};

static void
manager_head (void *data, struct zwlr_output_manager_v1 *manager, struct zwlr_output_head_v1 *head)
{
    ProtocolBackend *self = data;
    ProtocolHead *ph = g_new0 (ProtocolHead, 1);

    ph->backend = self;
    ph->proxy = head;
    ph->scale = 1.0;
    ph->modes = g_ptr_array_new_with_free_func ((GDestroyNotify) mode_destroy);
    zwlr_output_head_v1_add_listener (head, &head_listener, ph);
    ph->mate = mate_output_manager_v1_get_head (self->mate, head);
    mate_output_head_v1_add_listener (ph->mate, &mate_head_listener, ph);
    self->heads = g_list_append (self->heads, ph);
}

static void
manager_done (void *data, struct zwlr_output_manager_v1 *manager, uint32_t serial)
{
    ProtocolBackend *self = data;

    self->serial = serial;
    self->done = TRUE;
    schedule_changed (self);
}

static void
manager_finished (void *data, struct zwlr_output_manager_v1 *manager)
{
}

static const struct zwlr_output_manager_v1_listener manager_listener = {
    .head = manager_head,
    .done = manager_done,
    .finished = manager_finished,
};

static void
mate_manager_capabilities (void *data, struct mate_output_manager_v1 *manager, uint32_t capabilities)
{
}

static void
mate_manager_finished (void *data, struct mate_output_manager_v1 *manager)
{
}

static const struct mate_output_manager_v1_listener mate_manager_listener = {
    .capabilities = mate_manager_capabilities,
    .finished = mate_manager_finished,
};

static void
registry_global (void *data, struct wl_registry *registry, uint32_t name,
                 const char *interface, uint32_t version)
{
    ProtocolBackend *self = data;

    if (strcmp (interface, zwlr_output_manager_v1_interface.name) == 0)
    {
        self->wlr_name = name;
        self->wlr_version = MIN (version, WLR_OUTPUT_MANAGER_VERSION);
    }
    else if (strcmp (interface, mate_output_manager_v1_interface.name) == 0)
    {
        self->mate_name = name;
    }
}

static void
registry_global_remove (void *data, struct wl_registry *registry, uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

static ProtocolHead *
find_head (ProtocolBackend *self, const char *name)
{
    GList *l;

    for (l = self->heads; l != NULL; l = l->next)
    {
        ProtocolHead *ph = l->data;

        if (g_strcmp0 (ph->name, name) == 0)
            return ph;
    }

    return NULL;
}

static ProtocolMode *
find_mode (ProtocolHead *ph, int width, int height, int rate)
{
    ProtocolMode *best = NULL;
    guint i;

    for (i = 0; i < ph->modes->len; i++)
    {
        ProtocolMode *pm = g_ptr_array_index (ph->modes, i);

        if (pm->mode.width != width || pm->mode.height != height)
            continue;
        if (pm->mode.rate == rate)
            return pm;
        if (rate <= 0 && (best == NULL || pm->mode.rate > best->mode.rate))
            best = pm;
    }

    return best;
}

static void
apply_finish (ProtocolApply *call, const char *error)
{
    mate_output_configuration_v1_destroy (call->mate);
    zwlr_output_configuration_v1_destroy (call->configuration);
    call->done (error, call->data);
    g_free (call);
}

static void
configuration_succeeded (void *data, struct zwlr_output_configuration_v1 *configuration)
{
    apply_finish (data, NULL);
}

static void
configuration_failed (void *data, struct zwlr_output_configuration_v1 *configuration)
{
    apply_finish (data, _("The compositor could not apply the configuration."));
}

static void
configuration_cancelled (void *data, struct zwlr_output_configuration_v1 *configuration)
{
    apply_finish (data, _("The monitors changed while the configuration was being applied."));
}

static const struct zwlr_output_configuration_v1_listener configuration_listener = {
    .succeeded = configuration_succeeded,
    .failed = configuration_failed,
    .cancelled = configuration_cancelled,
};

static void
protocol_refresh (DisplayBackend *backend)
{
    schedule_changed ((ProtocolBackend *) backend);
}

static void
protocol_apply (DisplayBackend            *backend,
                GArray                    *heads,
                gboolean                   temporary,
                DisplayBackendAppliedFunc  done,
                gpointer                   data)
{
    ProtocolBackend *self = (ProtocolBackend *) backend;
    ProtocolApply *call = g_new0 (ProtocolApply, 1);
    gboolean primary_set = FALSE;
    guint i;

    call->done = done;
    call->data = data;
    call->configuration = zwlr_output_manager_v1_create_configuration (self->manager, self->serial);
    call->mate = mate_output_manager_v1_get_configuration (self->mate, call->configuration);
    zwlr_output_configuration_v1_add_listener (call->configuration, &configuration_listener, call);

    for (i = 0; i < heads->len; i++)
    {
        OutputHead *head = &g_array_index (heads, OutputHead, i);
        ProtocolHead *ph = find_head (self, head->name);
        struct zwlr_output_configuration_head_v1 *ch;
        ProtocolMode *pm;
        int width = head->width, height = head->height;

        if (ph == NULL)
            continue;

        if (!head->enabled)
        {
            zwlr_output_configuration_v1_disable_head (call->configuration, ph->proxy);
            continue;
        }

        ch = zwlr_output_configuration_v1_enable_head (call->configuration, ph->proxy);

        if (width <= 0 || height <= 0)
            display_head_get_preferred_size (head, &width, &height);
        pm = find_mode (ph, width, height, head->rate);
        if (pm)
            zwlr_output_configuration_head_v1_set_mode (ch, pm->proxy);
        else if (width > 0 && height > 0)
            zwlr_output_configuration_head_v1_set_custom_mode (ch, width, height, head->rate);

        zwlr_output_configuration_head_v1_set_position (ch, head->x, head->y);
        zwlr_output_configuration_head_v1_set_transform (ch, (head->transform & 0x3) | (ph->transform & 0x4));
        zwlr_output_configuration_head_v1_set_scale (ch, wl_fixed_from_double (head->scale > 0.0 ? head->scale : 1.0));
        if (head->adaptive_sync_supported == 1 &&
            zwlr_output_configuration_head_v1_get_version (ch) >=
                ZWLR_OUTPUT_CONFIGURATION_HEAD_V1_SET_ADAPTIVE_SYNC_SINCE_VERSION)
            zwlr_output_configuration_head_v1_set_adaptive_sync (ch, head->adaptive_sync
                                                                     ? ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED
                                                                     : ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED);

        if (head->primary && !primary_set)
        {
            mate_output_configuration_v1_set_primary (call->mate, ch);
            primary_set = TRUE;
        }
    }

    if (temporary)
        mate_output_configuration_v1_set_temporary (call->mate);

    zwlr_output_configuration_v1_apply (call->configuration);
    wl_display_flush (self->display);
}

static void
protocol_free (DisplayBackend *backend)
{
    ProtocolBackend *self = (ProtocolBackend *) backend;

    if (self->emit_id)
        g_source_remove (self->emit_id);
    g_list_free_full (self->heads, (GDestroyNotify) head_free);
    if (self->mate)
        mate_output_manager_v1_destroy (self->mate);
    if (self->manager)
    {
        zwlr_output_manager_v1_stop (self->manager);
        zwlr_output_manager_v1_destroy (self->manager);
    }
    if (self->registry)
        wl_registry_destroy (self->registry);
    wl_display_flush (self->display);
    g_free (self);
}

DisplayBackend *
display_backend_protocol_new (DisplayBackendChangedFunc changed, gpointer data)
{
    GdkDisplay *gdk_display = gdk_display_get_default ();
    ProtocolBackend *self;

    if (gdk_display == NULL || !GDK_IS_WAYLAND_DISPLAY (gdk_display))
        return NULL;

    self = g_new0 (ProtocolBackend, 1);
    self->display = gdk_wayland_display_get_wl_display (gdk_display);
    self->registry = wl_display_get_registry (self->display);
    wl_registry_add_listener (self->registry, &registry_listener, self);
    wl_display_roundtrip (self->display);

    if (self->wlr_name == 0 || self->mate_name == 0)
    {
        wl_registry_destroy (self->registry);
        g_free (self);
        return NULL;
    }

    self->base.name = "wlr-output-management";
    self->base.features = DISPLAY_BACKEND_PRIMARY | DISPLAY_BACKEND_IDENTITY |
                          DISPLAY_BACKEND_MIRROR | DISPLAY_BACKEND_TEMPORARY;
    self->base.changed = changed;
    self->base.changed_data = data;
    self->base.refresh = protocol_refresh;
    self->base.apply = protocol_apply;
    self->base.free = protocol_free;

    self->mate = wl_registry_bind (self->registry, self->mate_name,
                                   &mate_output_manager_v1_interface, MATE_OUTPUT_MANAGER_VERSION);
    mate_output_manager_v1_add_listener (self->mate, &mate_manager_listener, self);
    self->manager = wl_registry_bind (self->registry, self->wlr_name,
                                      &zwlr_output_manager_v1_interface, self->wlr_version);
    zwlr_output_manager_v1_add_listener (self->manager, &manager_listener, self);
    wl_display_roundtrip (self->display);
    wl_display_roundtrip (self->display);

    return &self->base;
}
