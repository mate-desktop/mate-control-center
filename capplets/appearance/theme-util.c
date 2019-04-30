/*
 * Copyright (C) 2007 The GNOME Foundation
 * Written by Thomas Wood <thos@gnome.org>
 *            Jens Granseuer <jensgr@gmx.net>
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "appearance.h"

#include <string.h>
#include <glib/gi18n.h>

#include "capplet-util.h"
#include "theme-util.h"

gboolean theme_is_writable (const gpointer theme)
{
  MateThemeCommonInfo *info = theme;
  GFile *file;
  GFileInfo *file_info;
  gboolean writable;

  if (info == NULL || info->path == NULL)
    return FALSE;

  file = g_file_new_for_path (info->path);
  file_info = g_file_query_info (file,
                                 G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE,
                                 G_FILE_QUERY_INFO_NONE,
                                 NULL, NULL);
  g_object_unref (file);

  if (file_info == NULL)
    return FALSE;

  writable = g_file_info_get_attribute_boolean (file_info,
                                                G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE);
  g_object_unref (file_info);

  return writable;
}

gboolean theme_delete (const gchar *name, ThemeType type)
{
  gboolean rc;
  GtkDialog *dialog;
  gchar *theme_dir;
  gint response;
  MateThemeCommonInfo *theme;
  GFile *dir;
  gboolean del_empty_parent;

  dialog = (GtkDialog *) gtk_message_dialog_new (NULL,
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_QUESTION,
						 GTK_BUTTONS_CANCEL,
						 _("Would you like to delete this theme?"));
  gtk_dialog_add_button (dialog, "gtk-delete", GTK_RESPONSE_ACCEPT);
  response = gtk_dialog_run (dialog);
  gtk_widget_destroy (GTK_WIDGET (dialog));
  if (response != GTK_RESPONSE_ACCEPT)
    return FALSE;

  /* Most theme types are put into separate subdirectories. For those
     we want to delete those directories as well. */
  del_empty_parent = TRUE;

  switch (type) {
    case THEME_TYPE_GTK:
      theme = (MateThemeCommonInfo *) mate_theme_info_find (name);
      theme_dir = g_build_filename (theme->path, "gtk-2.0", NULL);
      break;

    case THEME_TYPE_ICON:
      theme = (MateThemeCommonInfo *) mate_theme_icon_info_find (name);
      theme_dir = g_path_get_dirname (theme->path);
      del_empty_parent = FALSE;
      break;

    case THEME_TYPE_WINDOW:
      theme = (MateThemeCommonInfo *) mate_theme_info_find (name);
      theme_dir = g_build_filename (theme->path, "metacity-1", NULL);
      break;

    case THEME_TYPE_META:
      theme = (MateThemeCommonInfo *) mate_theme_meta_info_find (name);
      theme_dir = g_path_get_dirname (theme->path);
      del_empty_parent = FALSE;
      break;

    case THEME_TYPE_CURSOR:
      theme = (MateThemeCommonInfo *) mate_theme_cursor_info_find (name);
      theme_dir = g_build_filename (theme->path, "cursors", NULL);
      break;

    default:
      return FALSE;
  }

  dir = g_file_new_for_path (theme_dir);
  g_free (theme_dir);

  if (!capplet_file_delete_recursive (dir, NULL)) {
    GtkWidget *info_dialog = gtk_message_dialog_new (NULL,
						     GTK_DIALOG_MODAL,
						     GTK_MESSAGE_ERROR,
						     GTK_BUTTONS_OK,
						     _("Theme cannot be deleted"));
    gtk_dialog_run (GTK_DIALOG (info_dialog));
    gtk_widget_destroy (info_dialog);
    rc = FALSE;
  } else {
    if (del_empty_parent) {
      /* also delete empty parent directories */
      GFile *parent = g_file_get_parent (dir);
      g_file_delete (parent, NULL, NULL);
      g_object_unref (parent);
    }
    rc = TRUE;
  }

  g_object_unref (dir);
  return rc;
}

