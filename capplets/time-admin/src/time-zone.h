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

#ifndef __TIME_ZONE_H__
#define __TIME_ZONE_H__

#include "time-share.h"

#ifndef __sun
#  define TZ_DATA_FILE "/usr/share/zoneinfo/zone.tab"
#else
#  define TZ_DATA_FILE "/usr/share/lib/zoneinfo/tab/zone_sun.tab"
#endif

typedef struct TzDB
{
    GPtrArray  *locations;
    GHashTable *backward;
}TzDB;

typedef struct TzLocation
{
    gchar *country;
    gdouble latitude;
    gdouble longitude;
    gchar *zone;
    gchar *comment;

    gdouble dist; /* distance to clicked point for comparison */
}TzLocation;

typedef struct TzInfo
{
    gchar *tzname_normal;
    gchar *tzname_daylight;
    glong utc_offset;
    gint daylight;
}TzInfo;
TzDB      *tz_load_db                 (void);

void       SetupTimezoneDialog        (TimeAdmin *ta);

void       RunTimeZoneDialog          (GtkButton *button,
                                       gpointer   data);

void       TimeZoneDateBaseFree       (TzDB      *db);

GPtrArray *tz_get_locations           (TzDB *db);

TzInfo    *tz_info_from_location      (TzLocation *loc);

glong      tz_location_get_utc_offset (TzLocation *loc);

char      *tz_info_get_clean_name     (TzDB       *tz_db,
                                       const char *tz);

void       tz_info_free               (TzInfo     *tzinfo);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TzInfo, tz_info_free)

#endif
