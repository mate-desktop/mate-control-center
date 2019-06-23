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
    GMutex font_list_mutex;

    FT_Library library;

    GList *monitors;
    GdkPixbuf *fallback_icon;
    GCancellable *cancellable;
};

enum {
    CONFIG_CHANGED,
    NUM_SIGNALS
};

static guint signals[NUM_SIGNALS] = { 0, };

G_DEFINE_TYPE_WITH_PRIVATE (FontViewModel, font_view_model, GTK_TYPE_LIST_STORE);

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
} IterForFaceData;

static gboolean
iter_for_face_foreach (GtkTreeModel *model,
                       GtkTreePath *path,
                       GtkTreeIter *iter,
                       gpointer user_data)
{
    IterForFaceData *data = user_data;
    gchar *font_name, *match_name;
    gboolean retval;

    gtk_tree_model_get (GTK_TREE_MODEL (model), iter,
                        COLUMN_NAME, &font_name,
                        -1);

    match_name = font_utils_get_font_name (data->face);
    retval = (g_strcmp0 (font_name, match_name) == 0);

    g_free (match_name);
    g_free (font_name);

    if (retval) {
        data->iter = *iter;
        data->found = TRUE;
    }

    return retval;
}

gboolean
font_view_model_get_iter_for_face (FontViewModel *self,
                                   FT_Face face,
                                   GtkTreeIter *iter)
{
    IterForFaceData *data = g_slice_new0 (IterForFaceData);
    gboolean found;

    data->face = face;
    data->found = FALSE;

    gtk_tree_model_foreach (GTK_TREE_MODEL (self),
                            iter_for_face_foreach,
                            data);

    found = data->found;
    if (found && iter)
        *iter = data->iter;

    g_slice_free (IterForFaceData, data);

    return found;
}

typedef struct {
    FontViewModel *self;
    GFile *font_file;
    gchar *font_path;
    gint face_index;
    gchar *uri;
    GdkPixbuf *pixbuf;
    GtkTreeIter iter;
} ThumbInfoData;

static void
thumb_info_data_free (gpointer user_data)
{
    ThumbInfoData *thumb_info = user_data;

    g_object_unref (thumb_info->self);
    g_object_unref (thumb_info->font_file);
    g_clear_object (&thumb_info->pixbuf);
    g_free (thumb_info->font_path);
    g_free (thumb_info->uri);

    g_slice_free (ThumbInfoData, thumb_info);
}

static gboolean
one_thumbnail_done (gpointer user_data)
{
    ThumbInfoData *thumb_info = user_data;

    if (thumb_info->pixbuf != NULL)
        gtk_list_store_set (GTK_LIST_STORE (thumb_info->self), &(thumb_info->iter),
                            COLUMN_ICON, thumb_info->pixbuf,
                            -1);

    thumb_info_data_free (thumb_info);

    return FALSE;
}

static GdkPixbuf *
create_thumbnail (ThumbInfoData *thumb_info)
{
    GFile *file = thumb_info->font_file;
    MateDesktopThumbnailFactory *factory;
    guint64 mtime;

    GdkPixbuf *pixbuf = NULL;
    GFileInfo *info = NULL;

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
         thumb_info->uri, g_file_info_get_content_type (info));

    if (pixbuf != NULL)
        mate_desktop_thumbnail_factory_save_thumbnail (factory, pixbuf,
                                                       thumb_info->uri, (time_t) mtime);
    else
        mate_desktop_thumbnail_factory_create_failed_thumbnail (factory,
                                                                thumb_info->uri, (time_t) mtime);

  g_object_unref (factory);

 out:
  g_clear_object (&info);

  return pixbuf;
}

