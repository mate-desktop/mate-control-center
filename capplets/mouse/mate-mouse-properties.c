/* -*- mode: c; style: linux -*- */

/* mouse-properties-capplet.c
 * Copyright (C) 2001 Red Hat, Inc.
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Written by: Jonathon Blandford <jrb@redhat.com>,
 *             Bradford Hovinen <hovinen@ximian.com>,
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <config.h>

#include <glib/gi18n.h>
#include <string.h>
#include <gio/gio.h>
#include <gdk/gdkx.h>
#include <math.h>

#include "capplet-util.h"
#include "activate-settings-daemon.h"
#include "msd-input-helper.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <X11/Xatom.h>
#include <X11/extensions/XInput.h>

enum
{
	DOUBLE_CLICK_TEST_OFF,
	DOUBLE_CLICK_TEST_MAYBE,
	DOUBLE_CLICK_TEST_ON
};

typedef enum {
	ACCEL_PROFILE_DEFAULT,
	ACCEL_PROFILE_ADAPTIVE,
	ACCEL_PROFILE_FLAT
} AccelProfile;

#define MOUSE_SCHEMA "org.mate.peripherals-mouse"
#define INTERFACE_SCHEMA "org.mate.interface"
#define DOUBLE_CLICK_KEY "double-click"

#define TOUCHPAD_SCHEMA "org.mate.peripherals-touchpad"

/* State in testing the double-click speed. Global for a great deal of
 * convenience
 */
static gint double_click_state = DOUBLE_CLICK_TEST_OFF;

static GSettings *mouse_settings = NULL;
static GSettings *interface_settings = NULL;
static GSettings *touchpad_settings = NULL;

/* Double Click handling */

struct test_data_t
{
	gint *timeout_id;
	GtkWidget *image;
};

/* Timeout for the double click test */

static gboolean
test_maybe_timeout (struct test_data_t *data)
{
	double_click_state = DOUBLE_CLICK_TEST_OFF;

	gtk_image_set_from_resource (GTK_IMAGE (data->image), "/org/mate/mcc/mouse/double-click-off.svg");

	*data->timeout_id = 0;

	return FALSE;
}

/* Callback issued when the user clicks the double click testing area. */

static gboolean
event_box_button_press_event (GtkWidget   *widget,
			      GdkEventButton *event,
			      gpointer user_data)
{
	gint                       double_click_time;
	static struct test_data_t  data;
	static gint                test_on_timeout_id     = 0;
	static gint                test_maybe_timeout_id  = 0;
	static guint32             double_click_timestamp = 0;
	GtkWidget                 *image;

	if (event->type != GDK_BUTTON_PRESS)
		return FALSE;

	image = g_object_get_data (G_OBJECT (widget), "image");

	double_click_time = g_settings_get_int (mouse_settings, DOUBLE_CLICK_KEY);

	if (test_maybe_timeout_id != 0) {
		g_source_remove  (test_maybe_timeout_id);
		test_maybe_timeout_id = 0;
	}
	if (test_on_timeout_id != 0) {
		g_source_remove (test_on_timeout_id);
		test_on_timeout_id = 0;
	}

	switch (double_click_state) {
	case DOUBLE_CLICK_TEST_OFF:
		double_click_state = DOUBLE_CLICK_TEST_MAYBE;
		data.image = image;
		data.timeout_id = &test_maybe_timeout_id;
		test_maybe_timeout_id = g_timeout_add (double_click_time, (GSourceFunc) test_maybe_timeout, &data);
		break;
	case DOUBLE_CLICK_TEST_MAYBE:
		if (event->time - double_click_timestamp < double_click_time) {
			double_click_state = DOUBLE_CLICK_TEST_ON;
			data.image = image;
			data.timeout_id = &test_on_timeout_id;
			test_on_timeout_id = g_timeout_add (2500, (GSourceFunc) test_maybe_timeout, &data);
		}
		break;
	case DOUBLE_CLICK_TEST_ON:
		double_click_state = DOUBLE_CLICK_TEST_OFF;
		break;
	}

	double_click_timestamp = event->time;

	switch (double_click_state) {
	case DOUBLE_CLICK_TEST_ON:
		gtk_image_set_from_resource (GTK_IMAGE (image), "/org/mate/mcc/mouse/double-click-on.svg");
		break;
	case DOUBLE_CLICK_TEST_MAYBE:
		gtk_image_set_from_resource (GTK_IMAGE (image), "/org/mate/mcc/mouse/double-click-maybe.svg");
		break;
	case DOUBLE_CLICK_TEST_OFF:
		gtk_image_set_from_resource (GTK_IMAGE (image), "/org/mate/mcc/mouse/double-click-off.svg");
		break;
	}

	return TRUE;
}

