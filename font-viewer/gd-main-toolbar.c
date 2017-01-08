/*
 * Copyright (c) 2011 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Cosimo Cecchi <cosimoc@redhat.com>
 *
 */

#include "gd-main-toolbar.h"

#include <math.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (GdMainToolbar, gd_main_toolbar, GTK_TYPE_TOOLBAR)

struct _GdMainToolbarPrivate {
  GtkSizeGroup *size_group;
  GtkSizeGroup *vertical_size_group;

  GtkToolItem *left_group;
  GtkToolItem *center_group;
  GtkToolItem *right_group;

  GtkWidget *left_grid;
  GtkWidget *back;

  GtkWidget *title_label;
  GtkWidget *detail_label;

  GtkWidget *right_grid;

  GdMainToolbarMode mode;
};

enum {
  SELECTION_MODE_REQUEST = 1,
  GO_BACK_REQUEST,
  CLEAR_REQUEST,
  NUM_SIGNALS
};

static guint signals[NUM_SIGNALS] = { 0, };

static void
gd_main_toolbar_dispose (GObject *obj)
{
  GdMainToolbar *self = GD_MAIN_TOOLBAR (obj);

  g_clear_object (&self->priv->size_group);
  g_clear_object (&self->priv->vertical_size_group);

  G_OBJECT_CLASS (gd_main_toolbar_parent_class)->dispose (obj);
}

static gint
get_icon_margin (void)
{
  gint toolbar_size, menu_size;

  gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &menu_size, NULL);
  gtk_icon_size_lookup (GTK_ICON_SIZE_LARGE_TOOLBAR, &toolbar_size, NULL);
  return (gint) floor ((toolbar_size - menu_size) / 2.0);
}

static GtkSizeGroup *
get_vertical_size_group (void)
{
  GtkSizeGroup *retval;
  GtkWidget *w, *dummy;
  gint icon_margin;

  icon_margin = get_icon_margin ();

  dummy = gtk_toggle_button_new ();
  w = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU);
  g_object_set (w, "margin", icon_margin, NULL);
  gtk_container_add (GTK_CONTAINER (dummy), w);
  gtk_widget_show_all (dummy);

  retval = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  gtk_size_group_add_widget (retval, dummy);

  return retval;
}

static GtkWidget *
get_symbolic_button (const gchar *icon_name)
{
  GtkWidget *button, *w;

  button = gtk_button_new ();
  gtk_style_context_add_class (gtk_widget_get_style_context (button), "raised");

  w = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
  g_object_set (w, "margin", get_icon_margin (), NULL);
  gtk_widget_show (w);
  gtk_container_add (GTK_CONTAINER (button), w);

  return button;
}

static GtkWidget *
get_text_button (const gchar *label)
{
  GtkWidget *w;

  w = gtk_button_new_with_label (label);
  gtk_widget_set_vexpand (w, TRUE);
  gtk_style_context_add_class (gtk_widget_get_style_context (w), "raised");

  return w;
}

static void
on_back_button_clicked (GtkButton *b,
                        gpointer user_data)
{
  GdMainToolbar *self = user_data;
  g_signal_emit (self, signals[GO_BACK_REQUEST], 0);
}

