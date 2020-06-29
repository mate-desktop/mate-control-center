/*
 * Copyright (C) 2007 The GNOME Foundation
 * Written by Thomas Wood <thos@gnome.org>
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

#include <glib/gi18n.h>
#include <libmate-desktop/mate-gsettings.h>
#include "appearance.h"
#include "appearance-desktop.h"
#include "appearance-font.h"
#include "appearance-themes.h"
#include "appearance-style.h"
#include "appearance-ui.h"
#include "appearance-support.h"
#include "theme-installer.h"
#include "theme-thumbnail.h"
#include "activate-settings-daemon.h"
#include "capplet-util.h"

static AppearanceData *
init_appearance_data (int *argc, char ***argv, GOptionContext *context)
{
  AppearanceData *data = NULL;
  GtkBuilder *ui;
  GError *err = NULL;

  theme_thumbnail_factory_init (*argc, *argv);
  capplet_init (context, argc, argv);
  activate_settings_daemon ();

  /* set up the data */
  ui = gtk_builder_new ();
  gtk_builder_add_from_resource (ui, "/org/mate/mcc/appearance/data/appearance.ui", &err);

  if (err)
    {
      g_warning (_("Could not load user interface file: %s"), err->message);
      g_error_free (err);
      g_object_unref (ui);
    }
  else
    {
      data = g_new (AppearanceData, 1);
      data->settings = g_settings_new (APPEARANCE_SCHEMA);
      data->wp_settings = g_settings_new (WP_SCHEMA);

      if (mate_gsettings_schema_exists (CAJA_SCHEMA))
        data->caja_settings = g_settings_new (CAJA_SCHEMA);
      else
        data->caja_settings = NULL;

      data->interface_settings = g_settings_new (INTERFACE_SCHEMA);
      data->marco_settings = g_settings_new (MARCO_SCHEMA);
      data->mouse_settings = g_settings_new (MOUSE_SCHEMA);
      data->font_settings = g_settings_new (FONT_RENDER_SCHEMA);
      data->ui = ui;
      data->thumb_factory = mate_desktop_thumbnail_factory_new (MATE_DESKTOP_THUMBNAIL_SIZE_NORMAL);
    }

  return data;
}

static void
main_window_response (GtkWidget *widget,
                      gint response_id,
                      AppearanceData *data)
{
  if (response_id == GTK_RESPONSE_CLOSE ||
      response_id == GTK_RESPONSE_DELETE_EVENT)
  {
    gtk_main_quit ();

    themes_shutdown (data);
    style_shutdown (data);
    desktop_shutdown (data);
    font_shutdown (data);
    support_shutdown (data);

    g_object_unref (data->thumb_factory);
    g_object_unref (data->settings);
    g_object_unref (data->wp_settings);

    if (data->caja_settings)
      g_object_unref (data->caja_settings);

    g_object_unref (data->interface_settings);
    g_object_unref (data->marco_settings);
    g_object_unref (data->mouse_settings);
    g_object_unref (data->font_settings);
    g_object_unref (data->ui);
  }
  else if (response_id == GTK_RESPONSE_HELP)
  {
      GtkNotebook *nb;
      gint pindex;

      nb = GTK_NOTEBOOK (appearance_capplet_get_widget (data, "main_notebook"));
      pindex = gtk_notebook_get_current_page (nb);

      switch (pindex)
      {
        case 0: /* theme */
          capplet_help (GTK_WINDOW (widget), "goscustdesk-12");
          break;
        case 1: /* background */
          capplet_help (GTK_WINDOW (widget), "goscustdesk-7");
          break;
        case 2: /* fonts */
          capplet_help (GTK_WINDOW (widget), "goscustdesk-38");
          break;
        case 3: /* interface */
          capplet_help (GTK_WINDOW (widget), "goscustuserinter-2");
          break;
        default:
          capplet_help (GTK_WINDOW (widget), "prefs-look-and-feel");
          break;
       }
  }
}

