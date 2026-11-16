/* Where the Wayland display capplet gets and applies the monitor layout.
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

#include <config.h>

#include "display-backend.h"

DisplayBackend *
display_backend_new (DisplayBackendChangedFunc changed, gpointer data, GError **error)
{
    DisplayBackend *backend = NULL;

#ifdef HAVE_WAYLAND_PROTOCOLS
    backend = display_backend_protocol_new (changed, data);
#endif
    if (backend == NULL)
        backend = display_backend_wlrandr_new (changed, data, error);
    return backend;
}

guint
display_backend_get_features (DisplayBackend *backend)
{
    return backend->features;
}

const char *
display_backend_get_name (DisplayBackend *backend)
{
    return backend->name;
}

void
display_backend_refresh (DisplayBackend *backend)
{
    backend->refresh (backend);
}

void
display_backend_apply (DisplayBackend            *backend,
                       GArray                    *heads,
                       gboolean                   temporary,
                       DisplayBackendAppliedFunc  done,
                       gpointer                   data)
{
    backend->apply (backend, heads,
                    temporary && (backend->features & DISPLAY_BACKEND_TEMPORARY),
                    done, data);
}

void
display_backend_free (DisplayBackend *backend)
{
    if (backend)
        backend->free (backend);
}

void
display_heads_free (GArray *heads)
{
    guint i;

    if (heads == NULL)
        return;

    for (i = 0; i < heads->len; i++)
    {
        OutputHead *head = &g_array_index (heads, OutputHead, i);

        g_free (head->name);
        g_free (head->description);
        g_free (head->grab_info);
        g_free (head->id);
        g_free (head->display_name);
        g_free (head->mirror_of);
        if (head->modes)
            g_array_free (head->modes, TRUE);
    }

    g_array_free (heads, TRUE);
}

void
display_head_get_preferred_size (OutputHead *head, int *width, int *height)
{
    guint i;
    int best_w = 0, best_h = 0;

    for (i = 0; i < head->modes->len; i++)
    {
        Mode *mode = &g_array_index (head->modes, Mode, i);

        if (mode->preferred)
        {
            *width = mode->width;
            *height = mode->height;
            return;
        }

        if (mode->width * mode->height > best_w * best_h)
        {
            best_w = mode->width;
            best_h = mode->height;
        }
    }

    *width = best_w;
    *height = best_h;
}

gboolean
display_head_has_mode (OutputHead *head, int width, int height, int rate)
{
    guint i;

    for (i = 0; i < head->modes->len; i++)
    {
        Mode *mode = &g_array_index (head->modes, Mode, i);

        if (mode->width == width && mode->height == height && mode->rate == rate)
            return TRUE;
    }

    return FALSE;
}
