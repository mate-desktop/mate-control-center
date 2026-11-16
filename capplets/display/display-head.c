/* The Wayland display capplet's monitors and where they are placed.
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

#include "display-head.h"

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

void
display_head_get_size (OutputHead *head, int *width, int *height)
{
    if (head->enabled && head->width > 0 && head->height > 0)
    {
        *width = head->width;
        *height = head->height;
    }
    else
    {
        display_head_get_preferred_size (head, width, height);
    }

    if (head->transform & 1)
    {
        int tmp = *height;

        *height = *width;
        *width = tmp;
    }
}

void
display_head_get_logical_size (OutputHead *head, int *width, int *height)
{
    display_head_get_size (head, width, height);

    if (head->scale > 0.0)
    {
        *width = (int) ((double) *width / head->scale + 0.5);
        *height = (int) ((double) *height / head->scale + 0.5);
    }
}
