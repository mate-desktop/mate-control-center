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
#define  APPICON                 "mate-times-admin.png"
#define  ICONFILE               DATADIR APPICON

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
                            gpointer      *data)
{
    TimeAdmin *ta = (TimeAdmin *)data;
    if(TimeoutFlag == 0)
    {
        update_apply_timeout(ta);
    }
}
static GdkPixbuf * GetAppIcon(void)
{
    GdkPixbuf *Pixbuf;
    GError    *Error = NULL;

    Pixbuf = gdk_pixbuf_new_from_file(ICONFILE,&Error);
    if(!Pixbuf)
    {
        MessageReport(("Get Icon Fail"),Error->message,ERROR);
        g_error_free(Error);
    }

    return Pixbuf;
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

    gtk_widget_set_sensitive(ta->HourSpin,       is_authorized);
    gtk_widget_set_sensitive(ta->MinuteSpin,     is_authorized);
    gtk_widget_set_sensitive(ta->SecondSpin,     is_authorized);
    gtk_widget_set_sensitive(ta->TimeZoneButton, is_authorized);
    gtk_widget_set_sensitive(ta->Calendar,       is_authorized);
    gtk_widget_set_sensitive(ta->SaveButton,     is_authorized);
    gtk_widget_set_sensitive(ta->NtpSyncSwitch,  is_authorized);
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
    GtkWidget *Window;
    GdkPixbuf *AppIcon;
    GError    *error = NULL;

    Window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    ta->MainWindow = WindowLogin = Window;
    gtk_window_set_deletable(GTK_WINDOW(Window),FALSE);
    gtk_window_set_resizable(GTK_WINDOW(Window),FALSE);
    gtk_window_set_hide_titlebar_when_maximized(GTK_WINDOW(Window),TRUE);
    gtk_window_set_position(GTK_WINDOW(Window), GTK_WIN_POS_CENTER);
    gtk_window_set_title(GTK_WINDOW(Window),_("Time and Date Manager"));
    gtk_container_set_border_width(GTK_CONTAINER(Window),10);
    gtk_widget_set_size_request(Window, 300, 360);
    g_signal_connect(G_OBJECT(Window),
                    "delete-event",
                     G_CALLBACK(on_window_quit),
                     ta);

    AppIcon = GetAppIcon();
    if(AppIcon)
    {
        gtk_window_set_icon(GTK_WINDOW(Window),AppIcon);
        g_object_unref(AppIcon);
    }
	ta->Permission = polkit_permission_new_sync (TIME_ADMIN_PERMISSION,
                                                 NULL,
                                                 NULL,
                                                 &error);
    if (ta->Permission == NULL)
    {
        g_error_free (error);
        return;
    }
    ta->ButtonLock = gtk_lock_button_new(ta->Permission);
    gtk_lock_button_set_permission(GTK_LOCK_BUTTON (ta->ButtonLock),ta->Permission);
    gtk_widget_grab_focus(ta->ButtonLock);
    g_signal_connect(ta->Permission,
                    "notify",
                     G_CALLBACK (on_permission_changed),
                     ta);
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
         MessageReport(_("open file"),_("Create pid file failed"),ERROR);
         return -1;
    }
    chmod(LOCKFILE,0777);
    pid = getpid();
    sprintf(WriteBuf,"%d",pid);
    Length = write(fd,WriteBuf,strlen(WriteBuf));
    if(Length <= 0 )
    {
        MessageReport(_("write file"),_("write pid file failed"),ERROR);
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
             MessageReport(_("open file"),_("open pid file failed"),ERROR);
             return TRUE;
        }
        if(read(fd,ReadBuf,sizeof(ReadBuf)) <= 0)
        {
             MessageReport(_("read file"),_("read pid file failed"),ERROR);
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
static char *translate(const char *value)
{
    g_autofree gchar *zone_translated = NULL;
    char *name;

    zone_translated = g_strdup (_(value));
    name = g_strdup_printf (C_("timezone loc", "%s"),zone_translated);

    return name;
}
static GtkWidget * TimeZoneAndNtp(TimeAdmin *ta)
{
    GtkWidget  *table;
    GtkWidget  *TimeZoneLabel;
    GtkWidget  *NtpSyncLabel;
    const char *TimeZone;
    gboolean    NtpState;
    char       *ZoneName;

    table = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(table),TRUE);

    TimeZoneLabel = gtk_label_new(NULL);
    gtk_widget_set_halign(TimeZoneLabel,GTK_ALIGN_START);
    SetLableFontType(TimeZoneLabel,11,_("Time Zone:"));
    gtk_grid_attach(GTK_GRID(table) ,TimeZoneLabel, 0 , 0 , 1 , 1);

    SetupTimezoneDialog(ta);
    TimeZone = GetTimeZone(ta);
    ZoneName = translate(TimeZone);
    ta->TimeZoneButton = gtk_button_new_with_label(ZoneName);
    g_signal_connect (ta->TimeZoneButton,
                     "clicked",
                      G_CALLBACK (RunTimeZoneDialog),
                      ta);

    gtk_grid_attach(GTK_GRID(table) ,ta->TimeZoneButton,1 , 0 , 3 , 1);

    NtpSyncLabel = gtk_label_new(NULL);
    gtk_widget_set_halign(NtpSyncLabel,GTK_ALIGN_START);
    SetLableFontType(NtpSyncLabel,11,_("Ntp Sync:"));
    gtk_grid_attach(GTK_GRID(table) ,NtpSyncLabel,  0 , 1 , 1 , 1);

    ta->NtpSyncSwitch = gtk_switch_new();
    NtpState = GetNtpState(ta);
    ta->NtpState = NtpState;
    gtk_switch_set_state (GTK_SWITCH(ta->NtpSyncSwitch),
                          NtpState);
    gtk_grid_attach(GTK_GRID(table) ,ta->NtpSyncSwitch, 1 , 1 , 1 , 1);
    g_signal_connect (G_OBJECT(ta->NtpSyncSwitch),
                     "state-set",
                      G_CALLBACK (ChangeNtpSync),
                      ta);

    gtk_grid_set_row_spacing(GTK_GRID(table), 6);
    gtk_grid_set_column_spacing(GTK_GRID(table), 12);

    return table;

}