gboolean theme_model_iter_last (GtkTreeModel *model, GtkTreeIter *iter)
{
  GtkTreeIter walk, prev;
  gboolean valid;

  valid = gtk_tree_model_get_iter_first (model, &walk);

  if (valid) {
    do {
      prev = walk;
      valid = gtk_tree_model_iter_next (model, &walk);
    } while (valid);

    *iter = prev;
    return TRUE;
  }
  return FALSE;
}

gboolean theme_find_in_model (GtkTreeModel *model, const gchar *name, GtkTreeIter *iter)
{
  GtkTreeIter walk;
  gboolean valid;
  gchar *test;

  if (!name)
    return FALSE;

  for (valid = gtk_tree_model_get_iter_first (model, &walk); valid;
       valid = gtk_tree_model_iter_next (model, &walk))
  {
    gtk_tree_model_get (model, &walk, COL_NAME, &test, -1);

    if (test) {
      gint cmp = strcmp (test, name);
      g_free (test);

      if (!cmp) {
      	if (iter)
          *iter = walk;
        return TRUE;
      }
    }
  }

  return FALSE;
}

gboolean packagekit_available (void)
{
  GDBusConnection *connection;
  GDBusProxy *proxy;
  gboolean available = FALSE;
  GError   *error = NULL;
  GVariant *variant;

  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  if (connection == NULL) {
    return FALSE;
  }

  proxy = g_dbus_proxy_new_sync (connection,
                                 G_DBUS_PROXY_FLAGS_NONE,
                                 NULL,
                                 "org.freedesktop.DBus",
                                 "/org/freedesktop/DBus",
                                 "org.freedesktop.DBus",
                                 NULL,
                                 NULL);
  if (proxy == NULL) {
    g_object_unref (connection);
    return FALSE;
  }

  variant = g_dbus_proxy_call_sync (proxy, "NameHasOwner",
                                    g_variant_new ("(s)", "org.freedesktop.PackageKit"),
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);
  if (variant == NULL) {
    g_warning ("Could not ask org.freedesktop.DBus if PackageKit is available: %s",
               error->message);
    g_error_free (error);
    g_object_unref (proxy);
    g_object_unref (connection);

    return FALSE;
  } else {
    g_variant_get (variant, "(b)", &available);
    g_variant_unref (variant);
  }

  g_object_unref (proxy);
  g_object_unref (connection);

  return available;
}

void theme_install_file(GtkWindow* parent, const gchar* path)
{
  GDBusConnection *connection;
  GDBusProxy *proxy;
  GError* error = NULL;
  GVariant *variant;

  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);

  if (connection == NULL)
  {
    g_warning("Could not get session bus: %s", error->message);
    g_error_free (error);
    return;
  }

  proxy = g_dbus_proxy_new_sync (connection,
                                 G_DBUS_PROXY_FLAGS_NONE,
                                 NULL,
                                 "org.freedesktop.PackageKit",
                                 "/org/freedesktop/PackageKit",
                                 "org.freedesktop.PackageKit",
                                 NULL,
                                 &error);
  if (proxy == NULL) {
    g_warning ("Failed to connect to PackageKit: %s\n", error->message);
    g_error_free (error);
    g_object_unref (connection);
    return;
  }

  variant = g_dbus_proxy_call_sync (proxy, "InstallProvideFile",
                                    g_variant_new ("(s)", path),
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

  if (variant == NULL) {
    GtkWidget* dialog = gtk_message_dialog_new(NULL,
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR,
                                               GTK_BUTTONS_OK,
                                               _("Could not install theme engine"));
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG (dialog), "%s", error->message);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_error_free(error);
  } else {
    g_variant_unref (variant);
  }

  g_object_unref(proxy);
  g_object_unref (connection);
}
