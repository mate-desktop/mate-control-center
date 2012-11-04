/*
 *  Authors: Luca Cavalli <loopback@slackit.org>
 *
 *  Copyright 2005-2006 Luca Cavalli
 *  Copyright 2010 Perberos <perberos@gmail.com>
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
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02110-1301, USA.
 *
 */

#ifndef _MATE_DA_CAPPLET_H_
#define _MATE_DA_CAPPLET_H_

#include <gtk/gtk.h>

typedef struct _MateDACapplet {
	GtkBuilder* builder;

	GtkIconTheme* icon_theme;

	GtkWidget* window;

	GtkWidget* web_combo_box;
	GtkWidget* mail_combo_box;
	GtkWidget* term_combo_box;
	GtkWidget* media_combo_box;
	GtkWidget* video_combo_box;
	GtkWidget* visual_combo_box;
	GtkWidget* mobility_combo_box;
	/* Para el File Manager */
	GtkWidget* file_combo_box;
	GtkWidget* text_combo_box;
	GtkWidget* image_combo_box;

	/* navegador web
	 * por el momento default,new_win,new_tab no se utilizan */
	GtkWidget* web_browser_command_entry;
	GtkWidget* web_browser_command_label;
	GtkWidget* web_browser_terminal_checkbutton;
	GtkWidget* default_radiobutton;
	GtkWidget* new_win_radiobutton;
	GtkWidget* new_tab_radiobutton;

	/* Para el File Manager */
	GtkWidget* file_manager_command_entry;
	GtkWidget* file_manager_command_label;
	GtkWidget* file_manager_terminal_checkbutton;

	/* editor de texto */
	GtkWidget* text_editor_command_entry;
	GtkWidget* text_editor_command_label;
	GtkWidget* text_editor_terminal_checkbutton;

	/* cliente de correo */
	GtkWidget* mail_reader_command_entry;
	GtkWidget* mail_reader_command_label;
	GtkWidget* mail_reader_terminal_checkbutton;

	/* terminal */
	GtkWidget* terminal_command_entry;
	GtkWidget* terminal_command_label;
	GtkWidget* terminal_exec_flag_entry;
	GtkWidget* terminal_exec_flag_label;

	/* visor de imagenes */
	GtkWidget* image_viewer_command_entry;
	GtkWidget* image_viewer_command_label;
	GtkWidget* image_viewer_terminal_checkbutton;

	/* reproductor de musica */
	GtkWidget* media_player_command_entry;
	GtkWidget* media_player_command_label;
	GtkWidget* media_player_terminal_checkbutton;

	/* reproductor de videos */
	GtkWidget* video_player_command_entry;
	GtkWidget* video_player_command_label;
	GtkWidget* video_player_terminal_checkbutton;

	/* accesibilidad visual */
	GtkWidget* visual_command_entry;
	GtkWidget* visual_command_label;
	GtkWidget* visual_startup_checkbutton;

	/* accesibilidad motriz */
	GtkWidget* mobility_command_entry;
	GtkWidget* mobility_command_label;
	GtkWidget* mobility_startup_checkbutton;

	GList* web_browsers;
	GList* mail_readers;
	GList* terminals;
	GList* media_players;
	GList* video_players;
	GList* visual_ats;
	GList* mobility_ats;
	GList* file_managers;
	GList* text_editors;
	GList* image_viewers;
} MateDACapplet;

#endif
