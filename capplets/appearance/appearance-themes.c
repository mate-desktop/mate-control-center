/*
 * Copyright (C) 2007, 2010 The GNOME Foundation
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
#include "theme-thumbnail.h"
#include "mate-theme-apply.h"
#include "theme-installer.h"
#include "theme-save.h"
#include "theme-util.h"
#include "gtkrc-utils.h"

#include <glib/gi18n.h>
#include <libwindow-settings/mate-wm-manager.h>
#include <string.h>
#include <libmate-desktop/mate-desktop-thumbnail.h>
#include <libmate-desktop/mate-gsettings.h>

#define CUSTOM_THEME_NAME "__custom__"

enum {
	RESPONSE_APPLY_BG,
	RESPONSE_REVERT_FONT,
	RESPONSE_APPLY_FONT,
	RESPONSE_INSTALL_ENGINE
};

enum {
	TARGET_URI_LIST,
	TARGET_NS_URL
};

static const GtkTargetEntry drop_types[] =
{
	{"text/uri-list", 0, TARGET_URI_LIST},
	{"_NETSCAPE_URL", 0, TARGET_NS_URL}
};

static void theme_message_area_update(AppearanceData* data);

static time_t theme_get_mtime(const char* name)
{
	MateThemeMetaInfo* theme;
	time_t mtime = -1;

	theme = mate_theme_meta_info_find(name);
	if (theme != NULL)
	{
		GFile* file;
		GFileInfo* file_info;

		file = g_file_new_for_path(theme->path);
		file_info = g_file_query_info(file, G_FILE_ATTRIBUTE_TIME_MODIFIED, G_FILE_QUERY_INFO_NONE, NULL, NULL);
		g_object_unref(file);

		if (file_info != NULL)
		{
			mtime = g_file_info_get_attribute_uint64(file_info, G_FILE_ATTRIBUTE_TIME_MODIFIED);
			g_object_unref(file_info);
		}
	}

	return mtime;
}

static void theme_thumbnail_update(GdkPixbuf* pixbuf, gchar* theme_name, AppearanceData* data, gboolean cache)
{
	GtkTreeIter iter;
	GtkTreeModel* model = GTK_TREE_MODEL(data->theme_store);

	/* find item in model and update thumbnail */
	if (!pixbuf)
		return;

	if (theme_find_in_model(model, theme_name, &iter))
	{
		time_t mtime;

		gtk_list_store_set(data->theme_store, &iter, COL_THUMBNAIL, pixbuf, -1);

		/* cache thumbnail */
		if (cache && (mtime = theme_get_mtime(theme_name)) != -1)
		{
			gchar* path;

			/* try to share thumbs with caja, use themes:/// */
			path = g_strconcat("themes:///", theme_name, NULL);

			mate_desktop_thumbnail_factory_save_thumbnail(data->thumb_factory, pixbuf, path, mtime);

			g_free(path);
		}
	}
}

static GdkPixbuf* theme_get_thumbnail_from_cache(MateThemeMetaInfo* info, AppearanceData* data)
{
	GdkPixbuf* thumb = NULL;
	gchar* path, *thumb_filename;
	time_t mtime;

	if (info == data->theme_custom)
		return NULL;

	mtime = theme_get_mtime(info->name);

	if (mtime == -1)
		return NULL;

	/* try to share thumbs with caja, use themes:/// */
	path = g_strconcat ("themes:///", info->name, NULL);
	thumb_filename = mate_desktop_thumbnail_factory_lookup(data->thumb_factory, path, mtime);
	g_free(path);

	if (thumb_filename != NULL)
	{
		thumb = gdk_pixbuf_new_from_file(thumb_filename, NULL);
		g_free(thumb_filename);
	}

	return thumb;
}

static void
theme_thumbnail_done_cb (GdkPixbuf *pixbuf, gchar *theme_name, AppearanceData *data)
{
  theme_thumbnail_update (pixbuf, theme_name, data, TRUE);
}

static void theme_thumbnail_generate(MateThemeMetaInfo* info, AppearanceData* data)
{
	GdkPixbuf* thumb = theme_get_thumbnail_from_cache(info, data);

	if (thumb != NULL)
	{
		theme_thumbnail_update(thumb, info->name, data, FALSE);
		g_object_unref(thumb);
	}
	else
	{
		generate_meta_theme_thumbnail_async(info, (ThemeThumbnailFunc) theme_thumbnail_done_cb, data, NULL);
	}
}

static void theme_changed_on_disk_cb(MateThemeCommonInfo* theme, MateThemeChangeType change_type, MateThemeElement element_type, AppearanceData* data)
{
	if (theme->type == MATE_THEME_TYPE_METATHEME)
	{
		MateThemeMetaInfo* meta = (MateThemeMetaInfo*) theme;

		if (change_type == MATE_THEME_CHANGE_CREATED)
		{
			gtk_list_store_insert_with_values (data->theme_store, NULL, 0, COL_LABEL, meta->readable_name, COL_NAME, meta->name, COL_THUMBNAIL, data->theme_icon, -1);
			theme_thumbnail_generate(meta, data);
		}
		else if (change_type == MATE_THEME_CHANGE_DELETED)
		{
			GtkTreeIter iter;

			if (theme_find_in_model(GTK_TREE_MODEL(data->theme_store), meta->name, &iter))
			{
				gtk_list_store_remove(data->theme_store, &iter);
			}
		}
		else if (change_type == MATE_THEME_CHANGE_CHANGED)
		{
			theme_thumbnail_generate(meta, data);
		}
	}
}