static void
gd_main_toolbar_constructed (GObject *obj)
{
  GdMainToolbar *self = GD_MAIN_TOOLBAR (obj);
  GtkToolbar *tb = GTK_TOOLBAR (obj);
  GtkWidget *grid;

  G_OBJECT_CLASS (gd_main_toolbar_parent_class)->constructed (obj);

  self->priv->vertical_size_group = get_vertical_size_group ();

  /* left section */
  self->priv->left_group = gtk_tool_item_new ();
  gtk_widget_set_margin_right (GTK_WIDGET (self->priv->left_group), 12);
  gtk_toolbar_insert (tb, self->priv->left_group, -1);
  gtk_size_group_add_widget (self->priv->vertical_size_group,
                             GTK_WIDGET (self->priv->left_group));

  /* left button group */
  self->priv->left_grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (self->priv->left_grid), 12);
  gtk_container_add (GTK_CONTAINER (self->priv->left_group), self->priv->left_grid);

  self->priv->back = get_symbolic_button ("go-previous-symbolic");
  gtk_widget_set_no_show_all (self->priv->back, TRUE);
  gtk_container_add (GTK_CONTAINER (self->priv->left_grid), self->priv->back);

  g_signal_connect (self->priv->back, "clicked",
                    G_CALLBACK (on_back_button_clicked), self);

  /* center section */
  self->priv->center_group = gtk_tool_item_new ();
  gtk_tool_item_set_expand (self->priv->center_group, TRUE);
  gtk_toolbar_insert (tb, self->priv->center_group, -1);
  gtk_size_group_add_widget (self->priv->vertical_size_group,
                             GTK_WIDGET (self->priv->center_group));

  /* centered label group */
  grid = gtk_grid_new ();
  gtk_widget_set_halign (grid, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (grid, GTK_ALIGN_CENTER);
  gtk_container_add (GTK_CONTAINER (self->priv->center_group), grid);

  self->priv->title_label = gtk_label_new (NULL);
  gtk_label_set_ellipsize (GTK_LABEL (self->priv->title_label), PANGO_ELLIPSIZE_END);
  gtk_container_add (GTK_CONTAINER (grid), self->priv->title_label);

  self->priv->detail_label = gtk_label_new (NULL);
  gtk_widget_set_no_show_all (self->priv->detail_label, TRUE);
  gtk_widget_set_margin_left (self->priv->detail_label, 12);
  gtk_style_context_add_class (gtk_widget_get_style_context (self->priv->detail_label), "dim-label");
  gtk_container_add (GTK_CONTAINER (grid), self->priv->detail_label);

  /* right section */
  self->priv->right_group = gtk_tool_item_new ();
  gtk_widget_set_margin_left (GTK_WIDGET (self->priv->right_group), 12);
  gtk_toolbar_insert (tb, self->priv->right_group, -1);
  gtk_size_group_add_widget (self->priv->vertical_size_group,
                             GTK_WIDGET (self->priv->right_group));

  self->priv->right_grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (self->priv->right_grid), 12);
  gtk_container_add (GTK_CONTAINER (self->priv->right_group), self->priv->right_grid);

  self->priv->size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  gtk_size_group_add_widget (self->priv->size_group, GTK_WIDGET (self->priv->left_group));
  gtk_size_group_add_widget (self->priv->size_group, GTK_WIDGET (self->priv->right_group));
}

static void
gd_main_toolbar_init (GdMainToolbar *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GD_TYPE_MAIN_TOOLBAR, GdMainToolbarPrivate);
  self->priv->mode = GD_MAIN_TOOLBAR_MODE_INVALID;
}

static void
gd_main_toolbar_class_init (GdMainToolbarClass *klass)
{
  GObjectClass *oclass;

  oclass = G_OBJECT_CLASS (klass);
  oclass->constructed = gd_main_toolbar_constructed;
  oclass->dispose = gd_main_toolbar_dispose;

  signals[SELECTION_MODE_REQUEST] =
    g_signal_new ("selection-mode-request",
                  GD_TYPE_MAIN_TOOLBAR,
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE,
                  1, G_TYPE_BOOLEAN);
  signals[GO_BACK_REQUEST] =
    g_signal_new ("go-back-request",
                  GD_TYPE_MAIN_TOOLBAR,
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
  signals[CLEAR_REQUEST] =
    g_signal_new ("clear-request",
                  GD_TYPE_MAIN_TOOLBAR,
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  g_type_class_add_private (klass, sizeof (GdMainToolbarPrivate));
}

static void
on_selection_mode_button_clicked (GtkButton *b,
                                  gpointer user_data)
{
  GdMainToolbar *self = user_data;
  g_signal_emit (self, signals[SELECTION_MODE_REQUEST], 0, TRUE);
}

static void
on_selection_mode_done_button_clicked (GtkButton *b,
                                       gpointer user_data)
{
  GdMainToolbar *self = user_data;
  g_signal_emit (self, signals[SELECTION_MODE_REQUEST], 0, FALSE);
}

static void
gd_main_toolbar_populate_for_selection (GdMainToolbar *self)
{
  GtkWidget *w;

  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (self)),
                               "documents-selection-mode");
  gtk_widget_reset_style (GTK_WIDGET (self));

  /* right section */
  w = get_text_button (_("Done"));
  gtk_container_add (GTK_CONTAINER (self->priv->right_grid), w);

  g_signal_connect (w, "clicked",
                    G_CALLBACK (on_selection_mode_done_button_clicked), self);

  gtk_widget_show_all (GTK_WIDGET (self));
}

static void
gd_main_toolbar_populate_for_overview (GdMainToolbar *self)
{
  GtkWidget *button;

  /* right section */
  button = get_symbolic_button ("emblem-default-symbolic");
  gtk_container_add (GTK_CONTAINER (self->priv->right_grid), button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (on_selection_mode_button_clicked), self);

  gtk_widget_show_all (GTK_WIDGET (self));
}

static void
gd_main_toolbar_populate_for_preview (GdMainToolbar *self)
{
  gtk_widget_show (self->priv->back);
  gtk_widget_show_all (GTK_WIDGET (self));
}

