/*
 * This file is part of libslab.
 *
 * Copyright (c) 2006 Novell, Inc.
 *
 * Libslab is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Libslab is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libslab; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "slab-mate-util.h"
#include "libslab-utils.h"

#include <gio/gio.h>
#include <string.h>

MateDesktopItem *
load_desktop_item_from_unknown (const gchar *id)
{
	MateDesktopItem *item;
	gchar            *basename;

	GError *error = NULL;


	item = mate_desktop_item_new_from_uri (id, 0, &error);

	if (! error)
		return item;
	else {
		g_error_free (error);
		error = NULL;
	}

	item = mate_desktop_item_new_from_file (id, 0, &error);

	if (! error)
		return item;
	else {
		g_error_free (error);
		error = NULL;
	}

	item = mate_desktop_item_new_from_basename (id, 0, &error);

	if (! error)
		return item;
	else {
		g_error_free (error);
		error = NULL;
	}

	basename = g_strrstr (id, "/");

	if (basename) {
		basename++;

		item = mate_desktop_item_new_from_basename (basename, 0, &error);

		if (! error)
			return item;
		else {
			g_error_free (error);
			error = NULL;
		}
	}

	return NULL;
}

gboolean
open_desktop_item_exec (MateDesktopItem * desktop_item)
{
	GError *error = NULL;

	if (!desktop_item)
		return FALSE;

	mate_desktop_item_launch (desktop_item, NULL, MATE_DESKTOP_ITEM_LAUNCH_ONLY_ONE | MATE_DESKTOP_ITEM_LAUNCH_DO_NOT_REAP_CHILD, &error);

	if (error)
	{
		g_warning ("error launching %s [%s]\n",
			mate_desktop_item_get_location (desktop_item), error->message);

		g_error_free (error);
		return FALSE;
	}

	return TRUE;
}

gboolean
open_desktop_item_help (MateDesktopItem * desktop_item)
{
	const gchar *doc_path;
	gchar *help_uri;

	GError *error;

	if (!desktop_item)
		return FALSE;

	doc_path = mate_desktop_item_get_string (desktop_item, "DocPath");

	if (doc_path)
	{
		help_uri = g_strdup_printf ("help:%s", doc_path);

		error = NULL;
		if (!gtk_show_uri_on_window (NULL, help_uri, gtk_get_current_event_time (), &error))
		{
			g_warning ("error opening %s [%s]\n", help_uri, error->message);

			g_free (help_uri);
			g_error_free (error);
			return FALSE;
		}

		g_free (help_uri);
	}
	else
		return FALSE;

	return TRUE;
}

void
copy_file (const gchar * src_uri, const gchar * dst_uri)
{
	GFile *src;
	GFile *dst;
	GError *error = NULL;
	gboolean res;

	src = g_file_new_for_uri (src_uri);
	dst = g_file_new_for_uri (dst_uri);

	res = g_file_copy (src, dst,
			   G_FILE_COPY_NONE,
			   NULL, NULL, NULL, &error);

	if (!res)
	{
		g_warning ("error copying [%s] to [%s]: %s.", src_uri, dst_uri, error->message);
		g_error_free (error);
	}

	g_object_unref (src);
	g_object_unref (dst);
}
