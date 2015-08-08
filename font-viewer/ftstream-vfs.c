/* -*- mode: C; c-basic-offset: 4 -*- */

/*
 * ftstream-vfs: a FreeType/GIO stream bridge
 *
 * Copyright (C) 2002-2003  James Henstridge <james@daa.com.au>
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

#include <stdlib.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <gio/gio.h>

#include "ftstream-vfs.h"

static unsigned long
vfs_stream_read (FT_Stream stream,
		 unsigned long offset,
		 unsigned char *buffer,
		 unsigned long count)
{
    GFileInputStream *handle = stream->descriptor.pointer;
    gssize bytes_read = 0;

    if (!g_seekable_seek (G_SEEKABLE (handle), offset, G_SEEK_SET, NULL, NULL))
        return 0;

    if (count > 0) {
        bytes_read = g_input_stream_read (G_INPUT_STREAM (handle), buffer,
					  count, NULL, NULL);

        if (bytes_read == -1)
            return 0;
    }

    return bytes_read;
}

static void
vfs_stream_close (FT_Stream stream)
{
    GFileInputStream *handle = stream->descriptor.pointer;

    if (handle == NULL)
        return;

    /* this also closes the stream */
    g_object_unref (handle);

    stream->descriptor.pointer = NULL;
    stream->size = 0;
    stream->base = NULL;
}

static FT_Error
vfs_stream_open (FT_Stream stream,
		 const char *uri)
{
    GFile *file;
    GFileInfo *info;
    GFileInputStream *handle;

    file = g_file_new_for_uri (uri);
    handle = g_file_read (file, NULL, NULL);

    if (handle == NULL) {
	g_object_unref (file);
        return FT_Err_Cannot_Open_Resource;
    }

    info = g_file_query_info (file,
			      G_FILE_ATTRIBUTE_STANDARD_SIZE,
                              G_FILE_QUERY_INFO_NONE, NULL,
			      NULL);
    g_object_unref (file);

    if (info == NULL) {
        return FT_Err_Cannot_Open_Resource;
    }

    stream->size = g_file_info_get_size (info);

    g_object_unref (info);

    stream->descriptor.pointer = handle;
    stream->pathname.pointer = NULL;
    stream->pos = 0;

    stream->read = vfs_stream_read;
    stream->close = vfs_stream_close;

    return FT_Err_Ok;
}

/* load a typeface from a URI */
FT_Error
FT_New_Face_From_URI (FT_Library library,
		      const gchar* uri,
		      FT_Long face_index,
		      FT_Face *aface)
{
    FT_Open_Args args;
    FT_Stream stream;
    FT_Error error;

    stream = calloc (1, sizeof (*stream));

    if (stream == NULL)
	return FT_Err_Out_Of_Memory;

    error = vfs_stream_open (stream, uri);

    if (error != FT_Err_Ok) {
	free (stream);
	return error;
    }

    args.flags = FT_OPEN_STREAM;
    args.stream = stream;

    error = FT_Open_Face (library, &args, face_index, aface);

    if (error != FT_Err_Ok) {
	if (stream->close != NULL)
	    stream->close(stream);

	free (stream);
	return error;
    }

    /* so that freetype will free the stream */
    (*aface)->face_flags &= ~FT_FACE_FLAG_EXTERNAL_STREAM;

    return error;
}
