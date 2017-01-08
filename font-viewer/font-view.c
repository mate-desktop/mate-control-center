/* -*- mode: C; c-basic-offset: 4 -*- */

/*
 * font-view: a font viewer for MATE
 *
 * Copyright (C) 2002-2003  James Henstridge <james@daa.com.au>
 * Copyright (C) 2010 Cosimo Cecchi <cosimoc@gnome.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple - Place Suite 330, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TYPE1_TABLES_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H
#include <cairo/cairo-ft.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "font-model.h"
#include "gd-main-toolbar.h"
#include "sushi-font-widget.h"

#define FONT_VIEW_TYPE_APPLICATION font_view_application_get_type()
#define FONT_VIEW_APPLICATION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), FONT_VIEW_TYPE_APPLICATION, FontViewApplication))

typedef struct {
    GtkApplication parent;

    GtkWidget *main_window;
    GtkWidget *main_grid;
    GtkWidget *toolbar;
    GtkWidget *title_label;
    GtkWidget *side_grid;
    GtkWidget *font_widget;
    GtkWidget *info_button;
    GtkWidget *install_button;
    GtkWidget *back_button;
    GtkWidget *notebook;
    GtkWidget *swin_view;
    GtkWidget *swin_preview;
    GtkWidget *icon_view;

    GtkTreeModel *model;

    GFile *font_file;
} FontViewApplication;

typedef struct {
    GtkApplicationClass parent_class;
} FontViewApplicationClass;

G_DEFINE_TYPE (FontViewApplication, font_view_application, GTK_TYPE_APPLICATION);

static void font_view_application_do_overview (FontViewApplication *self);

static const gchar *app_menu =
    "<interface>"
    "  <menu id=\"app-menu\">"
    "    <section>"
    "      <item>"
    "        <attribute name=\"action\">app.about</attribute>"
    "	     <attribute name=\"label\" translatable=\"yes\">About Font Viewer</attribute>"
    "      </item>"
    "      <item>"
    "       <attribute name=\"action\">app.quit</attribute>"
    "	    <attribute name=\"label\" translatable=\"yes\">Quit</attribute>"
    "      </item>"
    "    </section>"
    "  <menu>"
    "</interface>";

#define VIEW_ITEM_WIDTH 140
#define VIEW_ITEM_WRAP_WIDTH 128
#define VIEW_COLUMN_SPACING 36
#define VIEW_MARGIN 16

#define WHITESPACE_CHARS "\f \t"

static void
strip_whitespace (gchar **copyright)
{
    GString *reassembled;
    gchar **split;
    const gchar *str;
    gint idx, n_stripped;
    size_t len;

    split = g_strsplit (*copyright, "\n", -1);
    reassembled = g_string_new (NULL);
    n_stripped = 0;

    for (idx = 0; split[idx] != NULL; idx++) {
        str = split[idx];

        len = strspn (str, WHITESPACE_CHARS);
        if (len)
            str += len;

        if (n_stripped++ > 0)
            g_string_append (reassembled, "\n");
        g_string_append (reassembled, str);
    }

    g_strfreev (split);
    g_free (*copyright);

    *copyright = g_string_free (reassembled, FALSE);
}

static void
add_row (GtkWidget *grid,
	 const gchar *name,
	 const gchar *value,
	 gboolean multiline)
{
    GtkWidget *name_w, *label;

    name_w = gtk_label_new (name);
    gtk_style_context_add_class (gtk_widget_get_style_context (name_w), "dim-label");
    gtk_misc_set_alignment (GTK_MISC (name_w), 1.0, 0.0);

    gtk_container_add (GTK_CONTAINER (grid), name_w);

    label = gtk_label_new (value);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    gtk_label_set_selectable (GTK_LABEL(label), TRUE);

    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

    if (multiline && g_utf8_strlen (value, -1) > 64) {
        gtk_label_set_width_chars (GTK_LABEL (label), 64);
        gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
    }
    gtk_label_set_max_width_chars (GTK_LABEL (label), 64);

    gtk_grid_attach_next_to (GTK_GRID (grid), label, 
                             name_w, GTK_POS_RIGHT,
                             1, 1);
}

static void
populate_grid (FontViewApplication *self,
               GtkWidget *grid,
	       FT_Face face)
{
    gchar *s;
    GFileInfo *info;
    PS_FontInfoRec ps_info;

    add_row (grid, _("Name"), face->family_name, FALSE);

    if (face->style_name)
        add_row (grid, _("Style"), face->style_name, FALSE);

    info = g_file_query_info (self->font_file,
                              G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
                              G_FILE_ATTRIBUTE_STANDARD_SIZE,
                              G_FILE_QUERY_INFO_NONE,
                              NULL, NULL);

    if (info != NULL) {
        s = g_content_type_get_description (g_file_info_get_content_type (info));
        add_row (grid, _("Type"), s, FALSE);
        g_free (s);

        g_object_unref (info);
    }

    if (FT_IS_SFNT (face)) {
        gint i, len;
        gchar *version = NULL, *copyright = NULL, *description = NULL;

        len = FT_Get_Sfnt_Name_Count (face);
        for (i = 0; i < len; i++) {
            FT_SfntName sname;

            if (FT_Get_Sfnt_Name (face, i, &sname) != 0)
                continue;

            /* only handle the unicode names for US langid */
            if (!(sname.platform_id == TT_PLATFORM_MICROSOFT &&
                  sname.encoding_id == TT_MS_ID_UNICODE_CS &&
                  sname.language_id == TT_MS_LANGID_ENGLISH_UNITED_STATES))
                continue;

            switch (sname.name_id) {
            case TT_NAME_ID_COPYRIGHT:
                g_free (copyright);
                copyright = g_convert ((gchar *)sname.string, sname.string_len,
                                       "UTF-8", "UTF-16BE", NULL, NULL, NULL);
                break;
            case TT_NAME_ID_VERSION_STRING:
            g_free (version);
            version = g_convert ((gchar *)sname.string, sname.string_len,
                                 "UTF-8", "UTF-16BE", NULL, NULL, NULL);
                break;
            case TT_NAME_ID_DESCRIPTION:
                g_free (description);
                description = g_convert ((gchar *)sname.string, sname.string_len,
                                         "UTF-8", "UTF-16BE", NULL, NULL, NULL);
                break;
            default:
                break;
            }
        }

	if (version) {
	    add_row (grid, _("Version"), version, FALSE);
	    g_free (version);
	}
	if (copyright) {
	    strip_whitespace (&copyright);
	    add_row (self->side_grid, _("Copyright"), copyright, TRUE);
	    g_free (copyright);
	}
	if (description) {
	    add_row (grid, _("Description"), description, TRUE);
	    g_free (description);
	}
    } else if (FT_Get_PS_Font_Info (face, &ps_info) == 0) {
	if (ps_info.version && g_utf8_validate (ps_info.version, -1, NULL))
	    add_row (grid, _("Version"), ps_info.version, FALSE);
	if (ps_info.notice && g_utf8_validate (ps_info.notice, -1, NULL))
	    add_row (grid, _("Copyright"), ps_info.notice, TRUE);
    }
}