static void
orientation_radio_button_release_event (GtkWidget   *widget,
				        GdkEventButton *event)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
}

static void
orientation_radio_button_toggled (GtkToggleButton *togglebutton,
				        GtkBuilder *dialog)
{
	gboolean left_handed;
	left_handed = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (WID ("left_handed_radio")));
	g_settings_set_boolean (mouse_settings, "left-handed", left_handed);
}

static void
synaptics_check_capabilities (GtkBuilder *dialog)
{
	GdkDisplay *display;
	int numdevices, i;
	XDeviceInfo *devicelist;
	Atom realtype, prop;
	int realformat;
	unsigned long nitems, bytes_after;
	unsigned char *data;

	display = gdk_display_get_default ();
	prop = XInternAtom (GDK_DISPLAY_XDISPLAY(display), "Synaptics Capabilities", True);
	if (!prop)
		return;

	devicelist = XListInputDevices (GDK_DISPLAY_XDISPLAY(display), &numdevices);
	for (i = 0; i < numdevices; i++) {
		if (devicelist[i].use != IsXExtensionPointer)
			continue;

		gdk_x11_display_error_trap_push (display);
		XDevice *device = XOpenDevice (GDK_DISPLAY_XDISPLAY(display),
					       devicelist[i].id);
		if (gdk_x11_display_error_trap_pop (display))
			continue;

		gdk_x11_display_error_trap_push (display);
		if ((XGetDeviceProperty (GDK_DISPLAY_XDISPLAY(display), device, prop, 0, 2, False,
					 XA_INTEGER, &realtype, &realformat, &nitems,
					 &bytes_after, &data) == Success) && (realtype != None)) {
			/* Property data is booleans for has_left, has_middle,
			 * has_right, has_double, has_triple */
			if (!data[0]) {
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (WID ("tap_to_click_toggle")), TRUE);
				gtk_widget_set_sensitive (WID ("tap_to_click_toggle"), FALSE);
			}

			XFree (data);
		}

		gdk_x11_display_error_trap_pop_ignored (display);

		XCloseDevice (GDK_DISPLAY_XDISPLAY(display), device);
	}
	XFreeDeviceList (devicelist);
}

static void
accel_profile_combobox_changed_callback (GtkWidget *combobox, void *data)
{
	AccelProfile value = gtk_combo_box_get_active (GTK_COMBO_BOX (combobox));
	g_settings_set_enum (mouse_settings, (const gchar *) "accel-profile", value);
}

static void
comboxbox_changed (GtkWidget *combobox, GtkBuilder *dialog, const char *key)
{
	gint value = gtk_combo_box_get_active (GTK_COMBO_BOX (combobox));
	gint value2, value3;
	GtkLabel *warn = GTK_LABEL (WID ("multi_finger_warning"));

	g_settings_set_int (touchpad_settings, (const gchar *) key, value);

	/* Show warning if some multi-finger click emulation is enabled. */
	value2 = g_settings_get_int (touchpad_settings, "two-finger-click");
	value3 = g_settings_get_int (touchpad_settings, "three-finger-click");
	gtk_widget_set_opacity (GTK_WIDGET (warn), (value2 || value3)?  1.0: 0.0);
}