/* Find out if the lockdown key has been set. */
static gboolean is_locked_down()
{
  gboolean is_locked;
  GSettings *settings;
  settings = g_settings_new (LOCKDOWN_SCHEMA);
  is_locked = g_settings_get_boolean (settings, DISABLE_THEMES_SETTINGS_KEY);
  g_object_unref (settings);
  return is_locked;
}

static MateThemeMetaInfo *
theme_load_from_gsettings (AppearanceData *data)
{
  MateThemeMetaInfo *theme;
  gchar *scheme;

  theme = mate_theme_meta_info_new ();

  theme->gtk_theme_name = g_settings_get_string (data->interface_settings, GTK_THEME_KEY);
  if (theme->gtk_theme_name == NULL)
    theme->gtk_theme_name = g_strdup ("Menta");

  scheme = g_settings_get_string (data->interface_settings, COLOR_SCHEME_KEY);
  if (scheme == NULL || !strcmp (scheme, "")) {
    g_free (scheme);
    scheme = gtkrc_get_color_scheme_for_theme (theme->gtk_theme_name);
  }
  theme->gtk_color_scheme = scheme;

  theme->marco_theme_name = g_settings_get_string (data->marco_settings, MARCO_THEME_KEY);
  if (theme->marco_theme_name == NULL)
    theme->marco_theme_name = g_strdup ("Menta");

  theme->icon_theme_name = g_settings_get_string (data->interface_settings, ICON_THEME_KEY);
  if (theme->icon_theme_name == NULL)
    theme->icon_theme_name = g_strdup ("menta");

  if (mate_gsettings_schema_exists (NOTIFICATION_SCHEMA)) {
    GSettings *notification_settings;
    notification_settings = g_settings_new (NOTIFICATION_SCHEMA);
    theme->notification_theme_name = g_settings_get_string (notification_settings, NOTIFICATION_THEME_KEY);
    g_object_unref (notification_settings);
  }
  else
    theme->notification_theme_name = NULL;

  theme->cursor_theme_name = g_settings_get_string (data->mouse_settings, CURSOR_THEME_KEY);
  theme->cursor_size = g_settings_get_int (data->mouse_settings, CURSOR_SIZE_KEY);

  if (theme->cursor_theme_name == NULL)
    theme->cursor_theme_name = g_strdup ("default");

  theme->application_font = g_settings_get_string (data->interface_settings, GTK_FONT_KEY);

  return theme;
}

static gchar *
theme_get_selected_name (GtkIconView *icon_view, AppearanceData *data)
{
  gchar *name = NULL;
  GList *selected = gtk_icon_view_get_selected_items (icon_view);

  if (selected) {
    GtkTreePath *path = selected->data;
    GtkTreeModel *model = gtk_icon_view_get_model (icon_view);
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter (model, &iter, path))
      gtk_tree_model_get (model, &iter, COL_NAME, &name, -1);

    g_list_foreach (selected, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (selected);
  }

  return name;
}

static const MateThemeMetaInfo *
theme_get_selected (GtkIconView *icon_view, AppearanceData *data)
{
  MateThemeMetaInfo *theme = NULL;
  gchar *name = theme_get_selected_name (icon_view, data);

  if (name != NULL) {
    if (!strcmp (name, data->theme_custom->name)) {
      theme = data->theme_custom;
    } else {
      theme = mate_theme_meta_info_find (name);
    }

    g_free (name);
  }

  return theme;
}

static void
theme_select_iter (GtkIconView *icon_view, GtkTreeIter *iter)
{
  GtkTreePath *path;

  path = gtk_tree_model_get_path (gtk_icon_view_get_model (icon_view), iter);
  gtk_icon_view_select_path (icon_view, path);
  gtk_icon_view_scroll_to_path (icon_view, path, FALSE, 0.5, 0.0);
  gtk_tree_path_free (path);
}

static void
theme_select_name (GtkIconView *icon_view, const gchar *theme)
{
  GtkTreeIter iter;
  GtkTreeModel *model = gtk_icon_view_get_model (icon_view);

  if (theme_find_in_model (model, theme, &iter))
    theme_select_iter (icon_view, &iter);
}

static gboolean
theme_is_equal (const MateThemeMetaInfo *a, const MateThemeMetaInfo *b)
{
  gboolean a_set, b_set;

  if (!(a->gtk_theme_name && b->gtk_theme_name) ||
      strcmp (a->gtk_theme_name, b->gtk_theme_name))
    return FALSE;

  if (!(a->icon_theme_name && b->icon_theme_name) ||
      strcmp (a->icon_theme_name, b->icon_theme_name))
    return FALSE;

  if (!(a->marco_theme_name && b->marco_theme_name) ||
      strcmp (a->marco_theme_name, b->marco_theme_name))
    return FALSE;

  if (!(a->cursor_theme_name && b->cursor_theme_name) ||
      strcmp (a->cursor_theme_name, b->cursor_theme_name))
    return FALSE;

  if (a->cursor_size != b->cursor_size)
    return FALSE;

  a_set = a->gtk_color_scheme && strcmp (a->gtk_color_scheme, "");
  b_set = b->gtk_color_scheme && strcmp (b->gtk_color_scheme, "");
  if ((a_set != b_set) ||
      (a_set && !mate_theme_color_scheme_equal (a->gtk_color_scheme, b->gtk_color_scheme)))
    return FALSE;

  return TRUE;
}

