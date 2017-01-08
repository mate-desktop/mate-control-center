/* -*- mode: C; c-basic-offset: 4 -*-
 * mate-font-view: 
 *
 * Copyright (C) 2012 Cosimo Cecchi <cosimoc@gnome.org>
 *
 * based on code from
 *
 * fontilus - a collection of font utilities for MATE
 * Copyright (C) 2002-2003  James Henstridge <james@daa.com.au>
 * 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <gtk/gtk.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <fontconfig/fontconfig.h>

#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmate-desktop/mate-desktop-thumbnail.h>

#include "font-model.h"
#include "font-utils.h"
#include "sushi-font-loader.h"

struct _FontViewModelPrivate {
    /* list of fonts in fontconfig database */
    FcFontSet *font_list;

    FT_Library library;

    GList *monitors;
    GdkPixbuf *fallback_icon;
};

enum {
    CONFIG_CHANGED,
    NUM_SIGNALS
};

static guint signals[NUM_SIGNALS] = { 0, };

static void ensure_thumbnail (FontViewModel *self, const gchar *path);

G_DEFINE_TYPE (FontViewModel, font_view_model, GTK_TYPE_LIST_STORE);

#define ATTRIBUTES_FOR_CREATING_THUMBNAIL \
    G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE"," \
    G_FILE_ATTRIBUTE_TIME_MODIFIED
#define ATTRIBUTES_FOR_EXISTING_THUMBNAIL \
    G_FILE_ATTRIBUTE_THUMBNAIL_PATH"," \
    G_FILE_ATTRIBUTE_THUMBNAILING_FAILED

typedef struct {
    const gchar *file;
    FT_Face face;
    GtkTreeIter iter;
    gboolean found;
} IterForFileData;

static gboolean
iter_for_file_foreach (GtkTreeModel *model,
                       GtkTreePath *path,
                       GtkTreeIter *iter,
                       gpointer user_data)
{
    IterForFileData *data = user_data;
    gchar *font_path, *font_name, *match_name;
    gboolean retval;

    gtk_tree_model_get (GTK_TREE_MODEL (model), iter,
                        COLUMN_NAME, &font_name,
                        COLUMN_PATH, &font_path,
                        -1);

    if (data->file) {
        retval = (g_strcmp0 (font_path, data->file) == 0);
    } else if (data->face) {
        match_name = font_utils_get_font_name (data->face);
        retval = (g_strcmp0 (font_name, match_name) == 0);
        g_free (match_name);
    }

    g_free (font_path);
    g_free (font_name);

    if (retval) {
        data->iter = *iter;
        data->found = TRUE;
    }

    return retval;
}

static gboolean
font_view_model_get_iter_internal (FontViewModel *self,
                                   const gchar *file,
                                   FT_Face face,
                                   GtkTreeIter *iter)
{
    IterForFileData *data = g_slice_new0 (IterForFileData);
    gboolean found;

    data->file = file;
    data->face = face;
    data->found = FALSE;

    gtk_tree_model_foreach (GTK_TREE_MODEL (self),
                            iter_for_file_foreach,
                            data);

    found = data->found;
    if (found && iter)
        *iter = data->iter;

    g_slice_free (IterForFileData, data);

    return found;
}

gboolean
font_view_model_get_iter_for_file (FontViewModel *self,
                                   const gchar *file,
                                   GtkTreeIter *iter)
{
    return font_view_model_get_iter_internal
        (self, file, NULL, iter);
}

gboolean
font_view_model_get_iter_for_face (FontViewModel *self,
                                   FT_Face face,
                                   GtkTreeIter *iter)
{
    return font_view_model_get_iter_internal
        (self, NULL, face, iter);
}

typedef struct {
    FontViewModel *self;
    GFile *font_file;
    gchar *font_path;
    GdkPixbuf *pixbuf;
} LoadThumbnailData;

static void
load_thumbnail_data_free (LoadThumbnailData *data)
{
    g_object_unref (data->self);
    g_object_unref (data->font_file);
    g_clear_object (&data->pixbuf);
    g_free (data->font_path);

    g_slice_free (LoadThumbnailData, data);
}

static gboolean
ensure_thumbnail_job_done (gpointer user_data)
{
    LoadThumbnailData *data = user_data;
    GtkTreeIter iter;

    if ((data->pixbuf != NULL) &&
        (font_view_model_get_iter_for_file (data->self, data->font_path, &iter)))
        gtk_list_store_set (GTK_LIST_STORE (data->self), &iter,
                            COLUMN_ICON, data->pixbuf,
                            -1);

    load_thumbnail_data_free (data);

    return FALSE;
}

