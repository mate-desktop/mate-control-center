/* mate-about-me-fingerprint.h
 * Copyright (C) 2008 Bastien Nocera <hadess@hadess.net>
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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "fingerprint-strings.h"
#include "capplet-util.h"

/* This must match the number of images on the 2nd page in the UI file */
#define MAX_ENROLL_STAGES 5

/* Translate fprintd strings */
#define TR(s) dgettext("fprintd", s)

static GDBusProxy *manager = NULL;
static gboolean is_disable = FALSE;

enum {
	STATE_NONE,
	STATE_CLAIMED,
	STATE_ENROLLING
};

typedef struct {
	GtkWidget *enable;
	GtkWidget *disable;

	GtkWidget *ass;
	GtkBuilder *dialog;

	GDBusProxy *device;
	gboolean is_swipe;
	int num_enroll_stages;
	int num_stages_done;
	char *name;
	const char *finger;
	gint state;
} EnrollData;

void set_fingerprint_label (GtkWidget *enable, GtkWidget *disable);
void fingerprint_button_clicked (GtkBuilder *dialog, GtkWidget *enable, GtkWidget *disable);

static void create_manager (void)
{
	GError *error = NULL;

	manager = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
						 G_DBUS_PROXY_FLAGS_NONE,
						 NULL,
						 "net.reactivated.Fprint",
						 "/net/reactivated/Fprint/Manager",
						 "net.reactivated.Fprint.Manager",
						 NULL,
						 &error);
	if (manager == NULL) {
		g_warning ("Unable to contact Fprint Manager daemon: %s\n", error->message);
		g_error_free (error);
	}
}

static GDBusProxy*
get_first_device (void)
{
	GDBusProxy *device = NULL;
	GVariant *ret;
	char *device_str;
	GError *error = NULL;

	ret = g_dbus_proxy_call_sync (manager,
				      "GetDefaultDevice",
				      g_variant_new ("()"),
				      G_DBUS_CALL_FLAGS_NONE,
				      -1,
				      NULL,
				      &error);
	if (ret == NULL) {
		g_warning ("Could not get default fprint device: %s", error->message);
		g_error_free (error);
		return NULL;
	}

	g_variant_get (ret, "(o)", &device_str);
	g_variant_unref (ret);

	device = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
						G_DBUS_PROXY_FLAGS_NONE,
						NULL,
						"net.reactivated.Fprint",
						device_str,
						"net.reactivated.Fprint.Device",
						NULL,
						&error);
	if (device == NULL) {
		g_warning ("Unable to contact Fprint Manager daemon: %s\n", error->message);
		g_error_free (error);
	}

	g_free (device_str);
	return device;
}

static const char *
get_reason_for_error (const char *dbus_error)
{
	if (g_str_equal (dbus_error, "net.reactivated.Fprint.Error.PermissionDenied"))
		return N_("You are not allowed to access the device. Contact your system administrator.");
	if (g_str_equal (dbus_error, "net.reactivated.Fprint.Error.AlreadyInUse"))
		return N_("The device is already in use.");
	if (g_str_equal (dbus_error, "net.reactivated.Fprint.Error.Internal"))
		return N_("An internal error occurred");

	return NULL;
}

