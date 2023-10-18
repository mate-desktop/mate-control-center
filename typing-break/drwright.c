/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003-2005 Imendio HB
 * Copyright (C) 2002-2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 CodeFactory AB
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <string.h>
#include <math.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

#ifdef HAVE_UBUNTU_APPINDICATOR
#include <libappindicator/app-indicator.h>
#else
#include <libayatana-appindicator/app-indicator.h>
#endif

#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmate-desktop/mate-desktop-utils.h>

#include "drwright.h"
#include "drw-break-window.h"
#include "drw-monitor.h"
#include "drw-utils.h"
#include "drw-timer.h"

typedef enum {
	STATE_START,
	STATE_RUNNING,
	STATE_WARN,
	STATE_BREAK_SETUP,
	STATE_BREAK,
	STATE_BREAK_DONE_SETUP,
	STATE_BREAK_DONE
} DrwState;

#define TYPING_MONITOR_ACTIVE_ICON "bar-green"
#define TYPING_MONITOR_ATTENTION_ICON "bar-red"

struct _DrWright {
	/* Widgets. */
	GtkWidget       *break_window;
	GList           *secondary_break_windows;

	DrwMonitor      *monitor;

	GtkWidget       *menu;
	GtkWidget       *break_item;

	DrwState         state;
	DrwTimer        *timer;
	DrwTimer        *idle_timer;

	gint             last_elapsed_time;
	gint             save_last_time;

	/* Time settings. */
	gint             type_time;
	gint             break_time;
	gint             warn_time;

	gboolean         enabled;

	guint            clock_timeout_id;
	AppIndicator    *indicator;
	GtkWidget      *warn_dialog;
};

static void     activity_detected_cb           (DrwMonitor     *monitor,
						DrWright       *drwright);
static gboolean maybe_change_state             (DrWright       *drwright);
static gint     get_time_left                  (DrWright       *drwright);
static gboolean update_status                  (DrWright       *drwright);
static void     break_window_done_cb           (GtkWidget      *window,
						DrWright       *dr);
static void     break_window_postpone_cb       (GtkWidget      *window,
						DrWright       *dr);
static void     break_window_destroy_cb        (GtkWidget      *window,
						DrWright       *dr);
static void     popup_break_cb                 (GSimpleAction  *action,
                                                GVariant       *parameter,
                                                gpointer        data);

static void     popup_preferences_cb           (GSimpleAction  *action,
                                                GVariant       *parameter,
                                                gpointer        data);

static void     popup_about_cb                 (GSimpleAction  *action,
                                                GVariant       *parameter,
                                                gpointer        data);
static void     init_app_indicator             (DrWright       *dr);
static GList *  create_secondary_break_windows (void);

static const GActionEntry action_entries[] = {
  {"Preferences", popup_preferences_cb, NULL, NULL, NULL, { 0 } },
  {"About", popup_about_cb, NULL, NULL, NULL, { 0 } },
  {"TakeABreak", popup_break_cb, NULL, NULL, NULL, { 0 } }
};

extern gboolean debug;

static void
setup_debug_values (DrWright *dr)
{
	dr->type_time = 5;
	dr->warn_time = 4;
	dr->break_time = 10;
}

static void
update_app_indicator (DrWright *dr)
{
	AppIndicatorStatus new_status;

	if (!dr->enabled) {
		app_indicator_set_status (dr->indicator,
					  APP_INDICATOR_STATUS_PASSIVE);
		return;
	}

	switch (dr->state) {
	case STATE_WARN:
	case STATE_BREAK_SETUP:
	case STATE_BREAK:
		new_status = APP_INDICATOR_STATUS_ATTENTION;
		break;
	default:
		new_status = APP_INDICATOR_STATUS_ACTIVE;
	}

	app_indicator_set_status (dr->indicator, new_status);
}

