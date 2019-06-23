/*
 * Copyright (C) 2019 MATE Developers
 * Copyright (C) 2018, 2019 zhuyaliang https://github.com/zhuyaliang/
 * Copyright (C) 2010-2018 The GNOME Project
 * Copyright (C) 2010 Intel, Inc
 *
 * Portions from Ubiquity, Copyright (C) 2009 Canonical Ltd.
 * Written by Evan Dandrea <evand@ubuntu.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Author: Thomas Wood <thomas.wood@intel.com>
 *
 */

#ifndef __TIME_MAP_H__
#define __TIME_MAP_H__

#include "time-zone.h"
G_BEGIN_DECLS

#define TYPE_TIMEZONE_MAP     (timezone_map_get_type ())
#define TIMEZONEMAP(object)   (G_TYPE_CHECK_INSTANCE_CAST ((object), TYPE_TIMEZONE_MAP,TimezoneMap))
typedef struct TimezoneMap
{
    GtkWidget parent_instance;
    GdkPixbuf *orig_background;
    GdkPixbuf *orig_background_dim;
    GdkPixbuf *orig_color_map;

    GdkPixbuf *background;
    GdkPixbuf *color_map;
    GdkPixbuf *pin;

    guchar *visible_map_pixels;
    gint visible_map_rowstride;

    gdouble selected_offset;

    TzDB *tzdb;
    TzLocation *location;

    gchar *bubble_text;
}TimezoneMap;


typedef struct TimezoneMapClass
{
    GtkWidgetClass parent_class;

} TimezoneMapClass;

GType         timezone_map_get_type        (void) G_GNUC_CONST;

void          timezone_map_set_bubble_text (TimezoneMap *map,
                                            const gchar *text);

gboolean      timezone_map_set_timezone    (TimezoneMap *map,
                                            const gchar *timezone);

TzLocation   *timezone_map_get_location    (TimezoneMap *map);

TimezoneMap * timezone_map_new             (void);

G_END_DECLS
#endif
