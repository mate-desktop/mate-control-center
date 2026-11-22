/*
 * pp-window.c: main window of the MATE panel properties tool
 *
 * Copyright (C) 2026 MATE Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

#include <config.h>

#include <glib/gi18n.h>

#include "pp-config.h"
#include "pp-panel.h"
#include "pp-window.h"

#define HELP_URI "help:mate-user-guide/gospanel-15"
#define MIN_PANEL_SIZE 12
#define MAX_SIZE_RATIO 5
#define LIST_DELAY_MS 300
#define WATCH_DELAY_MS 150

#define LOCK_ICON "changes-prevent-symbolic"

enum {
	COL_LOCKED,
	COL_LOCK_ICON,
	COL_ICON,
	COL_NAME,
	COL_ID,
	COL_EDITABLE,
	N_COLS
};

typedef struct {
	GtkWidget *window;
	GtkWidget *combo;
	GtkWidget *add_panel;
	GtkWidget *remove_panel;
	GtkWidget *notebook;
	GtkWidget *general_page;
	GtkWidget *background_page;
	GtkWidget *applets_page;

	GSettings *general;
	GSettings *toplevel;
	GSettings *background;
	char      *toplevel_id;
	gboolean   updating;
	gboolean   panel_running;
	gboolean   locked_down;
	int        panel_count;
	GtkWidget *lockdown_bar;
	GtkWidget *reset_button;
	guint      refresh_source;
	guint      watch_id;
	GPtrArray *watchers;

	GtkWidget *orientation_combo;
	GtkWidget *orientation_label;
	GtkWidget *size_spin;
	GtkWidget *arrows_toggle;
	GtkWidget *color_widgets;
	GtkWidget *image_widgets;
	GtkWidget *radio_none;
	GtkWidget *radio_color;
	GtkWidget *radio_image;
	GtkWidget *color_button;
	GtkWidget *opacity_scale;
	GtkWidget *image_chooser;
	GtkWidget *radio_tile;
	GtkWidget *radio_scale;
	GtkWidget *radio_stretch;

	GtkWidget    *info_bar;
	GtkWidget    *tree;
	GtkListStore *store;
	GtkWidget    *button_add;
	GtkWidget    *button_remove;
	GtkWidget    *button_edit;
	GtkWidget    *button_lock;
	GtkWidget    *button_up;
	GtkWidget    *button_down;
} PpWindow;

static void rebuild_pages (PpWindow *pw);
static void refresh_objects (PpWindow *pw);
static void rebuild_combo (PpWindow *pw, const char *select);

static void
show_error (PpWindow *pw, const char *primary, const char *secondary)
{
	GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (pw->window),
						    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						    "%s", primary);

	if (secondary != NULL)
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", secondary);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

static gboolean
confirm (PpWindow *pw, const char *primary, const char *secondary, const char *action)
{
	GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (pw->window),
						    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						    GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
						    "%s", primary);
	int response;

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", secondary);
	gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Cancel"), GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (dialog), action, GTK_RESPONSE_ACCEPT);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	return response == GTK_RESPONSE_ACCEPT;
}

static GtkWidget *
make_label (const char *text, GtkWidget *mnemonic)
{
	GtkWidget *label = gtk_label_new_with_mnemonic (text);

	gtk_widget_set_halign (label, GTK_ALIGN_START);
	if (mnemonic != NULL)
		gtk_label_set_mnemonic_widget (GTK_LABEL (label), mnemonic);

	return label;
}

static GtkWidget *
named (GtkWidget *widget, const char *name)
{
	gtk_widget_set_name (widget, name);
	return widget;
}

static GtkWidget *
page_box (void)
{
	GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);

	gtk_container_set_border_width (GTK_CONTAINER (box), 12);
	return box;
}

static void
clear_container (GtkWidget *container)
{
	GList *children = gtk_container_get_children (GTK_CONTAINER (container));
	GList *l;

	for (l = children; l != NULL; l = l->next)
		gtk_widget_destroy (l->data);

	g_list_free (children);
}

static GtkWidget *
indented (GtkWidget *child)
{
	GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

	gtk_widget_set_margin_start (child, 18);
	gtk_box_pack_start (GTK_BOX (box), child, TRUE, TRUE, 0);
	return box;
}

static void
update_size_range (PpWindow *pw)
{
	GdkDisplay *display = gdk_display_get_default ();
	int monitor_index;
	int orientation;
	int n;
	int max;
	GdkMonitor *monitor;
	GdkRectangle geometry;

	if (pw->size_spin == NULL || display == NULL)
		return;

	monitor_index = g_settings_get_int (pw->toplevel, "monitor");
	orientation = g_settings_get_enum (pw->toplevel, "orientation");
	n = gdk_display_get_n_monitors (display);
	monitor = n > 0 ? gdk_display_get_monitor (display, CLAMP (monitor_index, 0, n - 1)) : NULL;
	if (monitor == NULL)
		monitor = gdk_display_get_primary_monitor (display);
	if (monitor == NULL)
		return;

	gdk_monitor_get_geometry (monitor, &geometry);
	max = (orientation == 1 || orientation == 4 ? geometry.height : geometry.width) / MAX_SIZE_RATIO;
	max = MAX (max, g_settings_get_int (pw->toplevel, "size"));
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (pw->size_spin), MIN_PANEL_SIZE, MAX (max, MIN_PANEL_SIZE));
}

static void
update_orientation_sensitivity (PpWindow *pw)
{
	gboolean sensitive = g_settings_get_boolean (pw->toplevel, "expand") &&
			     g_settings_is_writable (pw->toplevel, "orientation");

	gtk_widget_set_sensitive (pw->orientation_combo, sensitive);
	gtk_widget_set_sensitive (pw->orientation_label, sensitive);
}

static void
toplevel_changed_cb (GSettings *settings, const char *key, PpWindow *pw)
{
	if (g_strcmp0 (key, "expand") == 0)
		update_orientation_sensitivity (pw);

	if (g_strcmp0 (key, "orientation") == 0 || g_strcmp0 (key, "monitor") == 0)
		update_size_range (pw);

	if (g_strcmp0 (key, "orientation") == 0 || g_strcmp0 (key, "monitor") == 0 ||
	    g_strcmp0 (key, "name") == 0)
		rebuild_combo (pw, pw->toplevel_id);
}

static void
buttons_toggled_cb (GtkToggleButton *button, PpWindow *pw)
{
	gtk_widget_set_sensitive (pw->arrows_toggle, gtk_toggle_button_get_active (button));
}

static void
build_general (PpWindow *pw)
{
	GtkWidget *box = pw->general_page;
	GtkWidget *grid = gtk_grid_new ();
	GtkWidget *combo;
	GtkWidget *spin;
	GtkWidget *expand;
	GtkWidget *autohide;
	GtkWidget *buttons;
	GtkWidget *arrows;

	clear_container (box);
	gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
	gtk_grid_set_column_spacing (GTK_GRID (grid), 12);

	combo = named (gtk_combo_box_text_new (), "orientation-combo");
	gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo), "top", C_("Orientation", "Top"));
	gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo), "bottom", C_("Orientation", "Bottom"));
	gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo), "left", C_("Orientation", "Left"));
	gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo), "right", C_("Orientation", "Right"));
	gtk_widget_set_hexpand (combo, TRUE);
	pw->orientation_combo = combo;
	pw->orientation_label = make_label (_("_Orientation:"), combo);
	gtk_grid_attach (GTK_GRID (grid), pw->orientation_label, 0, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (grid), combo, 1, 0, 2, 1);

	spin = named (gtk_spin_button_new_with_range (MIN_PANEL_SIZE, 256, 1), "size-spin");
	pw->size_spin = spin;
	update_size_range (pw);
	gtk_grid_attach (GTK_GRID (grid), make_label (_("_Size:"), spin), 0, 1, 1, 1);
	gtk_grid_attach (GTK_GRID (grid), spin, 1, 1, 1, 1);
	gtk_grid_attach (GTK_GRID (grid), make_label (_("pixels"), NULL), 2, 1, 1, 1);
	gtk_box_pack_start (GTK_BOX (box), grid, FALSE, FALSE, 0);

	expand = named (gtk_check_button_new_with_mnemonic (_("E_xpand")), "expand-toggle");
	autohide = named (gtk_check_button_new_with_mnemonic (_("_Autohide")), "autohide-toggle");
	buttons = named (gtk_check_button_new_with_mnemonic (_("Show hide _buttons")), "buttons-toggle");
	arrows = named (gtk_check_button_new_with_mnemonic (_("Arro_ws on hide buttons")), "arrows-toggle");
	pw->arrows_toggle = arrows;

	gtk_box_pack_start (GTK_BOX (box), expand, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box), autohide, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box), buttons, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box), indented (arrows), FALSE, FALSE, 0);

	g_settings_bind (pw->toplevel, "orientation", combo, "active-id", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (pw->toplevel, "size", spin, "value", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (pw->toplevel, "expand", expand, "active", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (pw->toplevel, "auto-hide", autohide, "active", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (pw->toplevel, "enable-buttons", buttons, "active", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (pw->toplevel, "enable-arrows", arrows, "active", G_SETTINGS_BIND_DEFAULT);

	g_signal_connect (buttons, "toggled", G_CALLBACK (buttons_toggled_cb), pw);
	buttons_toggled_cb (GTK_TOGGLE_BUTTON (buttons), pw);
	update_orientation_sensitivity (pw);
	g_signal_connect (pw->toplevel, "changed", G_CALLBACK (toplevel_changed_cb), pw);

	gtk_widget_show_all (box);
}

static void
background_update_sensitivity (PpWindow *pw)
{
	int type = g_settings_get_enum (pw->background, "type");

	gtk_widget_set_sensitive (pw->color_widgets, type == 1);
	gtk_widget_set_sensitive (pw->image_widgets, type == 2);
}

static void
background_sync (PpWindow *pw)
{
	int type = g_settings_get_enum (pw->background, "type");
	char *color_string = g_settings_get_string (pw->background, "color");
	char *image = g_settings_get_string (pw->background, "image");
	gboolean fit = g_settings_get_boolean (pw->background, "fit");
	gboolean stretch = g_settings_get_boolean (pw->background, "stretch");
	GdkRGBA color;

	pw->updating = TRUE;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (type == 1 ? pw->radio_color :
							 type == 2 ? pw->radio_image : pw->radio_none), TRUE);

	if (!gdk_rgba_parse (&color, color_string))
		gdk_rgba_parse (&color, "rgb(0,0,0)");
	gtk_range_set_value (GTK_RANGE (pw->opacity_scale), color.alpha * 100.0);
	color.alpha = 1.0;
	gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (pw->color_button), &color);

	if (image != NULL && image[0] != '\0')
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (pw->image_chooser), image);
	else
		gtk_file_chooser_unselect_all (GTK_FILE_CHOOSER (pw->image_chooser));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (stretch ? pw->radio_stretch :
							 fit ? pw->radio_scale : pw->radio_tile), TRUE);

	background_update_sensitivity (pw);
	pw->updating = FALSE;

	g_free (color_string);
	g_free (image);
}

static void
background_type_toggled_cb (GtkToggleButton *button, PpWindow *pw)
{
	int type = 0;

	if (pw->updating || !gtk_toggle_button_get_active (button))
		return;

	if (GTK_WIDGET (button) == pw->radio_color)
		type = 1;
	else if (GTK_WIDGET (button) == pw->radio_image)
		type = 2;

	g_settings_set_enum (pw->background, "type", type);
	background_update_sensitivity (pw);
}

static void
background_color_write (PpWindow *pw)
{
	GdkRGBA color;
	char *string;

	if (pw->updating)
		return;

	gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (pw->color_button), &color);
	color.alpha = gtk_range_get_value (GTK_RANGE (pw->opacity_scale)) / 100.0;
	if (color.alpha >= 0.98)
		color.alpha = 1.0;
	else if (color.alpha <= 0.02)
		color.alpha = 0.0;

	string = gdk_rgba_to_string (&color);
	g_settings_set_string (pw->background, "color", string);
	g_free (string);
}

static void
background_image_set_cb (GtkFileChooser *chooser, PpWindow *pw)
{
	char *filename;

	if (pw->updating)
		return;

	filename = gtk_file_chooser_get_filename (chooser);
	if (filename != NULL && filename[0] != '\0')
		g_settings_set_string (pw->background, "image", filename);
	else
		g_settings_reset (pw->background, "image");

	g_free (filename);
}

static void
background_style_toggled_cb (GtkToggleButton *button, PpWindow *pw)
{
	if (pw->updating || !gtk_toggle_button_get_active (button))
		return;

	g_settings_set_boolean (pw->background, "fit", GTK_WIDGET (button) == pw->radio_scale);
	g_settings_set_boolean (pw->background, "stretch", GTK_WIDGET (button) == pw->radio_stretch);
}

static void
background_changed_cb (GSettings *settings, const char *key, PpWindow *pw)
{
	background_sync (pw);
}

static void
build_background (PpWindow *pw)
{
	GtkWidget *box = pw->background_page;
	GtkWidget *grid;
	GtkWidget *scale_box;
	GtkWidget *ends;
	GtkWidget *small;
	GtkWidget *image_box;
	GtkWidget *chooser_row;
	GtkWidget *rotate;
	GtkFileFilter *filter;

	clear_container (box);

	pw->radio_none = named (gtk_radio_button_new_with_mnemonic (NULL, _("_None (use system theme)")), "background-none");
	pw->radio_color = named (gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (pw->radio_none),
										 _("Solid c_olor")), "background-color");
	pw->radio_image = named (gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (pw->radio_none),
										 _("Background _image")), "background-image");

	gtk_box_pack_start (GTK_BOX (box), pw->radio_none, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box), pw->radio_color, FALSE, FALSE, 0);

	grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
	gtk_grid_set_column_spacing (GTK_GRID (grid), 12);

	pw->color_button = named (gtk_color_button_new (), "color-button");
	gtk_widget_set_halign (pw->color_button, GTK_ALIGN_START);
	gtk_grid_attach (GTK_GRID (grid), make_label (_("Co_lor:"), pw->color_button), 0, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (grid), pw->color_button, 1, 0, 1, 1);

	pw->opacity_scale = named (gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0, 100, 1), "opacity-scale");
	gtk_scale_set_draw_value (GTK_SCALE (pw->opacity_scale), FALSE);
	gtk_widget_set_hexpand (pw->opacity_scale, TRUE);
	scale_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start (GTK_BOX (scale_box), pw->opacity_scale, FALSE, FALSE, 0);
	ends = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	small = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (small), _("<small>Transparent</small>"));
	gtk_box_pack_start (GTK_BOX (ends), small, FALSE, FALSE, 0);
	small = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (small), _("<small>Opaque</small>"));
	gtk_box_pack_end (GTK_BOX (ends), small, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (scale_box), ends, FALSE, FALSE, 0);
	gtk_grid_attach (GTK_GRID (grid), make_label (_("S_tyle:"), pw->opacity_scale), 0, 1, 1, 1);
	gtk_grid_attach (GTK_GRID (grid), scale_box, 1, 1, 1, 1);

	pw->color_widgets = indented (grid);
	gtk_box_pack_start (GTK_BOX (box), pw->color_widgets, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box), pw->radio_image, FALSE, FALSE, 0);

	image_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	pw->image_chooser = named (gtk_file_chooser_button_new (_("Select Background Image"),
								GTK_FILE_CHOOSER_ACTION_OPEN), "image-chooser");
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Images"));
	gtk_file_filter_add_pixbuf_formats (filter);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (pw->image_chooser), filter);
	chooser_row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
	gtk_box_pack_start (GTK_BOX (chooser_row), make_label (_("_File:"), pw->image_chooser), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (chooser_row), pw->image_chooser, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (image_box), chooser_row, FALSE, FALSE, 0);

	pw->radio_tile = named (gtk_radio_button_new_with_mnemonic (NULL, _("_Tile")), "style-tile");
	pw->radio_scale = named (gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (pw->radio_tile),
										 _("_Scale")), "style-scale");
	pw->radio_stretch = named (gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (pw->radio_tile),
										   _("St_retch")), "style-stretch");
	gtk_box_pack_start (GTK_BOX (image_box), pw->radio_tile, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (image_box), pw->radio_scale, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (image_box), pw->radio_stretch, FALSE, FALSE, 0);

	rotate = named (gtk_check_button_new_with_mnemonic (_("Rotate image when panel is _vertical")), "rotate-toggle");
	gtk_box_pack_start (GTK_BOX (image_box), rotate, FALSE, FALSE, 0);
	g_settings_bind (pw->background, "rotate", rotate, "active", G_SETTINGS_BIND_DEFAULT);

	pw->image_widgets = indented (image_box);
	gtk_box_pack_start (GTK_BOX (box), pw->image_widgets, FALSE, FALSE, 0);

	background_sync (pw);

	g_signal_connect (pw->radio_none, "toggled", G_CALLBACK (background_type_toggled_cb), pw);
	g_signal_connect (pw->radio_color, "toggled", G_CALLBACK (background_type_toggled_cb), pw);
	g_signal_connect (pw->radio_image, "toggled", G_CALLBACK (background_type_toggled_cb), pw);
	g_signal_connect_swapped (pw->color_button, "color-set", G_CALLBACK (background_color_write), pw);
	g_signal_connect_swapped (pw->opacity_scale, "value-changed", G_CALLBACK (background_color_write), pw);
	g_signal_connect (pw->image_chooser, "file-set", G_CALLBACK (background_image_set_cb), pw);
	g_signal_connect (pw->radio_tile, "toggled", G_CALLBACK (background_style_toggled_cb), pw);
	g_signal_connect (pw->radio_scale, "toggled", G_CALLBACK (background_style_toggled_cb), pw);
	g_signal_connect (pw->radio_stretch, "toggled", G_CALLBACK (background_style_toggled_cb), pw);
	g_signal_connect (pw->background, "changed", G_CALLBACK (background_changed_cb), pw);

	gtk_widget_show_all (box);
}

static gboolean
get_selected (PpWindow *pw, GtkTreeIter *iter)
{
	GtkTreeSelection *selection;

	if (pw->tree == NULL)
		return FALSE;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (pw->tree));
	return gtk_tree_selection_get_selected (selection, NULL, iter);
}

static char *
selected_object_id (PpWindow *pw)
{
	GtkTreeIter iter;
	char *id = NULL;

	if (get_selected (pw, &iter))
		gtk_tree_model_get (GTK_TREE_MODEL (pw->store), &iter, COL_ID, &id, -1);

	return id;
}

static gboolean
row_locked (PpWindow *pw, GtkTreeIter *iter)
{
	gboolean locked = FALSE;

	gtk_tree_model_get (GTK_TREE_MODEL (pw->store), iter, COL_LOCKED, &locked, -1);
	return locked;
}

static void
set_button_label (GtkWidget *button, const char *label, const char *icon)
{
	gtk_button_set_label (GTK_BUTTON (button), label);
	gtk_button_set_image (GTK_BUTTON (button), gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_BUTTON));
}

static void
update_applet_buttons (PpWindow *pw)
{
	GtkTreeIter iter;
	gboolean selected;
	gboolean locked = FALSE;
	gboolean editable = FALSE;
	gboolean can_up = FALSE;
	gboolean can_down = FALSE;

	if (pw->store == NULL)
		return;

	selected = get_selected (pw, &iter);

	if (selected) {
		GtkTreeIter other = iter;

		gtk_tree_model_get (GTK_TREE_MODEL (pw->store), &iter,
				    COL_LOCKED, &locked, COL_EDITABLE, &editable, -1);

		if (pw->panel_running && !locked) {
			if (gtk_tree_model_iter_previous (GTK_TREE_MODEL (pw->store), &other))
				can_up = !row_locked (pw, &other);

			other = iter;
			if (gtk_tree_model_iter_next (GTK_TREE_MODEL (pw->store), &other))
				can_down = !row_locked (pw, &other);
		}
	}

	if (pw->locked_down) {
		selected = FALSE;
		can_up = FALSE;
		can_down = FALSE;
	}

	gtk_widget_set_sensitive (pw->button_add, pw->panel_running && !pw->locked_down);
	gtk_widget_set_sensitive (pw->button_remove, selected && !locked);
	gtk_widget_set_sensitive (pw->button_edit, selected && editable);
	gtk_widget_set_sensitive (pw->button_lock, selected);
	gtk_widget_set_sensitive (pw->button_up, can_up);
	gtk_widget_set_sensitive (pw->button_down, can_down);

	if (selected && locked)
		set_button_label (pw->button_lock, _("Un_lock"), "changes-allow");
	else
		set_button_label (pw->button_lock, _("_Lock"), "changes-prevent");
}

static void
object_changed_cb (GSettings *settings, const char *key, PpWindow *pw);

static void
clear_watchers (PpWindow *pw)
{
	guint i;

	if (pw->watchers == NULL)
		return;

	for (i = 0; i < pw->watchers->len; i++)
		g_signal_handlers_disconnect_by_data (pw->watchers->pdata[i], pw);

	g_clear_pointer (&pw->watchers, g_ptr_array_unref);
}

static void
rebuild_watchers (PpWindow *pw, GPtrArray *objects)
{
	guint i;

	clear_watchers (pw);
	pw->watchers = g_ptr_array_new_with_free_func (g_object_unref);

	for (i = 0; i < objects->len; i++) {
		PpObject *object = objects->pdata[i];
		GSettings *settings = pp_config_object (object->id);

		g_signal_connect (settings, "changed", G_CALLBACK (object_changed_cb), pw);
		g_ptr_array_add (pw->watchers, settings);
	}
}

static void
refresh_objects (PpWindow *pw)
{
	GtkTreeSelection *selection;
	GPtrArray *objects;
	char *selected;
	GtkTreeIter iter;
	guint i;

	if (pw->store == NULL || pw->toplevel_id == NULL)
		return;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (pw->tree));
	selected = selected_object_id (pw);
	objects = pp_config_objects (pw->toplevel_id);
	rebuild_watchers (pw, objects);

	g_signal_handlers_block_by_func (selection, update_applet_buttons, pw);
	gtk_list_store_clear (pw->store);
	for (i = 0; i < objects->len; i++) {
		PpObject *object = objects->pdata[i];

		gtk_list_store_insert_with_values (pw->store, &iter, -1,
						   COL_LOCKED, object->locked,
						   COL_LOCK_ICON, object->locked ? LOCK_ICON : NULL,
						   COL_ICON, object->icon,
						   COL_NAME, object->name,
						   COL_ID, object->id,
						   COL_EDITABLE, pp_config_object_can_edit (object),
						   -1);

		if (selected != NULL && g_strcmp0 (selected, object->id) == 0)
			gtk_tree_selection_select_iter (selection, &iter);
	}
	g_signal_handlers_unblock_by_func (selection, update_applet_buttons, pw);

	g_ptr_array_unref (objects);
	g_free (selected);
	update_applet_buttons (pw);
}

static gboolean
refresh_timeout_cb (gpointer data)
{
	PpWindow *pw = data;

	pw->refresh_source = 0;
	refresh_objects (pw);

	return G_SOURCE_REMOVE;
}

static void
schedule_refresh (PpWindow *pw, guint delay)
{
	if (pw->refresh_source != 0)
		g_source_remove (pw->refresh_source);

	pw->refresh_source = g_timeout_add (delay, refresh_timeout_cb, pw);
}

static void
object_changed_cb (GSettings *settings, const char *key, PpWindow *pw)
{
	schedule_refresh (pw, WATCH_DELAY_MS);
}

static void
object_list_changed_cb (GSettings *settings, const char *key, PpWindow *pw)
{
	schedule_refresh (pw, LIST_DELAY_MS);
}

static void
add_clicked_cb (GtkButton *button, PpWindow *pw)
{
	GError *error = NULL;
	char *toplevel = g_strdup (pw->toplevel_id);
	gboolean shown = pp_panel_add_to_panel (toplevel, &error);

	g_free (toplevel);
	if (!shown) {
		show_error (pw, _("Could not open the Add to Panel dialog"),
			    error != NULL ? error->message : NULL);
		g_clear_error (&error);
	}
}

static void
remove_clicked_cb (GtkButton *button, PpWindow *pw)
{
	char *id = selected_object_id (pw);

	if (id == NULL)
		return;

	pp_config_object_remove (id);
	g_free (id);
	refresh_objects (pw);
}

static PpObject *
find_object (GPtrArray *objects, const char *id)
{
	guint i;

	for (i = 0; i < objects->len; i++) {
		PpObject *object = objects->pdata[i];

		if (g_strcmp0 (object->id, id) == 0)
			return object;
	}

	return NULL;
}

static void
edit_clicked_cb (GtkButton *button, PpWindow *pw)
{
	char *id = selected_object_id (pw);
	GPtrArray *objects;
	PpObject *object;
	GError *error = NULL;

	if (id == NULL)
		return;

	objects = pp_config_objects (pw->toplevel_id);
	object = find_object (objects, id);
	if (object != NULL && !pp_config_edit_object (object, &error)) {
		show_error (pw, _("Could not edit this item"), error != NULL ? error->message : NULL);
		g_clear_error (&error);
	}

	g_ptr_array_unref (objects);
	g_free (id);
}

static void
lock_clicked_cb (GtkButton *button, PpWindow *pw)
{
	GtkTreeIter iter;
	char *id = NULL;
	gboolean locked = FALSE;

	if (!get_selected (pw, &iter))
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (pw->store), &iter, COL_ID, &id, COL_LOCKED, &locked, -1);
	pp_config_object_set_locked (id, !locked);
	g_free (id);
	refresh_objects (pw);
}

static void
move_selected (PpWindow *pw, int direction)
{
	char *id = selected_object_id (pw);
	char *toplevel = g_strdup (pw->toplevel_id);
	GError *error = NULL;

	if (id == NULL) {
		g_free (toplevel);
		return;
	}

	if (!pp_config_object_move (toplevel, id, direction, &error) && error != NULL) {
		show_error (pw, _("Could not move this item"), error->message);
		g_clear_error (&error);
	}

	g_free (toplevel);
	g_free (id);
	refresh_objects (pw);
}

static void
up_clicked_cb (GtkButton *button, PpWindow *pw)
{
	move_selected (pw, -1);
}

static void
down_clicked_cb (GtkButton *button, PpWindow *pw)
{
	move_selected (pw, 1);
}

static void
row_activated_cb (GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *column, PpWindow *pw)
{
	if (gtk_widget_get_sensitive (pw->button_edit))
		edit_clicked_cb (NULL, pw);
}

static GtkWidget *
side_button (const char *label, const char *icon, const char *name, const char *tooltip)
{
	GtkWidget *button = named (gtk_button_new_with_mnemonic (label), name);

	gtk_button_set_image (GTK_BUTTON (button), gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_BUTTON));
	gtk_button_set_always_show_image (GTK_BUTTON (button), TRUE);
	gtk_widget_set_tooltip_text (button, tooltip);
	return button;
}

static void
update_info_bar (PpWindow *pw)
{
	if (pw->info_bar != NULL)
		gtk_widget_set_visible (pw->info_bar, !pw->panel_running);
}

static void
build_applets (PpWindow *pw)
{
	GtkWidget *box = pw->applets_page;
	GtkWidget *row;
	GtkWidget *scroll;
	GtkWidget *buttons;
	GtkWidget *label;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	clear_container (box);

	pw->info_bar = named (gtk_info_bar_new (), "panel-not-running");
	gtk_info_bar_set_message_type (GTK_INFO_BAR (pw->info_bar), GTK_MESSAGE_WARNING);
	label = gtk_label_new (_("The panel is not running. Adding and moving items is unavailable."));
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_xalign (GTK_LABEL (label), 0.0);
	gtk_container_add (GTK_CONTAINER (gtk_info_bar_get_content_area (GTK_INFO_BAR (pw->info_bar))), label);
	gtk_box_pack_start (GTK_BOX (box), pw->info_bar, FALSE, FALSE, 0);

	label = gtk_label_new (_("Items are listed in the order they appear on the panel, from left to right or from top to bottom."));
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_xalign (GTK_LABEL (label), 0.0);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

	row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start (GTK_BOX (box), row, TRUE, TRUE, 0);

	pw->store = gtk_list_store_new (N_COLS, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_ICON,
					G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
	pw->tree = named (gtk_tree_view_new_with_model (GTK_TREE_MODEL (pw->store)), "items-tree");
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (pw->tree), FALSE);
	g_object_unref (pw->store);

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_renderer_set_fixed_size (renderer, 24, -1);
	column = gtk_tree_view_column_new_with_attributes (_("Locked"), renderer, "icon-name", COL_LOCK_ICON, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (pw->tree), column);

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Item"));
	gtk_tree_view_column_set_expand (column, TRUE);
	renderer = gtk_cell_renderer_pixbuf_new ();
	g_object_set (renderer, "stock-size", GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "gicon", COL_ICON);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text", COL_NAME);
	gtk_tree_view_append_column (GTK_TREE_VIEW (pw->tree), column);

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_hexpand (scroll, TRUE);
	gtk_widget_set_vexpand (scroll, TRUE);
	gtk_container_add (GTK_CONTAINER (scroll), pw->tree);
	gtk_box_pack_start (GTK_BOX (row), scroll, TRUE, TRUE, 0);

	buttons = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	pw->button_add = side_button (_("_Add"), "list-add", "item-add", _("Add an item to this panel"));
	pw->button_remove = side_button (_("_Remove"), "list-remove", "item-remove", _("Remove the selected item from the panel"));
	pw->button_edit = side_button (_("_Edit"), "document-properties", "item-edit", _("Edit the properties of the selected item"));
	pw->button_lock = side_button (_("_Lock"), "changes-prevent", "item-lock", _("Lock or unlock the selected item on the panel"));
	pw->button_up = side_button (_("Move _Up"), "go-up", "item-up", _("Move the selected item toward the start of the panel"));
	pw->button_down = side_button (_("Move _Down"), "go-down", "item-down", _("Move the selected item toward the end of the panel"));
	gtk_box_pack_start (GTK_BOX (buttons), pw->button_add, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (buttons), pw->button_remove, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (buttons), pw->button_edit, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (buttons), pw->button_lock, FALSE, FALSE, 0);
	gtk_widget_set_margin_top (pw->button_up, 12);
	gtk_box_pack_start (GTK_BOX (buttons), pw->button_up, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (buttons), pw->button_down, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (row), buttons, FALSE, FALSE, 0);

	g_signal_connect (pw->button_add, "clicked", G_CALLBACK (add_clicked_cb), pw);
	g_signal_connect (pw->button_remove, "clicked", G_CALLBACK (remove_clicked_cb), pw);
	g_signal_connect (pw->button_edit, "clicked", G_CALLBACK (edit_clicked_cb), pw);
	g_signal_connect (pw->button_lock, "clicked", G_CALLBACK (lock_clicked_cb), pw);
	g_signal_connect (pw->button_up, "clicked", G_CALLBACK (up_clicked_cb), pw);
	g_signal_connect (pw->button_down, "clicked", G_CALLBACK (down_clicked_cb), pw);
	g_signal_connect_swapped (gtk_tree_view_get_selection (GTK_TREE_VIEW (pw->tree)), "changed",
				  G_CALLBACK (update_applet_buttons), pw);
	g_signal_connect (pw->tree, "row-activated", G_CALLBACK (row_activated_cb), pw);

	gtk_widget_show_all (box);
	update_info_bar (pw);
	refresh_objects (pw);
}

static void
clear_current (PpWindow *pw)
{
	if (pw->toplevel != NULL)
		g_signal_handlers_disconnect_by_data (pw->toplevel, pw);
	if (pw->background != NULL)
		g_signal_handlers_disconnect_by_data (pw->background, pw);

	clear_watchers (pw);
	g_clear_object (&pw->toplevel);
	g_clear_object (&pw->background);
}

static void
rebuild_pages (PpWindow *pw)
{
	clear_current (pw);
	pw->orientation_combo = NULL;
	pw->size_spin = NULL;
	pw->store = NULL;
	pw->tree = NULL;
	pw->info_bar = NULL;

	clear_container (pw->general_page);
	clear_container (pw->background_page);
	clear_container (pw->applets_page);

	if (pw->toplevel_id == NULL) {
		gtk_widget_set_sensitive (pw->notebook, FALSE);
		return;
	}

	gtk_widget_set_sensitive (pw->notebook, TRUE);
	pw->toplevel = pp_config_toplevel (pw->toplevel_id);
	pw->background = pp_config_background (pw->toplevel_id);

	build_general (pw);
	build_background (pw);
	build_applets (pw);
}

static void
combo_changed_cb (GtkComboBox *combo, PpWindow *pw)
{
	const char *id;

	if (pw->updating)
		return;

	id = gtk_combo_box_get_active_id (combo);
	if (id == NULL || g_strcmp0 (id, pw->toplevel_id) == 0)
		return;

	g_free (pw->toplevel_id);
	pw->toplevel_id = g_strdup (id);
	rebuild_pages (pw);
}

static void
rebuild_combo (PpWindow *pw, const char *select)
{
	char **ids = pp_config_toplevel_ids ();
	char *previous = g_strdup (select);
	char *active;
	gboolean found = FALSE;
	int count = g_strv_length (ids);
	int i;

	GHashTable *seen = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

	pw->updating = TRUE;
	gtk_combo_box_text_remove_all (GTK_COMBO_BOX_TEXT (pw->combo));
	for (i = 0; ids[i] != NULL; i++) {
		char *label = pp_config_toplevel_label (ids[i]);
		int n = GPOINTER_TO_INT (g_hash_table_lookup (seen, label)) + 1;

		g_hash_table_replace (seen, g_strdup (label), GINT_TO_POINTER (n));
		if (n > 1) {
			char *unique = g_strdup_printf ("%s (%d)", label, n);

			g_free (label);
			label = unique;
		}

		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (pw->combo), ids[i], label);
		if (g_strcmp0 (ids[i], previous) == 0)
			found = TRUE;
		g_free (label);
	}
	g_hash_table_unref (seen);

	if (found)
		gtk_combo_box_set_active_id (GTK_COMBO_BOX (pw->combo), previous);
	else if (count > 0)
		gtk_combo_box_set_active (GTK_COMBO_BOX (pw->combo), 0);
	pw->updating = FALSE;

	pw->panel_count = count;
	gtk_widget_set_sensitive (pw->remove_panel, count > 1 && !pw->locked_down);
	gtk_widget_set_sensitive (pw->combo, count > 0);

	active = g_strdup (gtk_combo_box_get_active_id (GTK_COMBO_BOX (pw->combo)));
	if (g_strcmp0 (active, pw->toplevel_id) != 0) {
		g_free (pw->toplevel_id);
		pw->toplevel_id = active;
		rebuild_pages (pw);
	} else {
		g_free (active);
	}

	g_free (previous);
	g_strfreev (ids);
}

static void
panel_list_changed_cb (GSettings *settings, const char *key, PpWindow *pw)
{
	rebuild_combo (pw, pw->toplevel_id);
}

static void
add_panel_clicked_cb (GtkButton *button, PpWindow *pw)
{
	char *id = pp_config_toplevel_create (gtk_widget_get_display (pw->window));

	rebuild_combo (pw, id);
	g_free (id);
}

static void
remove_panel_clicked_cb (GtkButton *button, PpWindow *pw)
{
	if (pw->toplevel_id == NULL)
		return;

	if (!confirm (pw, _("Delete this panel?"),
		      _("When a panel is deleted, the panel and its settings are lost."),
		      _("_Delete")))
		return;

	{
		char *id = g_strdup (pw->toplevel_id);
		gboolean deleted = pp_config_toplevel_delete (id);

		g_free (id);
		if (!deleted)
			show_error (pw, _("Cannot delete this panel"),
				    _("You must always have at least one panel."));
	}

	rebuild_combo (pw, NULL);
}

static void
reset_clicked_cb (GtkButton *button, PpWindow *pw)
{
	if (!confirm (pw, _("Reset all panels?"),
		      _("All panels and their contents will be replaced by the default layout. "
			"This cannot be undone."),
		      _("_Reset")))
		return;

	pp_config_reset_all ();
	rebuild_combo (pw, NULL);
}

static void
help_clicked_cb (GtkButton *button, PpWindow *pw)
{
	GError *error = NULL;

	if (!gtk_show_uri_on_window (GTK_WINDOW (pw->window), HELP_URI, gtk_get_current_event_time (), &error)) {
		show_error (pw, _("Could not display help"), error->message);
		g_clear_error (&error);
	}
}

static void
apply_lockdown (PpWindow *pw)
{
	gtk_widget_set_visible (pw->lockdown_bar, pw->locked_down);
	gtk_widget_set_sensitive (pw->general_page, !pw->locked_down);
	gtk_widget_set_sensitive (pw->background_page, !pw->locked_down);
	gtk_widget_set_sensitive (pw->add_panel, !pw->locked_down);
	gtk_widget_set_sensitive (pw->remove_panel, pw->panel_count > 1 && !pw->locked_down);
	gtk_widget_set_sensitive (pw->reset_button, !pw->locked_down);
	update_applet_buttons (pw);
}

static void
lockdown_changed_cb (GSettings *settings, const char *key, PpWindow *pw)
{
	pw->locked_down = g_settings_get_boolean (settings, key);
	apply_lockdown (pw);
}

static void
close_clicked_cb (GtkButton *button, PpWindow *pw)
{
	gtk_widget_destroy (pw->window);
}

static void
window_active_cb (GObject *object, GParamSpec *pspec, PpWindow *pw)
{
	if (gtk_window_is_active (GTK_WINDOW (pw->window)))
		schedule_refresh (pw, 50);
}

static void
panel_appeared_cb (GDBusConnection *connection, const char *name, const char *owner, gpointer data)
{
	PpWindow *pw = data;

	pw->panel_running = TRUE;
	update_info_bar (pw);
	schedule_refresh (pw, LIST_DELAY_MS);
}

static void
panel_vanished_cb (GDBusConnection *connection, const char *name, gpointer data)
{
	PpWindow *pw = data;

	pw->panel_running = FALSE;
	update_info_bar (pw);
	schedule_refresh (pw, 50);
}

static void
window_destroy_cb (GtkWidget *widget, PpWindow *pw)
{
	if (pw->refresh_source != 0)
		g_source_remove (pw->refresh_source);
	if (pw->watch_id != 0)
		g_bus_unwatch_name (pw->watch_id);

	g_signal_handlers_disconnect_by_data (pw->general, pw);
	clear_current (pw);
	g_object_unref (pw->general);
	g_free (pw->toplevel_id);
	g_free (pw);
}

static GtkWidget *
dialog_button (const char *label, const char *icon, const char *name)
{
	GtkWidget *button = named (gtk_button_new_with_mnemonic (label), name);

	gtk_button_set_image (GTK_BUTTON (button), gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_BUTTON));
	return button;
}

GtkWidget *
pp_window_new (GtkApplication *application)
{
	PpWindow *pw = g_new0 (PpWindow, 1);
	GtkWidget *vbox;
	GtkWidget *top;
	GtkWidget *buttons;
	GtkWidget *help;
	GtkWidget *reset;
	GtkWidget *close;

	pw->panel_running = pp_panel_is_running ();

	pw->window = application != NULL ? gtk_application_window_new (application)
				       : gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (pw->window), _("Panel Properties"));
	gtk_window_set_icon_name (GTK_WINDOW (pw->window), "mate-panel");
	gtk_window_set_default_size (GTK_WINDOW (pw->window), 560, 480);
	gtk_container_set_border_width (GTK_CONTAINER (pw->window), 12);
	g_object_set_data (G_OBJECT (pw->window), "pp-window", pw);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
	gtk_container_add (GTK_CONTAINER (pw->window), vbox);

	pw->lockdown_bar = named (gtk_info_bar_new (), "locked-down");
	gtk_info_bar_set_message_type (GTK_INFO_BAR (pw->lockdown_bar), GTK_MESSAGE_INFO);
	{
		GtkWidget *label = gtk_label_new (_("The panels have been locked down by your administrator. "
						    "Their settings can be viewed but not changed."));

		gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
		gtk_label_set_xalign (GTK_LABEL (label), 0.0);
		gtk_container_add (GTK_CONTAINER (gtk_info_bar_get_content_area (GTK_INFO_BAR (pw->lockdown_bar))), label);
	}
	gtk_widget_set_no_show_all (pw->lockdown_bar, TRUE);
	gtk_widget_show (gtk_info_bar_get_content_area (GTK_INFO_BAR (pw->lockdown_bar)));
	gtk_widget_show_all (gtk_info_bar_get_content_area (GTK_INFO_BAR (pw->lockdown_bar)));
	gtk_box_pack_start (GTK_BOX (vbox), pw->lockdown_bar, FALSE, FALSE, 0);

	top = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	pw->combo = named (gtk_combo_box_text_new (), "panel-combo");
	gtk_widget_set_hexpand (pw->combo, TRUE);
	pw->add_panel = named (gtk_button_new_from_icon_name ("list-add", GTK_ICON_SIZE_BUTTON), "panel-add");
	pw->remove_panel = named (gtk_button_new_from_icon_name ("list-remove", GTK_ICON_SIZE_BUTTON), "panel-remove");
	gtk_widget_set_tooltip_text (pw->add_panel, _("Create a new panel"));
	gtk_widget_set_tooltip_text (pw->remove_panel, _("Delete the selected panel"));
	gtk_box_pack_start (GTK_BOX (top), make_label (_("_Panel:"), pw->combo), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (top), pw->combo, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (top), pw->add_panel, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (top), pw->remove_panel, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), top, FALSE, FALSE, 0);

	pw->notebook = named (gtk_notebook_new (), "notebook");
	pw->general_page = page_box ();
	pw->background_page = page_box ();
	pw->applets_page = page_box ();
	gtk_notebook_append_page (GTK_NOTEBOOK (pw->notebook), pw->general_page, gtk_label_new (_("General")));
	gtk_notebook_append_page (GTK_NOTEBOOK (pw->notebook), pw->background_page, gtk_label_new (_("Background")));
	gtk_notebook_append_page (GTK_NOTEBOOK (pw->notebook), pw->applets_page, gtk_label_new (_("Applets")));
	gtk_box_pack_start (GTK_BOX (vbox), pw->notebook, TRUE, TRUE, 0);

	buttons = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	help = dialog_button (_("_Help"), "help-browser", "help-button");
	reset = dialog_button (_("Re_set Panels"), "edit-undo", "reset-button");
	pw->reset_button = reset;
	close = dialog_button (_("_Close"), "window-close", "close-button");
	gtk_widget_set_tooltip_text (reset, _("Replace all panels with the default layout"));
	gtk_box_pack_start (GTK_BOX (buttons), help, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (buttons), reset, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (buttons), close, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), buttons, FALSE, FALSE, 0);

	pw->general = pp_config_general ();

	g_signal_connect (pw->combo, "changed", G_CALLBACK (combo_changed_cb), pw);
	g_signal_connect (pw->add_panel, "clicked", G_CALLBACK (add_panel_clicked_cb), pw);
	g_signal_connect (pw->remove_panel, "clicked", G_CALLBACK (remove_panel_clicked_cb), pw);
	g_signal_connect (help, "clicked", G_CALLBACK (help_clicked_cb), pw);
	g_signal_connect (reset, "clicked", G_CALLBACK (reset_clicked_cb), pw);
	g_signal_connect (close, "clicked", G_CALLBACK (close_clicked_cb), pw);
	g_signal_connect (pw->window, "notify::is-active", G_CALLBACK (window_active_cb), pw);
	g_signal_connect (pw->window, "destroy", G_CALLBACK (window_destroy_cb), pw);
	g_signal_connect (pw->general, "changed::toplevel-id-list", G_CALLBACK (panel_list_changed_cb), pw);
	g_signal_connect (pw->general, "changed::object-id-list", G_CALLBACK (object_list_changed_cb), pw);
	g_signal_connect (pw->general, "changed::locked-down", G_CALLBACK (lockdown_changed_cb), pw);

	pw->watch_id = g_bus_watch_name (G_BUS_TYPE_SESSION, PP_PANEL_BUS_NAME, G_BUS_NAME_WATCHER_FLAGS_NONE,
					 panel_appeared_cb, panel_vanished_cb, pw, NULL);

	pw->locked_down = g_settings_get_boolean (pw->general, "locked-down");
	rebuild_combo (pw, NULL);
	apply_lockdown (pw);

	return pw->window;
}
