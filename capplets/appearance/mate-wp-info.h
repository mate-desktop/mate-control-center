/*
 *  Authors: Rodney Dawes <dobey@ximian.com>
 *
 *  Copyright 2003-2006 Novell, Inc. (www.novell.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of version 2 of the GNU General Public License
 *  as published by the Free Software Foundation
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02110-1301, USA.
 *
 */

#ifndef _MATE_WP_INFO_H_
#define _MATE_WP_INFO_H_

#include <glib.h>
#include <libmate-desktop/mate-desktop-thumbnail.h>

typedef struct _MateWPInfo {
    char* uri;
    char* thumburi;
    char* name;
    char* mime_type;

    goffset size;

    time_t mtime;
} MateWPInfo;

MateWPInfo* mate_wp_info_new(const char* uri, MateDesktopThumbnailFactory* thumbs);
void mate_wp_info_free(MateWPInfo* info);

#endif