static void
theme_set_custom_from_theme (const MateThemeMetaInfo *info, AppearanceData *data)
{
  MateThemeMetaInfo *custom = data->theme_custom;
  GtkIconView *icon_view = GTK_ICON_VIEW (appearance_capplet_get_widget (data, "theme_list"));
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreePath *path;

  if (info == custom)
    return;

  /* if info is not NULL, we'll copy those theme settings over */
  if (info != NULL) {
    g_free (custom->gtk_theme_name);
    g_free (custom->icon_theme_name);
    g_free (custom->marco_theme_name);
    g_free (custom->gtk_color_scheme);
    g_free (custom->cursor_theme_name);
    g_free (custom->application_font);
    custom->gtk_color_scheme = NULL;
    custom->application_font = NULL;

    /* these settings are guaranteed to be non-NULL */
    custom->gtk_theme_name = g_strdup (info->gtk_theme_name);
    custom->icon_theme_name = g_strdup (info->icon_theme_name);
    custom->marco_theme_name = g_strdup (info->marco_theme_name);
    custom->cursor_theme_name = g_strdup (info->cursor_theme_name);
    custom->cursor_size = info->cursor_size;

    /* these can be NULL */
    if (info->gtk_color_scheme)
      custom->gtk_color_scheme = g_strdup (info->gtk_color_scheme);
    else
      custom->gtk_color_scheme = g_strdup ("");

    if (info->application_font)
      custom->application_font = g_strdup (info->application_font);
    else
      custom->application_font = g_strdup (GTK_FONT_DEFAULT_VALUE);
  }

  /* select the custom theme */
  model = gtk_icon_view_get_model (icon_view);
  if (!theme_find_in_model (model, custom->name, &iter)) {
    GtkTreeIter child;

    gtk_list_store_insert_with_values (data->theme_store, &child, 0,
        COL_LABEL, custom->readable_name,
        COL_NAME, custom->name,
        COL_THUMBNAIL, data->theme_icon,
        -1);
    gtk_tree_model_sort_convert_child_iter_to_iter (
        GTK_TREE_MODEL_SORT (model), &iter, &child);
  }

  path = gtk_tree_model_get_path (model, &iter);
  gtk_icon_view_select_path (icon_view, path);
  gtk_icon_view_scroll_to_path (icon_view, path, FALSE, 0.5, 0.0);
  gtk_tree_path_free (path);

  /* update the theme thumbnail */
  theme_thumbnail_generate (custom, data);
}

/** GUI Callbacks **/

static void custom_font_cb(GtkWidget* button, AppearanceData* data)
{
  g_free(data->revert_application_font);
  g_free(data->revert_documents_font);
  g_free(data->revert_desktop_font);
  g_free(data->revert_windowtitle_font);
  g_free(data->revert_monospace_font);
  data->revert_application_font = NULL;
  data->revert_documents_font = NULL;
  data->revert_desktop_font = NULL;
  data->revert_windowtitle_font = NULL;
  data->revert_monospace_font = NULL;
}