static void
comboxbox_two_finger_changed_callback (GtkWidget *combobox, void *data)
{
	comboxbox_changed (combobox, GTK_BUILDER (data), "two-finger-click");
}

static void
comboxbox_three_finger_changed_callback (GtkWidget *combobox, void *data)
{
	comboxbox_changed (combobox, GTK_BUILDER (data), "three-finger-click");
}

/* Set up the property editors in the dialog. */
static void
setup_dialog (GtkBuilder *dialog)
{
	GtkRadioButton    *radio;

	/* Orientation radio buttons */
	radio = GTK_RADIO_BUTTON (WID ("left_handed_radio"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radio),
		g_settings_get_boolean(mouse_settings, "left-handed"));
	/* explicitly connect to button-release so that you can change orientation with either button */
	g_signal_connect (WID ("right_handed_radio"), "button_release_event",
		G_CALLBACK (orientation_radio_button_release_event), NULL);
	g_signal_connect (WID ("left_handed_radio"), "button_release_event",
		G_CALLBACK (orientation_radio_button_release_event), NULL);
	g_signal_connect (WID ("left_handed_radio"), "toggled",
		G_CALLBACK (orientation_radio_button_toggled), dialog);

	/* Locate pointer toggle */
	g_settings_bind (mouse_settings, "locate-pointer", WID ("locate_pointer_toggle"),
		"active", G_SETTINGS_BIND_DEFAULT);

	/* Middle Button Emulation */
	g_settings_bind (mouse_settings, "middle-button-enabled", WID ("middle_button_emulation_toggle"),
		"active", G_SETTINGS_BIND_DEFAULT);

	/* Middle Button Paste */
	g_settings_bind (interface_settings, "gtk-enable-primary-paste", WID ("middle_button_paste_toggle"),
		"active", G_SETTINGS_BIND_DEFAULT);


	/* Double-click time */
	g_settings_bind (mouse_settings, DOUBLE_CLICK_KEY,
		gtk_range_get_adjustment (GTK_RANGE (WID ("delay_scale"))), "value",
		G_SETTINGS_BIND_DEFAULT);

	gtk_image_set_from_resource (GTK_IMAGE (WID ("double_click_image")), "/org/mate/mcc/mouse/double-click-off.svg");
	g_object_set_data (G_OBJECT (WID ("double_click_eventbox")), "image", WID ("double_click_image"));
	g_signal_connect (WID ("double_click_eventbox"), "button_press_event",
			  G_CALLBACK (event_box_button_press_event), NULL);

	/* speed */
	g_settings_bind (mouse_settings, "motion-acceleration",
		gtk_range_get_adjustment (GTK_RANGE (WID ("accel_scale"))), "value",
		G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (mouse_settings, "motion-threshold",
		gtk_range_get_adjustment (GTK_RANGE (WID ("sensitivity_scale"))), "value",
		G_SETTINGS_BIND_DEFAULT);

	g_signal_connect (WID ("mouse_accel_profile"), "changed",
			  G_CALLBACK (accel_profile_combobox_changed_callback), NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (WID ("mouse_accel_profile")),
				  g_settings_get_enum (mouse_settings, "accel-profile"));

	/* DnD threshold */
	g_settings_bind (mouse_settings, "drag-threshold",
		gtk_range_get_adjustment (GTK_RANGE (WID ("drag_threshold_scale"))), "value",
		G_SETTINGS_BIND_DEFAULT);

	/* Trackpad page */
	if (touchpad_is_present () == FALSE)
		gtk_notebook_remove_page (GTK_NOTEBOOK (WID ("prefs_widget")), -1);
	else {
		g_settings_bind (touchpad_settings, "touchpad-enabled",
			WID ("touchpad_enable"), "active",
			G_SETTINGS_BIND_DEFAULT);
		g_settings_bind (touchpad_settings, "touchpad-enabled",
			WID ("vbox_touchpad_general"), "sensitive",
			G_SETTINGS_BIND_DEFAULT);
		g_settings_bind (touchpad_settings, "touchpad-enabled",
			WID ("vbox_touchpad_scrolling"), "sensitive",
			G_SETTINGS_BIND_DEFAULT);
		g_settings_bind (touchpad_settings, "touchpad-enabled",
			WID ("vbox_touchpad_pointer_speed"), "sensitive",
			G_SETTINGS_BIND_DEFAULT);
		g_settings_bind (touchpad_settings, "disable-while-typing",
			WID ("disable_w_typing_toggle"), "active",
			G_SETTINGS_BIND_DEFAULT);
		g_settings_bind (touchpad_settings, "tap-to-click",
			WID ("tap_to_click_toggle"), "active",
			G_SETTINGS_BIND_DEFAULT);
		g_settings_bind (touchpad_settings, "vertical-edge-scrolling", WID ("vert_edge_scroll_toggle"), "active", G_SETTINGS_BIND_DEFAULT);
		g_settings_bind (touchpad_settings, "horizontal-edge-scrolling", WID ("horiz_edge_scroll_toggle"), "active", G_SETTINGS_BIND_DEFAULT);
		g_settings_bind (touchpad_settings, "vertical-two-finger-scrolling", WID ("vert_twofinger_scroll_toggle"), "active", G_SETTINGS_BIND_DEFAULT);
		g_settings_bind (touchpad_settings, "horizontal-two-finger-scrolling", WID ("horiz_twofinger_scroll_toggle"), "active", G_SETTINGS_BIND_DEFAULT);
		g_settings_bind (touchpad_settings, "natural-scroll", WID ("natural_scroll_toggle"), "active", G_SETTINGS_BIND_DEFAULT);

		char * emulation_values[] = { _("Disabled"), _("Left button"), _("Middle button"), _("Right button") };

		GtkWidget *two_click_comboxbox = gtk_combo_box_text_new ();
		GtkWidget *three_click_comboxbox = gtk_combo_box_text_new ();
		gtk_box_pack_start (GTK_BOX (WID ("hbox_two_finger_click")), two_click_comboxbox, FALSE, FALSE, 6);
		gtk_box_pack_start (GTK_BOX (WID ("hbox_three_finger_click")), three_click_comboxbox, FALSE, FALSE, 6);
		int i;
		for (i=0; i<4; i++) {
			gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (two_click_comboxbox), emulation_values[i]);
			gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (three_click_comboxbox), emulation_values[i]);
		}

		g_signal_connect (two_click_comboxbox, "changed", G_CALLBACK (comboxbox_two_finger_changed_callback), dialog);
		g_signal_connect (three_click_comboxbox, "changed", G_CALLBACK (comboxbox_three_finger_changed_callback), dialog);
		gtk_combo_box_set_active (GTK_COMBO_BOX (two_click_comboxbox), g_settings_get_int (touchpad_settings, "two-finger-click"));
		gtk_combo_box_set_active (GTK_COMBO_BOX (three_click_comboxbox), g_settings_get_int (touchpad_settings, "three-finger-click"));
		gtk_widget_show (two_click_comboxbox);
		gtk_widget_show (three_click_comboxbox);

		/* speed */
		g_settings_bind (touchpad_settings, "motion-acceleration",
			gtk_range_get_adjustment (GTK_RANGE (WID ("touchpad_accel_scale"))), "value",
			G_SETTINGS_BIND_DEFAULT);
		g_settings_bind (touchpad_settings, "motion-threshold",
			gtk_range_get_adjustment (GTK_RANGE (WID ("touchpad_sensitivity_scale"))), "value",
			G_SETTINGS_BIND_DEFAULT);

		synaptics_check_capabilities (dialog);
	}

}