static gboolean
grab_keyboard_on_window (GdkWindow *window,
			 guint32    activate_time)
{
	GdkDisplay *display;
	GdkSeat *seat;
	GdkGrabStatus status;

	display = gdk_window_get_display (window);
	seat = gdk_display_get_default_seat (display);

	status = gdk_seat_grab (seat,
	                        window,
	                        GDK_SEAT_CAPABILITY_KEYBOARD,
	                        TRUE,
	                        NULL,
	                        NULL,
	                        NULL,
	                        NULL);

	if (status == GDK_GRAB_SUCCESS) {
		return TRUE;
	}

	return FALSE;
}

static gboolean
break_window_map_event_cb (GtkWidget *widget,
			   GdkEvent  *event,
			   DrWright  *dr)
{
	grab_keyboard_on_window (gtk_widget_get_window (dr->break_window), gtk_get_current_event_time ());

        return FALSE;
}

static gboolean
maybe_change_state (DrWright *dr)
{
	gint elapsed_time;
	gint elapsed_idle_time;

	if (debug) {
		drw_timer_start (dr->idle_timer);
	}

	elapsed_time = drw_timer_elapsed (dr->timer) + dr->save_last_time;
	elapsed_idle_time = drw_timer_elapsed (dr->idle_timer);

	if (elapsed_time > dr->last_elapsed_time + dr->warn_time) {
		/* If the timeout is delayed by the amount of warning time, then
		 * we must have been suspended or stopped, so we just start
		 * over.
		 */
		dr->state = STATE_START;
	}

	switch (dr->state) {
	case STATE_START:
		if (dr->break_window) {
			gtk_widget_destroy (dr->break_window);
			dr->break_window = NULL;
		}

		dr->save_last_time = 0;

		drw_timer_start (dr->timer);
		drw_timer_start (dr->idle_timer);

		if (dr->enabled) {
			dr->state = STATE_RUNNING;
		}

		update_status (dr);
		break;

	case STATE_RUNNING:
	case STATE_WARN:
		if (elapsed_idle_time >= dr->break_time) {
			dr->state = STATE_BREAK_DONE_SETUP;
 		} else if (elapsed_time >= dr->type_time) {
			dr->state = STATE_BREAK_SETUP;
		} else if (dr->state != STATE_WARN
			   && elapsed_time >= dr->type_time - dr->warn_time) {
			dr->state = STATE_WARN;
		}
		break;

	case STATE_BREAK_SETUP:
		/* Don't allow more than one break window to coexist, can happen
		 * if a break is manually enforced.
		 */
		if (dr->break_window) {
			dr->state = STATE_BREAK;
			break;
		}

		drw_timer_start (dr->timer);

		dr->break_window = drw_break_window_new ();

		g_signal_connect (dr->break_window, "map_event",
				  G_CALLBACK (break_window_map_event_cb),
				  dr);

		g_signal_connect (dr->break_window,
				  "done",
				  G_CALLBACK (break_window_done_cb),
				  dr);

		g_signal_connect (dr->break_window,
				  "postpone",
				  G_CALLBACK (break_window_postpone_cb),
				  dr);

		g_signal_connect (dr->break_window,
				  "destroy",
				  G_CALLBACK (break_window_destroy_cb),
				  dr);

		dr->secondary_break_windows = create_secondary_break_windows ();

		gtk_widget_show (dr->break_window);

		dr->save_last_time = elapsed_time;
		dr->state = STATE_BREAK;
		break;

	case STATE_BREAK:
		if (elapsed_time - dr->save_last_time >= dr->break_time) {
			dr->state = STATE_BREAK_DONE_SETUP;
		}
		break;

	case STATE_BREAK_DONE_SETUP:

		dr->state = STATE_BREAK_DONE;
		break;

	case STATE_BREAK_DONE:
		dr->state = STATE_START;
		if (dr->break_window) {
			gtk_widget_destroy (dr->break_window);
			dr->break_window = NULL;
		}
		break;
	}

	dr->last_elapsed_time = elapsed_time;

	update_app_indicator (dr);

	return TRUE;
}

