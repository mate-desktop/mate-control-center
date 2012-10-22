#include <config.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#define GSM_SERVICE_DBUS   "org.mate.SessionManager"
#define GSM_PATH_DBUS      "/org/mate/SessionManager"
#define GSM_INTERFACE_DBUS "org.mate.SessionManager"

enum {
        GSM_LOGOUT_MODE_NORMAL = 0,
        GSM_LOGOUT_MODE_NO_CONFIRMATION,
        GSM_LOGOUT_MODE_FORCE
};

#include "capplet-util.h"
#include "activate-settings-daemon.h"

#define ACCESSIBILITY_KEY       "accessibility"
#define ACCESSIBILITY_SCHEMA    "org.mate.interface"

static gboolean initial_state;

static GtkBuilder *
create_builder (void)
{
	GtkBuilder *builder;
	GError *error = NULL;
	static const gchar *uifile = UIDIR "/at-enable-dialog.ui";

	builder = gtk_builder_new ();

	if (gtk_builder_add_from_file (builder, uifile, &error)) {
		GObject *object;
		gchar *prog;

		object = gtk_builder_get_object (builder, "at_enable_image");
		gtk_image_set_from_file (GTK_IMAGE (object),
					 PIXMAPDIR "/at-startup.png");

		object = gtk_builder_get_object (builder,
						 "at_applications_image");
		gtk_image_set_from_file (GTK_IMAGE (object),
					 PIXMAPDIR "/at-support.png");

		prog = g_find_program_in_path ("mdmsetup");
		if (prog == NULL) {
			object = gtk_builder_get_object (builder,
							 "login_button");
			gtk_widget_hide (GTK_WIDGET (object));
		}

		g_free (prog);
	} else {
		g_warning ("Could not load UI: %s", error->message);
		g_error_free (error);
		g_object_unref (builder);
		builder = NULL;
	}

	return builder;
}

static void
cb_at_preferences (GtkDialog *dialog, gint response_id)
{
	g_spawn_command_line_async ("mate-default-applications-properties --show-page=a11y", NULL);
}

static void
cb_keyboard_preferences (GtkDialog *dialog, gint response_id)
{
	g_spawn_command_line_async ("mate-keyboard-properties --a11y", NULL);
}

static void
cb_mouse_preferences (GtkDialog *dialog, gint response_id)
{
	g_spawn_command_line_async ("mate-mouse-properties --show-page=accessibility", NULL);
}

static void
cb_login_preferences (GtkDialog *dialog, gint response_id)
{
	g_spawn_command_line_async ("mdmsetup", NULL);
}

/* get_session_bus(), get_sm_proxy(), and do_logout() are all
 * based on code from mate-session-save.c from mate-session.
 */
static DBusGConnection *
get_session_bus (void)
{
        DBusGConnection *bus;
        GError *error = NULL;

        bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

        if (bus == NULL) {
                g_warning ("Couldn't connect to session bus: %s", error->message);
                g_error_free (error);
        }

        return bus;
}

static DBusGProxy *
get_sm_proxy (void)
{
        DBusGConnection *connection;
        DBusGProxy      *sm_proxy;

        if (!(connection = get_session_bus ()))
		return NULL;

        sm_proxy = dbus_g_proxy_new_for_name (connection,
					      GSM_SERVICE_DBUS,
					      GSM_PATH_DBUS,
					      GSM_INTERFACE_DBUS);

        return sm_proxy;
}

static gboolean
do_logout (GError **err)
{
        DBusGProxy *sm_proxy;
        GError     *error;
        gboolean    res;

        sm_proxy = get_sm_proxy ();
        if (sm_proxy == NULL)
		return FALSE;

        res = dbus_g_proxy_call (sm_proxy,
                                 "Logout",
                                 &error,
                                 G_TYPE_UINT, 0,   /* '0' means 'log out normally' */
                                 G_TYPE_INVALID,
                                 G_TYPE_INVALID);

        if (sm_proxy)
                g_object_unref (sm_proxy);

	return res;
}

