/* Tests for the display capplet's head sizes and placement.
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

static void
add_head (GArray *heads, const char *name, gboolean enabled, int x, int y,
          int width, int height, int preferred_width, int preferred_height,
          int transform, double scale)
{
    OutputHead head = { 0 };
    Mode mode = { preferred_width, preferred_height, 60000, TRUE };

    head.name = g_strdup (name);
    head.enabled = enabled;
    head.x = x;
    head.y = y;
    head.width = width;
    head.height = height;
    head.transform = transform;
    head.scale = scale;
    head.modes = g_array_new (FALSE, FALSE, sizeof (Mode));
    g_array_append_val (head.modes, mode);
    g_array_append_val (heads, head);
}

static OutputHead *
nth (GArray *heads, guint i)
{
    return &g_array_index (heads, OutputHead, i);
}

static void
test_sizes (void)
{
    GArray *heads = g_array_new (FALSE, FALSE, sizeof (OutputHead));
    int w, h;

    add_head (heads, "A", TRUE, 0, 0, 1920, 1080, 1920, 1080, 0, 1.0);
    add_head (heads, "B", TRUE, 0, 0, 3840, 2160, 3840, 2160, 1, 2.0);
    add_head (heads, "C", FALSE, 0, 0, 0, 0, 1366, 768, 0, 1.0);
    add_head (heads, "D", TRUE, 0, 0, 2560, 1600, 2560, 1600, 0, 1.5);

    display_head_get_size (nth (heads, 0), &w, &h);
    g_assert_cmpint (w, ==, 1920);
    g_assert_cmpint (h, ==, 1080);
    display_head_get_size (nth (heads, 1), &w, &h);
    g_assert_cmpint (w, ==, 2160);
    g_assert_cmpint (h, ==, 3840);
    display_head_get_logical_size (nth (heads, 1), &w, &h);
    g_assert_cmpint (w, ==, 1080);
    g_assert_cmpint (h, ==, 1920);
    display_head_get_size (nth (heads, 2), &w, &h);
    g_assert_cmpint (w, ==, 1366);
    g_assert_cmpint (h, ==, 768);
    display_head_get_logical_size (nth (heads, 3), &w, &h);
    g_assert_cmpint (w, ==, 1707);
    g_assert_cmpint (h, ==, 1067);

    display_heads_free (heads);
}

int
main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/display-head/sizes", test_sizes);

    return g_test_run ();
}
