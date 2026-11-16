/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Bastien Nocera <hadess@hadess.net>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <string.h>
#include <sys/types.h>
#include <X11/Xatom.h>

#include "msd-input-helper.h"

/* Without X the compositor owns the input devices; it may describe them
 * through mate_input_device_manager_v1 (see msd-input-wayland.c). */

static gboolean
kind_matches (guint kind, gboolean touchpads)
{
        if (touchpads)
                return kind == INPUT_KIND_TOUCHPAD;

        return kind == INPUT_KIND_MOUSE || kind == INPUT_KIND_TRACKBALL ||
               kind == INPUT_KIND_POINTING_STICK;
}

/* Without X: whether a touchpad is listed; TRUE when nobody lists devices. */
static gboolean
input_devices_have_touchpad (void)
{
        GArray *devices = input_devices_describe ();
        guint i;

        if (devices == NULL)
                return TRUE;

        for (i = 0; i < devices->len; i++)
                if (g_array_index (devices, InputDeviceDescription, i).kind == INPUT_KIND_TOUCHPAD)
                        return TRUE;

        return FALSE;
}

gboolean
touchpad_lacks_left_button (void)
{
        GArray *devices = input_devices_describe ();
        guint i;

        if (devices == NULL)
                return FALSE;

        for (i = 0; i < devices->len; i++) {
                InputDeviceDescription *device = &g_array_index (devices, InputDeviceDescription, i);

                if (device->kind == INPUT_KIND_TOUCHPAD && !(device->buttons & INPUT_BUTTON_LEFT))
                        return TRUE;
        }

        return FALSE;
}

gboolean
pointer_support (gboolean touchpads, InputDeviceDescription *support)
{
        GArray *devices = input_devices_describe ();
        gboolean found = FALSE;
        guint i;

        memset (support, 0, sizeof (*support));
        if (devices == NULL)
                return FALSE;

        for (i = 0; i < devices->len; i++) {
                InputDeviceDescription *device = &g_array_index (devices, InputDeviceDescription, i);

                if (!kind_matches (device->kind, touchpads))
                        continue;

                found = TRUE;
                support->config |= device->config;
                support->accel_profiles |= device->accel_profiles;
                support->scroll_methods |= device->scroll_methods;
                support->tap_fingers = MAX (support->tap_fingers, device->tap_fingers);
        }

        return found;
}

gboolean
supports_xinput_devices (void)
{
        gint op_code, event, error;

        return XQueryExtension (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                                "XInputExtension",
                                &op_code,
                                &event,
                                &error);
}

static gboolean
device_has_property (XDevice    *device,
                     const char *property_name)
{
        GdkDisplay *display;
        Atom realtype, prop;
        int realformat;
        unsigned long nitems, bytes_after;
        unsigned char *data;

        display = gdk_display_get_default ();
        prop = XInternAtom (GDK_DISPLAY_XDISPLAY (display), property_name, True);
        if (!prop)
                return FALSE;

        gdk_x11_display_error_trap_push (display);
        if ((XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (display), device, prop, 0, 1, False,
                                XA_INTEGER, &realtype, &realformat, &nitems,
                                &bytes_after, &data) == Success) && (realtype != None)) {
                gdk_x11_display_error_trap_pop_ignored (display);
                XFree (data);
                return TRUE;
        }

        gdk_x11_display_error_trap_pop_ignored (display);
        return FALSE;
}

XDevice*
device_is_touchpad (XDeviceInfo *deviceinfo)
{
        GdkDisplay *display;
        XDevice *device;

        display = gdk_display_get_default ();
        if (deviceinfo->type != XInternAtom (GDK_DISPLAY_XDISPLAY (display), XI_TOUCHPAD, True))
                return NULL;

        gdk_x11_display_error_trap_push (display);
        device = XOpenDevice (GDK_DISPLAY_XDISPLAY (display), deviceinfo->id);
        if (gdk_x11_display_error_trap_pop (display) || (device == NULL))
                return NULL;

        if (device_has_property (device, "libinput Tapping Enabled") ||
            device_has_property (device, "Synaptics Off")) {
                return device;
        }

        XCloseDevice (GDK_DISPLAY_XDISPLAY (display), device);
        return NULL;
}

static gboolean
device_is_driven_by_libinput (XDeviceInfo *deviceinfo)
{
        GdkDisplay *display = gdk_display_get_default ();
        XDevice *device;
        gboolean libinput;

        /* Xwayland's devices stand for the compositor's, which use libinput */
        if (g_str_has_prefix (deviceinfo->name, "xwayland-"))
                return TRUE;

        gdk_x11_display_error_trap_push (display);
        device = XOpenDevice (GDK_DISPLAY_XDISPLAY (display), deviceinfo->id);
        if (gdk_x11_display_error_trap_pop (display) || (device == NULL))
                return FALSE;

        /* xf86-input-libinput sets this on every device it drives */
        libinput = device_has_property (device, "libinput Send Events Modes Available");
        XCloseDevice (GDK_DISPLAY_XDISPLAY (display), device);

        return libinput;
}

/*
 * libinput has no motion threshold, only an acceleration speed and profile,
 * so the threshold setting does nothing for devices it drives: always outside
 * X11, and under X11 with xf86-input-libinput. Under X11 the answer is TRUE
 * only when there is at least one such pointer and all of them use libinput;
 * a device on evdev or synaptics still honours the threshold.
 */
gboolean
pointer_threshold_ignored (gboolean touchpads)
{
        GdkDisplay *display = gdk_display_get_default ();
        Atom touchpad_type;
        XDeviceInfo *device_info;
        gint n_devices, i;
        gint n_matching = 0;
        gboolean all_libinput = TRUE;

        if (!GDK_IS_X11_DISPLAY (display))
                return TRUE;

        if (supports_xinput_devices () == FALSE)
                return FALSE;

        touchpad_type = XInternAtom (GDK_DISPLAY_XDISPLAY (display), XI_TOUCHPAD, True);
        device_info = XListInputDevices (GDK_DISPLAY_XDISPLAY (display), &n_devices);
        if (device_info == NULL)
                return FALSE;

        for (i = 0; i < n_devices && all_libinput; i++) {
                gboolean is_touchpad;

                if (device_info[i].use != IsXExtensionPointer)
                        continue;
                /* The XTEST pointer is a virtual device of the X server */
                if (strstr (device_info[i].name, "XTEST") != NULL)
                        continue;

                is_touchpad = touchpad_type != None && device_info[i].type == touchpad_type;
                if (is_touchpad != touchpads)
                        continue;

                n_matching++;
                all_libinput = device_is_driven_by_libinput (&device_info[i]);
        }
        XFreeDeviceList (device_info);

        return n_matching > 0 && all_libinput;
}

gboolean
touchpad_is_present (void)
{
        XDeviceInfo *device_info;
        gint n_devices;
        guint i;
        gboolean retval;

        if (!GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
                return input_devices_have_touchpad ();

        if (supports_xinput_devices () == FALSE)
                return TRUE;

        retval = FALSE;

        device_info = XListInputDevices (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &n_devices);
        if (device_info == NULL)
                return FALSE;

        for (i = 0; i < n_devices; i++) {
                XDevice *device;

                device = device_is_touchpad (&device_info[i]);
                if (device != NULL) {
                        retval = TRUE;
                        break;
                }
        }
        if (device_info != NULL)
                XFreeDeviceList (device_info);

        return retval;
}