static void
ensure_thumbnails_job (GTask *task,
                       gpointer source_object,
                       gpointer user_data,
                       GCancellable *cancellable)
{
    GList *thumb_infos = user_data, *l;

    for (l = thumb_infos; l != NULL; l = l->next) {
        gboolean thumb_failed;
        gchar *thumb_path = NULL;
        ThumbInfoData *thumb_info = l->data;

        GError *error = NULL;
        GFile *thumb_file = NULL;
        GFileInputStream *is = NULL;
        GFileInfo *info = NULL;

        if (thumb_info->face_index == 0) {
            thumb_info->uri = g_file_get_uri (thumb_info->font_file);
            info = g_file_query_info (thumb_info->font_file,
                                      ATTRIBUTES_FOR_EXISTING_THUMBNAIL,
                                      G_FILE_QUERY_INFO_NONE,
                                      NULL, &error);

            if (error != NULL) {
                gchar *font_path;

                font_path = g_file_get_path (thumb_info->font_file);
                g_debug ("Can't query info for file %s: %s\n", font_path, error->message);
                g_free (font_path);

                goto next;
            }

            thumb_failed = g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_THUMBNAILING_FAILED);
            if (thumb_failed)
                goto next;

            thumb_path = g_strdup (g_file_info_get_attribute_byte_string (info, G_FILE_ATTRIBUTE_THUMBNAIL_PATH));
        } else {
            gchar *file_uri;
            gchar *checksum;
            gchar *filename;

            file_uri = g_file_get_uri (thumb_info->font_file);
            thumb_info->uri = g_strdup_printf ("%s#0x%08X", file_uri, thumb_info->face_index);
            g_free (file_uri);

            checksum = g_compute_checksum_for_data (G_CHECKSUM_MD5,
                                                    (const guchar *) thumb_info->uri,
                                                    strlen (thumb_info->uri));
            filename = g_strdup_printf ("%s.png", checksum);
            g_free (checksum);

            thumb_path = g_build_filename (g_get_user_cache_dir (),
                                           "thumbnails",
                                           "large",
                                           filename,
                                           NULL);
            g_free (filename);

            if (!g_file_test (thumb_path, G_FILE_TEST_IS_REGULAR)) {
                g_clear_pointer (&thumb_path, g_free);
            }
        }

        if (thumb_path != NULL) {
            thumb_file = g_file_new_for_path (thumb_path);
            is = g_file_read (thumb_file, NULL, &error);

            if (error != NULL) {
                g_debug ("Can't read file %s: %s\n", thumb_path, error->message);
                goto next;
            }

            thumb_info->pixbuf = gdk_pixbuf_new_from_stream_at_scale (G_INPUT_STREAM (is),
                                                                      128, 128, TRUE,
                                                                      NULL, &error);

            if (error != NULL) {
                g_debug ("Can't read thumbnail pixbuf %s: %s\n", thumb_path, error->message);
                goto next;
            }
        } else {
            thumb_info->pixbuf = create_thumbnail (thumb_info);
        }

    next:
        g_clear_error (&error);
        g_clear_object (&is);
        g_clear_object (&thumb_file);
        g_clear_object (&info);
        g_clear_pointer (&thumb_path, g_free);

        g_main_context_invoke (NULL, one_thumbnail_done,
                               thumb_info);
    }

    g_list_free (thumb_infos);
}

typedef struct {
    gchar *font_path;
    gint face_index;
    gchar *font_name;
} FontInfoData;

static void
font_info_data_free (gpointer user_data)
{
    FontInfoData *font_info = user_data;

    g_free (font_info->font_path);
    g_free (font_info->font_name);
    g_slice_free (FontInfoData, font_info);
}

static void
font_infos_loaded (GObject *source_object,
                   GAsyncResult *result,
                   gpointer user_data)
{
    FontViewModel *self = FONT_VIEW_MODEL (source_object);
    GTask *task = NULL;
    GList *l, *thumb_infos = NULL;
    GList *font_infos = g_task_propagate_pointer (G_TASK (result), NULL);

    for (l = font_infos; l != NULL; l = l->next) {
        FontInfoData *font_info = l->data;
        gchar *collation_key;
        GtkTreeIter iter;
        ThumbInfoData *thumb_info;

        collation_key = g_utf8_collate_key (font_info->font_name, -1);
        gtk_list_store_insert_with_values (GTK_LIST_STORE (self), &iter, -1,
                                           COLUMN_NAME, font_info->font_name,
                                           COLUMN_PATH, font_info->font_path,
                                           COLUMN_FACE_INDEX, font_info->face_index,
                                           COLUMN_ICON, self->priv->fallback_icon,
                                           COLUMN_COLLATION_KEY, collation_key,
                                           -1);
        g_free (collation_key);

        thumb_info = g_slice_new0 (ThumbInfoData);
        thumb_info->font_file = g_file_new_for_path (font_info->font_path);
        thumb_info->face_index = font_info->face_index;
        thumb_info->iter = iter;
        thumb_info->self = g_object_ref (self);

        font_info_data_free (font_info);
        thumb_infos = g_list_prepend (thumb_infos, thumb_info);
    }

    g_signal_emit (self, signals[CONFIG_CHANGED], 0);
    g_list_free (font_infos);

    task = g_task_new (NULL, NULL, NULL, NULL);
    g_task_set_task_data (task, thumb_infos, NULL);
    g_task_run_in_thread (task, ensure_thumbnails_job);
    g_object_unref (task);
}