static void
cb_dialog_response (GtkDialog *dialog, gint response_id)
{
	if (response_id == GTK_RESPONSE_HELP)
		capplet_help (GTK_WINDOW (dialog),
			      "goscustaccess-11");
	else if (response_id == GTK_RESPONSE_CLOSE || response_id == GTK_RESPONSE_DELETE_EVENT)
		gtk_main_quit ();
	else {
	        g_message ("CLOSE AND LOGOUT!");

		if (!do_logout (NULL))
			gtk_main_quit ();
	}
}

static void
close_logout_update (GtkBuilder *builder)
{
	GSettings *settings = g_settings_new (ACCESSIBILITY_SCHEMA);
	gboolean curr_state = g_settings_get_boolean (settings, ACCESSIBILITY_KEY);
	GObject *btn = gtk_builder_get_object (builder,
					       "at_close_logout_button");

	gtk_widget_set_sensitive (GTK_WIDGET (btn), initial_state != curr_state);
	g_object_unref (settings);
}

static void
at_enable_toggled (GtkToggleButton *toggle_button,
		   GtkBuilder      *builder)
{
	GSettings *settings = g_settings_new (ACCESSIBILITY_SCHEMA);
	gboolean is_enabled = gtk_toggle_button_get_active (toggle_button);

	g_settings_set_boolean (settings, ACCESSIBILITY_KEY, is_enabled);
	g_object_unref (settings);
}

static void
at_enable_update (GSettings *settings,
		  GtkBuilder  *builder)
{
	gboolean is_enabled = g_settings_get_boolean (settings, ACCESSIBILITY_KEY);
	GObject *btn = gtk_builder_get_object (builder, "at_enable_toggle");

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (btn),
				      is_enabled);
}

static void
at_enable_changed (GSettings *settings,
		   gchar       *key,
		   gpointer     user_data)
{
	at_enable_update (settings, user_data);
	close_logout_update (user_data);
}

static void
setup_dialog (GtkBuilder *builder, GSettings *settings)
{
	GtkWidget *widget;
	GObject *object;
	GObject *peditor;

	object = gtk_builder_get_object (builder, "at_enable_toggle");
	g_signal_connect (object, "toggled",
			  G_CALLBACK (at_enable_toggled),
			  builder);

	initial_state = g_settings_get_boolean (settings, ACCESSIBILITY_KEY);

	at_enable_update (settings, builder);

	g_signal_connect (settings, "changed::" ACCESSIBILITY_KEY, G_CALLBACK (at_enable_changed),
				 builder);

	object = gtk_builder_get_object (builder, "at_pref_button");
	g_signal_connect (object, "clicked",
			  G_CALLBACK (cb_at_preferences), NULL);

	object = gtk_builder_get_object (builder, "keyboard_button");
	g_signal_connect (object, "clicked",
			  G_CALLBACK (cb_keyboard_preferences), NULL);

	object = gtk_builder_get_object (builder, "mouse_button");
	g_signal_connect (object, "clicked",
			  G_CALLBACK (cb_mouse_preferences), NULL);

	object = gtk_builder_get_object (builder, "login_button");
	g_signal_connect (object, "clicked",
			  G_CALLBACK (cb_login_preferences), NULL);

	widget = GTK_WIDGET (gtk_builder_get_object (builder,
						     "at_properties_dialog"));
	capplet_set_icon (widget, "preferences-desktop-accessibility");

	g_signal_connect (G_OBJECT (widget),
			  "response",
			  G_CALLBACK (cb_dialog_response), NULL);

	gtk_widget_show (widget);
}

int
main (int argc, char *argv[])
{
	GSettings  *settings;
	GtkBuilder *builder;

	capplet_init (NULL, &argc, &argv);

	activate_settings_daemon ();

	settings = g_settings_new (ACCESSIBILITY_SCHEMA);
	builder = create_builder ();

	if (builder) {

		setup_dialog (builder, settings);

		gtk_main ();

		g_object_unref (builder);
		g_object_unref (settings);
	}

	return 0;
}