static GtkWidget *
get_error_dialog (const char *title,
		  const char *dbus_error,
		  GtkWindow *parent)
{
	GtkWidget *error_dialog;
	const char *reason;

	if (dbus_error == NULL)
		g_warning ("get_error_dialog called with reason == NULL");

	error_dialog =
		gtk_message_dialog_new (parent,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				"%s", title);
	reason = get_reason_for_error (dbus_error);
	gtk_message_dialog_format_secondary_text
		(GTK_MESSAGE_DIALOG (error_dialog), "%s", reason ? _(reason) : _(dbus_error));

	gtk_window_set_title (GTK_WINDOW (error_dialog), ""); /* as per HIG */
	gtk_container_set_border_width (GTK_CONTAINER (error_dialog), 5);
	gtk_dialog_set_default_response (GTK_DIALOG (error_dialog),
					 GTK_RESPONSE_OK);
	gtk_window_set_modal (GTK_WINDOW (error_dialog), TRUE);
	gtk_window_set_position (GTK_WINDOW (error_dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	return error_dialog;
}

void
set_fingerprint_label (GtkWidget *enable, GtkWidget *disable)
{
	gchar **enrolled_fingers;
	GDBusProxy *device;
	GError *error = NULL;
	GVariant *ret;

	gtk_widget_set_no_show_all (enable, TRUE);
	gtk_widget_set_no_show_all (disable, TRUE);

	if (manager == NULL) {
		create_manager ();
		if (manager == NULL) {
			gtk_widget_hide (enable);
			gtk_widget_hide (disable);
			return;
		}
	}

	device = get_first_device ();
	if (device == NULL) {
		gtk_widget_hide (enable);
		gtk_widget_hide (disable);
		return;
	}

	ret = g_dbus_proxy_call_sync (device,
				      "ListEnrolledFingers",
				      g_variant_new ("(s)", ""),
				      G_DBUS_CALL_FLAGS_NONE,
				      -1,
				      NULL,
				      &error);
	if (ret == NULL) {
		gchar *error_name = g_dbus_error_get_remote_error (error);
		if (!g_str_equal (error_name, "net.reactivated.Fprint.Error.NoEnrolledPrints")) {
			gtk_widget_hide (enable);
			gtk_widget_hide (disable);
			g_object_unref (device);
			g_free(error_name);
			return;
		}
		g_free(error_name);
		enrolled_fingers = NULL;
	} else {
		g_variant_get (ret, "(^as)", &enrolled_fingers);
		g_variant_unref (ret);
	}

	if (enrolled_fingers == NULL || g_strv_length (enrolled_fingers) == 0) {
		gtk_widget_hide (disable);
		gtk_widget_show (enable);
		is_disable = FALSE;
	} else {
		gtk_widget_hide (enable);
		gtk_widget_show (disable);
		is_disable = TRUE;
	}
	g_strfreev (enrolled_fingers);
	g_object_unref (device);
}

static void
delete_fingerprints (void)
{
	GVariant *ret;
	GDBusProxy *device;
	GError *error = NULL;

	if (manager == NULL) {
		create_manager ();
		if (manager == NULL)
			return;
	}

	device = get_first_device ();
	if (device == NULL)
		return;

	ret = g_dbus_proxy_call_sync (device,
				      "DeleteEnrolledFingers",
				      g_variant_new ("(s)", ""),
				      G_DBUS_CALL_FLAGS_NONE,
				      -1,
				      NULL,
				      &error);
	if (ret == NULL) {
		g_warning ("Could not delete enrolled fingers: %s", error->message);
		g_error_free (error);
	} else {
		g_variant_unref (ret);
	}

	g_object_unref (device);
}

static void
delete_fingerprints_question (GtkBuilder *dialog, GtkWidget *enable, GtkWidget *disable)
{
	GtkWidget *question;
	GtkWidget *button;

	question = gtk_message_dialog_new (GTK_WINDOW (WID ("about-me-dialog")),
					   GTK_DIALOG_MODAL,
					   GTK_MESSAGE_QUESTION,
					   GTK_BUTTONS_NONE,
					   _("Delete registered fingerprints?"));
	gtk_dialog_add_button (GTK_DIALOG (question), "gtk-cancel", GTK_RESPONSE_CANCEL);

	button = gtk_button_new_with_mnemonic (_("_Delete Fingerprints"));
	gtk_button_set_image (GTK_BUTTON (button), gtk_image_new_from_icon_name ("edit-delete", GTK_ICON_SIZE_BUTTON));
	gtk_widget_set_can_default (button, TRUE);
	gtk_widget_show (button);
	gtk_dialog_add_action_widget (GTK_DIALOG (question), button, GTK_RESPONSE_OK);

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (question),
						  _("Do you want to delete your registered fingerprints so fingerprint login is disabled?"));
	gtk_container_set_border_width (GTK_CONTAINER (question), 5);
	gtk_dialog_set_default_response (GTK_DIALOG (question), GTK_RESPONSE_OK);
	gtk_window_set_position (GTK_WINDOW (question), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_modal (GTK_WINDOW (question), TRUE);

	if (gtk_dialog_run (GTK_DIALOG (question)) == GTK_RESPONSE_OK) {
		delete_fingerprints ();
		set_fingerprint_label (enable, disable);
	}

	gtk_widget_destroy (question);
}

static void
enroll_data_destroy (EnrollData *data)
{
	GVariant *ret;
	switch (data->state) {
	case STATE_ENROLLING:
		ret = g_dbus_proxy_call_sync (data->device,
					      "EnrollStop",
					      g_variant_new ("()"),
					      G_DBUS_CALL_FLAGS_NONE,
					      -1,
					      NULL,
					      NULL);
		if (ret != NULL) {
			g_variant_unref (ret);
		}
		/* fall-through */
	case STATE_CLAIMED:
		ret = g_dbus_proxy_call_sync (data->device,
					      "Release",
					      g_variant_new ("()"),
					      G_DBUS_CALL_FLAGS_NONE,
					      -1,
					      NULL,
					      NULL);
		if (ret != NULL) {
			g_variant_unref (ret);
		}
		/* fall-through */
	case STATE_NONE:
		g_free (data->name);
		g_object_unref (data->device);
		g_object_unref (data->dialog);
		gtk_widget_destroy (data->ass);

		g_free (data);
	}
}

static const char *
selected_finger (GtkBuilder *dialog)
{
	int index;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (WID ("radiobutton1")))) {
		gtk_widget_set_sensitive (WID ("finger_combobox"), FALSE);
		return "right-index-finger";
	}
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (WID ("radiobutton2")))) {
		gtk_widget_set_sensitive (WID ("finger_combobox"), FALSE);
		return "left-index-finger";
	}
	gtk_widget_set_sensitive (WID ("finger_combobox"), TRUE);
	index = gtk_combo_box_get_active (GTK_COMBO_BOX (WID ("finger_combobox")));
	switch (index) {
	case 0:
		return "left-thumb";
	case 1:
		return "left-middle-finger";
	case 2:
		return "left-ring-finger";
	case 3:
		return "left-little-finger";
	case 4:
		return "right-thumb";
	case 5:
		return "right-middle-finger";
	case 6:
		return "right-ring-finger";
	case 7:
		return "right-little-finger";
	default:
		g_assert_not_reached ();
	}

	return NULL;
}

