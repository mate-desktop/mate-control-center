/*
 * Copyright (C) 2026 MATE developers
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <config.h>

#include <gdk/gdk.h>

#include "msd-input-wayland.h"

#ifdef HAVE_WAYLAND_PROTOCOLS

#include <gdk/gdkwayland.h>
#include <wayland-client.h>

#include "mate-input-device-v1-client-protocol.h"

#define MAX_ROUNDTRIPS 10

typedef struct {
        struct mate_input_device_manager_v1 *manager;
        GPtrArray *devices;
        gboolean   done;
} Listing;

typedef struct {
        struct mate_input_device_v1 *proxy;
        InputDeviceDescription       description;
} Device;

static void
device_name (void *data, struct mate_input_device_v1 *proxy, const char *name)
{
}

static void
device_id (void *data, struct mate_input_device_v1 *proxy, const char *id)
{
}

static void
device_usb_ids (void *data, struct mate_input_device_v1 *proxy, uint32_t vendor, uint32_t product)
{
}

static void
device_kind (void *data, struct mate_input_device_v1 *proxy, uint32_t kind)
{
        ((Device *) data)->description.kind = kind;
}

static void
device_capabilities (void *data, struct mate_input_device_v1 *proxy, uint32_t capabilities)
{
}

static void
device_buttons (void *data, struct mate_input_device_v1 *proxy, uint32_t buttons)
{
        ((Device *) data)->description.buttons = buttons;
}

static void
device_config (void *data, struct mate_input_device_v1 *proxy, uint32_t config)
{
        ((Device *) data)->description.config = config;
}

static void
device_tap_fingers (void *data, struct mate_input_device_v1 *proxy, uint32_t fingers)
{
        ((Device *) data)->description.tap_fingers = fingers;
}

static void
device_accel_profiles (void *data, struct mate_input_device_v1 *proxy, uint32_t profiles)
{
        ((Device *) data)->description.accel_profiles = profiles;
}

static void
device_scroll_methods (void *data, struct mate_input_device_v1 *proxy, uint32_t methods)
{
        ((Device *) data)->description.scroll_methods = methods;
}

static void
device_click_methods (void *data, struct mate_input_device_v1 *proxy, uint32_t methods)
{
}

static void
device_send_events (void *data, struct mate_input_device_v1 *proxy, uint32_t modes)
{
}

static void
device_output (void *data, struct mate_input_device_v1 *proxy, const char *output)
{
}

static void
device_done (void *data, struct mate_input_device_v1 *proxy)
{
}

static void
device_removed (void *data, struct mate_input_device_v1 *proxy)
{
}

static const struct mate_input_device_v1_listener device_listener = {
        .name = device_name,
        .id = device_id,
        .usb_ids = device_usb_ids,
        .kind = device_kind,
        .capabilities = device_capabilities,
        .buttons = device_buttons,
        .config = device_config,
        .tap_fingers = device_tap_fingers,
        .accel_profiles = device_accel_profiles,
        .scroll_methods = device_scroll_methods,
        .click_methods = device_click_methods,
        .send_events = device_send_events,
        .output = device_output,
        .done = device_done,
        .removed = device_removed,
};

static void
manager_device (void *data, struct mate_input_device_manager_v1 *manager, struct mate_input_device_v1 *proxy)
{
        Listing *listing = data;
        Device *device = g_new0 (Device, 1);

        device->proxy = proxy;
        mate_input_device_v1_add_listener (proxy, &device_listener, device);
        g_ptr_array_add (listing->devices, device);
}

static void
manager_done (void *data, struct mate_input_device_manager_v1 *manager)
{
        ((Listing *) data)->done = TRUE;
}

static void
manager_finished (void *data, struct mate_input_device_manager_v1 *manager)
{
        ((Listing *) data)->done = TRUE;
}

static const struct mate_input_device_manager_v1_listener manager_listener = {
        .device = manager_device,
        .done = manager_done,
        .finished = manager_finished,
};

static void
registry_global (void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
        Listing *listing = data;

        if (listing->manager == NULL &&
            g_strcmp0 (interface, mate_input_device_manager_v1_interface.name) == 0)
                listing->manager = wl_registry_bind (registry, name, &mate_input_device_manager_v1_interface, 1);
}

static void
registry_global_remove (void *data, struct wl_registry *registry, uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
        .global = registry_global,
        .global_remove = registry_global_remove,
};

static GArray *
list_devices (void)
{
        GdkDisplay *gdk_display = gdk_display_get_default ();
        struct wl_display *display;
        struct wl_event_queue *queue;
        struct wl_display *wrapper;
        struct wl_registry *registry;
        Listing listing = { NULL, NULL, FALSE };
        GArray *devices = NULL;
        guint i;

        if (!GDK_IS_WAYLAND_DISPLAY (gdk_display))
                return NULL;

        display = gdk_wayland_display_get_wl_display (gdk_display);
        queue = wl_display_create_queue (display);
        wrapper = wl_proxy_create_wrapper (display);
        wl_proxy_set_queue ((struct wl_proxy *) wrapper, queue);
        registry = wl_display_get_registry (wrapper);
        wl_registry_add_listener (registry, &registry_listener, &listing);
        listing.devices = g_ptr_array_new_with_free_func (g_free);

        wl_display_roundtrip_queue (display, queue);

        if (listing.manager != NULL) {
                mate_input_device_manager_v1_add_listener (listing.manager, &manager_listener, &listing);
                for (i = 0; i < MAX_ROUNDTRIPS && !listing.done; i++) {
                        if (wl_display_roundtrip_queue (display, queue) < 0)
                                break;
                }

                if (listing.done) {
                        devices = g_array_new (FALSE, TRUE, sizeof (InputDeviceDescription));
                        for (i = 0; i < listing.devices->len; i++) {
                                Device *device = g_ptr_array_index (listing.devices, i);

                                g_array_append_val (devices, device->description);
                        }
                }

                for (i = 0; i < listing.devices->len; i++)
                        mate_input_device_v1_destroy (((Device *) g_ptr_array_index (listing.devices, i))->proxy);
                mate_input_device_manager_v1_destroy (listing.manager);
        }

        g_ptr_array_free (listing.devices, TRUE);
        wl_registry_destroy (registry);
        wl_proxy_wrapper_destroy (wrapper);
        wl_display_flush (display);
        wl_event_queue_destroy (queue);

        return devices;
}

#else

static GArray *
list_devices (void)
{
        return NULL;
}

#endif

GArray *
input_devices_describe (void)
{
        static gboolean listed = FALSE;
        static GArray *devices = NULL;

        if (!listed) {
                devices = list_devices ();
                listed = TRUE;
        }

        return devices;
}