/* Construct the dialog */

static GtkBuilder *
create_dialog (void)
{
	GtkBuilder   *dialog;
	GtkSizeGroup *size_group;
	GError       *error = NULL;

	dialog = gtk_builder_new ();
	if (gtk_builder_add_from_resource (dialog, "/org/mate/mcc/mouse/mate-mouse-properties.ui", &error) == 0) {
		g_warning ("Error loading UI file: %s", error->message);
		g_error_free (error);
		g_object_unref (dialog);
		return NULL;
	}

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (size_group, WID ("acceleration_label"));
	gtk_size_group_add_widget (size_group, WID ("sensitivity_label"));
	gtk_size_group_add_widget (size_group, WID ("threshold_label"));
	gtk_size_group_add_widget (size_group, WID ("timeout_label"));

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (size_group, WID ("acceleration_fast_label"));
	gtk_size_group_add_widget (size_group, WID ("sensitivity_high_label"));
	gtk_size_group_add_widget (size_group, WID ("threshold_large_label"));
	gtk_size_group_add_widget (size_group, WID ("timeout_long_label"));

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (size_group, WID ("acceleration_slow_label"));
	gtk_size_group_add_widget (size_group, WID ("sensitivity_low_label"));
	gtk_size_group_add_widget (size_group, WID ("threshold_small_label"));
	gtk_size_group_add_widget (size_group, WID ("timeout_short_label"));

	return dialog;
}