static void
install_button_refresh_appearance (FontViewApplication *self,
                                   GError *error)
{
    gchar *path;

    if (error != NULL) {
        gtk_button_set_label (GTK_BUTTON (self->install_button), _("Install Failed"));
        gtk_widget_set_sensitive (self->install_button, FALSE);
    } else {
        path = g_file_get_path (self->font_file);

        if (font_view_model_get_iter_for_file (FONT_VIEW_MODEL (self->model), path, NULL)) {
            gtk_button_set_label (GTK_BUTTON (self->install_button), _("Installed"));
            gtk_widget_set_sensitive (self->install_button, FALSE);
        } else {
            gtk_button_set_label (GTK_BUTTON (self->install_button), _("Install"));
            gtk_widget_set_sensitive (self->install_button, TRUE);
        }

        g_free (path);
    }
}

static void
font_install_finished_cb (GObject      *source_object,
                          GAsyncResult *res,
                          gpointer      user_data)
{
    FontViewApplication *self = user_data;
    GError *err = NULL;

    g_file_copy_finish (G_FILE (source_object), res, &err);
    install_button_refresh_appearance (self, err);

    if (err != NULL) {
        g_debug ("Install failed: %s", err->message);
        g_error_free (err);
    }
}

static void
install_button_clicked_cb (GtkButton *button,
                           gpointer user_data)
{
    FontViewApplication *self = user_data;
    GFile *dest;
    gchar *dest_path, *dest_filename;
    GError *err = NULL;

    /* first check if ~/.fonts exists */
    dest_path = g_build_filename (g_get_home_dir (), ".fonts", NULL);
    if (!g_file_test (dest_path, G_FILE_TEST_EXISTS)) {
        GFile *f = g_file_new_for_path (dest_path);
        g_file_make_directory_with_parents (f, NULL, &err);
        g_object_unref (f);
        if (err) {
            /* TODO: show error dialog */
            g_warning ("Could not create fonts directory: %s", err->message);
            g_error_free (err);
            g_free (dest_path);
            return;
        }
    }
    g_free (dest_path);

    /* create destination filename */
    dest_filename = g_file_get_basename (self->font_file);
    dest_path = g_build_filename (g_get_home_dir (), ".fonts", dest_filename,
				  NULL);
    g_free (dest_filename);

    dest = g_file_new_for_path (dest_path);

    /* TODO: show error dialog if file exists */
    g_file_copy_async (self->font_file, dest, G_FILE_COPY_NONE, 0, NULL, NULL, NULL,
                       font_install_finished_cb, self);

    g_object_unref (dest);
    g_free (dest_path);
}