static gboolean
update_status (DrWright *dr)
{
	gint       min;
	gchar     *str;

	if (!dr->enabled) {
		app_indicator_set_status (dr->indicator,
					  APP_INDICATOR_STATUS_PASSIVE);
		return TRUE;
	}

	min = get_time_left (dr);

	if (min >= 1) {
		str = g_strdup_printf (_("Take a break now (next in %dm)"), min);
	} else {
		str = g_strdup_printf (_("Take a break now (next in less than one minute)"));
	}

	gtk_menu_item_set_label (GTK_MENU_ITEM (dr->break_item), str);

	g_free (str);

	return TRUE;
}

static gint
get_time_left (DrWright *dr)
{
	gint elapsed_time;

	elapsed_time = drw_timer_elapsed (dr->timer);

	return floor (0.5 + (dr->type_time - elapsed_time - dr->save_last_time) / 60.0);
}

static void
activity_detected_cb (DrwMonitor *monitor,
		      DrWright   *dr)
{
	drw_timer_start (dr->idle_timer);
}

static void
gsettings_notify_cb (GSettings *settings,
		 gchar       *key,
		 gpointer     user_data)
{
	DrWright  *dr = user_data;

	if (!strcmp (key, "type-time")) {
		dr->type_time = 60 * g_settings_get_int (settings, key);
		dr->warn_time = MIN (dr->type_time / 10, 5*60);

		dr->state = STATE_START;
	}
	else if (!strcmp (key, "break-time")) {
		dr->break_time = 60 * g_settings_get_int (settings, key);
		dr->state = STATE_START;
	}
	else if (!strcmp (key, "enabled")) {
		dr->enabled = g_settings_get_boolean (settings, key);
		dr->state = STATE_START;

		gtk_widget_set_sensitive (dr->break_item, dr->enabled);

		update_status (dr);
	}

	maybe_change_state (dr);
}

static void
popup_break_cb (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       data)
{
	DrWright *dr = data;
	if (dr->enabled) {
		dr->state = STATE_BREAK_SETUP;
		maybe_change_state (dr);
	}
}

static void
popup_preferences_cb (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       data)
{
	DrWright *dr = data;
	GdkScreen *screen;
	GError    *error = NULL;

	screen = gtk_widget_get_screen (dr->menu);

	if (!mate_gdk_spawn_command_line_on_screen (screen, "mate-keyboard-properties --typing-break", &error)) {
		GtkWidget *error_dialog;

		error_dialog = gtk_message_dialog_new (NULL, 0,
						       GTK_MESSAGE_ERROR,
						       GTK_BUTTONS_CLOSE,
						       _("Unable to bring up the typing break properties dialog with the following error: %s"),
						       error->message);
		g_signal_connect (error_dialog,
				  "response",
				  G_CALLBACK (gtk_widget_destroy), NULL);
		gtk_window_set_resizable (GTK_WINDOW (error_dialog), FALSE);
		gtk_widget_show (error_dialog);

		g_error_free (error);
	}
}

static void
popup_about_cb (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       data)
{
	gint   i;
	gchar *authors[] = {
		N_("Written by Richard Hult <richard@imendio.com>"),
		N_("Eye candy added by Anders Carlsson"),
		NULL
	};

	for (i = 0; authors [i]; i++)
		authors [i] = _(authors [i]);

	gtk_show_about_dialog (NULL,
			       "authors", authors,
			       "comments",  _("A computer break reminder."),
			       "logo-icon-name", "mate-typing-monitor",
			       "translator-credits", _("translator-credits"),
			       "version", VERSION,
			       NULL);
}

static void
break_window_done_cb (GtkWidget *window,
		      DrWright  *dr)
{
	gtk_widget_destroy (dr->break_window);

	dr->state = STATE_BREAK_DONE_SETUP;
	dr->break_window = NULL;

	update_status (dr);
	maybe_change_state (dr);
}

