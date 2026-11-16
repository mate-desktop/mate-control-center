/* Labels on each monitor naming it, for the Wayland display capplet.
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

#ifndef DISPLAY_LABELER_H
#define DISPLAY_LABELER_H

#include <gtk/gtk.h>

#include "display-head.h"

G_BEGIN_DECLS

typedef struct DisplayLabeler DisplayLabeler;

DisplayLabeler *display_labeler_new       (void);
void            display_labeler_free      (DisplayLabeler *labeler);
/* Labels every enabled head on its monitor, or only the first when @mirror. */
void            display_labeler_show      (DisplayLabeler *labeler,
                                           GArray         *heads,
                                           gboolean        mirror);
void            display_labeler_hide      (DisplayLabeler *labeler);
/* The colour of head @index of @count, the same as MateRRLabeler's. */
void            display_labeler_get_color (guint           index,
                                           guint           count,
                                           GdkRGBA        *color);
/* The display name, else the description, else the connector. */
const char     *display_head_get_label    (OutputHead     *head);

G_END_DECLS

#endif