static void
back_button_clicked_cb (GtkButton *button,
                        gpointer user_data)
{
    FontViewApplication *self = user_data;
    font_view_application_do_overview (self);
}

static void
font_widget_loaded_cb (SushiFontWidget *font_widget,
                       gpointer user_data)
{
    FontViewApplication *self = user_data;
    FT_Face face = sushi_font_widget_get_ft_face (font_widget);

    if (face == NULL)
        return;

    gd_main_toolbar_set_labels (GD_MAIN_TOOLBAR (self->toolbar),
                                face->family_name, face->style_name);
}

static void
info_button_clicked_cb (GtkButton *button,
                        gpointer user_data)
{
    FontViewApplication *self = user_data;
    GtkWidget *grid, *dialog;
    FT_Face face = sushi_font_widget_get_ft_face (SUSHI_FONT_WIDGET (self->font_widget));

    if (face == NULL)
        return;

    grid = gtk_grid_new ();
    gtk_orientable_set_orientation (GTK_ORIENTABLE (grid), GTK_ORIENTATION_VERTICAL);
    g_object_set (grid,
                  "margin-top", 6,
                  "margin-left", 16,
                  "margin-right", 16,
                  "margin-bottom", 6,
                  NULL);
    gtk_grid_set_column_spacing (GTK_GRID (grid), 8);
    gtk_grid_set_row_spacing (GTK_GRID (grid), 2);

    populate_grid (self, grid, face);

    dialog = gtk_dialog_new_with_buttons (NULL, GTK_WINDOW (self->main_window),
                                          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                          NULL);
    gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), grid);
    g_signal_connect (dialog, "response",
                      G_CALLBACK (gtk_widget_destroy), NULL);
    gtk_widget_show_all (dialog);
}

static void
font_view_application_do_open (FontViewApplication *self)
{
    gchar *uri;
    GtkWidget *font_widget;
    GdkRGBA white = { 1.0, 1.0, 1.0, 1.0 };
    GdkRGBA black = { 0.0, 0.0, 0.0, 1.0 };
    GtkWidget *w;

    w = gtk_bin_get_child (GTK_BIN (self->swin_preview));
    if (w)
        gtk_widget_destroy (w);

    self->info_button = gd_main_toolbar_add_button (GD_MAIN_TOOLBAR (self->toolbar),
                                                    NULL, _("Info"), 
                                                    FALSE);
    g_signal_connect (self->info_button, "clicked",
                      G_CALLBACK (info_button_clicked_cb), self);

    /* add install button */
    self->install_button = gd_main_toolbar_add_button (GD_MAIN_TOOLBAR (self->toolbar),
                                                       NULL, _("Install"), 
                                                       FALSE);
    install_button_refresh_appearance (self, NULL);
    g_signal_connect (self->install_button, "clicked",
                      G_CALLBACK (install_button_clicked_cb), self);

    self->back_button = gd_main_toolbar_add_button (GD_MAIN_TOOLBAR (self->toolbar),
                                                    "go-previous-symbolic", _("Back"),
                                                    TRUE);
    g_signal_connect (self->back_button, "clicked",
                      G_CALLBACK (back_button_clicked_cb), self);

    gtk_widget_set_vexpand (self->toolbar, FALSE);

    uri = g_file_get_uri (self->font_file);
    self->font_widget = font_widget = GTK_WIDGET (sushi_font_widget_new (uri));

    gtk_widget_override_color (font_widget, GTK_STATE_NORMAL, &black);
    gtk_widget_override_background_color (font_widget, GTK_STATE_FLAG_NORMAL, &white);
    g_free (uri);

    w = gtk_viewport_new (NULL, NULL);
    gtk_viewport_set_shadow_type (GTK_VIEWPORT (w), GTK_SHADOW_NONE);

    gtk_container_add (GTK_CONTAINER (w), font_widget);
    gtk_container_add (GTK_CONTAINER (self->swin_preview), w);

    g_signal_connect (font_widget, "loaded",
                      G_CALLBACK (font_widget_loaded_cb), self);

    gtk_widget_show_all (self->swin_preview);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (self->notebook), 1);
}