static void
break_window_postpone_cb (GtkWidget *window,
			  DrWright  *dr)
{
	gint elapsed_time;

	gtk_widget_destroy (dr->break_window);

	dr->state = STATE_RUNNING;
	dr->break_window = NULL;

	elapsed_time = drw_timer_elapsed (dr->timer);

	if (elapsed_time + dr->save_last_time >= dr->type_time) {
		/* Typing time has expired, but break was postponed.
		 * We'll warn again in (elapsed * sqrt (typing_time))^2 */
		gfloat postpone_time = (((float) elapsed_time) / dr->break_time)
					* sqrt (dr->type_time);
		postpone_time *= postpone_time;
		dr->save_last_time = dr->type_time - MAX (dr->warn_time, (gint) postpone_time);
	}

	drw_timer_start (dr->timer);
	maybe_change_state (dr);
	update_status (dr);
	update_app_indicator (dr);
}

static void
break_window_destroy_cb (GtkWidget *window,
			 DrWright  *dr)
{
	GList *l;

	for (l = dr->secondary_break_windows; l; l = l->next) {
		gtk_widget_destroy (l->data);
	}

	g_list_free (dr->secondary_break_windows);
	dr->secondary_break_windows = NULL;
}

static void
init_app_indicator (DrWright *dr)
{
	dr->indicator =
		app_indicator_new_with_path ("typing-break-indicator",
					     TYPING_MONITOR_ACTIVE_ICON,
					     APP_INDICATOR_CATEGORY_APPLICATION_STATUS,
					     IMAGEDIR);
	if (dr->enabled) {
		app_indicator_set_status (dr->indicator,
					  APP_INDICATOR_STATUS_ACTIVE);
	} else {
		app_indicator_set_status (dr->indicator,
					  APP_INDICATOR_STATUS_PASSIVE);
	}

	app_indicator_set_menu (dr->indicator, GTK_MENU (dr->menu));
	app_indicator_set_attention_icon (dr->indicator, TYPING_MONITOR_ATTENTION_ICON);

	update_status (dr);
	update_app_indicator (dr);
}

static GList *
create_secondary_break_windows (void)
{
	GdkDisplay *display;
	GdkScreen  *screen;
	GtkWidget  *window;
	GList      *windows = NULL;
	gint        scale;

	display = gdk_display_get_default ();

	screen = gdk_display_get_default_screen (display);

	if (screen != gdk_screen_get_default ()) {
		/* Handled by DrwBreakWindow. */

		window = gtk_window_new (GTK_WINDOW_POPUP);

		windows = g_list_prepend (windows, window);
		scale = gtk_widget_get_scale_factor (GTK_WIDGET (window));

		gtk_window_set_screen (GTK_WINDOW (window), screen);

		gtk_window_set_default_size (GTK_WINDOW (window),
					     WidthOfScreen (gdk_x11_screen_get_xscreen (screen)) / scale,
					     HeightOfScreen (gdk_x11_screen_get_xscreen (screen)) / scale);

		gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);
		drw_setup_background (GTK_WIDGET (window));
		gtk_window_stick (GTK_WINDOW (window));
		gtk_widget_show (window);
	}

	return windows;
}

