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

#ifndef __MSD_INPUT_WAYLAND_H
#define __MSD_INPUT_WAYLAND_H

#include <glib.h>

G_BEGIN_DECLS

/* The values of mate_input_device_v1 (mate-input-device-v1.xml). */
enum {
        INPUT_KIND_MOUSE          = 2,
        INPUT_KIND_TOUCHPAD       = 3,
        INPUT_KIND_POINTING_STICK = 4,
        INPUT_KIND_TRACKBALL      = 5
};

enum {
        INPUT_BUTTON_LEFT = 1 << 0
};

enum {
        INPUT_CONFIG_TAP              = 1 << 0,
        INPUT_CONFIG_DWT              = 1 << 3,
        INPUT_CONFIG_NATURAL_SCROLL   = 1 << 5,
        INPUT_CONFIG_LEFT_HANDED      = 1 << 6,
        INPUT_CONFIG_MIDDLE_EMULATION = 1 << 7,
        INPUT_CONFIG_ACCEL_SPEED      = 1 << 8
};

enum {
        INPUT_SCROLL_TWO_FINGER = 1 << 0,
        INPUT_SCROLL_EDGE       = 1 << 1
};

typedef struct {
        guint kind;
        guint buttons;
        guint config;
        guint tap_fingers;
        guint accel_profiles;
        guint scroll_methods;
} InputDeviceDescription;

/* The devices the compositor describes, or NULL when it offers no
 * mate_input_device_manager_v1. Owned by the function; do not free. */
GArray *input_devices_describe (void);

G_END_DECLS

#endif /* __MSD_INPUT_WAYLAND_H */