/* Callback issued when a button is clicked on the dialog */

static void
dialog_response_cb (GtkDialog *dialog, gint response_id, gpointer data)
{
	if (response_id == GTK_RESPONSE_HELP)
		capplet_help (GTK_WINDOW (dialog),
			      "goscustperiph-5");
	else
		gtk_main_quit ();
}

int
main (int argc, char **argv)
{
	GtkBuilder     *dialog;
	GtkWidget      *dialog_win, *w;
	gchar *start_page = NULL;

	GOptionContext *context;
	GOptionEntry cap_options[] = {
		{"show-page", 'p', G_OPTION_FLAG_IN_MAIN,
		 G_OPTION_ARG_STRING,
		 &start_page,
		 /* TRANSLATORS: don't translate the terms in brackets */
		 N_("Specify the name of the page to show (general)"),
		 N_("page") },
		{NULL}
	};

	context = g_option_context_new (_("- MATE Mouse Preferences"));
	g_option_context_add_main_entries (context, cap_options, GETTEXT_PACKAGE);
	capplet_init (context, &argc, &argv);

	activate_settings_daemon ();

	mouse_settings = g_settings_new (MOUSE_SCHEMA);
	interface_settings = g_settings_new (INTERFACE_SCHEMA);
	touchpad_settings = g_settings_new (TOUCHPAD_SCHEMA);

	dialog = create_dialog ();

	if (dialog) {
		setup_dialog (dialog);

		dialog_win = WID ("mouse_properties_dialog");
		g_signal_connect (dialog_win, "response",
				  G_CALLBACK (dialog_response_cb), NULL);

                GtkNotebook* nb = GTK_NOTEBOOK (WID ("prefs_widget"));
                gtk_widget_add_events (GTK_WIDGET (nb), GDK_SCROLL_MASK);
                g_signal_connect (GTK_WIDGET (nb), "scroll-event",
                                  G_CALLBACK (capplet_dialog_page_scroll_event_cb),
                                  GTK_WINDOW (dialog_win));


		if (start_page != NULL) {
			gchar *page_name;

			page_name = g_strconcat (start_page, "_vbox", NULL);
			g_free (start_page);

			w = WID (page_name);
			if (w != NULL) {
				gint pindex;

				pindex = gtk_notebook_page_num (nb, w);
				if (pindex != -1)
					gtk_notebook_set_current_page (nb, pindex);
			}
			g_free (page_name);
		}

		capplet_set_icon (dialog_win, "input-mouse");
		gtk_widget_show (dialog_win);

		gtk_main ();

		g_object_unref (dialog);
	}

	g_object_unref (mouse_settings);
	g_object_unref (interface_settings);
	g_object_unref (touchpad_settings);

	return 0;
}