static void
theme_message_area_response_cb (GtkWidget *w,
                                gint response_id,
                                AppearanceData *data)
{
  const MateThemeMetaInfo *theme;
  gchar *tmpfont;
  gchar *engine_path;

  theme = theme_get_selected (GTK_ICON_VIEW (appearance_capplet_get_widget (data, "theme_list")), data);
  if (!theme)
    return;

  switch (response_id)
  {
    case RESPONSE_APPLY_BG:
      g_settings_set_string (data->wp_settings, WP_FILE_KEY, theme->background_image);
      break;

    case RESPONSE_REVERT_FONT:
      if (data->revert_application_font != NULL) {
        g_settings_set_string (data->interface_settings, GTK_FONT_KEY, data->revert_application_font);
        g_free (data->revert_application_font);
        data->revert_application_font = NULL;
      }

      if (data->revert_documents_font != NULL) {
        g_settings_set_string (data->interface_settings, DOCUMENT_FONT_KEY, data->revert_documents_font);
        g_free (data->revert_documents_font);
        data->revert_documents_font = NULL;
      }

      if (data->caja_settings && data->revert_desktop_font != NULL) {
        g_settings_set_string (data->caja_settings, DESKTOP_FONT_KEY, data->revert_desktop_font);
        g_free (data->revert_desktop_font);
        data->revert_desktop_font = NULL;
      }

      if (data->revert_windowtitle_font != NULL) {
        g_settings_set_string (data->marco_settings, WINDOW_TITLE_FONT_KEY, data->revert_windowtitle_font);
        g_free (data->revert_windowtitle_font);
        data->revert_windowtitle_font = NULL;
      }

      if (data->revert_monospace_font != NULL) {
        g_settings_set_string (data->interface_settings, MONOSPACE_FONT_KEY, data->revert_monospace_font);
        g_free (data->revert_monospace_font);
        data->revert_monospace_font = NULL;
      }
      break;

    case RESPONSE_APPLY_FONT:
      if (theme->application_font) {
        tmpfont = g_settings_get_string (data->interface_settings, GTK_FONT_KEY);
        if (tmpfont != NULL) {
          g_free (data->revert_application_font);

          if (strcmp (theme->application_font, tmpfont) == 0) {
            g_free (tmpfont);
            data->revert_application_font = NULL;
          } else
            data->revert_application_font = tmpfont;
        }
        g_settings_set_string (data->interface_settings, GTK_FONT_KEY, theme->application_font);
      }

      if (theme->documents_font) {
        tmpfont = g_settings_get_string (data->interface_settings, DOCUMENT_FONT_KEY);
        if (tmpfont != NULL) {
          g_free (data->revert_documents_font);

          if (strcmp (theme->documents_font, tmpfont) == 0) {
            g_free (tmpfont);
            data->revert_documents_font = NULL;
          } else
            data->revert_documents_font = tmpfont;
        }
        g_settings_set_string (data->interface_settings, DOCUMENT_FONT_KEY, theme->documents_font);
      }

      if (data->caja_settings && theme->desktop_font) {
        tmpfont = g_settings_get_string (data->caja_settings, DESKTOP_FONT_KEY);
        if (tmpfont != NULL) {
          g_free (data->revert_desktop_font);

          if (strcmp (theme->desktop_font, tmpfont) == 0) {
            g_free (tmpfont);
            data->revert_desktop_font = NULL;
          } else
            data->revert_desktop_font = tmpfont;
        }
        g_settings_set_string (data->caja_settings, DESKTOP_FONT_KEY, theme->desktop_font);
      }

      if (theme->windowtitle_font) {
        tmpfont = g_settings_get_string (data->marco_settings, WINDOW_TITLE_FONT_KEY);
        if (tmpfont != NULL) {
          g_free (data->revert_windowtitle_font);

          if (strcmp (theme->windowtitle_font, tmpfont) == 0) {
            g_free (tmpfont);
            data->revert_windowtitle_font = NULL;
          } else
            data->revert_windowtitle_font = tmpfont;
        }
        g_settings_set_string (data->marco_settings, WINDOW_TITLE_FONT_KEY, theme->windowtitle_font);
      }

      if (theme->monospace_font) {
        tmpfont = g_settings_get_string (data->interface_settings, MONOSPACE_FONT_KEY);
        if (tmpfont != NULL) {
          g_free (data->revert_monospace_font);

          if (strcmp (theme->monospace_font, tmpfont) == 0) {
            g_free (tmpfont);
            data->revert_monospace_font = NULL;
          } else
            data->revert_monospace_font = tmpfont;
        }
        g_settings_set_string (data->interface_settings, MONOSPACE_FONT_KEY, theme->monospace_font);
      }
      break;

    case RESPONSE_INSTALL_ENGINE:
      engine_path = gtk_theme_info_missing_engine(theme->gtk_theme_name, FALSE);

      if (engine_path != NULL) {
        theme_install_file(GTK_WINDOW(gtk_widget_get_toplevel(data->install_button)), engine_path);
        g_free (engine_path);
      }

      theme_message_area_update(data);
      break;
  }
}

