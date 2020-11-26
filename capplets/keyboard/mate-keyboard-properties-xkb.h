/* -*- mode: c; style: linux -*- */

/* mate-keyboard-properties-xkb.h
 * Copyright (C) 2003-2007 Sergey V Udaltsov
 *
 * Written by Sergey V. Udaltsov <svu@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef __MATE_KEYBOARD_PROPERTY_XKB_H
#define __MATE_KEYBOARD_PROPERTY_XKB_H

#include <glib.h>
#include <gio/gio.h>

#include "libmatekbd/matekbd-keyboard-config.h"

G_BEGIN_DECLS

#define CWID(s) GTK_WIDGET (gtk_builder_get_object (chooser_dialog, s))
extern XklEngine *engine;
extern XklConfigRegistry *config_registry;
extern GSettings *xkb_kbd_settings;
extern GSettings *xkb_general_settings;
extern MatekbdKeyboardConfig initial_config;

extern void setup_xkb_tabs (GtkBuilder * dialog);

extern void xkb_layouts_fill_selected_tree (GtkBuilder * dialog);

extern void xkb_layouts_register_buttons_handlers (GtkBuilder * dialog);

extern void xkb_layouts_register_gsettings_listener (GtkBuilder * dialog);

extern void xkb_options_register_gsettings_listener (GtkBuilder * dialog);

extern void xkb_layouts_prepare_selected_tree (GtkBuilder * dialog);

extern void xkb_options_load_options (GtkBuilder * dialog);

extern void xkb_options_popup_dialog (GtkBuilder * dialog);

extern void clear_xkb_elements_list (GSList * list);

extern char *xci_desc_to_utf8 (XklConfigItem * ci);

extern gchar *xkb_layout_description_utf8 (const gchar * visible);

extern void enable_disable_restoring (GtkBuilder * dialog);

extern void preview_toggled (GtkBuilder * dialog, GtkWidget * button);

extern void choose_model (GtkBuilder * dialog);

extern void xkb_layout_choose (GtkBuilder * dialog);

extern GSList *xkb_layouts_get_selected_list (void);

extern GSList *xkb_options_get_selected_list (void);

extern void xkb_layouts_set_selected_list(GSList *list);

extern void xkb_options_set_selected_list(GSList *list);

extern GtkWidget *xkb_layout_preview_create_widget (GtkBuilder *
						    chooser_dialog);

extern void xkb_layout_preview_update (GtkBuilder * chooser_dialog);

extern void xkb_layout_preview_set_drawing_layout (GtkWidget * kbdraw,
						   const gchar * id);

extern gchar *xkb_layout_chooser_get_selected_id (GtkBuilder *
						  chooser_dialog);

extern void xkb_save_default_group (gint group_no);

extern gint xkb_get_default_group (void);

G_END_DECLS

#endif /* __MATE_KEYBOARD_PROPERTY_XKB_H */
