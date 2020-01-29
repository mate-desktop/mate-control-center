/*   time-admin
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

#ifndef __TIME_SHARE_H__
#define __TIME_SHARE_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <locale.h>
#include <libintl.h>
#include <gio/gio.h>

extern int TimeoutFlag;
typedef struct
{
    GtkWidget        *MainWindow;
    GtkWidget        *HourSpin;
    GtkWidget        *MinuteSpin;
    GtkWidget        *SecondSpin;
    GtkWidget        *TimeZoneButton;
    GtkWidget        *TimeZoneEntry;
    GtkWidget        *NtpSyncSwitch;
    GtkWidget        *Calendar;
    GtkWidget        *SaveButton;
    int               UpdateTimeId;
    int               ApplyId;
    gboolean          NtpState;
    GDBusConnection  *Connection;
    GDBusProxy       *proxy;
    GtkWidget        *dialog;
    GtkWidget        *TZconfire;
    GtkWidget        *TZclose;
    GtkWidget        *TimezoneEntry;
    GtkWidget        *SearchBar;
    GtkWidget        *map;
    GtkListStore     *CityListStore;
    GtkTreeModelSort *CityModelSort;
    GtkWidget        *ButtonLock;
    GPermission      *Permission;

}TimeAdmin;

int          ErrorMessage                (const char  *Title,
                                          const char  *Msg);

void         SetTooltip                  (GtkWidget   *box,
                                          gboolean     mode);
#endif
