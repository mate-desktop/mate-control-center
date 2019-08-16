/*   mate-user-admin
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
#include <glib/gi18n.h>
#include <polkit/polkit.h>

#include "capplet-util.h"
#include "time-tool.h"
#include "time-zone.h"
#include "time-map.h"

#define  LOCKFILE               "/tmp/time-admin.pid"
#define  TIME_ADMIN_PERMISSION  "org.freedesktop.timedate1.set-time"

static char *translate(const char *value)
{
    g_autofree gchar *zone_translated = NULL;
    char *name;

    zone_translated = g_strdup (_(value));
    name = g_strdup_printf (C_("timezone loc", "%s"),zone_translated);

    return name;
}

static void
QuitApp (TimeAdmin *ta)
{
    if (ta->UpdateTimeId > 0)
        g_source_remove (ta->UpdateTimeId);

    if (ta->ApplyId > 0)
        g_source_remove (ta->ApplyId);

    gtk_main_quit ();
}

static gboolean CheckClockHealth(gpointer data)
{
    TimeAdmin *ta = (TimeAdmin *)data;
    Update_Clock_Start(ta);
    ta->ApplyId = 0;

    return FALSE;
}

static void update_apply_timeout(TimeAdmin *ta)
{
    Update_Clock_Stop(ta);
    if (ta->ApplyId > 0)
    {
         g_source_remove (ta->ApplyId);
         ta->ApplyId = 0;
    }
    ta->ApplyId = g_timeout_add (10000, (GSourceFunc)CheckClockHealth,ta);
}

static void ChangeTimeValue(GtkSpinButton *spin_button,
                            gpointer       data)
{
    TimeAdmin *ta = (TimeAdmin *)data;
    if(TimeoutFlag == 0)
    {
        update_apply_timeout(ta);
    }
}

static gboolean on_window_quit (GtkWidget *widget,
                                GdkEvent  *event,
                                gpointer   user_data)
{
    TimeAdmin *ta = (TimeAdmin *)user_data;

    QuitApp(ta);
    return TRUE;
}

static void CloseWindow (GtkButton *button,gpointer data)
{
    TimeAdmin *ta = (TimeAdmin *)data;

    QuitApp(ta);
}

static void UpdatePermission(TimeAdmin *ta)
{
    gboolean is_authorized;

    is_authorized = g_permission_get_allowed (G_PERMISSION (ta->Permission));
    gtk_widget_set_sensitive(ta->TimeZoneButton, is_authorized);
    gtk_widget_set_sensitive(ta->NtpSyncSwitch,  is_authorized);
    gtk_widget_set_sensitive(ta->SaveButton,     is_authorized && !ta->NtpState);
}

static void on_permission_changed (GPermission *permission,
                                   GParamSpec  *pspec,
                                   gpointer     data)
{
    TimeAdmin *ua = (TimeAdmin *)data;
    UpdatePermission(ua);
}

static void InitMainWindow(TimeAdmin *ta)
{
    GError     *error = NULL;
    GtkBuilder *builder;

    builder = gtk_builder_new_from_resource ("/org/mate/mcc/ta/time-admin.ui");
    gtk_builder_add_callback_symbols (builder,
                                      "on_window_quit",       G_CALLBACK (on_window_quit),
                                      "on_button1_clicked",   G_CALLBACK (RunTimeZoneDialog),
                                      "on_button2_clicked",   G_CALLBACK (SaveModifyTime),
                                      "on_button3_clicked",   G_CALLBACK (CloseWindow),
                                      "on_spin1_changed",     G_CALLBACK (ChangeTimeValue),
                                      "on_spin2_changed",     G_CALLBACK (ChangeTimeValue),
                                      "on_spin3_changed",     G_CALLBACK (ChangeTimeValue),
                                      "on_switch1_state_set", G_CALLBACK (ChangeNtpSync),
                                      NULL);
    gtk_builder_connect_signals (builder, ta);
    ta->MainWindow = GTK_WIDGET (gtk_builder_get_object (builder, "window1"));
    ta->HourSpin = GTK_WIDGET (gtk_builder_get_object (builder, "spin1"));
    ta->MinuteSpin = GTK_WIDGET (gtk_builder_get_object (builder, "spin2"));
    ta->SecondSpin = GTK_WIDGET (gtk_builder_get_object (builder, "spin3"));
    ta->TimeZoneButton = GTK_WIDGET (gtk_builder_get_object (builder, "button1"));
    ta->TimeZoneEntry = GTK_WIDGET (gtk_builder_get_object (builder, "entry1"));
    ta->NtpSyncSwitch = GTK_WIDGET (gtk_builder_get_object (builder, "switch1"));
    ta->Calendar = GTK_WIDGET (gtk_builder_get_object (builder, "calendar1"));
    ta->SaveButton = GTK_WIDGET (gtk_builder_get_object (builder, "button2"));
    ta->ButtonLock = GTK_WIDGET (gtk_builder_get_object (builder, "button4"));
    g_object_unref (builder);

    /* Make sure that every window gets an icon */
    gtk_window_set_default_icon_name ("preferences-system-time");

    ta->Permission = polkit_permission_new_sync (TIME_ADMIN_PERMISSION, NULL, NULL, &error);
    if (ta->Permission == NULL)
    {
        g_warning ("Failed to acquire %s: %s", TIME_ADMIN_PERMISSION, error->message);
        g_error_free (error);
    }
    gtk_lock_button_set_permission(GTK_LOCK_BUTTON (ta->ButtonLock),ta->Permission);
    g_signal_connect(ta->Permission, "notify", G_CALLBACK (on_permission_changed), ta);

    /* NTP sync switch */
    ta->NtpState = GetNtpState(ta);
    gtk_switch_set_state (GTK_SWITCH(ta->NtpSyncSwitch), ta->NtpState);

    /* Time zone */
    SetupTimezoneDialog(ta);
    const char *TimeZone = GetTimeZone(ta);
    char       *ZoneName = translate(TimeZone);
    gtk_entry_set_text (GTK_ENTRY (ta->TimeZoneEntry), ZoneName);
    g_free (ZoneName);

    /* Local time & date */
    /* time */
    struct tm *LocalTime = GetCurrentTime();
    ta->UpdateTimeId     = 0;
    ta->ApplyId          = 0;
    ta_refresh_time (ta, LocalTime);
    /* date */
    ta_refresh_date (ta, LocalTime);

    Update_Clock_Start(ta);
}