static gboolean
icon_view_release_cb (GtkWidget *widget,
                      GdkEventButton *event,
                      gpointer user_data)
{
    FontViewApplication *self = user_data;
    GtkTreePath *path;
    GtkTreeIter iter;
    gchar *font_path;

    /* eat double/triple click events */
    if (event->type != GDK_BUTTON_RELEASE)
        return TRUE;

    path = gtk_icon_view_get_path_at_pos (GTK_ICON_VIEW (widget),
                                          event->x, event->y);

    if (path != NULL &&
        gtk_tree_model_get_iter (self->model, &iter, path)) {
        gtk_tree_model_get (self->model, &iter,
                            COLUMN_PATH, &font_path,
                            -1);

        if (font_path != NULL) {
            self->font_file = g_file_new_for_path (font_path);
            font_view_application_do_open (self);
        }
        gtk_tree_path_free (path);
        g_free (font_path);
    }

    return FALSE;
}

static void
font_view_application_do_overview (FontViewApplication *self)
{
    g_clear_object (&self->font_file);

    if (self->back_button) {
        gtk_widget_destroy (self->back_button);
        self->back_button = NULL;
    }

    if (self->info_button) {
        gtk_widget_destroy (self->info_button);
        self->info_button = NULL;
    }

    if (self->install_button) {
        gtk_widget_destroy (self->install_button);
        self->install_button = NULL;
    }

    gd_main_toolbar_set_labels (GD_MAIN_TOOLBAR (self->toolbar), _("All Fonts"), NULL);

    if (self->icon_view == NULL) {
        GtkWidget *icon_view;
        GtkCellRenderer *cell;

        self->icon_view = icon_view = gtk_icon_view_new_with_model (self->model);
        g_object_set (icon_view,
                      "column-spacing", VIEW_COLUMN_SPACING,
                      "margin", VIEW_MARGIN,
                      NULL);

        gtk_widget_set_vexpand (GTK_WIDGET (icon_view), TRUE);
        gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (icon_view), GTK_SELECTION_NONE);
        gtk_container_add (GTK_CONTAINER (self->swin_view), icon_view);

        cell = gtk_cell_renderer_pixbuf_new ();
        g_object_set (cell,
                      "xalign", 0.5,
                      "yalign", 0.5,
                      NULL);

        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (icon_view), cell, FALSE);
        gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (icon_view), cell,
                                       "pixbuf", COLUMN_ICON);
        
        cell = gtk_cell_renderer_text_new ();
        g_object_set (cell,
                      "alignment", PANGO_ALIGN_CENTER,
                      "xalign", 0.5,
                      "wrap-mode", PANGO_WRAP_WORD_CHAR,
                      "wrap-width", VIEW_ITEM_WRAP_WIDTH,
                      NULL);
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (icon_view), cell, FALSE);
        gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (icon_view), cell,
                                       "text", COLUMN_NAME);

        g_signal_connect (icon_view, "button-release-event",
                          G_CALLBACK (icon_view_release_cb), self);
    }

    gtk_notebook_set_current_page (GTK_NOTEBOOK (self->notebook), 0);
    gtk_widget_show_all (self->swin_view);
}

static void
font_view_application_open (GApplication *application,
                            GFile **files,
                            gint n_files,
                            const gchar *hint)
{
    FontViewApplication *self = FONT_VIEW_APPLICATION (application);

    self->font_file = g_object_ref (files[0]);
    font_view_application_do_open (self);
}

static void
action_quit (GSimpleAction *action,
             GVariant *parameter,
             gpointer user_data)
{
    FontViewApplication *self = user_data;
    gtk_widget_destroy (self->main_window);
}

