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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "time-tool.h"
#include <glib/gi18n.h>

struct tm *GetCurrentTime(void)
{
    time_t tt;
    tzset();
    tt=time(NULL);

    return localtime(&tt);
}

void
ta_refresh_time (TimeAdmin *ta, struct tm *LocalTime)
{
    gchar *str;

    gtk_spin_button_set_value (GTK_SPIN_BUTTON (ta->HourSpin), LocalTime->tm_hour);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (ta->MinuteSpin), LocalTime->tm_min);
    str = g_strdup_printf ("%02d", LocalTime->tm_sec);
    gtk_entry_set_text (GTK_ENTRY (ta->SecondSpin), str);
    g_free (str);
}

void
ta_refresh_date (TimeAdmin *ta, struct tm *LocalTime)
{
    gtk_calendar_select_month (GTK_CALENDAR (ta->Calendar),
                               LocalTime->tm_mon,
                               LocalTime->tm_year+1900);
    gtk_calendar_select_day   (GTK_CALENDAR (ta->Calendar),
                               LocalTime->tm_mday);
    gtk_calendar_clear_marks  (GTK_CALENDAR (ta->Calendar));
    gtk_calendar_mark_day     (GTK_CALENDAR (ta->Calendar),
                               LocalTime->tm_mday);
}

static void
UpdateDate (TimeAdmin *ta)
{
    if (ta->NtpState)
    {
        struct tm *LocalTime = GetCurrentTime();
        ta_refresh_time (ta, LocalTime);
        ta_refresh_date (ta, LocalTime);
    }
}

static gboolean
UpdateClock (gpointer data)
{
    TimeAdmin *ta = (TimeAdmin *)data;

    TimeoutFlag = 1;
    UpdateDate (ta);
    TimeoutFlag = 0;

    return TRUE;
}

void Update_Clock_Start(TimeAdmin *ta)
{
    if(ta->UpdateTimeId <= 0)
    {
        ta->UpdateTimeId = g_timeout_add(1000,(GSourceFunc)UpdateClock,ta);
    }
}

void Update_Clock_Stop(TimeAdmin *ta)
{
    if(ta->UpdateTimeId > 0)
    {
        g_source_remove(ta->UpdateTimeId);
        ta->UpdateTimeId = 0;
    }
}

gboolean GetNtpState(TimeAdmin *ta)
{
    GDBusProxy *proxy = NULL;
    GError     *error = NULL;
    GVariant   *ret;
    GVariant   *ntp;

    proxy = g_dbus_proxy_new_sync (ta->Connection,
                                   G_DBUS_PROXY_FLAGS_NONE,
                                   NULL,
                                  "org.freedesktop.timedate1",
                                  "/org/freedesktop/timedate1",
                                  "org.freedesktop.DBus.Properties",
                                   NULL,
                                   &error);
    if(proxy == NULL)
    {
        goto EXIT;
    }

    ret = g_dbus_proxy_call_sync (proxy,
                                 "Get",
                                  g_variant_new ("(ss)",
                                 "org.freedesktop.timedate1",
                                 "NTP"),
                                  G_DBUS_CALL_FLAGS_NONE,
                                 -1,
                                  NULL,
                                  &error);
    if(ret == NULL)
    {
        goto EXIT;
    }
    g_variant_get (ret, "(v)", &ntp);
    return g_variant_get_boolean (ntp);

EXIT:
    ErrorMessage (_("GetNtpState"), error->message);
    g_error_free(error);
    return FALSE;

}

