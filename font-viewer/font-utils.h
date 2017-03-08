/* -*- mode: C; c-basic-offset: 4 -*-
 * mate-font-viewer:
 *
 * Copyright (C) 2012 Cosimo Cecchi <cosimoc@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __FONT_UTILS_H__
#define __FONT_UTILS_H__

#include <ft2build.h>
#include FT_FREETYPE_H
#include <glib.h>

gchar * font_utils_get_font_name (FT_Face face);
gchar * font_utils_get_font_name_for_file (FT_Library library,
                                           const gchar *path,
                                           gint face_index);

#endif /* __FONT_UTILS_H__ */