static void
finger_radio_button_toggled (GtkToggleButton *button, EnrollData *data)
{
	GtkBuilder *dialog = data->dialog;
	char *msg;

	data->finger = selected_finger (data->dialog);

	msg = g_strdup_printf (TR(finger_str_to_msg (data->finger, data->is_swipe)), data->name);
	gtk_label_set_text (GTK_LABEL (WID("enroll-label")), msg);
	g_free (msg);
}

static void
finger_combobox_changed (GtkComboBox *combobox, EnrollData *data)
{
	GtkBuilder *dialog = data->dialog;
	char *msg;

	data->finger = selected_finger (data->dialog);

	msg = g_strdup_printf (TR(finger_str_to_msg (data->finger, data->is_swipe)), data->name);
	gtk_label_set_text (GTK_LABEL (WID("enroll-label")), msg);
	g_free (msg);
}

static void
assistant_cancelled (GtkAssistant *ass, EnrollData *data)
{
	GtkWidget *enable, *disable;

	enable = data->enable;
	disable = data->disable;

	enroll_data_destroy (data);
	set_fingerprint_label (enable, disable);
}

static void
on_signal (GDBusProxy *proxy G_GNUC_UNUSED,
	   gchar      *sender_name G_GNUC_UNUSED,
	   gchar      *signal_name,
	   GVariant   *parameters,
	   EnrollData *data)
{
	char *msg;
	gchar *result;
	gboolean done;

	if (!g_str_equal (signal_name, "EnrollStatus")) {
		return;
	}

	GtkBuilder *dialog = data->dialog;

	g_variant_get (parameters, "(sb)", &result, &done);

	if (g_str_equal (result, "enroll-completed") || g_str_equal (result, "enroll-stage-passed")) {
		char *name, *path;

		data->num_stages_done++;
		name = g_strdup_printf ("image%d", data->num_stages_done);
		path = g_build_filename (MATECC_PIXMAP_DIR, "print_ok.png", NULL);
		gtk_image_set_from_file (GTK_IMAGE (WID (name)), path);
		g_free (name);
		g_free (path);
	}
	if (g_str_equal (result, "enroll-completed")) {
		gtk_label_set_text (GTK_LABEL (WID ("status-label")), _("Done!"));
		gtk_assistant_set_page_complete (GTK_ASSISTANT (data->ass), WID ("page2"), TRUE);
	}

	if (done) {
		GVariant *ret;
		ret = g_dbus_proxy_call_sync (data->device,
					      "EnrollStop",
					      g_variant_new ("()"),
					      G_DBUS_CALL_FLAGS_NONE,
					      -1,
					      NULL,
					      NULL);
		if (ret != NULL) {
			g_variant_unref (ret);
		}
		data->state = STATE_CLAIMED;
		if (g_str_equal (result, "enroll-completed") == FALSE) {
			/* The enrollment failed, restart it */
			ret = g_dbus_proxy_call_sync (data->device,
						      "EnrollStart",
						      g_variant_new ("(s)", data->finger),
						      G_DBUS_CALL_FLAGS_NONE,
						      -1,
						      NULL,
						      NULL);
			if (ret != NULL) {
				g_variant_unref (ret);
			}
			data->state = STATE_ENROLLING;
			g_free (result);
			result = g_strdup ("enroll-retry-scan");
		} else {
			g_free (result);
			return;
		}
	}

	msg = g_strdup_printf (TR(enroll_result_str_to_msg (result, data->is_swipe)), data->name);
	gtk_label_set_text (GTK_LABEL (WID ("status-label")), msg);
	g_free (msg);
	g_free (result);
}

