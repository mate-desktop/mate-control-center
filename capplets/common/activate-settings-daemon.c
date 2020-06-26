#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include "activate-settings-daemon.h"

static void popup_error_message (void)
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING,
				   GTK_BUTTONS_OK, _("Unable to start the settings manager 'mate-settings-daemon'.\n"
				   "Without the MATE settings manager running, some preferences may not take effect. This could "
				   "indicate a problem with DBus, or a non-MATE (e.g. KDE) settings manager may already "
				   "be active and conflicting with the MATE settings manager."));

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

/* Returns FALSE if activation failed, else TRUE */
gboolean
activate_settings_daemon (void)
{
  GError     *error = NULL;
  GDBusProxy *proxy = NULL;
  GVariant *ret;

  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         NULL,
                                         "org.mate.SettingsDaemon",
                                         "/org/mate/SettingsDaemon",
                                         "org.mate.SettingsDaemon",
                                         NULL,
                                         &error);
  if (proxy == NULL) {
    popup_error_message ();
    g_error_free (error);
    return FALSE;
  }

  ret = g_dbus_proxy_call_sync (proxy,
                                "Awake",
                                g_variant_new ("()"),
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);
  if (ret == NULL) {
    popup_error_message ();
    g_error_free (error);
    return FALSE;
  } else {
    g_variant_get (ret, "()");
    g_variant_unref (ret);
  }

  return TRUE;
}
