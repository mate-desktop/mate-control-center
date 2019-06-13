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

#define TERMINAL_SCHEMA       "org.mate.applications-terminal"
#define TERMINAL_KEY          "exec"

#define VISUAL_SCHEMA         "org.mate.applications-at-visual"
#define VISUAL_KEY            "exec"
#define VISUAL_STARTUP_SCHEMA "org.gnome.desktop.a11y.applications"
#define VISUAL_STARTUP_KEY    "screen-reader-enabled"

#define MOBILITY_SCHEMA       "org.mate.applications-at-mobility"
#define MOBILITY_KEY          "exec"
#define MOBILITY_STARTUP_KEY  "startup"

#define CALCULATOR_SCHEMA     "org.mate.applications-calculator"
#define CALCULATOR_KEY        "exec"

#define MESSENGER_SCHEMA     "org.mate.applications-messenger"
#define MESSENGER_KEY        "exec"

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
	GtkWidget* file_combo_box;
	GtkWidget* text_combo_box;
	GtkWidget* image_combo_box;
	GtkWidget* document_combo_box;
	GtkWidget* word_combo_box;
	GtkWidget* spreadsheet_combo_box;
	GtkWidget* calculator_combo_box;
        GtkWidget* messenger_combo_box;

	/* Visual Accessibility */
	GtkWidget* visual_startup_checkbutton;

	/* Mobility Accessibility */
	GtkWidget* mobility_startup_checkbutton;

	/* Lists of available apps */
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
	GList* document_viewers;
	GList* word_editors;
	GList* spreadsheet_editors;
	GList* calculators;
        GList* messengers;

	/* Settings objects */
	GSettings* terminal_settings;
	GSettings* visual_settings;
	GSettings* visual_startup_settings;
	GSettings* mobility_settings;
	GSettings* calculator_settings;
	GSettings* messenger_settings;
} MateDACapplet;

#endif