static void
theme_message_area_update (AppearanceData *data)
{
  const MateThemeMetaInfo *theme;
  gboolean show_apply_background = FALSE;
  gboolean show_apply_font = FALSE;
  gboolean show_revert_font = FALSE;
  gboolean show_error;
  const gchar *message;
  gchar *font;
  GError *error = NULL;

  theme = theme_get_selected (GTK_ICON_VIEW (appearance_capplet_get_widget (data, "theme_list")), data);

  if (!theme) {
    if (data->theme_message_area != NULL)
      gtk_widget_hide (data->theme_message_area);
    return;
  }

  show_error = !mate_theme_meta_info_validate (theme, &error);

  if (!show_error) {
    if (theme->background_image != NULL) {
      gchar *background;

      background = g_settings_get_string (data->wp_settings, WP_FILE_KEY);
      show_apply_background =
          (!background || strcmp (theme->background_image, background) != 0);
      g_free (background);
    }

    if (theme->application_font) {
      font = g_settings_get_string (data->interface_settings, GTK_FONT_KEY);
      show_apply_font =
          (!font || strcmp (theme->application_font, font) != 0);
      g_free (font);
    }

    if (!show_apply_font && theme->documents_font) {
      font = g_settings_get_string (data->interface_settings, DOCUMENT_FONT_KEY);
      show_apply_font =
          (!font || strcmp (theme->application_font, font) != 0);
      g_free (font);
    }

    if (data->caja_settings && !show_apply_font && theme->desktop_font) {
      font = g_settings_get_string (data->caja_settings, DESKTOP_FONT_KEY);
      show_apply_font =
          (!font || strcmp (theme->application_font, font) != 0);
      g_free (font);
    }

    if (!show_apply_font && theme->windowtitle_font) {
      font = g_settings_get_string (data->marco_settings, WINDOW_TITLE_FONT_KEY);
      show_apply_font =
          (!font || strcmp (theme->application_font, font) != 0);
      g_free (font);
    }

    if (!show_apply_font && theme->monospace_font) {
      font = g_settings_get_string (data->interface_settings, MONOSPACE_FONT_KEY);
      show_apply_font =
          (!font || strcmp (theme->application_font, font) != 0);
      g_free (font);
    }

    show_revert_font = (data->revert_application_font != NULL ||
      data->revert_documents_font != NULL || data->revert_desktop_font != NULL ||
      data->revert_windowtitle_font != NULL || data->revert_monospace_font != NULL);
  }

  if (data->theme_message_area == NULL) {
    GtkWidget *hbox;
    GtkWidget *parent;
    GtkWidget *content;

    if (!show_apply_background && !show_revert_font && !show_apply_font && !show_error)
      return;

    data->theme_message_area = gtk_info_bar_new ();
    gtk_widget_set_no_show_all (data->theme_message_area, TRUE);

    g_signal_connect (data->theme_message_area, "response",
                      (GCallback) theme_message_area_response_cb, data);

    data->apply_background_button = gtk_info_bar_add_button (
        GTK_INFO_BAR (data->theme_message_area),
        _("Apply Background"),
        RESPONSE_APPLY_BG);
    data->apply_font_button = gtk_info_bar_add_button (
        GTK_INFO_BAR (data->theme_message_area),
        _("Apply Font"),
        RESPONSE_APPLY_FONT);
    data->revert_font_button = gtk_info_bar_add_button (
        GTK_INFO_BAR (data->theme_message_area),
        _("Revert Font"),
        RESPONSE_REVERT_FONT);
    data->install_button = gtk_info_bar_add_button (
        GTK_INFO_BAR (data->theme_message_area),
        _("Install"),
        RESPONSE_INSTALL_ENGINE);

    data->theme_message_label = gtk_label_new (NULL);
    gtk_widget_show (data->theme_message_label);
    gtk_label_set_line_wrap (GTK_LABEL (data->theme_message_label), TRUE);
    gtk_label_set_xalign (GTK_LABEL (data->theme_message_label), 0.0);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 9);
    gtk_widget_show (hbox);
    data->theme_info_icon = gtk_image_new_from_icon_name ("dialog-information", GTK_ICON_SIZE_DIALOG);
    data->theme_error_icon = gtk_image_new_from_icon_name ("dialog-warning", GTK_ICON_SIZE_DIALOG);
    gtk_widget_set_halign (data->theme_info_icon, GTK_ALIGN_CENTER);
    gtk_widget_set_valign (data->theme_info_icon, GTK_ALIGN_START);
    gtk_widget_set_halign (data->theme_error_icon, GTK_ALIGN_CENTER);
    gtk_widget_set_valign (data->theme_error_icon, GTK_ALIGN_START);
    gtk_box_pack_start (GTK_BOX (hbox), data->theme_info_icon, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), data->theme_error_icon, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), data->theme_message_label, TRUE, TRUE, 0);
    content = gtk_info_bar_get_content_area (GTK_INFO_BAR (data->theme_message_area));
    gtk_container_add (GTK_CONTAINER (content), hbox);

    parent = appearance_capplet_get_widget (data, "theme_list_vbox");
    gtk_box_pack_start (GTK_BOX (parent), data->theme_message_area, FALSE, FALSE, 0);
  }

  if (show_error)
    message = error->message;
  else if (show_apply_background && show_apply_font && show_revert_font)
    message = _("The current theme suggests a background and a font. Also, the last applied font suggestion can be reverted.");
  else if (show_apply_background && show_revert_font)
    message = _("The current theme suggests a background. Also, the last applied font suggestion can be reverted.");
  else if (show_apply_background && show_apply_font)
    message = _("The current theme suggests a background and a font.");
  else if (show_apply_font && show_revert_font)
    message = _("The current theme suggests a font. Also, the last applied font suggestion can be reverted.");
  else if (show_apply_background)
    message = _("The current theme suggests a background.");
  else if (show_revert_font)
    message = _("The last applied font suggestion can be reverted.");
  else if (show_apply_font)
    message = _("The current theme suggests a font.");
  else
    message = NULL;

  if (show_apply_background)
    gtk_widget_show (data->apply_background_button);
  else
    gtk_widget_hide (data->apply_background_button);

  if (show_apply_font)
    gtk_widget_show (data->apply_font_button);
  else
    gtk_widget_hide (data->apply_font_button);

  if (show_revert_font)
    gtk_widget_show (data->revert_font_button);
  else
    gtk_widget_hide (data->revert_font_button);

  if (show_error
      && g_error_matches (error, MATE_THEME_ERROR, MATE_THEME_ERROR_GTK_ENGINE_NOT_AVAILABLE)
      && packagekit_available ())
    gtk_widget_show (data->install_button);
  else
    gtk_widget_hide (data->install_button);

  if (show_error || show_apply_background || show_apply_font || show_revert_font) {
    gtk_widget_show (data->theme_message_area);
    gtk_widget_queue_draw (data->theme_message_area);

    if (show_error) {
      gtk_widget_show (data->theme_error_icon);
      gtk_widget_hide (data->theme_info_icon);
    } else {
      gtk_widget_show (data->theme_info_icon);
      gtk_widget_hide (data->theme_error_icon);
    }
  } else {
    gtk_widget_hide (data->theme_message_area);
  }

  gtk_label_set_text (GTK_LABEL (data->theme_message_label), message);
  g_clear_error (&error);
}