static void
action_about (GSimpleAction *action,
              GVariant *parameter,
              gpointer user_data)
{
    FontViewApplication *self = user_data;
    const gchar *authors[] = {
        "Mate Developer",
        "Cosimo Cecchi",
        "James Henstridge",
        NULL
    };

    gtk_show_about_dialog (GTK_WINDOW (self->main_window),
                           "version", VERSION,
                           "authors", authors,
                           "program-name", _("Font Viewer"),
                           "comments", _("View fonts on your system"),
                           "logo-icon-name", "preferences-desktop-font",
                           "translator-credits", _("translator-credits"),
                           "license-type", GTK_LICENSE_GPL_2_0,
                           "wrap-license", TRUE,
                           NULL);
                           
}

static GActionEntry action_entries[] = {
    { "about", action_about, NULL, NULL, NULL },
    { "quit", action_quit, NULL, NULL, NULL }
};

static void
font_view_application_startup (GApplication *application)
{
    FontViewApplication *self = FONT_VIEW_APPLICATION (application);
    GtkWidget *window, *swin;
    GtkBuilder *builder;
    GMenuModel *menu;

    G_APPLICATION_CLASS (font_view_application_parent_class)->startup (application);

    g_action_map_add_action_entries (G_ACTION_MAP (self), action_entries, 
                                     G_N_ELEMENTS (action_entries), self);
    builder = gtk_builder_new ();
    gtk_builder_add_from_string (builder, app_menu, -1, NULL);
    menu = G_MENU_MODEL (gtk_builder_get_object (builder, "app-menu"));
    gtk_application_set_app_menu (GTK_APPLICATION (self), menu);

    g_object_unref (builder);
    g_object_unref (menu);

    self->model = font_view_model_new ();

    self->main_window = window = gtk_application_window_new (GTK_APPLICATION (application));
    gtk_window_set_resizable (GTK_WINDOW (window), TRUE);
    gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);
    gtk_window_set_hide_titlebar_when_maximized (GTK_WINDOW (window), TRUE);
    gtk_window_set_title (GTK_WINDOW (window), _("Font Viewer"));

    self->main_grid = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (self->main_window), self->main_grid);

    self->toolbar = gd_main_toolbar_new ();
    gtk_style_context_add_class (gtk_widget_get_style_context (self->toolbar), "menubar");
    gtk_container_add (GTK_CONTAINER (self->main_grid), self->toolbar);

    self->notebook = gtk_notebook_new ();
    gtk_container_add (GTK_CONTAINER (self->main_grid), self->notebook);
    gtk_widget_set_hexpand (self->notebook, TRUE);
    gtk_widget_set_vexpand (self->notebook, TRUE);
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (self->notebook), FALSE);

    self->swin_view = swin = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (swin), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin),
				    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (self->notebook), swin);

    self->swin_preview = swin = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (swin), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin),
         			    GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gtk_container_add (GTK_CONTAINER (self->notebook), swin);

    gtk_widget_show_all (window);
}

static void
font_view_application_activate (GApplication *application)
{
    FontViewApplication *self = FONT_VIEW_APPLICATION (application);

    G_APPLICATION_CLASS (font_view_application_parent_class)->activate (application);
    font_view_application_do_overview (self);
}

static void
font_view_application_dispose (GObject *obj)
{
    FontViewApplication *self = FONT_VIEW_APPLICATION (obj);

    g_clear_object (&self->model);

    G_OBJECT_CLASS (font_view_application_parent_class)->dispose (obj);
}

static void
font_view_application_init (FontViewApplication *self)
{
    /* do nothing */
}

static void
font_view_application_class_init (FontViewApplicationClass *klass)
{
    GObjectClass *oclass = G_OBJECT_CLASS (klass);
    GApplicationClass *aclass = G_APPLICATION_CLASS (klass);

    aclass->activate = font_view_application_activate;
    aclass->startup = font_view_application_startup;
    aclass->open = font_view_application_open;

    oclass->dispose = font_view_application_dispose;
}

static GApplication *
font_view_application_new (void)
{
    g_type_init ();
    return g_object_new (FONT_VIEW_TYPE_APPLICATION,
                         "application-id", "org.mate.font-viewer",
                         "flags", G_APPLICATION_HANDLES_OPEN,
                         NULL);
}

int
main (int argc,
      char **argv)
{
    GApplication *app;
    gint retval;

    bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    app = font_view_application_new ();
    retval = g_application_run (app, argc, argv);

    g_object_unref (app);
    return retval;
}
