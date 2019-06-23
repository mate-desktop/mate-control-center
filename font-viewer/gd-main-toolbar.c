/*
 * Copyright (c) 2011, 2012 Red Hat, Inc.
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

typedef enum {
  CHILD_NORMAL = 0,
  CHILD_TOGGLE = 1,
  CHILD_MENU = 2,
} ChildType;

struct _GdMainToolbarPrivate {
  GtkSizeGroup *size_group;
  GtkSizeGroup *vertical_size_group;

  GtkToolItem *left_group;
  GtkToolItem *center_group;
  GtkToolItem *right_group;

  GtkWidget *left_grid;
  GtkWidget *center_grid;

  GtkWidget *labels_grid;
  GtkWidget *title_label;
  GtkWidget *detail_label;

  GtkWidget *modes_box;

  GtkWidget *center_menu;
  GtkWidget *center_menu_child;

  GtkWidget *right_grid;

  gboolean show_modes;
};

enum {
        PROP_0,
        PROP_SHOW_MODES,
};

G_DEFINE_TYPE_WITH_PRIVATE (GdMainToolbar, gd_main_toolbar, GTK_TYPE_TOOLBAR)

static void
gd_main_toolbar_dispose (GObject *obj)
{
  GdMainToolbar *self = GD_MAIN_TOOLBAR (obj);

  g_clear_object (&self->priv->size_group);
  g_clear_object (&self->priv->vertical_size_group);

  G_OBJECT_CLASS (gd_main_toolbar_parent_class)->dispose (obj);
}

static GtkWidget *
get_empty_button (ChildType type)
{
  GtkWidget *button;

  switch (type)
    {
    case CHILD_MENU:
      button = gtk_menu_button_new ();
      break;
    case CHILD_TOGGLE:
      button = gtk_toggle_button_new ();
      break;
    case CHILD_NORMAL:
    default:
      button = gtk_button_new ();
      break;
    }

  return button;
}

static GtkWidget *
get_symbolic_button (const gchar *icon_name,
                     ChildType    type)
{
  GtkWidget *button, *w;

  switch (type)
    {
    case CHILD_MENU:
      button = gtk_menu_button_new ();
      gtk_widget_destroy (gtk_bin_get_child (GTK_BIN (button)));
      break;
    case CHILD_TOGGLE:
      button = gtk_toggle_button_new ();
      break;
    case CHILD_NORMAL:
    default:
      button = gtk_button_new ();
      break;
    }

  gtk_style_context_add_class (gtk_widget_get_style_context (button), "raised");
  gtk_style_context_add_class (gtk_widget_get_style_context (button), "image-button");

  w = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
  gtk_widget_show (w);
  gtk_container_add (GTK_CONTAINER (button), w);

  return button;
}

static GtkWidget *
get_text_button (const gchar *label,
                 ChildType    type)
{
  GtkWidget *button, *w;

  switch (type)
    {
    case CHILD_MENU:
      button = gtk_menu_button_new ();
      gtk_widget_destroy (gtk_bin_get_child (GTK_BIN (button)));

      w = gtk_label_new (label);
      gtk_widget_show (w);
      gtk_container_add (GTK_CONTAINER (button), w);
      break;
    case CHILD_TOGGLE:
      button = gtk_toggle_button_new_with_label (label);
      break;
    case CHILD_NORMAL:
    default:
      button = gtk_button_new_with_label (label);
      break;
    }

  gtk_style_context_add_class (gtk_widget_get_style_context (button), "raised");
  gtk_style_context_add_class (gtk_widget_get_style_context (button), "text-button");

  return button;
}

static GtkSizeGroup *
get_vertical_size_group (GdMainToolbar *self)
{
  GtkSizeGroup *retval;
  GtkWidget *dummy;
  GtkToolItem *container;

  dummy = get_text_button ("Dummy", CHILD_NORMAL);
  container = gtk_tool_item_new ();
  gtk_widget_set_no_show_all (GTK_WIDGET (container), TRUE);
  gtk_container_add (GTK_CONTAINER (container), dummy);
  gtk_toolbar_insert (GTK_TOOLBAR (self), container, -1);

  retval = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  gtk_size_group_add_widget (retval, dummy);

  return retval;
}

gboolean
gd_main_toolbar_get_show_modes (GdMainToolbar *self)
{
  return self->priv->show_modes;
}

void
gd_main_toolbar_set_show_modes (GdMainToolbar *self,
                                gboolean show_modes)
{
  if (self->priv->show_modes == show_modes)
    return;

  self->priv->show_modes = show_modes;
  if (self->priv->show_modes)
    {
      gtk_widget_set_no_show_all (self->priv->labels_grid, TRUE);
      gtk_widget_hide (self->priv->labels_grid);

      gtk_widget_set_valign (self->priv->center_grid, GTK_ALIGN_FILL);
      gtk_widget_set_no_show_all (self->priv->modes_box, FALSE);
      gtk_widget_show_all (self->priv->modes_box);
    }
  else
    {
      gtk_widget_set_no_show_all (self->priv->modes_box, TRUE);
      gtk_widget_hide (self->priv->modes_box);

      gtk_widget_set_valign (self->priv->center_grid, GTK_ALIGN_CENTER);
      gtk_widget_set_no_show_all (self->priv->labels_grid, FALSE);
      gtk_widget_show_all (self->priv->labels_grid);
    }

  g_object_notify (G_OBJECT (self), "show-modes");
}

static void
gd_main_toolbar_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{

  GdMainToolbar *self = GD_MAIN_TOOLBAR (object);

  switch (prop_id)
    {
    case PROP_SHOW_MODES:
      gd_main_toolbar_set_show_modes (GD_MAIN_TOOLBAR (self), g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gd_main_toolbar_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GdMainToolbar *self = GD_MAIN_TOOLBAR (object);

  switch (prop_id)
    {
    case PROP_SHOW_MODES:
      g_value_set_boolean (value, self->priv->show_modes);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gd_main_toolbar_constructed (GObject *obj)
{
  GdMainToolbar *self = GD_MAIN_TOOLBAR (obj);
  GtkToolbar *tb = GTK_TOOLBAR (obj);
  GtkWidget *grid;

  G_OBJECT_CLASS (gd_main_toolbar_parent_class)->constructed (obj);

  self->priv->vertical_size_group = get_vertical_size_group (self);

  /* left section */
  self->priv->left_group = gtk_tool_item_new ();
  gtk_widget_set_margin_end (GTK_WIDGET (self->priv->left_group), 12);
  gtk_toolbar_insert (tb, self->priv->left_group, -1);
  gtk_size_group_add_widget (self->priv->vertical_size_group,
                             GTK_WIDGET (self->priv->left_group));

  /* left button group */
  self->priv->left_grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (self->priv->left_grid), 12);
  gtk_container_add (GTK_CONTAINER (self->priv->left_group), self->priv->left_grid);
  gtk_widget_set_halign (self->priv->left_grid, GTK_ALIGN_START);

  /* center section */
  self->priv->center_group = gtk_tool_item_new ();
  gtk_tool_item_set_expand (self->priv->center_group, TRUE);
  gtk_toolbar_insert (tb, self->priv->center_group, -1);
  self->priv->center_grid = gtk_grid_new ();
  gtk_widget_set_halign (self->priv->center_grid, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (self->priv->center_grid, GTK_ALIGN_CENTER);
  gtk_container_add (GTK_CONTAINER (self->priv->center_group), self->priv->center_grid);
  gtk_size_group_add_widget (self->priv->vertical_size_group,
                             GTK_WIDGET (self->priv->center_group));

  /* centered label group */
  self->priv->labels_grid = grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_container_add (GTK_CONTAINER (self->priv->center_grid), grid);

  self->priv->title_label = gtk_label_new (NULL);
  gtk_label_set_ellipsize (GTK_LABEL (self->priv->title_label), PANGO_ELLIPSIZE_END);
  gtk_container_add (GTK_CONTAINER (grid), self->priv->title_label);

  self->priv->detail_label = gtk_label_new (NULL);
  gtk_label_set_ellipsize (GTK_LABEL (self->priv->detail_label), PANGO_ELLIPSIZE_END);
  gtk_widget_set_no_show_all (self->priv->detail_label, TRUE);
  gtk_style_context_add_class (gtk_widget_get_style_context (self->priv->detail_label), "dim-label");
  gtk_container_add (GTK_CONTAINER (grid), self->priv->detail_label);

  /* centered mode group */
  self->priv->modes_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_homogeneous (GTK_BOX (self->priv->modes_box), TRUE);
  gtk_widget_set_no_show_all (self->priv->modes_box, TRUE);
  gtk_style_context_add_class (gtk_widget_get_style_context (self->priv->modes_box), "linked");
  gtk_container_add (GTK_CONTAINER (self->priv->center_grid), self->priv->modes_box);

  /* right section */
  self->priv->right_group = gtk_tool_item_new ();
  gtk_widget_set_margin_start (GTK_WIDGET (self->priv->right_group), 12);
  gtk_toolbar_insert (tb, self->priv->right_group, -1);
  gtk_size_group_add_widget (self->priv->vertical_size_group,
                             GTK_WIDGET (self->priv->right_group));

  self->priv->right_grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (self->priv->right_grid), 12);
  gtk_container_add (GTK_CONTAINER (self->priv->right_group), self->priv->right_grid);
  gtk_widget_set_halign (self->priv->right_grid, GTK_ALIGN_END);

  self->priv->size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  gtk_size_group_add_widget (self->priv->size_group, GTK_WIDGET (self->priv->left_group));
  gtk_size_group_add_widget (self->priv->size_group, GTK_WIDGET (self->priv->right_group));
}