static void
theme_selection_changed_cb (GtkWidget *icon_view, AppearanceData *data)
{
  GList *selection;
  MateThemeMetaInfo *theme = NULL;
  gboolean is_custom = FALSE;

  selection = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (icon_view));

  if (selection) {
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *name;

    model = gtk_icon_view_get_model (GTK_ICON_VIEW (icon_view));
    gtk_tree_model_get_iter (model, &iter, selection->data);
    gtk_tree_model_get (model, &iter, COL_NAME, &name, -1);

    is_custom = !strcmp (name, CUSTOM_THEME_NAME);

    if (is_custom)
      theme = data->theme_custom;
    else
      theme = mate_theme_meta_info_find (name);

    if (theme) {
      mate_meta_theme_set (theme);
      theme_message_area_update (data);
    }

    g_free (name);
    g_list_foreach (selection, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (selection);

    gtk_widget_set_sensitive (appearance_capplet_get_widget (data, "theme_delete"),
			    theme_is_writable (theme));
    gtk_widget_set_sensitive (appearance_capplet_get_widget (data, "theme_save"), is_custom);
  }
}

static void
theme_custom_cb (GtkWidget *button, AppearanceData *data)
{
  GtkWidget *w, *parent;

  w = appearance_capplet_get_widget (data, "theme_details");
  parent = appearance_capplet_get_widget (data, "appearance_window");
  gtk_window_set_transient_for (GTK_WINDOW (w), GTK_WINDOW (parent));
  gtk_widget_show_all (w);
}

static void
theme_save_cb (GtkWidget *button, AppearanceData *data)
{
  theme_save_dialog_run (data->theme_custom, data);
}

static void
theme_install_cb (GtkWidget *button, AppearanceData *data)
{
  mate_theme_installer_run (
      GTK_WINDOW (appearance_capplet_get_widget (data, "appearance_window")));
}