static GtkWidget *GetSpinButton(int Initial,int Maximum,TimeAdmin *ta)
{
    GtkWidget *SpinButton;
    GtkAdjustment *Adjustment;

    Adjustment = gtk_adjustment_new (Initial, 0, Maximum, 1, 0, 0);
    SpinButton = gtk_spin_button_new (Adjustment, 1, 0);
    gtk_widget_set_sensitive(SpinButton,!ta->NtpState);
    gtk_widget_set_halign(SpinButton,GTK_ALIGN_START);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON (SpinButton), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON (SpinButton),TRUE);
    gtk_widget_set_hexpand (SpinButton,TRUE);
    g_signal_connect (SpinButton,
                     "changed",
                      G_CALLBACK (ChangeTimeValue),
                      ta);

    SetTooltip(SpinButton,!ta->NtpState);
    return SpinButton;
}
static GtkWidget *SetClock(TimeAdmin *ta)
{
    GtkWidget *table;
    GtkWidget *TimeLabel;
    struct tm *LocalTime;

    table = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(table),TRUE);

    TimeLabel = gtk_label_new(NULL);
    SetLableFontType(TimeLabel,13,_("Set Time"));
    gtk_widget_set_halign(TimeLabel,GTK_ALIGN_CENTER);
    gtk_widget_set_valign(TimeLabel,GTK_ALIGN_START);
    gtk_widget_set_hexpand(TimeLabel,FALSE);
    gtk_grid_attach(GTK_GRID(table) ,TimeLabel, 1 , 0 , 1 , 1);

    LocalTime = GetCurrentTime();
    ta->UpdateTimeId = 0;
    ta->ApplyId      = 0;

    ta->HourSpin = GetSpinButton(LocalTime->tm_hour,23,ta);
    gtk_grid_attach(GTK_GRID(table) ,ta->HourSpin, 0 , 1 , 1 , 1);

    ta->MinuteSpin = GetSpinButton(LocalTime->tm_min,59,ta);
    gtk_grid_attach(GTK_GRID(table) ,ta->MinuteSpin, 1 , 1 , 1 , 1);

    ta->SecondSpin = GetSpinButton (LocalTime->tm_sec,59,ta);
    gtk_grid_attach(GTK_GRID(table) ,ta->SecondSpin, 2 , 1 , 1 , 1);

    Update_Clock_Start(ta);

    gtk_grid_set_row_spacing(GTK_GRID(table), 6);
    gtk_grid_set_column_spacing(GTK_GRID(table), 12);

    return table;
}

