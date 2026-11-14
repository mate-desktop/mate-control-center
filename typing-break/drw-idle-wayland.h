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

#ifndef __DRW_IDLE_WAYLAND_H__
#define __DRW_IDLE_WAYLAND_H__

#include <glib.h>

/* Input idle state on Wayland, from ext-idle-notify-v1. */
typedef struct _DrwIdleWayland DrwIdleWayland;

/* NULL when the display is not Wayland or the compositor lacks the protocol.
 * The user is "idle" after timeout_ms without input. */
DrwIdleWayland *drw_idle_wayland_new     (guint           timeout_ms);
gboolean        drw_idle_wayland_is_idle (DrwIdleWayland *idle);
void            drw_idle_wayland_free    (DrwIdleWayland *idle);

#endif /* __DRW_IDLE_WAYLAND_H__ */
