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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __FTSTREAM_VFS_H__
#define __FTSTREAM_VFS_H__

#include <ft2build.h>
#include FT_FREETYPE_H
#include <glib.h>

FT_Error FT_New_Face_From_URI (FT_Library library,
			       const gchar* uri,
			       FT_Long face_index,
			       FT_Face *aface);

#endif /* __FTSTREAM_VFS_H__ */