static void
assistant_prepare (GtkAssistant *ass, GtkWidget *page, EnrollData *data)
{
	const char *name;

	name = g_object_get_data (G_OBJECT (page), "name");
	if (name == NULL)
		return;

	if (g_str_equal (name, "enroll")) {
		GError *error = NULL;
		GtkBuilder *dialog = data->dialog;
		char *path;
		guint i;
		GVariant *ret;
		GtkWidget *d;
		char *msg;
		gchar *error_name;
		GDBusProxy *p;

		ret = g_dbus_proxy_call_sync (data->device,
					      "Claim",
					      g_variant_new ("(s)", ""),
					      G_DBUS_CALL_FLAGS_NONE,
					      -1,
					      NULL,
					      &error);
		if (error != NULL) {
			/* translators:
			 * The variable is the name of the device, for example:
			 * "Could you not access "Digital Persona U.are.U 4000/4000B" device */
			msg = g_strdup_printf (_("Could not access '%s' device"), data->name);
			error_name = g_dbus_error_get_remote_error (error);
			d = get_error_dialog (msg, error_name, GTK_WINDOW (data->ass));
			g_error_free (error);
			gtk_dialog_run (GTK_DIALOG (d));
			gtk_widget_destroy (d);
			g_free (msg);
			g_free (error_name);

			enroll_data_destroy (data);

			return;
		} else {
			g_variant_unref (ret);
		}
		data->state = STATE_CLAIMED;

		p = g_dbus_proxy_new_sync (g_dbus_proxy_get_connection (data->device),
					   G_DBUS_PROXY_FLAGS_NONE,
					   NULL,
					   g_dbus_proxy_get_name (data->device),
					   g_dbus_proxy_get_object_path (data->device),
					   "org.freedesktop.DBus.Properties",
					   NULL,
					   &error);
		if (p == NULL) {
			g_warning ("Unable to contact Fprint Device daemon: %s\n", error->message);
			g_error_free (error);
			return;
		}
		ret = g_dbus_proxy_call_sync (p,
					      "Get",
					      g_variant_new ("(ss)",
							     "net.reactivated.Fprint.Device",
							     "num-enroll-stages"),
					      G_DBUS_CALL_FLAGS_NONE,
					      -1,
					      NULL,
					      &error);
		g_object_unref (p);
		if (ret == NULL) {
out:
			/* translators:
			 * The variable is the name of the device, for example:
			 * "Could you not access "Digital Persona U.are.U 4000/4000B" device */
			msg = g_strdup_printf (_("Could not access '%s' device"), data->name);
			d = get_error_dialog (msg, "net.reactivated.Fprint.Error.Internal", GTK_WINDOW (data->ass));
			gtk_dialog_run (GTK_DIALOG (d));
			gtk_widget_destroy (d);
			g_free (msg);

			enroll_data_destroy (data);

			g_error_free (error);
			return;
		} else {
			GVariant *value;
			g_variant_get (ret, "(v)", &value);
			g_variant_get (value, "i", &data->num_enroll_stages);
			g_variant_unref (value);
			g_variant_unref (ret);
		}

		if (data->num_enroll_stages < 1) {
			goto out;
		}

		/* Hide the extra "bulbs" if not needed */
		for (i = MAX_ENROLL_STAGES; i > data->num_enroll_stages; i--) {
			char *image_name;

			image_name = g_strdup_printf ("image%d", i);
			gtk_widget_hide (WID (image_name));
			g_free (image_name);
		}
		/* And set the right image */
		{
			char *filename;

			filename = g_strdup_printf ("%s.png", data->finger);
			path = g_build_filename (MATECC_PIXMAP_DIR, filename, NULL);
			g_free (filename);
		}
		for (i = 1; i <= data->num_enroll_stages; i++) {
			char *image_name;
			image_name = g_strdup_printf ("image%d", i);
			gtk_image_set_from_file (GTK_IMAGE (WID (image_name)), path);
			g_free (image_name);
		}
		g_free (path);

		g_signal_connect (data->device,
				  "g-signal",
				  G_CALLBACK (on_signal),
				  data);
		ret = g_dbus_proxy_call_sync (data->device,
					      "EnrollStart",
					      g_variant_new ("(s)", data->finger),
					      G_DBUS_CALL_FLAGS_NONE,
					      -1,
					      NULL,
					      &error);
		if (ret == NULL) {
			/* translators:
			 * The variable is the name of the device, for example:
			 * "Could you not access "Digital Persona U.are.U 4000/4000B" device */
			msg = g_strdup_printf (_("Could not start finger capture on '%s' device"), data->name);
			error_name = g_dbus_error_get_remote_error (error);
			d = get_error_dialog (msg, error_name, GTK_WINDOW (data->ass));
			g_error_free (error);
			gtk_dialog_run (GTK_DIALOG (d));
			gtk_widget_destroy (d);
			g_free (msg);
			g_free (error_name);

			enroll_data_destroy (data);

			return;
		}
		data->state = STATE_ENROLLING;;
		g_variant_unref (ret);
	} else {
		GVariant *value;
		if (data->state == STATE_ENROLLING) {
			value = g_dbus_proxy_call_sync (data->device,
							"EnrollStop",
							g_variant_new ("()"),
							G_DBUS_CALL_FLAGS_NONE,
							-1,
							NULL,
							NULL);
			if (value != NULL) {
				g_variant_unref (value);
			}
			data->state = STATE_CLAIMED;
		}
		if (data->state == STATE_CLAIMED) {
			value = g_dbus_proxy_call_sync (data->device,
							"Release",
							g_variant_new ("()"),
							G_DBUS_CALL_FLAGS_NONE,
							-1,
							NULL,
							NULL);
			if (value != NULL) {
				g_variant_unref (value);
			}
			data->state = STATE_NONE;
		}
	}
}