static int RecordPid(void)
{
    int pid = 0;
    int fd;
    int Length = 0;
    char WriteBuf[30] = { 0 };

    fd = open(LOCKFILE,O_WRONLY|O_CREAT|O_TRUNC,0777);
    if(fd < 0)
    {
         ErrorMessage (_("open file"), _("Create pid file failed"));
         return -1;
    }
    chmod(LOCKFILE,0777);
    pid = getpid();
    sprintf(WriteBuf,"%d",pid);
    Length = write(fd,WriteBuf,strlen(WriteBuf));
    if(Length <= 0 )
    {
        ErrorMessage (_("write file"), _("write pid file failed"));
        return -1;
    }
    close(fd);

    return 0;
}

/******************************************************************************
* Function:              ProcessRuning
*
* Explain: Check whether the process has been started,If the process is not started,
*          record the current process ID =====>"/tmp/user-admin.pid"
*
* Input:
*
*
* Output:  start        :TRUE
*          not start    :FALSE
*
* Author:  zhuyaliang  31/07/2018
******************************************************************************/
static gboolean ProcessRuning(void)
{
    int fd;
    int pid = 0;
    gboolean Run = FALSE;
    char ReadBuf[30] = { 0 };

    if(access(LOCKFILE,F_OK) == 0)
    {
        fd = open(LOCKFILE,O_RDONLY);
        if(fd < 0)
        {
             ErrorMessage (_("open file"), _("open pid file failed"));
             return TRUE;
        }
        if(read(fd,ReadBuf,sizeof(ReadBuf)) <= 0)
        {
             ErrorMessage (_("read file"), _("read pid file failed"));
             goto ERROREXIT;
        }
        pid = atoi(ReadBuf);
        if(kill(pid,0) == 0)
        {
             goto ERROREXIT;
        }
    }

    if(RecordPid() < 0)
        Run = TRUE;

    return Run;
ERROREXIT:
    close(fd);
    return TRUE;
}

static gboolean InitDbusProxy(TimeAdmin *ta)
{
    GError *error = NULL;

    ta->Connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
    if(ta->Connection == NULL)
    {
        ErrorMessage (_("g_bus_get_sync"), error->message);
        goto EXIT;
    }
    ta->proxy = g_dbus_proxy_new_sync (ta->Connection,
                                       G_DBUS_PROXY_FLAGS_NONE,
                                       NULL,
                                      "org.freedesktop.timedate1",
                                      "/org/freedesktop/timedate1",
                                      "org.freedesktop.timedate1",
                                       NULL,
                                      &error);
    if(ta->proxy == NULL)
    {
        ErrorMessage (_("g_bus_proxy_new"), error->message);
        goto EXIT;
    }

    return TRUE;
EXIT:
    g_error_free(error);
    return FALSE;
}

int main(int argc, char **argv)
{
    TimeAdmin ta;

    capplet_init (NULL, &argc, &argv);

    /* Check whether the process has been started */
    if(ProcessRuning() == TRUE)
        exit(0);
    if(InitDbusProxy(&ta) == FALSE)
        exit(0);

    /* Create the main window */
    InitMainWindow(&ta);

    UpdatePermission(&ta);
    gtk_widget_show_all(ta.MainWindow);
    gtk_main();

    return TRUE;
}