static GdkPixbuf *
create_thumbnail (LoadThumbnailData *data)
{
    GFile *file = data->font_file;
    MateDesktopThumbnailFactory *factory;
    gchar *uri;
    guint64 mtime;

    GdkPixbuf *pixbuf = NULL;
    GFileInfo *info = NULL;

    uri = g_file_get_uri (file);
    info = g_file_query_info (file, ATTRIBUTES_FOR_CREATING_THUMBNAIL,
                              G_FILE_QUERY_INFO_NONE,
                              NULL, NULL);

    /* we don't care about reporting errors here, just fail the
     * thumbnail.
     */
    if (info == NULL)
        goto out;

    mtime = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_TIME_MODIFIED);

    factory = mate_desktop_thumbnail_factory_new (MATE_DESKTOP_THUMBNAIL_SIZE_NORMAL);
    pixbuf = mate_desktop_thumbnail_factory_generate_thumbnail
        (factory,
         uri, g_file_info_get_content_type (info));

    if (pixbuf != NULL)
        mate_desktop_thumbnail_factory_save_thumbnail (factory, pixbuf,
                                                        uri, (time_t) mtime);
    else
        mate_desktop_thumbnail_factory_create_failed_thumbnail (factory,
                                                                 uri, (time_t) mtime);

  g_object_unref (factory);

 out:
  g_clear_object (&info);

  return pixbuf;
}

static gboolean
ensure_thumbnail_job (GIOSchedulerJob *job,
                      GCancellable *cancellable,
                      gpointer user_data)
{
    LoadThumbnailData *data = user_data;
    gboolean thumb_failed;
    const gchar *thumb_path;

    GError *error = NULL;
    GFile *thumb_file = NULL;
    GFileInputStream *is = NULL;
    GFileInfo *info = NULL;

    info = g_file_query_info (data->font_file,
                              ATTRIBUTES_FOR_EXISTING_THUMBNAIL,
                              G_FILE_QUERY_INFO_NONE,
                              NULL, &error);

    if (error != NULL) {
        g_debug ("Can't query info for file %s: %s\n", data->font_path, error->message);
        goto out;
    }

    thumb_failed = g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_THUMBNAILING_FAILED);
    if (thumb_failed)
        goto out;

    thumb_path = g_file_info_get_attribute_byte_string (info, G_FILE_ATTRIBUTE_THUMBNAIL_PATH);

    if (thumb_path != NULL) {
        thumb_file = g_file_new_for_path (thumb_path);
        is = g_file_read (thumb_file, NULL, &error);

        if (error != NULL) {
            g_debug ("Can't read file %s: %s\n", thumb_path, error->message);
            goto out;
        }

        data->pixbuf = gdk_pixbuf_new_from_stream_at_scale (G_INPUT_STREAM (is),
                                                            128, 128, TRUE,
                                                            NULL, &error);

        if (error != NULL) {
            g_debug ("Can't read thumbnail pixbuf %s: %s\n", thumb_path, error->message);
            goto out;
        }
    } else {
        data->pixbuf = create_thumbnail (data);
    }

 out:
    g_clear_error (&error);
    g_clear_object (&is);
    g_clear_object (&thumb_file);
    g_clear_object (&info);

    g_io_scheduler_job_send_to_mainloop_async (job, ensure_thumbnail_job_done,
                                               data, NULL);

    return FALSE;
}

static void
ensure_thumbnail (FontViewModel *self,
                  const gchar *path)
{
    LoadThumbnailData *data;

    data = g_slice_new0 (LoadThumbnailData);
    data->self = g_object_ref (self);
    data->font_file = g_file_new_for_path (path);
    data->font_path = g_strdup (path);

    g_io_scheduler_push_job (ensure_thumbnail_job, data,
                             NULL, G_PRIORITY_DEFAULT, NULL);
}

/* make sure the font list is valid */
static void
ensure_font_list (FontViewModel *self)
{
    FcPattern *pat;
    FcObjectSet *os;
    gint i;
    FcChar8 *file;
    gchar *font_name, *collation_key;

    if (self->priv->font_list) {
            FcFontSetDestroy (self->priv->font_list);
            self->priv->font_list = NULL;
    }

    gtk_list_store_clear (GTK_LIST_STORE (self));

    /* always reinitialize the font database */
    if (!FcInitReinitialize())
        return;

    pat = FcPatternCreate ();
    os = FcObjectSetBuild (FC_FILE, FC_FAMILY, FC_WEIGHT, FC_SLANT, NULL);

    self->priv->font_list = FcFontList (NULL, pat, os);

    FcPatternDestroy (pat);
    FcObjectSetDestroy (os);

    if (!self->priv->font_list)
        return;

    for (i = 0; i < self->priv->font_list->nfont; i++) {
	FcPatternGetString (self->priv->font_list->fonts[i], FC_FILE, 0, &file);
        font_name = font_utils_get_font_name_for_file (self->priv->library, (const gchar *) file);

        if (!font_name)
            continue;

        collation_key = g_utf8_collate_key (font_name, -1);

        gtk_list_store_insert_with_values (GTK_LIST_STORE (self), NULL, -1,
                                           COLUMN_NAME, font_name,
                                           COLUMN_POINTER, self->priv->font_list->fonts[i],
                                           COLUMN_PATH, file,
                                           COLUMN_ICON, self->priv->fallback_icon,
                                           COLUMN_COLLATION_KEY, collation_key,
                                           -1);

        ensure_thumbnail (self, (const gchar *) file);

        g_free (font_name);
        g_free (collation_key);
    }
}

