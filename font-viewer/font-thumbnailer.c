/* -*- mode: C; c-basic-offset: 4 -*- */
/*
 * font-thumbnailer: a thumbnailer for font files, using FreeType
 *
 * Copyright (C) 2002-2003  James Henstridge <james@daa.com.au>
 * Copyright (C) 2012 Cosimo Cecchi <cosimoc@gnome.org>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <locale.h>
#include <ft2build.h>
#include <math.h>
#include FT_FREETYPE_H

#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

#include <cairo/cairo-ft.h>

#include "sushi-font-loader.h"
#include "totem-resources.h"

static const gchar *
get_ft_error (FT_Error error)
{
#undef __FTERRORS_H__
#define FT_ERRORDEF(e,v,s) case e: return s;
#define FT_ERROR_START_LIST
#define FT_ERROR_END_LIST
    switch (error) {
#include FT_ERRORS_H
    default:
	return "unknown";
    }
}

#define THUMB_SIZE 128
#define PADDING_VERTICAL 2
#define PADDING_HORIZONTAL 4

static gboolean
check_font_contain_text (FT_Face face,
                         const gchar *text)
{
  gunichar *string;
  glong len, idx, map;
  FT_CharMap charmap;
  gboolean retval;

  string = g_utf8_to_ucs4_fast (text, -1, &len);

  for (map = 0; map < face->num_charmaps; map++) {
    charmap = face->charmaps[map];
    FT_Set_Charmap (face, charmap);

    retval = TRUE;

    for (idx = 0; idx < len; idx++) {
      gunichar c = string[idx];

      if (!FT_Get_Char_Index (face, c)) {
        retval = FALSE;
        break;
      }
    }

    if (retval)
      break;
  }

  g_free (string);

  return retval;
}

static gchar *
check_for_ascii_glyph_numbers (FT_Face face,
                               gboolean *found_ascii)
{
    GString *ascii_string, *string;
    gulong c;
    guint glyph, found = 0;

    string = g_string_new (NULL);
    ascii_string = g_string_new (NULL);
    *found_ascii = FALSE;

    c = FT_Get_First_Char (face, &glyph);

    do {
        if (glyph == 65 || glyph == 97) {
            g_string_append_unichar (ascii_string, (gunichar) c);
            found++;
        }

        if (found == 2)
            break;

        g_string_append_unichar (string, (gunichar) c);
        c = FT_Get_Next_Char (face, c, &glyph);
    } while (glyph != 0);

    if (found == 2) {
        *found_ascii = TRUE;
        g_string_free (string, TRUE);
        return g_string_free (ascii_string, FALSE);
    } else {
        g_string_free (ascii_string, TRUE);
        return g_string_free (string, FALSE);
    }
}

static gchar *
build_fallback_thumbstr (FT_Face face)
{
    gchar *chars;
    gint idx, total_chars;
    GString *retval;
    gchar *ptr, *end;
    gboolean found_ascii;

    chars = check_for_ascii_glyph_numbers (face, &found_ascii);

    if (found_ascii)
        return chars;

    idx = 0;
    retval = g_string_new (NULL);
    total_chars = g_utf8_strlen (chars, -1);

    while (idx < 2) {
        total_chars = (gint) floor (total_chars / 2.0);
        ptr = g_utf8_offset_to_pointer (chars, total_chars);
        end = g_utf8_find_next_char (ptr, NULL);

        g_string_append_len (retval, ptr, end - ptr);
        idx++;
    }

  return g_string_free (retval, FALSE);
}

int
main (int argc,
      char **argv)
{
    FT_Error error;
    FT_Library library;
    FT_Face face;
    GFile *file;
    gint font_size, thumb_size = THUMB_SIZE;
    gchar *thumbstr_utf8 = NULL, *help, *uri;
    gchar **arguments = NULL;
    GOptionContext *context;
    GError *gerror = NULL;
    gchar *contents = NULL;
    gboolean retval, default_thumbstr = TRUE;
    gint rv = 1;
    GdkRGBA black = { 0.0, 0.0, 0.0, 1.0 };
    cairo_surface_t *surface;
    cairo_t *cr;
    cairo_text_extents_t text_extents;
    cairo_font_face_t *font;
    gchar *str = NULL;
    gdouble scale, scale_x, scale_y;
    gint face_index = 0;
    gchar *fragment;

    const GOptionEntry options[] = {
	    { "text", 't', 0, G_OPTION_ARG_STRING, &thumbstr_utf8,
	      N_("Text to thumbnail (default: Aa)"), N_("TEXT") },
	    { "size", 's', 0, G_OPTION_ARG_INT, &thumb_size,
	      N_("Thumbnail size (default: 128)"), N_("SIZE") },
	    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &arguments,
	      NULL, N_("FONT-FILE OUTPUT-FILE") },
	    { NULL }
    };

    bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    setlocale (LC_ALL, "");

    context = g_option_context_new (NULL);
    g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);

    retval = g_option_context_parse (context, &argc, &argv, &gerror);
    if (!retval) {
	g_printerr ("Error parsing arguments: %s\n", gerror->message);

	g_option_context_free  (context);
	g_error_free (gerror);
        return 1;
    }

    if (!arguments || g_strv_length (arguments) != 2) {
	help = g_option_context_get_help (context, TRUE, NULL);
	g_printerr ("%s", help);

	g_option_context_free (context);
	goto out;
    }

    g_option_context_free (context);

    if (thumbstr_utf8 != NULL)
	default_thumbstr = FALSE;

    error = FT_Init_FreeType (&library);
    if (error) {
	g_printerr("Could not initialise freetype: %s\n", get_ft_error (error));
	goto out;
    }

    totem_resources_monitor_start (arguments[0], 30 * G_USEC_PER_SEC);

    fragment = strrchr (arguments[0], '#');
    if (fragment)
	face_index = strtol (fragment + 1, NULL, 0);

    file = g_file_new_for_commandline_arg (arguments[0]);
    uri = g_file_get_uri (file);
    g_object_unref (file);

    face = sushi_new_ft_face_from_uri (library, uri, face_index, &contents, &gerror);
    if (gerror) {
	g_printerr ("Could not load face '%s': %s\n", uri,
		    gerror->message);
        g_free (uri);
        g_error_free (gerror);
	goto out;
    }

    g_free (uri);

    if (default_thumbstr) {
        if (check_font_contain_text (face, "Aa"))
            str = g_strdup ("Aa");
        else
            str = build_fallback_thumbstr (face);
    } else {
        str = thumbstr_utf8;
    }

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                          thumb_size, thumb_size);
    cr = cairo_create (surface);

    font = cairo_ft_font_face_create_for_ft_face (face, 0);
    cairo_set_font_face (cr, font);
    cairo_font_face_destroy (font);

    font_size = thumb_size - 2 * PADDING_VERTICAL;
    cairo_set_font_size (cr, font_size);
    cairo_text_extents (cr, str, &text_extents);

    if ((text_extents.width) > (thumb_size - 2 * PADDING_HORIZONTAL)) {
        scale_x = (gdouble) (thumb_size - 2 * PADDING_HORIZONTAL) / (text_extents.width);
    } else {
        scale_x = 1.0;
    }

    if ((text_extents.height) > (thumb_size - 2 * PADDING_VERTICAL)) {
        scale_y = (gdouble) (thumb_size - 2 * PADDING_VERTICAL) / (text_extents.height);
    } else {
        scale_y = 1.0;
    }

    scale = MIN (scale_x, scale_y);
    cairo_scale (cr, scale, scale);
    cairo_translate (cr,
                     PADDING_HORIZONTAL - text_extents.x_bearing + (thumb_size - scale * text_extents.width) / 2.0,
                     PADDING_VERTICAL - text_extents.y_bearing + (thumb_size - scale * text_extents.height) / 2.0);

    gdk_cairo_set_source_rgba (cr, &black);
    cairo_show_text (cr, str);
    cairo_destroy (cr);

    cairo_surface_write_to_png (surface, arguments[1]);
    cairo_surface_destroy (surface);

    totem_resources_monitor_stop ();

    error = FT_Done_Face (face);
    if (error) {
	g_printerr("Could not unload face: %s\n", get_ft_error (error));
	goto out;
    }

    error = FT_Done_FreeType (library);
    if (error) {
	g_printerr ("Could not finalize freetype library: %s\n",
		   get_ft_error (error));
	goto out;
    }

    rv = 0; /* success */

  out:

    g_strfreev (arguments);
    g_free (str);
    g_free (contents);

    return rv;
}

