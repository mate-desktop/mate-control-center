/* Arranging the Wayland display capplet's monitors.
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

#ifndef DISPLAY_LAYOUT_H
#define DISPLAY_LAYOUT_H

#include "display-head.h"

G_BEGIN_DECLS

/* Positions are logical pixels and only enabled heads count. A valid layout
 * is connected through shared stretches of edge and has no overlaps, except
 * heads with the same rectangle (mirrors).
 */

/* Puts @head at the valid spot nearest to @x,@y; the others stay. Along the
 * shared edge it lines up with the other head's edges within @snap pixels.
 */
void     display_layout_move      (GArray     *heads,
                                   OutputHead *head,
                                   int         x,
                                   int         y,
                                   int         snap);
/* Makes the layout valid with the least movement, keeping @fixed (or, when
 * NULL, the primary or first enabled head) in place.
 */
void     display_layout_tidy      (GArray     *heads,
                                   OutputHead *fixed);
/* After @head changed size: the heads right of or below it follow its edge. */
void     display_layout_resized   (GArray     *heads,
                                   OutputHead *head,
                                   int         old_width,
                                   int         old_height);
/* Shifts every head so the enabled ones start at 0,0. */
void     display_layout_normalize (GArray     *heads);
gboolean display_layout_is_valid  (GArray     *heads);

G_END_DECLS

#endif