static void
on_left_grid_clear (GtkWidget *w,
                    gpointer user_data)
{
  GdMainToolbar *self = user_data;

  if (w != self->priv->back)
    gtk_widget_destroy (w);
}

static void
gd_main_toolbar_clear (GdMainToolbar *self)
{
  GtkStyleContext *context;

  /* reset labels */
  gtk_label_set_text (GTK_LABEL (self->priv->title_label), "");
  gtk_label_set_text (GTK_LABEL (self->priv->detail_label), "");

  /* clear all on the left, except the back button */
  gtk_widget_hide (self->priv->back);
  gtk_container_foreach (GTK_CONTAINER (self->priv->left_grid),
                         on_left_grid_clear, self);

  /* clear all on the right */
  gtk_container_foreach (GTK_CONTAINER (self->priv->right_grid), 
                         (GtkCallback) gtk_widget_destroy, self);

  context = gtk_widget_get_style_context (GTK_WIDGET (self));
  if (gtk_style_context_has_class (context, "documents-selection-mode"))
    {
      gtk_style_context_remove_class (context, "documents-selection-mode");
      gtk_widget_reset_style (GTK_WIDGET (self));
    }

  g_signal_emit (self, signals[CLEAR_REQUEST], 0);
}

/**
 * gd_main_toolbar_set_mode:
 * @mode:
 *
 */
void
gd_main_toolbar_set_mode (GdMainToolbar *self,
                          GdMainToolbarMode mode)
{
  if (mode == self->priv->mode)
    return;

  gd_main_toolbar_clear (self);
  self->priv->mode = mode;

  switch (mode)
    {
    case GD_MAIN_TOOLBAR_MODE_OVERVIEW:
      gd_main_toolbar_populate_for_overview (self);
      break;
    case GD_MAIN_TOOLBAR_MODE_SELECTION:
      gd_main_toolbar_populate_for_selection (self);
      break;
    case GD_MAIN_TOOLBAR_MODE_PREVIEW:
      gd_main_toolbar_populate_for_preview (self);
      break;
    default:
      g_assert_not_reached ();
      break;
    }
}

GdMainToolbarMode
gd_main_toolbar_get_mode (GdMainToolbar *self)
{
  return self->priv->mode;
}

/**
 * gd_main_toolbar_set_labels:
 * @self:
 * @primary: (allow-none):
 * @detail: (allow-none):
 *
 */
void
gd_main_toolbar_set_labels (GdMainToolbar *self,
                            const gchar *primary,
                            const gchar *detail)
{
  gchar *real_primary = NULL;

  if (primary != NULL)
    real_primary = g_markup_printf_escaped ("<b>%s</b>", primary);

  if (real_primary == NULL)
    {
      gtk_label_set_markup (GTK_LABEL (self->priv->title_label), "");
      gtk_widget_hide (self->priv->title_label);
    }
  else
    {
      gtk_label_set_markup (GTK_LABEL (self->priv->title_label), real_primary);
      gtk_widget_show (self->priv->title_label);
    }

  if (detail == NULL)
    {
      gtk_label_set_text (GTK_LABEL (self->priv->detail_label), "");
      gtk_widget_hide (self->priv->detail_label);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (self->priv->detail_label), detail);
      gtk_widget_show (self->priv->detail_label);
    }

  g_free (real_primary);
}

/**
 * gd_main_toolbar_set_back_visible:
 * @self:
 * @visible:
 *
 */
void
gd_main_toolbar_set_back_visible (GdMainToolbar *self,
                                  gboolean visible)
{
  if (visible != gtk_widget_get_visible (self->priv->back))
    gtk_widget_set_visible (self->priv->back, visible);

}

GtkWidget *
gd_main_toolbar_new (void)
{
  return g_object_new (GD_TYPE_MAIN_TOOLBAR, NULL);
}

GtkWidget *
gd_main_toolbar_add_button (GdMainToolbar *self,
                            const gchar *icon_name,
                            const gchar *label,
                            gboolean pack_start)
{
  GtkWidget *button;

  if (icon_name != NULL)
    {
      button = get_symbolic_button (icon_name);
      if (label != NULL)
        gtk_widget_set_tooltip_text (button, label);
    }
  else if (label != NULL)
    {
      button = get_text_button (label);
    }

  if (pack_start)
    gtk_container_add (GTK_CONTAINER (self->priv->left_grid), button);
  else
    gtk_container_add (GTK_CONTAINER (self->priv->right_grid), button);    

  gtk_widget_show_all (button);

  return button;
}