DrWright *
drwright_new (void)
{
	DrWright           *dr;
	GtkBuilder         *ui_builder;
	GSettings          *settings;
	GSimpleActionGroup *action_group;

	static const gchar *ui_description =
	  "<interface>"
	    "<object class=\"GtkImage\" id=\"menu_icon_pre\">"
	      "<property name=\"visible\">True</property>"
	      "<property name=\"can_focus\">False</property>"
	      "<property name=\"icon-name\">preferences-desktop</property>"
	    "</object>"
	    "<object class=\"GtkImage\" id=\"menu_icon_about\">"
	      "<property name=\"visible\">True</property>"
	      "<property name=\"can_focus\">False</property>"
	      "<property name=\"icon-name\">help-about</property>"
	    "</object>"
	    "<object class=\"GtkMenu\" id=\"pop_menu\">"
	        "<property name=\"visible\">1</property>"
	    "<child>"
	      "<object class=\"GtkImageMenuItem\" id=\"preferences_item\">"
	        "<property name=\"visible\">1</property>"
	        "<property name=\"label\" translatable=\"yes\">_Preferences</property>"
	        "<property name=\"use-underline\">1</property>"
	        "<property name=\"image\">menu_icon_pre</property>"
	        "<property name=\"action-name\">win.Preferences</property>"
	      "</object>"
	    "</child>"
	    "<child>"
	      "<object class=\"GtkImageMenuItem\" id=\"about_item\">"
	        "<property name=\"visible\">1</property>"
	        "<property name=\"label\" translatable=\"yes\">_About</property>"
	        "<property name=\"use-underline\">1</property>"
	        "<property name=\"image\">menu_icon_about</property>"
	        "<property name=\"action-name\">win.About</property>"
	      "</object>"
	    "</child>"
	    "<child>"
	      "<object class=\"GtkSeparatorMenuItem\">"
	        "<property name=\"visible\">1</property>"
	      "</object>"
	    "</child>"
	    "<child>"
	      "<object class=\"GtkMenuItem\" id=\"take_break_item\">"
	        "<property name=\"visible\">1</property>"
	        "<property name=\"label\" translatable=\"yes\">_Take a Break</property>"
	        "<property name=\"use-underline\">1</property>"
	        "<property name=\"action-name\">win.TakeABreak</property>"
	      "</object>"
	    "</child>"
	 "</interface>";

	dr = g_new0 (DrWright, 1);

	settings = g_settings_new (TYPING_BREAK_SCHEMA);

	g_signal_connect (settings, "changed", G_CALLBACK (gsettings_notify_cb), dr);

	dr->type_time = 60 * g_settings_get_int (settings, "type-time");

	dr->warn_time = MIN (dr->type_time / 12, 60*3);

	dr->break_time = 60 * g_settings_get_int (settings, "break-time");

	dr->enabled = g_settings_get_boolean (settings, "enabled");

	if (debug) {
		setup_debug_values (dr);
	}

	ui_builder = gtk_builder_new ();
#ifdef ENABLE_NLS
	gtk_builder_set_translation_domain (ui_builder, GETTEXT_PACKAGE);
#endif /* ENABLE_NLS */
	gtk_builder_add_from_string (ui_builder, ui_description, -1, NULL);
	dr->menu = (GtkWidget*) g_object_ref (gtk_builder_get_object (ui_builder, "pop_menu"));
	dr->break_item = (GtkWidget *)gtk_builder_get_object (ui_builder, "take_break_item");
	gtk_widget_set_sensitive (dr->break_item, dr->enabled);

	action_group = g_simple_action_group_new ();
	g_action_map_add_action_entries (G_ACTION_MAP (action_group),
	                                 action_entries,
	                                 G_N_ELEMENTS (action_entries),
	                                 dr);

	gtk_widget_insert_action_group (dr->menu, "win", G_ACTION_GROUP (action_group));

	dr->timer = drw_timer_new ();
	dr->idle_timer = drw_timer_new ();

	dr->state = STATE_START;

	dr->monitor = drw_monitor_new ();

	g_signal_connect (dr->monitor,
			  "activity",
			  G_CALLBACK (activity_detected_cb),
			  dr);

	init_app_indicator (dr);

	g_timeout_add_seconds (12,
			       (GSourceFunc) update_status,
			       dr);

	g_timeout_add_seconds (1,
			       (GSourceFunc) maybe_change_state,
			       dr);

	g_object_unref (action_group);
	g_object_unref (ui_builder);

	return dr;
}