const gchar *GetTimeZone(TimeAdmin *ta)
{
    GDBusProxy *proxy = NULL;
    GError     *error = NULL;
    GVariant   *ret;
    GVariant   *timezone;

    proxy = g_dbus_proxy_new_sync (ta->Connection,
                                   G_DBUS_PROXY_FLAGS_NONE,
                                   NULL,
                                  "org.freedesktop.timedate1",
                                  "/org/freedesktop/timedate1",
                                  "org.freedesktop.DBus.Properties",
                                   NULL,
                                   &error);
    if(proxy == NULL)
    {
        goto EXIT;
    }

    ret = g_dbus_proxy_call_sync (proxy,
                                 "Get",
                                  g_variant_new ("(ss)",
                                 "org.freedesktop.timedate1",
                                 "Timezone"),
                                  G_DBUS_CALL_FLAGS_NONE,
                                 -1,
                                  NULL,
                                  &error);
    if(ret == NULL)
    {
        goto EXIT;
    }
    g_variant_get (ret, "(v)", &timezone);
    return g_variant_get_string (timezone,0);

EXIT:
    g_error_free(error);
    return NULL;

}

void SetTimeZone(GDBusProxy *proxy,const char *zone)
{
    GError *error = NULL;
    GVariant *ret;

    ret = g_dbus_proxy_call_sync (proxy,
                                 "SetTimezone",
                                  g_variant_new ("(sb)",zone,1),
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &error);

    if(ret == NULL)
    {
        ErrorMessage (_("Set time zone"), error->message);
        g_error_free (error);
    }
}

static void
ChangeSpinBttonState (TimeAdmin *ta)
{
    gtk_widget_set_sensitive (ta->SaveButton, !ta->NtpState);
    SetTooltip (ta->SaveButton, !ta->NtpState);
    SetTooltip (ta->HourSpin,   !ta->NtpState);
    SetTooltip (ta->MinuteSpin, !ta->NtpState);
    SetTooltip (ta->SecondSpin, !ta->NtpState);
    SetTooltip (ta->Calendar,   !ta->NtpState);
}

gboolean ChangeNtpSync (GtkSwitch *widget, gboolean state, gpointer data)
{
    TimeAdmin *ta = (TimeAdmin *)data;
    GError *error = NULL;
    GVariant *ret;

    ret = g_dbus_proxy_call_sync (ta->proxy,
                                 "SetNTP",
                                  g_variant_new ("(bb)",state,state),
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &error);

    if (ret == NULL)
    {
        ErrorMessage (_("Set Ntp sync"), error->message);
        g_error_free(error);
        return TRUE;
    }
    else
    {
        ta->NtpState = state;
        ChangeSpinBttonState (ta);
        Update_Clock_Start (ta);
        UpdateDate (ta);
    }
    return FALSE;
}

static guint GetTimeStamp(TimeAdmin *ta)
{
    guint year,month,day,hour,min,sec;
    GDateTime *dt;
    char      *st;

    gtk_calendar_get_date(GTK_CALENDAR(ta->Calendar),&year,&month,&day);
    hour = gtk_spin_button_get_value(GTK_SPIN_BUTTON(ta->HourSpin));
    min  = gtk_spin_button_get_value(GTK_SPIN_BUTTON(ta->MinuteSpin));
    sec = atoi(gtk_entry_get_text(GTK_ENTRY(ta->SecondSpin)));

    dt = g_date_time_new_local(year,month+1,day,hour,min,sec);
    st = g_date_time_format(dt,"%s");
    return atoi(st);

}

static gboolean
SetTime (GDBusProxy *proxy, gint64 TimeSec)
{
    GError *error = NULL;
    GVariant *ret;

    ret = g_dbus_proxy_call_sync (proxy,
                                 "SetTime",
                                  g_variant_new ("(xbb)",TimeSec * 1000 * 1000,0,0),
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &error);

    if(ret == NULL)
    {
        ErrorMessage (_("Set Ntp sync"), error->message);
        g_error_free(error);
        return FALSE;
    }

    return TRUE;
}

void SaveModifyTime (GtkButton *button, gpointer data)
{
    TimeAdmin *ta = (TimeAdmin *)data;

    if (SetTime (ta->proxy, GetTimeStamp(ta)))
    {
        struct tm *LocalTime = GetCurrentTime();
        ta_refresh_time (ta, LocalTime);
        ta_refresh_date (ta, LocalTime);
    }
}
