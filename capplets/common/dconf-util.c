/*
 * dconf-util.c: helper API for dconf
 *
 * Copyright (C) 2012 Stefano Karapetsas
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Authors:
 *  Stefano Karapetsas <stefano@karapetsas.com>
 *  Vincent Untz <vuntz@gnome.org>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <dconf.h>

#include "dconf-util.h"

static DConfClient *
dconf_util_client_get (void)
{
    return dconf_client_new ();
}

gboolean
dconf_util_write_sync (const gchar  *key,
                       GVariant     *value,
                       GError      **error)
{
    gboolean     ret;
    DConfClient *client = dconf_util_client_get ();

    ret = dconf_client_write_sync (client, key, value, NULL, NULL, error);

    g_object_unref (client);

    return ret;
}

gboolean
dconf_util_recursive_reset (const gchar  *dir,
                            GError      **error)
{
    gboolean     ret;
    DConfClient *client = dconf_util_client_get ();

    ret = dconf_client_write_sync (client, dir, NULL, NULL, NULL, error);

    g_object_unref (client);

    return ret;
}

gchar **
dconf_util_list_subdirs (const gchar *dir,
                         gboolean     remove_trailing_slash)
{
    GArray       *array;
    gchar       **children;
    int       len;
    int       i;
    DConfClient  *client = dconf_util_client_get ();

    array = g_array_new (TRUE, TRUE, sizeof (gchar *));

    children = dconf_client_list (client, dir, &len);

    g_object_unref (client);

    for (i = 0; children[i] != NULL; i++) {
        if (dconf_is_rel_dir (children[i], NULL)) {
            char *val = g_strdup (children[i]);

            if (remove_trailing_slash)
                val[strlen (val) - 1] = '\0';

            array = g_array_append_val (array, val);
        }
    }

    g_strfreev (children);

    return (gchar **) g_array_free (array, FALSE);
}