int
main (int argc, char **argv)
{
  AppearanceData *data;
  GtkWidget *w;
  GtkWidget *nb;
  GtkWidget *nb_custom_theme;
  GtkStyleContext *context;

  gchar *install_filename = NULL;
  gchar *start_page = NULL;
  gchar **wallpaper_files = NULL;
  GOptionContext *option_context;
  GOptionEntry option_entries[] = {
      { "install-theme",
        'i',
        G_OPTION_FLAG_IN_MAIN,
        G_OPTION_ARG_FILENAME,
        &install_filename,
        N_("Specify the filename of a theme to install"),
        N_("filename") },
      { "show-page",
        'p',
        G_OPTION_FLAG_IN_MAIN,
        G_OPTION_ARG_STRING,
        &start_page,
        /* TRANSLATORS: don't translate the terms in brackets */
        N_("Specify the name of the page to show (theme|background|fonts|interface)"),
        N_("page") },
      { G_OPTION_REMAINING,
      	0,
      	G_OPTION_FLAG_IN_MAIN,
      	G_OPTION_ARG_FILENAME_ARRAY,
      	&wallpaper_files,
      	NULL,
      	N_("[WALLPAPER...]") },
      { NULL }
    };

  option_context = g_option_context_new (NULL);
  g_option_context_add_main_entries (option_context, option_entries, GETTEXT_PACKAGE);

  /* init */
  data = init_appearance_data (&argc, &argv, option_context);
  if (!data)
    return 1;

  /* init tabs */
  themes_init (data);
  style_init (data);
  desktop_init (data, (const gchar **) wallpaper_files);
  g_strfreev (wallpaper_files);
  font_init (data);
  ui_init (data);

  /* init support for other window managers */
  support_init (data);

  /* prepare the main window */
  w = appearance_capplet_get_widget (data, "appearance_window");
  context = gtk_widget_get_style_context (GTK_WIDGET (w));
  gtk_style_context_add_class (context, "appearance-window");

  capplet_set_icon (w, "preferences-desktop-theme");
  gtk_widget_show_all (w);

  g_signal_connect_after (w, "response",
                          (GCallback) main_window_response, data);

  /* default to background page if files were given on the command line */
  if (wallpaper_files && !install_filename && !start_page)
    start_page = g_strdup ("background");

  nb = appearance_capplet_get_widget (data, "main_notebook");
  gtk_widget_add_events (nb, GDK_SCROLL_MASK);
  g_signal_connect (nb,
                    "scroll-event",
                    G_CALLBACK (capplet_notebook_scroll_event_cb),
                    NULL);

  if (start_page != NULL) {
    gchar *page_name;

    page_name = g_strconcat (start_page, "_vbox", NULL);
    g_free (start_page);

    w = appearance_capplet_get_widget (data, page_name);
    if (w != NULL) {
      gint pindex;
      GtkNotebook *notebook;

      notebook = GTK_NOTEBOOK (nb);
      pindex = gtk_notebook_page_num (notebook, w);
      if (pindex != -1)
        gtk_notebook_set_current_page (notebook, pindex);
    }
    g_free (page_name);
  }

  if (install_filename != NULL) {
    GFile *inst = g_file_new_for_commandline_arg (install_filename);
    g_free (install_filename);
    mate_theme_install (inst, GTK_WINDOW (w));
    g_object_unref (inst);
  }

  g_option_context_free (option_context);

  nb_custom_theme = appearance_capplet_get_widget (data, "notebook2");
  gtk_widget_add_events (nb_custom_theme, GDK_SCROLL_MASK);
  g_signal_connect (nb_custom_theme,
                    "scroll-event",
                    G_CALLBACK (capplet_notebook_scroll_event_cb),
                    NULL);

  /* start the mainloop */
  gtk_main ();

  /* free stuff */
  g_free (data);

  return 0;
}