static void
gd_main_toolbar_init (GdMainToolbar *self)
{
  self->priv = gd_main_toolbar_get_instance_private (self);
}

static void
gd_main_toolbar_class_init (GdMainToolbarClass *klass)
{
  GObjectClass *oclass;

  oclass = G_OBJECT_CLASS (klass);
  oclass->constructed = gd_main_toolbar_constructed;
  oclass->set_property = gd_main_toolbar_set_property;
  oclass->get_property = gd_main_toolbar_get_property;
  oclass->dispose = gd_main_toolbar_dispose;

  g_object_class_install_property (oclass,
                                   PROP_SHOW_MODES,
                                   g_param_spec_boolean ("show-modes",
                                                         "Show Modes",
                                                         "Show Modes",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
}

void
gd_main_toolbar_clear (GdMainToolbar *self)
{
  /* reset labels */
  gtk_label_set_text (GTK_LABEL (self->priv->title_label), "");
  gtk_label_set_text (GTK_LABEL (self->priv->detail_label), "");

  /* clear all added buttons */
  gtk_container_foreach (GTK_CONTAINER (self->priv->left_grid),
                         (GtkCallback) gtk_widget_destroy, self);
  gtk_container_foreach (GTK_CONTAINER (self->priv->modes_box),
                         (GtkCallback) gtk_widget_destroy, self);
  gtk_container_foreach (GTK_CONTAINER (self->priv->right_grid),
                         (GtkCallback) gtk_widget_destroy, self);
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

GtkWidget *
gd_main_toolbar_new (void)
{
  return g_object_new (GD_TYPE_MAIN_TOOLBAR, NULL);
}

static GtkWidget *
add_button_internal (GdMainToolbar *self,
                     const gchar *icon_name,
                     const gchar *label,
                     gboolean pack_start,
                     ChildType type)
{
  GtkWidget *button;

  if (icon_name != NULL)
    {
      button = get_symbolic_button (icon_name, type);
      if (label != NULL)
        gtk_widget_set_tooltip_text (button, label);
    }
  else if (label != NULL)
    {
      button = get_text_button (label, type);
    }
  else
    {
      button = get_empty_button (type);
    }

  gd_main_toolbar_add_widget (self, button, pack_start);

  gtk_widget_show_all (button);

  return button;
}

/**
 * gd_main_toolbar_set_labels_menu:
 * @self:
 * @menu: (allow-none):
 *
 */
void
gd_main_toolbar_set_labels_menu (GdMainToolbar *self,
                                 GMenuModel    *menu)
{
  GtkWidget *button, *grid, *w;

  if (menu == NULL &&
      ((gtk_widget_get_parent (self->priv->labels_grid) == self->priv->center_grid) ||
       self->priv->center_menu_child == NULL))
    return;

  if (menu != NULL)
    {
      g_object_ref (self->priv->labels_grid);
      gtk_container_remove (GTK_CONTAINER (self->priv->center_grid),
                            self->priv->labels_grid);

      self->priv->center_menu_child = grid = gtk_grid_new ();
      gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
      gtk_container_add (GTK_CONTAINER (grid), self->priv->labels_grid);
      g_object_unref (self->priv->labels_grid);

      w = gtk_image_new_from_icon_name ("pan-down-symbolic", GTK_ICON_SIZE_BUTTON);
      gtk_container_add (GTK_CONTAINER (grid), w);

      self->priv->center_menu = button = gtk_menu_button_new ();
      gtk_style_context_add_class (gtk_widget_get_style_context (self->priv->center_menu),
                                   "selection-menu");
      gtk_widget_destroy (gtk_bin_get_child (GTK_BIN (button)));
      gtk_widget_set_halign (button, GTK_ALIGN_CENTER);
      gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (button), menu);
      gtk_container_add (GTK_CONTAINER (self->priv->center_menu), grid);

      gtk_container_add (GTK_CONTAINER (self->priv->center_grid), button);
    }
  else
    {
      g_object_ref (self->priv->labels_grid);
      gtk_container_remove (GTK_CONTAINER (self->priv->center_menu_child),
                            self->priv->labels_grid);
      gtk_widget_destroy (self->priv->center_menu);

      self->priv->center_menu = NULL;
      self->priv->center_menu_child = NULL;

      gtk_container_add (GTK_CONTAINER (self->priv->center_grid),
                         self->priv->labels_grid);
      g_object_unref (self->priv->labels_grid);
    }

  gtk_widget_show_all (self->priv->center_grid);
}

/**
 * gd_main_toolbar_add_mode:
 * @self:
 * @label:
 *
 * Returns: (transfer none):
 */
GtkWidget *
gd_main_toolbar_add_mode (GdMainToolbar *self,
                          const gchar *label)
{
  GtkWidget *button;
  GList *group;

  button = gtk_radio_button_new_with_label (NULL, label);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
  gtk_widget_set_size_request (button, 100, -1);
  gtk_style_context_add_class (gtk_widget_get_style_context (button), "raised");
  gtk_style_context_add_class (gtk_widget_get_style_context (button), "text-button");

  group = gtk_container_get_children (GTK_CONTAINER (self->priv->modes_box));
  if (group != NULL)
    {
      gtk_radio_button_join_group (GTK_RADIO_BUTTON (button), GTK_RADIO_BUTTON (group->data));
      g_list_free (group);
    }

  gtk_container_add (GTK_CONTAINER (self->priv->modes_box), button);
  gtk_widget_show (button);

  return button;
}

/**
 * gd_main_toolbar_add_button:
 * @self:
 * @icon_name: (allow-none):
 * @label: (allow-none):
 * @pack_start:
 *
 * Returns: (transfer none):
 */
GtkWidget *
gd_main_toolbar_add_button (GdMainToolbar *self,
                            const gchar *icon_name,
                            const gchar *label,
                            gboolean pack_start)
{
  return add_button_internal (self, icon_name, label, pack_start, CHILD_NORMAL);
}

/**
 * gd_main_toolbar_add_menu:
 * @self:
 * @icon_name: (allow-none):
 * @label: (allow-none):
 * @pack_start:
 *
 * Returns: (transfer none):
 */
GtkWidget *
gd_main_toolbar_add_menu (GdMainToolbar *self,
                          const gchar *icon_name,
                          const gchar *label,
                          gboolean pack_start)
{
  return add_button_internal (self, icon_name, label, pack_start, CHILD_MENU);
}

/**
 * gd_main_toolbar_add_toggle:
 * @self:
 * @icon_name: (allow-none):
 * @label: (allow-none):
 * @pack_start:
 *
 * Returns: (transfer none):
 */
GtkWidget *
gd_main_toolbar_add_toggle (GdMainToolbar *self,
                            const gchar *icon_name,
                            const gchar *label,
                            gboolean pack_start)
{
  return add_button_internal (self, icon_name, label, pack_start, CHILD_TOGGLE);
}

/**
 * gd_main_toolbar_add_widget:
 * @self:
 * @widget:
 * @pack_start:
 *
 */
void
gd_main_toolbar_add_widget (GdMainToolbar *self,
                            GtkWidget *widget,
                            gboolean pack_start)
{
  if (pack_start)
    gtk_container_add (GTK_CONTAINER (self->priv->left_grid), widget);
  else
    gtk_container_add (GTK_CONTAINER (self->priv->right_grid), widget);
}