static GtkWidget *SetDate(TimeAdmin *ta)
{
    GtkWidget *table;
    GtkWidget *DateLabel;
    struct tm *LocalTime;

    table = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(table),TRUE);

    DateLabel = gtk_label_new(NULL);
    SetLableFontType(DateLabel,13,_("Set Date"));
    gtk_grid_attach(GTK_GRID(table) ,DateLabel, 1 , 0 , 2 , 2);

    LocalTime = GetCurrentTime();
    ta->Calendar = gtk_calendar_new ();
    gtk_widget_set_sensitive(ta->Calendar,!ta->NtpState);
    SetTooltip(ta->Calendar,!ta->NtpState);
    gtk_calendar_mark_day(GTK_CALENDAR(ta->Calendar),LocalTime->tm_mday);
    ta->OldDay = LocalTime->tm_mday;
    gtk_grid_attach(GTK_GRID(table) ,ta->Calendar, 0 , 2 , 4 , 3);

    ta->CloseButton = gtk_button_new_with_label (_("Close"));
    gtk_grid_attach(GTK_GRID(table) ,ta->CloseButton, 3 , 5 , 1 , 1);
    g_signal_connect (ta->CloseButton,
                     "clicked",
                      G_CALLBACK (CloseWindow),
                      ta);

	gtk_grid_attach(GTK_GRID(table) ,ta->ButtonLock, 0 , 5 , 1 , 1);

	ta->SaveButton  = gtk_button_new_with_label (_("Save"));
    gtk_widget_set_sensitive(ta->SaveButton,!ta->NtpState);
    gtk_grid_attach(GTK_GRID(table) ,ta->SaveButton, 2 , 5 , 1 , 1);
    g_signal_connect (ta->SaveButton,
                     "clicked",
                      G_CALLBACK (SaveModifyTime),
                      ta);

    gtk_grid_set_row_spacing(GTK_GRID(table), 6);
    gtk_grid_set_column_spacing(GTK_GRID(table), 12);

    return table;

}
static void CreateClockInterface(TimeAdmin *ta)
{
    GtkWidget *Vbox;
    GtkWidget *Vbox1;
    GtkWidget *Vbox2;
    GtkWidget *Vbox3;

    Vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_add(GTK_CONTAINER(ta->MainWindow), Vbox);

    Vbox1 = TimeZoneAndNtp(ta);
    gtk_box_pack_start(GTK_BOX(Vbox),Vbox1,TRUE,TRUE,8);

    Vbox2 = SetClock(ta);
    gtk_box_pack_start(GTK_BOX(Vbox),Vbox2,TRUE,TRUE,8);
    Vbox3 = SetDate(ta);
    gtk_box_pack_start(GTK_BOX(Vbox),Vbox3,TRUE,TRUE,8);
}
static gboolean InitDbusProxy(TimeAdmin *ta)
{
    GError *error = NULL;

    ta->Connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
    if(ta->Connection == NULL)
    {
        MessageReport(_("g_bus_get_sync"),error->message,ERROR);
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
        MessageReport(_("g_bus_proxy_new"),error->message,ERROR);
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

    /* Create the main window */
    InitMainWindow(&ta);

    /* Check whether the process has been started */
    if(ProcessRuning() == TRUE)
        exit(0);
    if(InitDbusProxy(&ta) == FALSE)
    {
        exit(0);
    }
    CreateClockInterface(&ta);
	UpdatePermission(&ta);
    gtk_widget_show_all(ta.MainWindow);
    gtk_main();

    return TRUE;
}
