/* The Wayland display capplet's monitors and where they are placed.
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

#ifndef DISPLAY_HEAD_H
#define DISPLAY_HEAD_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct
{
    int       width;
    int       height;
    int       rate;                    /* mHz */
    gboolean  preferred;
} Mode;

typedef struct GrabInfo GrabInfo;

typedef struct
{
    char       *name;                  /* connector */
    char       *description;
    gboolean    enabled;
    int         x;
    int         y;
    int         transform;             /* wl_output.transform value */
    double      scale;
    int         adaptive_sync_supported;  /* -1 unknown, 0 no, 1 yes */
    gboolean    adaptive_sync;
    int         width;                 /* current mode */
    int         height;
    int         rate;
    gboolean    preferred;
    GArray     *modes;                 /* Mode */
    GrabInfo   *grab_info;

    /* Only with DISPLAY_BACKEND_IDENTITY (else NULL or FALSE). */
    char       *id;
    char       *display_name;
    gboolean    internal;
    /* With DISPLAY_BACKEND_PRIMARY: read, and applied by display_backend_apply (). */
    gboolean    primary;
    /* With DISPLAY_BACKEND_MIRROR: the id of the head whose picture this one shows. */
    char       *mirror_of;
} OutputHead;

void     display_heads_free              (GArray     *heads);
void     display_head_get_preferred_size (OutputHead *head,
                                          int        *width,
                                          int        *height);
gboolean display_head_has_mode           (OutputHead *head,
                                          int         width,
                                          int         height,
                                          int         rate);
/* The current mode, or the preferred one when off, rotated: physical pixels. */
void     display_head_get_size           (OutputHead *head,
                                          int        *width,
                                          int        *height);
/* The same divided by the scale: the logical pixels positions are in. */
void     display_head_get_logical_size   (OutputHead *head,
                                          int        *width,
                                          int        *height);

G_END_DECLS

#endif