static gboolean
ensure_font_list_idle (gpointer user_data)
{
    FontViewModel *self = user_data;
    ensure_font_list (self);

    return FALSE;
}

static int
font_view_model_sort_func (GtkTreeModel *model,
                           GtkTreeIter *a,
                           GtkTreeIter *b,
                           gpointer user_data)
{
    gchar *key_a = NULL, *key_b = NULL;
    int retval;

    gtk_tree_model_get (model, a,
                        COLUMN_COLLATION_KEY, &key_a,
                        -1);
    gtk_tree_model_get (model, b,
                        COLUMN_COLLATION_KEY, &key_b,
                        -1);

    retval = g_strcmp0 (key_a, key_b);

    g_free (key_a);
    g_free (key_b);

    return retval;
}

static void
file_monitor_changed_cb (GFileMonitor *monitor,
                         GFile *file,
                         GFile *other_file,
                         GFileMonitorEvent event,
                         gpointer user_data)
{
    FontViewModel *self = user_data;

    if (event == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT ||
        event == G_FILE_MONITOR_EVENT_DELETED ||
        event == G_FILE_MONITOR_EVENT_CREATED) {
        ensure_font_list (self);
        g_signal_emit (self, signals[CONFIG_CHANGED], 0);
    }
}

static void
create_file_monitors (FontViewModel *self)
{
    FcConfig *config;
    FcStrList *str_list;
    FcChar8 *path;
    GFile *file;
    GFileMonitor *monitor;

    config = FcConfigGetCurrent ();
    str_list = FcConfigGetFontDirs (config);

    while ((path = FcStrListNext (str_list)) != NULL) {
        file = g_file_new_for_path ((const gchar *) path);
        monitor = g_file_monitor (file, G_FILE_MONITOR_NONE,
                                  NULL, NULL);

        if (monitor != NULL) {
            self->priv->monitors = g_list_prepend (self->priv->monitors, monitor);
            g_signal_connect (monitor, "changed",
                              G_CALLBACK (file_monitor_changed_cb), self);
        }

        g_object_unref (file);
    }

    FcStrListDone (str_list);
}

static GdkPixbuf *
get_fallback_icon (void)
{
    GtkIconTheme *icon_theme;
    GtkIconInfo *icon_info;
    GdkPixbuf *pix;
    GIcon *icon = NULL;

    icon_theme = gtk_icon_theme_get_default ();
    icon = g_content_type_get_icon ("application/x-font-ttf");
    icon_info = gtk_icon_theme_lookup_by_gicon (icon_theme, icon,
                                                128, GTK_ICON_LOOKUP_GENERIC_FALLBACK);
    g_object_unref (icon);

    if (!icon_info)
        return NULL;

    pix = gtk_icon_info_load_icon (icon_info, NULL);
    gtk_icon_info_free (icon_info);

    return pix;
}

static void
font_view_model_init (FontViewModel *self)
{
    GType types[NUM_COLUMNS] =
        { G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING };

    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, FONT_VIEW_TYPE_MODEL, FontViewModelPrivate);

    if (FT_Init_FreeType (&self->priv->library) != FT_Err_Ok)
        g_critical ("Can't initialize FreeType library");

    gtk_list_store_set_column_types (GTK_LIST_STORE (self),
                                     NUM_COLUMNS, types);

    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (self),
                                          COLUMN_NAME,
                                          GTK_SORT_ASCENDING);
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (self),
                                     COLUMN_NAME,
                                     font_view_model_sort_func,
                                     NULL, NULL);


    self->priv->fallback_icon = get_fallback_icon ();

    g_idle_add (ensure_font_list_idle, self);
    create_file_monitors (self);
}

static void
font_view_model_finalize (GObject *obj)
{
    FontViewModel *self = FONT_VIEW_MODEL (obj);

    if (self->priv->font_list) {
            FcFontSetDestroy (self->priv->font_list);
            self->priv->font_list = NULL;
    }

    if (self->priv->library != NULL) {
        FT_Done_FreeType (self->priv->library);
        self->priv->library = NULL;
    }

    g_clear_object (&self->priv->fallback_icon);
    g_list_free_full (self->priv->monitors, (GDestroyNotify) g_object_unref);

    G_OBJECT_CLASS (font_view_model_parent_class)->finalize (obj);
}

static void
font_view_model_class_init (FontViewModelClass *klass)
{
    GObjectClass *oclass = G_OBJECT_CLASS (klass);
    oclass->finalize = font_view_model_finalize;

    signals[CONFIG_CHANGED] = 
        g_signal_new ("config-changed",
                      FONT_VIEW_TYPE_MODEL,
                      G_SIGNAL_RUN_FIRST,
                      0, NULL, NULL, NULL,
                      G_TYPE_NONE, 0);

    g_type_class_add_private (klass, sizeof (FontViewModelPrivate));
}

GtkTreeModel *
font_view_model_new (void)
{
    return g_object_new (FONT_VIEW_TYPE_MODEL, NULL);
}

