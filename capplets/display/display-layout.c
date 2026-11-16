/* Arranging the Wayland display capplet's monitors.
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

typedef struct
{
    int x, y, width, height;
} Rect;

typedef struct
{
    OutputHead *head;
    Rect        rect;
    gboolean    placed;
    guint       group;
} Item;

typedef struct
{
    gboolean found;
    gint64   cost;
    int      dx, dy;
} Best;

static GArray *
collect (GArray *heads)
{
    GArray *items = g_array_new (FALSE, TRUE, sizeof (Item));
    guint i;

    for (i = 0; i < heads->len; i++)
    {
        OutputHead *head = &g_array_index (heads, OutputHead, i);
        Item item = { 0 };

        if (!head->enabled)
            continue;

        item.head = head;
        item.rect.x = head->x;
        item.rect.y = head->y;
        display_head_get_logical_size (head, &item.rect.width, &item.rect.height);
        item.group = items->len;
        g_array_append_val (items, item);
    }

    return items;
}

static Item *
item_at (GArray *items, guint i)
{
    return &g_array_index (items, Item, i);
}

static void
write_back (GArray *items)
{
    guint i;

    for (i = 0; i < items->len; i++)
    {
        item_at (items, i)->head->x = item_at (items, i)->rect.x;
        item_at (items, i)->head->y = item_at (items, i)->rect.y;
    }
}

static gboolean
same (const Rect *a, const Rect *b)
{
    return a->x == b->x && a->y == b->y &&
           a->width == b->width && a->height == b->height;
}

static gboolean
overlaps (const Rect *a, const Rect *b)
{
    return !same (a, b) &&
           a->x < b->x + b->width && b->x < a->x + a->width &&
           a->y < b->y + b->height && b->y < a->y + a->height;
}

static gboolean
touches (const Rect *a, const Rect *b)
{
    if ((a->x + a->width == b->x || b->x + b->width == a->x) &&
        a->y < b->y + b->height && b->y < a->y + a->height)
        return TRUE;

    return (a->y + a->height == b->y || b->y + b->height == a->y) &&
           a->x < b->x + b->width && b->x < a->x + a->width;
}

static gboolean
joined (const Rect *a, const Rect *b)
{
    return same (a, b) || touches (a, b);
}

static gboolean
fits (GArray *items, guint group, int dx, int dy)
{
    guint i, j;

    for (i = 0; i < items->len; i++)
    {
        Item *moving = item_at (items, i);
        Rect r;

        if (moving->placed || moving->group != group)
            continue;

        r = moving->rect;
        r.x += dx;
        r.y += dy;

        for (j = 0; j < items->len; j++)
        {
            Item *other = item_at (items, j);

            if (other->placed && overlaps (&r, &other->rect))
                return FALSE;
        }
    }

    return TRUE;
}

static int
along (int want, int lo, int hi, int target1, int target2, int snap)
{
    int value = CLAMP (want, lo, hi);
    int targets[2] = { target1, target2 };
    int best = snap + 1;
    int i;

    for (i = 0; i < 2; i++)
    {
        int distance = ABS (targets[i] - want);

        if (targets[i] >= lo && targets[i] <= hi && distance <= snap && distance < best)
        {
            value = targets[i];
            best = distance;
        }
    }

    return value;
}

static void
consider (GArray *items, guint group, int dx, int dy, int want_dx, int want_dy, Best *best)
{
    gint64 cost = (gint64) (dx - want_dx) * (dx - want_dx) +
                  (gint64) (dy - want_dy) * (dy - want_dy);

    if (best->found && cost >= best->cost)
        return;

    if (!fits (items, group, dx, dy))
        return;

    best->found = TRUE;
    best->cost = cost;
    best->dx = dx;
    best->dy = dy;
}

static gboolean
place_group (GArray *items, guint group, int want_dx, int want_dy, int snap, Best *best)
{
    guint i, j;

    best->found = FALSE;

    for (i = 0; i < items->len; i++)
    {
        Item *moving = item_at (items, i);
        const Rect *a = &moving->rect;

        if (moving->placed || moving->group != group)
            continue;

        for (j = 0; j < items->len; j++)
        {
            Item *other = item_at (items, j);
            const Rect *b = &other->rect;
            int dx, dy;

            if (!other->placed)
                continue;

            dy = along (want_dy,
                        b->y - a->height + 1 - a->y, b->y + b->height - 1 - a->y,
                        b->y - a->y, b->y + b->height - a->height - a->y, snap);
            consider (items, group, b->x + b->width - a->x, dy, want_dx, want_dy, best);
            consider (items, group, b->x - a->width - a->x, dy, want_dx, want_dy, best);

            dx = along (want_dx,
                        b->x - a->width + 1 - a->x, b->x + b->width - 1 - a->x,
                        b->x - a->x, b->x + b->width - a->width - a->x, snap);
            consider (items, group, dx, b->y + b->height - a->y, want_dx, want_dy, best);
            consider (items, group, dx, b->y - a->height - a->y, want_dx, want_dy, best);
        }
    }

    return best->found;
}

static void
translate_group (GArray *items, guint group, int dx, int dy)
{
    guint i;

    for (i = 0; i < items->len; i++)
    {
        Item *item = item_at (items, i);

        if (item->placed || item->group != group)
            continue;

        item->rect.x += dx;
        item->rect.y += dy;
        item->placed = TRUE;
    }
}

void
display_layout_move (GArray *heads, OutputHead *head, int x, int y, int snap)
{
    GArray *items;
    Item *moving = NULL;
    Best best;
    guint i;

    items = collect (heads);

    for (i = 0; i < items->len; i++)
    {
        Item *item = item_at (items, i);

        item->placed = item->head != head;
        if (item->head == head)
            moving = item;
    }

    if (moving == NULL || items->len == 1 ||
        !place_group (items, moving->group, x - moving->rect.x, y - moving->rect.y, snap, &best))
    {
        head->x = x;
        head->y = y;
    }
    else
    {
        head->x = moving->rect.x + best.dx;
        head->y = moving->rect.y + best.dy;
    }

    g_array_free (items, TRUE);
}

static gboolean
attaches (GArray *items, Item *item)
{
    gboolean attached = FALSE;
    guint i;

    for (i = 0; i < items->len; i++)
    {
        Item *other = item_at (items, i);

        if (!other->placed)
            continue;

        if (overlaps (&item->rect, &other->rect))
            return FALSE;

        if (joined (&item->rect, &other->rect))
            attached = TRUE;
    }

    return attached;
}

static void
attach_in_place (GArray *items)
{
    gboolean progress = TRUE;
    guint i;

    while (progress)
    {
        progress = FALSE;

        for (i = 0; i < items->len; i++)
        {
            Item *item = item_at (items, i);

            if (!item->placed && attaches (items, item))
            {
                item->placed = TRUE;
                progress = TRUE;
            }
        }
    }
}

static guint
find_group (GArray *items, guint i)
{
    while (item_at (items, i)->group != i)
        i = item_at (items, i)->group;

    return i;
}

static void
form_groups (GArray *items, gboolean rigid)
{
    guint i, j;

    for (i = 0; i < items->len; i++)
        item_at (items, i)->group = i;

    if (!rigid)
        return;

    for (i = 0; i < items->len; i++)
    {
        for (j = i + 1; j < items->len; j++)
        {
            Item *a = item_at (items, i);
            Item *b = item_at (items, j);

            if (!a->placed && !b->placed && joined (&a->rect, &b->rect))
                item_at (items, find_group (items, j))->group = find_group (items, i);
        }
    }

    for (i = 0; i < items->len; i++)
        item_at (items, i)->group = find_group (items, i);

    for (i = 0; i < items->len; i++)
    {
        for (j = i + 1; j < items->len; j++)
        {
            Item *a = item_at (items, i);
            Item *b = item_at (items, j);

            if (!a->placed && !b->placed && a->group == b->group &&
                overlaps (&a->rect, &b->rect))
            {
                form_groups (items, FALSE);
                return;
            }
        }
    }
}

static gboolean
place_nearest_group (GArray *items, gboolean rigid)
{
    Best chosen = { FALSE, 0, 0, 0 };
    guint chosen_group = 0;
    guint i;

    form_groups (items, rigid);

    for (i = 0; i < items->len; i++)
    {
        Item *item = item_at (items, i);
        Best best;

        if (item->placed || item->group != i)
            continue;

        if (place_group (items, i, 0, 0, 0, &best) &&
            (!chosen.found || best.cost < chosen.cost))
        {
            chosen = best;
            chosen_group = i;
        }
    }

    if (!chosen.found)
        return FALSE;

    translate_group (items, chosen_group, chosen.dx, chosen.dy);
    return TRUE;
}

void
display_layout_tidy (GArray *heads, OutputHead *fixed)
{
    GArray *items;
    Item *anchor = NULL;
    guint i;

    items = collect (heads);
    if (items->len == 0)
    {
        g_array_free (items, TRUE);
        return;
    }

    for (i = 0; i < items->len && anchor == NULL; i++)
        if (item_at (items, i)->head == fixed)
            anchor = item_at (items, i);

    for (i = 0; i < items->len && anchor == NULL; i++)
        if (item_at (items, i)->head->primary)
            anchor = item_at (items, i);

    if (anchor == NULL)
        anchor = item_at (items, 0);

    anchor->placed = TRUE;

    for (;;)
    {
        gboolean done = TRUE;

        attach_in_place (items);

        for (i = 0; i < items->len; i++)
            if (!item_at (items, i)->placed)
                done = FALSE;

        if (done)
            break;

        if (!place_nearest_group (items, TRUE) && !place_nearest_group (items, FALSE))
            break;
    }

    write_back (items);
    g_array_free (items, TRUE);
}

void
display_layout_resized (GArray *heads, OutputHead *head, int old_width, int old_height)
{
    int width, height;
    int old_right, old_bottom;
    guint i;

    display_head_get_logical_size (head, &width, &height);
    if (width == old_width && height == old_height)
        return;

    old_right = head->x + old_width;
    old_bottom = head->y + old_height;

    for (i = 0; i < heads->len; i++)
    {
        OutputHead *other = &g_array_index (heads, OutputHead, i);
        int w, h;

        if (other == head || !other->enabled)
            continue;

        display_head_get_logical_size (other, &w, &h);

        if (other->x >= old_right)
            other->x += width - old_width;
        else if (other->x + w == old_right)
            other->x = head->x + width - w;

        if (other->y >= old_bottom)
            other->y += height - old_height;
        else if (other->y + h == old_bottom)
            other->y = head->y + height - h;
    }

    display_layout_tidy (heads, head);
}

void
display_layout_normalize (GArray *heads)
{
    gboolean any = FALSE;
    int min_x = 0, min_y = 0;
    guint i;

    for (i = 0; i < heads->len; i++)
    {
        OutputHead *head = &g_array_index (heads, OutputHead, i);

        if (!head->enabled)
            continue;

        if (!any || head->x < min_x)
            min_x = head->x;
        if (!any || head->y < min_y)
            min_y = head->y;
        any = TRUE;
    }

    if (!any || (min_x == 0 && min_y == 0))
        return;

    for (i = 0; i < heads->len; i++)
    {
        g_array_index (heads, OutputHead, i).x -= min_x;
        g_array_index (heads, OutputHead, i).y -= min_y;
    }
}

gboolean
display_layout_is_valid (GArray *heads)
{
    GArray *items;
    gboolean valid = TRUE;
    gboolean progress = TRUE;
    guint i, j;

    items = collect (heads);

    for (i = 0; i < items->len && valid; i++)
        for (j = i + 1; j < items->len && valid; j++)
            if (overlaps (&item_at (items, i)->rect, &item_at (items, j)->rect))
                valid = FALSE;

    if (valid && items->len > 0)
    {
        item_at (items, 0)->placed = TRUE;

        while (progress)
        {
            progress = FALSE;
            for (i = 0; i < items->len; i++)
            {
                Item *item = item_at (items, i);

                if (item->placed)
                    continue;

                for (j = 0; j < items->len; j++)
                {
                    Item *other = item_at (items, j);

                    if (other->placed && joined (&item->rect, &other->rect))
                    {
                        item->placed = TRUE;
                        progress = TRUE;
                        break;
                    }
                }
            }
        }

        for (i = 0; i < items->len; i++)
            if (!item_at (items, i)->placed)
                valid = FALSE;
    }

    g_array_free (items, TRUE);
    return valid;
}

static gboolean
head_has_size (OutputHead *head, int width, int height)
{
    guint i;

    for (i = 0; i < head->modes->len; i++)
    {
        Mode *mode = &g_array_index (head->modes, Mode, i);

        if (mode->width == width && mode->height == height)
            return TRUE;
    }

    return FALSE;
}

gboolean
display_heads_have_size (GArray *heads, int width, int height)
{
    guint i;

    for (i = 0; i < heads->len; i++)
        if (!head_has_size (&g_array_index (heads, OutputHead, i), width, height))
            return FALSE;

    return heads->len > 0;
}

gboolean
display_heads_get_clone_size (GArray *heads, int *width, int *height)
{
    OutputHead *first;
    int best_w = 0, best_h = 0;
    guint i;

    if (heads->len < 2)
        return FALSE;

    first = &g_array_index (heads, OutputHead, 0);
    for (i = 0; i < first->modes->len; i++)
    {
        Mode *mode = &g_array_index (first->modes, Mode, i);

        if ((gint64) mode->width * mode->height > (gint64) best_w * best_h &&
            display_heads_have_size (heads, mode->width, mode->height))
        {
            best_w = mode->width;
            best_h = mode->height;
        }
    }

    if (best_w <= 0 || best_h <= 0)
        return FALSE;

    *width = best_w;
    *height = best_h;
    return TRUE;
}

static int
rate_for_size (OutputHead *head, int width, int height)
{
    int highest = 0;
    int preferred = 0;
    guint i;

    for (i = 0; i < head->modes->len; i++)
    {
        Mode *mode = &g_array_index (head->modes, Mode, i);

        if (mode->width != width || mode->height != height)
            continue;

        if (head->enabled && mode->rate == head->rate &&
            head->width == width && head->height == height)
            return mode->rate;

        if (mode->preferred)
            preferred = mode->rate;
        highest = MAX (highest, mode->rate);
    }

    return preferred ? preferred : highest;
}

void
display_heads_mirror (GArray *heads, int width, int height)
{
    double scale = 0.0;
    guint i;

    for (i = 0; i < heads->len && scale <= 0.0; i++)
    {
        OutputHead *head = &g_array_index (heads, OutputHead, i);

        if (head->enabled)
            scale = head->scale;
    }

    if (scale <= 0.0)
        scale = 1.0;

    for (i = 0; i < heads->len; i++)
    {
        OutputHead *head = &g_array_index (heads, OutputHead, i);

        if (!head_has_size (head, width, height))
            continue;

        head->rate = rate_for_size (head, width, height);
        head->enabled = TRUE;
        head->width = width;
        head->height = height;
        head->x = 0;
        head->y = 0;
        head->transform = 0;
        head->scale = scale;
    }
}

void
display_heads_unmirror (GArray *heads)
{
    int x = 0;
    guint i;

    for (i = 0; i < heads->len; i++)
    {
        OutputHead *head = &g_array_index (heads, OutputHead, i);
        int w, h;

        if (!head->enabled)
            continue;

        display_head_get_logical_size (head, &w, &h);
        head->x = x;
        head->y = 0;
        x += w;
    }
}

gboolean
display_heads_are_mirrored (GArray *heads)
{
    GArray *items = collect (heads);
    gboolean mirrored = items->len >= 2;
    guint i;

    for (i = 1; i < items->len && mirrored; i++)
        if (!same (&item_at (items, 0)->rect, &item_at (items, i)->rect))
            mirrored = FALSE;

    g_array_free (items, TRUE);
    return mirrored;
}
