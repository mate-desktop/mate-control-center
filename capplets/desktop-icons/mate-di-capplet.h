/*
 *  Authors: Luca Cavalli <loopback@slackit.org>
 *
 *  Copyright 2005-2006 Luca Cavalli
 *  Copyright 2010 Perberos <perberos@gmail.com>
 *  Copyright 2019 Laurent Napias
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of version 2 of the GNU General Public License
 *  as published by the Free Software Foundation
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef _MATE_DI_CAPPLET_H_
#define _MATE_DI_CAPPLET_H_

#include <gtk/gtk.h>

/* Shmma stteings */
#define SHOW_ICONS_SCHEMA    	"org.mate.background"
#define DESKTOP_ICONS_SCHEMA    "org.mate.caja.desktop"

/* Icons keys */
#define SHOW_ICONS_KEY    		"show-desktop-icons"
#define COMPUTER_ICON_KEY		"computer-icon-visible"
#define HOME_ICON_KEY			"home-icon-visible"
#define TRASH_ICON_KEY			"trash-icon-visible"
#define NETWORK_ICON_KEY		"network-icon-visible"
#define VOLUMES_ICON_KEY		"volumes-visible"

/* Icons names keys */
#define COMPUTER_NAME_KEY		"computer-icon-name"
#define HOME_NAME_KEY			"home-icon-name"
#define TRASH_NAME_KEY			"trash-icon-name"
#define NETWORK_NAME_KEY		"network-icon-name"

typedef struct _MateDICapplet {
	GtkBuilder* builder;

	GtkWidget* window;
	
	GtkWidget* desktop_icons_caption;

	/* Icons */
	GtkWidget* button_show_icons_toggle;
	GtkWidget* button_computer_toggle;
	GtkWidget* button_home_toggle;
	GtkWidget* button_trash_toggle;
	GtkWidget* button_network_toggle;
	GtkWidget* button_volumes_toggle;

	/* One GtkBox per icon to show/hide or rename */
	GtkWidget* hbox_computer;
	GtkWidget* hbox_home;
	GtkWidget* hbox_trash;
	GtkWidget* hbox_network;

	/* To enter new name for icons */
	GtkWidget* entry_computer;
	GtkWidget* entry_home;
	GtkWidget* entry_trash;
	GtkWidget* entry_network;

	/* To rename icon or set to default name */
	GtkWidget* button_edit_computer;
	GtkWidget* button_edit_home;
	GtkWidget* button_edit_trash;
	GtkWidget* button_edit_network;

	/* Settings objects */
	GSettings* show_icons_settings;
	GSettings* desktop_icons_settings;

} MateDICapplet;

static void close_cb(GtkWidget* window, gint response, MateDICapplet* capplet);
static void show_icons_cb(GtkCheckButton* button, MateDICapplet* capplet);
static void edit_computer_icon_name_cb(GtkWidget* button, MateDICapplet* capplet);
void on_activate_entry_computer_cb(GtkWidget *entry, MateDICapplet* capplet);
static void edit_home_icon_name_cb(GtkWidget* button, MateDICapplet* capplet);
void on_activate_entry_home_cb(GtkWidget *entry, MateDICapplet* capplet);
static void edit_trash_icon_name_cb(GtkWidget* button, MateDICapplet* capplet);
void on_activate_entry_trash_cb(GtkWidget *entry, MateDICapplet* capplet);
static void edit_network_icon_name_cb(GtkWidget* button, MateDICapplet* capplet);
void on_activate_entry_network_cb(GtkWidget *entry, MateDICapplet* capplet);
void get_user_icons_names(MateDICapplet* capplet);
static void init_dialog(MateDICapplet* capplet);

#endif
