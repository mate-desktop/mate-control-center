/* Tests for arranging the Wayland display capplet's monitors.
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

#include "display-layout.h"

static OutputHead *
add_head (GArray *heads, const char *name, int x, int y, int width, int height,
          int transform, double scale)
{
    OutputHead head = { 0 };
    Mode mode = { width, height, 60000, TRUE };

    head.name = g_strdup (name);
    head.enabled = TRUE;
    head.x = x;
    head.y = y;
    head.width = width;
    head.height = height;
    head.transform = transform;
    head.scale = scale;
    head.modes = g_array_new (FALSE, FALSE, sizeof (Mode));
    g_array_append_val (head.modes, mode);
    g_array_append_val (heads, head);

    return &g_array_index (heads, OutputHead, heads->len - 1);
}

static OutputHead *
nth (GArray *heads, guint i)
{
    return &g_array_index (heads, OutputHead, i);
}

static GArray *
two_heads (void)
{
    GArray *heads = g_array_new (FALSE, FALSE, sizeof (OutputHead));

    add_head (heads, "A", 0, 0, 1920, 1080, 0, 1.0);
    add_head (heads, "B", 1920, 0, 1280, 1024, 0, 1.0);

    return heads;
}

static void
assert_at (OutputHead *head, int x, int y)
{
    g_assert_cmpint (head->x, ==, x);
    g_assert_cmpint (head->y, ==, y);
}

static void
test_snap_sides (void)
{
    GArray *heads = two_heads ();
    OutputHead *b = nth (heads, 1);

    display_layout_move (heads, b, 1925, 15, 20);
    assert_at (b, 1920, 0);
    display_layout_move (heads, b, -1275, 60, 20);
    assert_at (b, -1280, 56);
    display_layout_move (heads, b, 10, -1030, 20);
    assert_at (b, 0, -1024);
    display_layout_move (heads, b, 650, 1090, 20);
    assert_at (b, 640, 1080);
    assert_at (nth (heads, 0), 0, 0);
    g_assert_true (display_layout_is_valid (heads));

    display_heads_free (heads);
}

static void
test_offsets (void)
{
    GArray *heads = two_heads ();
    OutputHead *b = nth (heads, 1);

    display_layout_move (heads, b, 1920, -200, 20);
    assert_at (b, 1920, -200);
    display_layout_move (heads, b, 5000, -200, 20);
    assert_at (b, 1920, -200);
    display_layout_move (heads, b, 1940, -25, 0);
    assert_at (b, 1920, -25);
    display_layout_move (heads, b, 1920, -1500, 20);
    assert_at (b, 1919, -1024);
    g_assert_true (display_layout_is_valid (heads));

    display_heads_free (heads);
}

static void
test_no_overlap_while_moving (void)
{
    GArray *heads = two_heads ();
    OutputHead *b = nth (heads, 1);

    display_layout_move (heads, b, 500, 300, 20);
    assert_at (b, 500, 1080);
    g_assert_true (display_layout_is_valid (heads));

    display_heads_free (heads);
}

static void
test_close_gaps (void)
{
    GArray *heads = two_heads ();
    OutputHead *c = add_head (heads, "C", 3200, 0, 1024, 768, 0, 1.0);
    OutputHead *b = nth (heads, 1);

    display_layout_move (heads, b, 0, 1080, 0);
    assert_at (b, 0, 1080);
    g_assert_false (display_layout_is_valid (heads));

    display_layout_tidy (heads, b);
    assert_at (nth (heads, 0), 0, 0);
    assert_at (b, 0, 1080);
    assert_at (c, 1920, 0);
    g_assert_true (display_layout_is_valid (heads));

    display_heads_free (heads);
}

static void
test_close_gaps_keeps_groups (void)
{
    GArray *heads = g_array_new (FALSE, FALSE, sizeof (OutputHead));

    add_head (heads, "A", 0, 0, 1920, 1080, 0, 1.0);
    add_head (heads, "B", 3000, 100, 1280, 1024, 0, 1.0);
    add_head (heads, "C", 4280, 100, 1024, 768, 0, 1.0);

    display_layout_tidy (heads, NULL);
    assert_at (nth (heads, 0), 0, 0);
    assert_at (nth (heads, 1), 1920, 100);
    assert_at (nth (heads, 2), 3200, 100);
    g_assert_true (display_layout_is_valid (heads));

    display_heads_free (heads);
}

static void
test_push_overlaps_apart (void)
{
    GArray *heads = two_heads ();

    nth (heads, 1)->x = 1000;
    g_assert_false (display_layout_is_valid (heads));

    display_layout_tidy (heads, nth (heads, 0));
    assert_at (nth (heads, 0), 0, 0);
    assert_at (nth (heads, 1), 1920, 0);

    nth (heads, 1)->x = 600;
    nth (heads, 1)->y = 900;
    display_layout_tidy (heads, nth (heads, 1));
    assert_at (nth (heads, 1), 600, 900);
    assert_at (nth (heads, 0), 0, -180);
    g_assert_true (display_layout_is_valid (heads));

    display_heads_free (heads);
}

static void
test_tidy_anchor (void)
{
    GArray *heads = two_heads ();

    nth (heads, 1)->x = 2500;
    nth (heads, 1)->primary = TRUE;
    display_layout_tidy (heads, NULL);
    assert_at (nth (heads, 1), 2500, 0);
    assert_at (nth (heads, 0), 580, 0);

    display_heads_free (heads);
}

static void
test_mirrors_stay (void)
{
    GArray *heads = g_array_new (FALSE, FALSE, sizeof (OutputHead));

    add_head (heads, "A", 0, 0, 1920, 1080, 0, 1.0);
    add_head (heads, "B", 0, 0, 1920, 1080, 0, 1.0);
    add_head (heads, "C", 1920, 0, 1280, 1024, 0, 1.0);

    g_assert_true (display_layout_is_valid (heads));
    display_layout_tidy (heads, NULL);
    assert_at (nth (heads, 0), 0, 0);
    assert_at (nth (heads, 1), 0, 0);
    assert_at (nth (heads, 2), 1920, 0);

    display_heads_free (heads);
}

static void
test_scale (void)
{
    GArray *heads = g_array_new (FALSE, FALSE, sizeof (OutputHead));
    OutputHead *b;

    add_head (heads, "A", 0, 0, 1920, 1080, 0, 1.0);
    b = add_head (heads, "B", 5000, 0, 3840, 2160, 0, 2.0);

    display_layout_move (heads, b, 1925, 5, 20);
    assert_at (b, 1920, 0);
    display_layout_move (heads, b, 10, 1085, 20);
    assert_at (b, 0, 1080);
    display_layout_move (heads, b, 1000, 500, 0);
    assert_at (b, 1000, 1080);
    g_assert_true (display_layout_is_valid (heads));

    display_heads_free (heads);
}

static void
test_rotation (void)
{
    GArray *heads = g_array_new (FALSE, FALSE, sizeof (OutputHead));
    OutputHead *b;

    add_head (heads, "A", 0, 0, 1920, 1080, 0, 1.0);
    b = add_head (heads, "B", 5000, 0, 1920, 1080, 1, 1.0);

    display_layout_move (heads, b, 830, 1085, 20);
    assert_at (b, 840, 1080);
    display_layout_move (heads, b, 1925, -830, 20);
    assert_at (b, 1920, -840);
    g_assert_true (display_layout_is_valid (heads));

    display_heads_free (heads);
}

static void
test_resized (void)
{
    GArray *heads = two_heads ();
    OutputHead *c = add_head (heads, "C", 0, 1080, 1024, 768, 0, 1.0);
    OutputHead *a = nth (heads, 0);

    a->width = 1280;
    a->height = 720;
    display_layout_resized (heads, a, 1920, 1080);
    assert_at (a, 0, 0);
    assert_at (nth (heads, 1), 1280, 0);
    assert_at (c, 0, 720);
    g_assert_true (display_layout_is_valid (heads));

    nth (heads, 1)->scale = 2.0;
    display_layout_resized (heads, nth (heads, 1), 1280, 1024);
    assert_at (nth (heads, 1), 1280, 0);
    g_assert_true (display_layout_is_valid (heads));

    display_heads_free (heads);
}

static void
test_normalize (void)
{
    GArray *heads = two_heads ();

    display_layout_move (heads, nth (heads, 1), -1280, -200, 0);
    display_layout_normalize (heads);
    assert_at (nth (heads, 1), 0, 0);
    assert_at (nth (heads, 0), 1280, 200);

    display_heads_free (heads);
}

static void
test_validity (void)
{
    GArray *heads = two_heads ();

    g_assert_true (display_layout_is_valid (heads));
    nth (heads, 1)->x = 1921;
    g_assert_false (display_layout_is_valid (heads));
    nth (heads, 1)->x = 1919;
    g_assert_false (display_layout_is_valid (heads));
    nth (heads, 1)->x = 1920;
    nth (heads, 1)->y = 1080;
    g_assert_false (display_layout_is_valid (heads));
    nth (heads, 1)->enabled = FALSE;
    g_assert_true (display_layout_is_valid (heads));

    display_heads_free (heads);
}

int
main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/display-layout/snap-sides", test_snap_sides);
    g_test_add_func ("/display-layout/offsets", test_offsets);
    g_test_add_func ("/display-layout/no-overlap-while-moving", test_no_overlap_while_moving);
    g_test_add_func ("/display-layout/close-gaps", test_close_gaps);
    g_test_add_func ("/display-layout/close-gaps-keeps-groups", test_close_gaps_keeps_groups);
    g_test_add_func ("/display-layout/push-overlaps-apart", test_push_overlaps_apart);
    g_test_add_func ("/display-layout/tidy-anchor", test_tidy_anchor);
    g_test_add_func ("/display-layout/mirrors-stay", test_mirrors_stay);
    g_test_add_func ("/display-layout/scale", test_scale);
    g_test_add_func ("/display-layout/rotation", test_rotation);
    g_test_add_func ("/display-layout/resized", test_resized);
    g_test_add_func ("/display-layout/normalize", test_normalize);
    g_test_add_func ("/display-layout/validity", test_validity);

    return g_test_run ();
}