static void
theme_delete_cb (GtkWidget *button, AppearanceData *data)
{
  GtkIconView *icon_view = GTK_ICON_VIEW (appearance_capplet_get_widget (data, "theme_list"));
  GList *selected = gtk_icon_view_get_selected_items (icon_view);

  if (selected) {
    GtkTreePath *path = selected->data;
    GtkTreeModel *model = gtk_icon_view_get_model (icon_view);
    GtkTreeIter iter;
    gchar *name = NULL;

    if (gtk_tree_model_get_iter (model, &iter, path))
      gtk_tree_model_get (model, &iter, COL_NAME, &name, -1);

    if (name != NULL &&
        strcmp (name, data->theme_custom->name) &&
        theme_delete (name, THEME_TYPE_META)) {
      /* remove theme from the model, too */
      GtkTreeIter child;

      if (gtk_tree_model_iter_next (model, &iter) ||
          theme_model_iter_last (model, &iter))
        theme_select_iter (icon_view, &iter);

      gtk_tree_model_get_iter (model, &iter, path);
      gtk_tree_model_sort_convert_iter_to_child_iter (
          GTK_TREE_MODEL_SORT (model), &child, &iter);
      gtk_list_store_remove (data->theme_store, &child);
    }

    g_list_foreach (selected, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (selected);
    g_free (name);
  }
}

static void
theme_details_changed_cb (AppearanceData *data)
{
  MateThemeMetaInfo *gsettings_theme;
  const MateThemeMetaInfo *selected;
  GtkIconView *icon_view;
  gboolean done = FALSE;

  /* load new state from gsettings */
  gsettings_theme = theme_load_from_gsettings (data);

  /* check if it's our currently selected theme */
  icon_view = GTK_ICON_VIEW (appearance_capplet_get_widget (data, "theme_list"));
  selected = theme_get_selected (icon_view, data);

  if (!selected || !(done = theme_is_equal (selected, gsettings_theme))) {
    /* look for a matching metatheme */
    GList *theme_list, *l;

    theme_list = mate_theme_meta_info_find_all ();

    for (l = theme_list; l; l = l->next) {
      MateThemeMetaInfo *info = l->data;

      if (theme_is_equal (gsettings_theme, info)) {
        theme_select_name (icon_view, info->name);
        done = TRUE;
        break;
      }
    }
    g_list_free (theme_list);
  }

  if (!done)
    /* didn't find a match, set or update custom */
    theme_set_custom_from_theme (gsettings_theme, data);

  mate_theme_meta_info_free (gsettings_theme);
}

static void
theme_setting_changed_cb (GObject *settings,
                          GParamSpec *pspec,
                          AppearanceData *data)
{
  theme_details_changed_cb (data);
}

static void
theme_gsettings_changed (GSettings *settings,
                         gchar *key,
                         AppearanceData *data)
{
  theme_details_changed_cb (data);
}

static gint
theme_list_sort_func (MateThemeMetaInfo *a,
                      MateThemeMetaInfo *b)
{
  return strcmp (a->readable_name, b->readable_name);
}

static gint
theme_store_sort_func (GtkTreeModel *model,
                      GtkTreeIter *a,
                      GtkTreeIter *b,
                      gpointer user_data)
{
  gchar *a_name, *a_label;
  gint rc;

  gtk_tree_model_get (model, a, COL_NAME, &a_name, COL_LABEL, &a_label, -1);

  if (!strcmp (a_name, CUSTOM_THEME_NAME)) {
    rc = -1;
  } else {
    gchar *b_name, *b_label;

    gtk_tree_model_get (model, b, COL_NAME, &b_name, COL_LABEL, &b_label, -1);

    if (!strcmp (b_name, CUSTOM_THEME_NAME)) {
      rc = 1;
    } else {
      gchar *a_case, *b_case;

      a_case = g_utf8_casefold (a_label, -1);
      b_case = g_utf8_casefold (b_label, -1);
      rc = g_utf8_collate (a_case, b_case);
      g_free (a_case);
      g_free (b_case);
    }

    g_free (b_name);
    g_free (b_label);
  }

  g_free (a_name);
  g_free (a_label);

  return rc;
}

static void
theme_drag_data_received_cb (GtkWidget *widget,
                             GdkDragContext *context,
                             gint x, gint y,
                             GtkSelectionData *selection_data,
                             guint info, guint time,
                             AppearanceData *data)
{
  gchar **uris;

  if (!(info == TARGET_URI_LIST || info == TARGET_NS_URL))
    return;

  uris = g_uri_list_extract_uris ((gchar *) gtk_selection_data_get_data (selection_data));

  if (uris != NULL && uris[0] != NULL) {
    GFile *f = g_file_new_for_uri (uris[0]);

    mate_theme_install (f,
        GTK_WINDOW (appearance_capplet_get_widget (data, "appearance_window")));
    g_object_unref (f);
  }

  g_strfreev (uris);
}

static void background_or_font_changed(GSettings *settings, gchar *key, AppearanceData* data)
{
	theme_message_area_update(data);
}

void themes_init(AppearanceData* data)
{
  GtkWidget *w, *del_button;
  GList *theme_list, *l;
  GtkListStore *theme_store;
  GtkTreeModel *sort_model;
  MateThemeMetaInfo *meta_theme = NULL;
  GtkIconView *icon_view;
  GtkCellRenderer *renderer;
  GtkSettings *settings;
  char *url;

  /* initialise some stuff */
  mate_theme_init ();
  mate_wm_manager_init ();

  data->revert_application_font = NULL;
  data->revert_documents_font = NULL;
  data->revert_desktop_font = NULL;
  data->revert_windowtitle_font = NULL;
  data->revert_monospace_font = NULL;
  data->theme_save_dialog = NULL;
  data->theme_message_area = NULL;
  data->theme_info_icon = NULL;
  data->theme_error_icon = NULL;
  data->theme_icon = gdk_pixbuf_new_from_file (MATECC_PIXMAP_DIR "/theme-thumbnailing.png", NULL);
  data->theme_store = theme_store =
      gtk_list_store_new (NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);

  /* set up theme list */
  theme_list = mate_theme_meta_info_find_all ();
  mate_theme_info_register_theme_change ((ThemeChangedCallback) theme_changed_on_disk_cb, data);

  data->theme_custom = theme_load_from_gsettings (data);
  data->theme_custom->name = g_strdup (CUSTOM_THEME_NAME);
  data->theme_custom->readable_name = g_strdup_printf ("<i>%s</i>", _("Custom"));

  for (l = theme_list; l; l = l->next) {
    MateThemeMetaInfo *info = l->data;

    gtk_list_store_insert_with_values (theme_store, NULL, 0,
        COL_LABEL, info->readable_name,
        COL_NAME, info->name,
        COL_THUMBNAIL, data->theme_icon,
        -1);

    if (!meta_theme && theme_is_equal (data->theme_custom, info))
      meta_theme = info;
  }

  if (!meta_theme) {
    /* add custom theme */
    meta_theme = data->theme_custom;

    gtk_list_store_insert_with_values (theme_store, NULL, 0,
        COL_LABEL, meta_theme->readable_name,
        COL_NAME, meta_theme->name,
        COL_THUMBNAIL, data->theme_icon,
        -1);

    theme_thumbnail_generate (meta_theme, data);
  }

  theme_list = g_list_sort (theme_list, (GCompareFunc) theme_list_sort_func);

  g_list_foreach (theme_list, (GFunc) theme_thumbnail_generate, data);
  g_list_free (theme_list);

  icon_view = GTK_ICON_VIEW (appearance_capplet_get_widget (data, "theme_list"));

  renderer = gtk_cell_renderer_pixbuf_new ();
  g_object_set (renderer, "xpad", 5, "ypad", 5,
                          "xalign", 0.5, "yalign", 1.0, NULL);
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (icon_view), renderer, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (icon_view), renderer,
                                  "pixbuf", COL_THUMBNAIL, NULL);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "alignment", PANGO_ALIGN_CENTER,
                          "wrap-mode", PANGO_WRAP_WORD_CHAR,
                          "wrap-width", gtk_icon_view_get_item_width (icon_view),
                          "width", gtk_icon_view_get_item_width (icon_view),
                          "xalign", 0.5, "yalign", 0.0, NULL);

  gtk_cell_layout_pack_end (GTK_CELL_LAYOUT (icon_view), renderer, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (icon_view), renderer,
                                  "markup", COL_LABEL, NULL);

  sort_model = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (theme_store));
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sort_model), COL_LABEL, theme_store_sort_func, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sort_model), COL_LABEL, GTK_SORT_ASCENDING);
  gtk_icon_view_set_model (icon_view, GTK_TREE_MODEL (sort_model));

  g_signal_connect (icon_view, "selection-changed", (GCallback) theme_selection_changed_cb, data);
  g_signal_connect_after (icon_view, "realize", (GCallback) theme_select_name, meta_theme->name);

  w = appearance_capplet_get_widget (data, "theme_install");
  gtk_button_set_image (GTK_BUTTON (w),
                        gtk_image_new_from_icon_name ("document-open", GTK_ICON_SIZE_BUTTON));
  g_signal_connect (w, "clicked", (GCallback) theme_install_cb, data);

  w = appearance_capplet_get_widget (data, "theme_save");
  gtk_button_set_image (GTK_BUTTON (w),
                        gtk_image_new_from_icon_name ("document-save-as", GTK_ICON_SIZE_BUTTON));
  g_signal_connect (w, "clicked", (GCallback) theme_save_cb, data);

  w = appearance_capplet_get_widget (data, "theme_custom");
  gtk_button_set_image (GTK_BUTTON (w),
                        gtk_image_new_from_icon_name ("gtk-edit", GTK_ICON_SIZE_BUTTON));
  g_signal_connect (w, "clicked", (GCallback) theme_custom_cb, data);

  del_button = appearance_capplet_get_widget (data, "theme_delete");
  g_signal_connect (del_button, "clicked", (GCallback) theme_delete_cb, data);

  w = appearance_capplet_get_widget (data, "theme_vbox");
  gtk_drag_dest_set (w, GTK_DEST_DEFAULT_ALL,
		     drop_types, G_N_ELEMENTS (drop_types),
		     GDK_ACTION_COPY | GDK_ACTION_LINK | GDK_ACTION_MOVE);
  g_signal_connect (w, "drag-data-received", (GCallback) theme_drag_data_received_cb, data);
  if (is_locked_down ())
    gtk_widget_set_sensitive (w, FALSE);

  w = appearance_capplet_get_widget (data, "more_themes_linkbutton");
  url = g_settings_get_string (data->settings, MORE_THEMES_URL_KEY);
  if (url != NULL && url[0] != '\0') {
    gtk_link_button_set_uri (GTK_LINK_BUTTON (w), url);
    gtk_widget_show (w);
  } else {
    gtk_widget_hide (w);
  }
  g_free (url);

  /* listen to gsettings changes, too */
  g_signal_connect (data->marco_settings, "changed::" MARCO_THEME_KEY, G_CALLBACK (theme_gsettings_changed), data);
  g_signal_connect (data->mouse_settings, "changed::" CURSOR_THEME_KEY, G_CALLBACK (theme_gsettings_changed), data);
  g_signal_connect (data->mouse_settings, "changed::" CURSOR_SIZE_KEY, G_CALLBACK (theme_gsettings_changed), data);
  g_signal_connect (data->wp_settings, "changed::" WP_FILE_KEY, G_CALLBACK (background_or_font_changed), data);
  g_signal_connect (data->interface_settings, "changed::" GTK_FONT_KEY, G_CALLBACK (background_or_font_changed), data);
  g_signal_connect (data->interface_settings, "changed::" DOCUMENT_FONT_KEY, G_CALLBACK (background_or_font_changed), data);

  if (data->caja_settings)
    g_signal_connect (data->caja_settings, "changed::" DESKTOP_FONT_KEY, G_CALLBACK (background_or_font_changed), data);

  g_signal_connect (data->marco_settings, "changed::" WINDOW_TITLE_FONT_KEY, G_CALLBACK (background_or_font_changed), data);
  g_signal_connect (data->interface_settings, "changed::" MONOSPACE_FONT_KEY, G_CALLBACK (background_or_font_changed), data);

  settings = gtk_settings_get_default ();
  g_signal_connect (settings, "notify::gtk-color-scheme", (GCallback) theme_setting_changed_cb, data);
  g_signal_connect (settings, "notify::gtk-theme-name", (GCallback) theme_setting_changed_cb, data);
  g_signal_connect (settings, "notify::gtk-icon-theme-name", (GCallback) theme_setting_changed_cb, data);

  /* monitor individual font choice buttons, so
     "revert font" option (if any) can be cleared */
  w = appearance_capplet_get_widget (data, "application_font");
  g_signal_connect (w, "font_set", (GCallback) custom_font_cb, data);
  w = appearance_capplet_get_widget (data, "document_font");
  g_signal_connect (w, "font_set", (GCallback) custom_font_cb, data);
  w = appearance_capplet_get_widget (data, "desktop_font");
  g_signal_connect (w, "font_set", (GCallback) custom_font_cb, data);
  w = appearance_capplet_get_widget (data, "window_title_font");
  g_signal_connect (w, "font_set", (GCallback) custom_font_cb, data);
  w = appearance_capplet_get_widget (data, "monospace_font");
  g_signal_connect (w, "font_set", (GCallback) custom_font_cb, data);
}

void
themes_shutdown (AppearanceData *data)
{
  mate_theme_meta_info_free (data->theme_custom);

  if (data->theme_icon)
    g_object_unref (data->theme_icon);
  if (data->theme_save_dialog)
    gtk_widget_destroy (data->theme_save_dialog);
  g_free (data->revert_application_font);
  g_free (data->revert_documents_font);
  g_free (data->revert_desktop_font);
  g_free (data->revert_windowtitle_font);
  g_free (data->revert_monospace_font);
}