static void
enroll_fingerprints (GtkWindow *parent, GtkWidget *enable, GtkWidget *disable)
{
	GDBusProxy *device;
	GtkBuilder *dialog;
	EnrollData *data;
	GtkWidget *ass;
	char *msg;
	GError *error = NULL;
	GVariant *ret;

	device = NULL;

	if (manager == NULL) {
		create_manager ();
		if (manager != NULL)
			device = get_first_device ();
	} else {
		device = get_first_device ();
	}

	if (manager == NULL || device == NULL) {
		GtkWidget *d;

		d = get_error_dialog (_("Could not access any fingerprint readers"),
				      _("Please contact your system administrator for help."),
				      parent);
		gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (d);
		return;
	}

	GDBusProxy *p;
	p = g_dbus_proxy_new_sync (g_dbus_proxy_get_connection (device),
				   G_DBUS_PROXY_FLAGS_NONE,
				   NULL,
				   g_dbus_proxy_get_name (device),
				   g_dbus_proxy_get_object_path (device),
				   "org.freedesktop.DBus.Properties",
				   NULL,
				   &error);
	if (p == NULL) {
		g_warning ("Unable to contact Fprint Device daemon: %s\n", error->message);
		g_error_free (error);
		return;
	}

	data = g_new0 (EnrollData, 1);
	data->device = device;
	data->enable = enable;
	data->disable = disable;

	/* Get some details about the device */
	ret = g_dbus_proxy_call_sync (p,
				      "GetAll",
				      g_variant_new ("(s)", "net.reactivated.Fprint.Device"),
				      G_DBUS_CALL_FLAGS_NONE,
				      -1,
				      NULL,
				      NULL);
	if (ret != NULL) {
		GVariantIter *iter;
		const gchar *key;
		GVariant *value;

		g_variant_get (ret, "(a{sv})", &iter);
		while (g_variant_iter_loop (iter, "{sv}", &key, &value))
		{
			if (g_str_equal (key, "name") && g_variant_get_type (value) == G_VARIANT_TYPE_STRING) {
				data->name = g_strdup (g_variant_get_string (value, NULL));
			} else if (g_str_equal (key, "scan-type") && g_variant_get_type (value) == G_VARIANT_TYPE_STRING) {
				if (g_str_equal (g_variant_get_string (value, NULL), "swipe"))
					data->is_swipe = TRUE;
			}
		}
		g_variant_iter_free (iter);
		g_variant_unref (ret);
	}
	g_object_unref (p);

	dialog = gtk_builder_new ();
	if (gtk_builder_add_from_resource (dialog, "/org/mate/mcc/am/mate-about-me-fingerprint.ui", &error) == 0)
	{
		g_warning ("Could not parse UI definition: %s", error->message);
		g_error_free (error);
	}
	data->dialog = dialog;

	ass = WID ("assistant");
	gtk_window_set_title (GTK_WINDOW (ass), _("Enable Fingerprint Login"));
	gtk_window_set_transient_for (GTK_WINDOW (ass), parent);
	gtk_window_set_position (GTK_WINDOW (ass), GTK_WIN_POS_CENTER_ON_PARENT);
	g_signal_connect (G_OBJECT (ass), "cancel",
			  G_CALLBACK (assistant_cancelled), data);
	g_signal_connect (G_OBJECT (ass), "close",
			  G_CALLBACK (assistant_cancelled), data);
	g_signal_connect (G_OBJECT (ass), "prepare",
			  G_CALLBACK (assistant_prepare), data);

	/* Page 1 */
	gtk_combo_box_set_active (GTK_COMBO_BOX (WID ("finger_combobox")), 0);

	g_signal_connect (G_OBJECT (WID ("radiobutton1")), "toggled",
			  G_CALLBACK (finger_radio_button_toggled), data);
	g_signal_connect (G_OBJECT (WID ("radiobutton2")), "toggled",
			  G_CALLBACK (finger_radio_button_toggled), data);
	g_signal_connect (G_OBJECT (WID ("radiobutton3")), "toggled",
			  G_CALLBACK (finger_radio_button_toggled), data);
	g_signal_connect (G_OBJECT (WID ("finger_combobox")), "changed",
			  G_CALLBACK (finger_combobox_changed), data);

	data->finger = selected_finger (dialog);

	g_object_set_data (G_OBJECT (WID("page1")), "name", "intro");

	/* translators:
	 * The variable is the name of the device, for example:
	 * "To enable fingerprint login, you need to save one of your fingerprints, using the
	 * 'Digital Persona U.are.U 4000/4000B' device." */
	msg = g_strdup_printf (_("To enable fingerprint login, you need to save one of your fingerprints, using the '%s' device."),
			       data->name);
	gtk_label_set_text (GTK_LABEL (WID("intro-label")), msg);
	g_free (msg);

	gtk_assistant_set_page_complete (GTK_ASSISTANT (ass), WID("page1"), TRUE);

	/* Page 2 */
	if (data->is_swipe != FALSE)
		gtk_assistant_set_page_title (GTK_ASSISTANT (ass), WID("page2"), _("Swipe finger on reader"));
	else
		gtk_assistant_set_page_title (GTK_ASSISTANT (ass), WID("page2"), _("Place finger on reader"));

	g_object_set_data (G_OBJECT (WID("page2")), "name", "enroll");

	msg = g_strdup_printf (TR(finger_str_to_msg (data->finger, data->is_swipe)), data->name);
	gtk_label_set_text (GTK_LABEL (WID("enroll-label")), msg);
	g_free (msg);

	/* Page 3 */
	g_object_set_data (G_OBJECT (WID("page3")), "name", "summary");

	data->ass = ass;
	gtk_widget_show_all (ass);
}

void
fingerprint_button_clicked (GtkBuilder *dialog,
			    GtkWidget *enable,
			    GtkWidget *disable)
{
	bindtextdomain ("fprintd", MATELOCALEDIR);
	bind_textdomain_codeset ("fprintd", "UTF-8");

	if (is_disable != FALSE) {
		delete_fingerprints_question (dialog, enable, disable);
	} else {
		enroll_fingerprints (GTK_WINDOW (WID ("about-me-dialog")), enable, disable);
	}
}