static void
load_font_infos (GTask *task,
                 gpointer source_object,
                 gpointer user_data,
                 GCancellable *cancellable)
{
    FontViewModel *self = FONT_VIEW_MODEL (source_object);
    gint i, n_fonts;
    GList *font_infos = NULL;

    n_fonts = self->priv->font_list->nfont;

    for (i = 0; i < n_fonts; i++) {
        FontInfoData *font_info;
        FcChar8 *file;
        int index;
        gchar *font_name;

        if (g_cancellable_is_cancelled (cancellable))
            break;

        g_mutex_lock (&self->priv->font_list_mutex);
        FcPatternGetString (self->priv->font_list->fonts[i], FC_FILE, 0, &file);
        FcPatternGetInteger (self->priv->font_list->fonts[i], FC_INDEX, 0, &index);
        g_mutex_unlock (&self->priv->font_list_mutex);

        font_name = font_utils_get_font_name_for_file (self->priv->library,
                                                       (const gchar *) file,
                                                       index);

        if (!font_name)
            continue;

        font_info = g_slice_new0 (FontInfoData);
        font_info->font_name = font_name;
        font_info->font_path = g_strdup ((const gchar *) file);
        font_info->face_index = index;

        font_infos = g_list_prepend (font_infos, font_info);
    }

    g_task_return_pointer (task, font_infos, NULL);
}

/* make sure the font list is valid */
static void
ensure_font_list (FontViewModel *self)
{
    FcPattern *pat;
    FcObjectSet *os;
    GTask *task;

    /* always reinitialize the font database */
    if (!FcInitReinitialize())
        return;

    if (self->priv->cancellable) {
        g_cancellable_cancel (self->priv->cancellable);
        g_clear_object (&self->priv->cancellable);
    }

    gtk_list_store_clear (GTK_LIST_STORE (self));

    pat = FcPatternCreate ();
    os = FcObjectSetBuild (FC_FILE, FC_INDEX, FC_FAMILY, FC_WEIGHT, FC_SLANT, NULL);

    g_mutex_lock (&self->priv->font_list_mutex);

    if (self->priv->font_list) {
        FcFontSetDestroy (self->priv->font_list);
        self->priv->font_list = NULL;
    }

    self->priv->font_list = FcFontList (NULL, pat, os);

    g_mutex_unlock (&self->priv->font_list_mutex);

    FcPatternDestroy (pat);
    FcObjectSetDestroy (os);

    if (!self->priv->font_list)
        return;

    self->priv->cancellable = g_cancellable_new ();

    task = g_task_new (self, self->priv->cancellable, font_infos_loaded, NULL);
    g_task_set_return_on_cancel (task, TRUE);
    g_task_run_in_thread (task, load_font_infos);
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
        event == G_FILE_MONITOR_EVENT_CREATED)
        ensure_font_list (self);
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
                                                128, 0);
    g_object_unref (icon);

    if (!icon_info)
        return NULL;

    pix = gtk_icon_info_load_icon (icon_info, NULL);
    g_object_unref (icon_info);

    return pix;
}

static void
font_view_model_init (FontViewModel *self)
{
    GType types[NUM_COLUMNS] =
        { G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, GDK_TYPE_PIXBUF, G_TYPE_STRING };

    self->priv = font_view_model_get_instance_private (self);

    if (FT_Init_FreeType (&self->priv->library) != FT_Err_Ok)
        g_critical ("Can't initialize FreeType library");

    g_mutex_init (&self->priv->font_list_mutex);

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

    if (self->priv->cancellable) {
        g_cancellable_cancel (self->priv->cancellable);
        g_clear_object (&self->priv->cancellable);
    }

    if (self->priv->font_list) {
        FcFontSetDestroy (self->priv->font_list);
        self->priv->font_list = NULL;
    }

    if (self->priv->library != NULL) {
        FT_Done_FreeType (self->priv->library);
        self->priv->library = NULL;
    }

    g_mutex_clear (&self->priv->font_list_mutex);
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

}

GtkTreeModel *
font_view_model_new (void)
{
    return g_object_new (FONT_VIEW_TYPE_MODEL, NULL);
}

