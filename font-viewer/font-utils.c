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

#include "font-utils.h"

#include "sushi-font-loader.h"

gchar *
font_utils_get_font_name (FT_Face face)
{
  gchar *name;

  if (g_strcmp0 (face->style_name, "Regular") == 0)
    name = g_strdup (face->family_name);
  else
    name = g_strconcat (face->family_name, ", ", face->style_name, NULL);

  return name;
}

gchar *
font_utils_get_font_name_for_file (FT_Library library,
                                   const gchar *path,
                                   gint face_index)
{
    GFile *file;
    gchar *uri, *contents = NULL, *name = NULL;
    GError *error = NULL;
    FT_Face face;

    file = g_file_new_for_path (path);
    uri = g_file_get_uri (file);

    face = sushi_new_ft_face_from_uri (library, uri, face_index, &contents,
                                       &error);
    if (face != NULL) {
        name = font_utils_get_font_name (face);
        FT_Done_Face (face);
    } else if (error != NULL) {
        g_warning ("Can't get font name: %s\n", error->message);
        g_error_free (error);
    }

    g_free (uri);
    g_object_unref (file);
    g_free (contents);

    return name;
}

