/*  time-admin
*   Copyright (C) 2018  zhuyaliang https://github.com/zhuyaliang/
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.

*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.

*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef __TIME_TOOL_H__
#define __TIME_TOOL_H__

#include "time-share.h"
struct tm    *GetCurrentTime    (void);
void          Update_Clock_Start(TimeAdmin   *ta);

void          ta_refresh_time   (TimeAdmin   *ta,
                                 struct tm   *LocalTime);

void          ta_refresh_date   (TimeAdmin   *ta,
                                 struct tm   *LocalTime);

void          Update_Clock_Stop (TimeAdmin   *ta);

gboolean      ChangeNtpSync     (GtkSwitch   *widget,
                                 gboolean     state,
                                 gpointer     data);

void          SelectDate        (GtkCalendar *calendar,
                                 gpointer     data);

void          SaveModifyTime    (GtkButton   *button,
                                 gpointer     data);

gboolean      GetNtpState       (TimeAdmin   *ta);

const gchar  *GetTimeZone       (TimeAdmin   *ta);

void          SetTimeZone       (GDBusProxy  *proxy,
                                 const char  *zone);
#endif
