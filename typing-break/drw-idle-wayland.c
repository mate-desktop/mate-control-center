/*
 * Copyright (C) 2026 MATE Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <config.h>
#include <string.h>
#include <gdk/gdk.h>
#include <gdk/gdkwayland.h>
#include <wayland-client.h>

#include "ext-idle-notify-v1-client-protocol.h"
#include "drw-idle-wayland.h"

struct _DrwIdleWayland {
	struct wl_registry              *registry;
	struct ext_idle_notifier_v1     *notifier;
	struct ext_idle_notification_v1 *notification;
	guint32                          notifier_version;
	gboolean                         idle;
};

static void
registry_global (void               *data,
		 struct wl_registry *registry,
		 uint32_t            name,
		 const char         *interface,
		 uint32_t            version)
{
	DrwIdleWayland *idle = data;

	if (strcmp (interface, ext_idle_notifier_v1_interface.name) == 0) {
		idle->notifier_version = MIN (version, 2);
		idle->notifier = wl_registry_bind (registry, name,
						   &ext_idle_notifier_v1_interface,
						   idle->notifier_version);
	}
}

static void
registry_global_remove (void               *data,
			struct wl_registry *registry,
			uint32_t            name)
{
}

static const struct wl_registry_listener registry_listener = {
	registry_global,
	registry_global_remove
};

static void
notification_idled (void                            *data,
		    struct ext_idle_notification_v1 *notification)
{
	DrwIdleWayland *idle = data;

	idle->idle = TRUE;
}

static void
notification_resumed (void                            *data,
		      struct ext_idle_notification_v1 *notification)
{
	DrwIdleWayland *idle = data;

	idle->idle = FALSE;
}

static const struct ext_idle_notification_v1_listener notification_listener = {
	notification_idled,
	notification_resumed
};

DrwIdleWayland *
drw_idle_wayland_new (guint timeout_ms)
{
	GdkDisplay        *display = gdk_display_get_default ();
	struct wl_display *wl_display;
	struct wl_seat    *wl_seat;
	DrwIdleWayland    *idle;

	if (!GDK_IS_WAYLAND_DISPLAY (display))
		return NULL;

	wl_display = gdk_wayland_display_get_wl_display (display);
	wl_seat = gdk_wayland_seat_get_wl_seat (gdk_display_get_default_seat (display));
	if (wl_seat == NULL)
		return NULL;

	idle = g_new0 (DrwIdleWayland, 1);

	idle->registry = wl_display_get_registry (wl_display);
	wl_registry_add_listener (idle->registry, &registry_listener, idle);
	wl_display_roundtrip (wl_display);

	if (idle->notifier == NULL) {
		drw_idle_wayland_free (idle);
		return NULL;
	}

	/* Version 2 can ignore idle inhibitors (a playing video) so only input
	 * counts for typing breaks. */
#ifdef EXT_IDLE_NOTIFIER_V1_GET_INPUT_IDLE_NOTIFICATION_SINCE_VERSION
	if (idle->notifier_version >= EXT_IDLE_NOTIFIER_V1_GET_INPUT_IDLE_NOTIFICATION_SINCE_VERSION)
		idle->notification = ext_idle_notifier_v1_get_input_idle_notification (idle->notifier,
											timeout_ms,
											wl_seat);
	else
#endif
		idle->notification = ext_idle_notifier_v1_get_idle_notification (idle->notifier,
										  timeout_ms,
										  wl_seat);
	ext_idle_notification_v1_add_listener (idle->notification, &notification_listener, idle);
	wl_display_flush (wl_display);

	return idle;
}

gboolean
drw_idle_wayland_is_idle (DrwIdleWayland *idle)
{
	return idle->idle;
}

void
drw_idle_wayland_free (DrwIdleWayland *idle)
{
	if (idle == NULL)
		return;

	if (idle->notification)
		ext_idle_notification_v1_destroy (idle->notification);
	if (idle->notifier)
		ext_idle_notifier_v1_destroy (idle->notifier);
	if (idle->registry)
		wl_registry_destroy (idle->registry);
	g_free (idle);
}
