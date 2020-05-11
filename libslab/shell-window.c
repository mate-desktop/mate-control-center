/*
 * This file is part of libslab.
 *
 * Copyright (c) 2006 Novell, Inc.
 *
 * Libslab is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Libslab is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libslab; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "shell-window.h"

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "app-resizer.h"

#define SHELL_WINDOW_BORDER_WIDTH 6

G_DEFINE_TYPE (ShellWindow, shell_window, GTK_TYPE_FRAME);

static void
shell_window_class_init (ShellWindowClass * klass)
{
}

static void
shell_window_init (ShellWindow * window)
{
	window->_hbox = NULL;
	window->_left_pane = NULL;
	window->_right_pane = NULL;
}

GtkWidget *
shell_window_new (AppShellData * app_data)
{
	ShellWindow *window = g_object_new (SHELL_WINDOW_TYPE, NULL);

	gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);
	gtk_frame_set_shadow_type(GTK_FRAME(window), GTK_SHADOW_NONE);

	window->_hbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
	gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (window->_hbox));

	return GTK_WIDGET (window);
}

void
shell_window_clear_resize_handler (ShellWindow * win)
{
	if (win->resize_handler_id)
	{
		g_signal_handler_disconnect (win, win->resize_handler_id);
		win->resize_handler_id = 0;
	}
}

void
shell_window_set_contents (ShellWindow * shell, GtkWidget * left_pane, GtkWidget * right_pane)
{
	shell->_left_pane = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_margin_top (GTK_WIDGET (shell->_left_pane), 15);
	gtk_widget_set_margin_bottom (GTK_WIDGET (shell->_left_pane), 15);
	gtk_widget_set_margin_start (GTK_WIDGET (shell->_left_pane), 15);
	gtk_widget_set_margin_end (GTK_WIDGET (shell->_left_pane), 15);

	shell->_right_pane = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

	gtk_box_pack_start (shell->_hbox, shell->_left_pane, FALSE, FALSE, 0);
	gtk_box_pack_start (shell->_hbox, shell->_right_pane, TRUE, TRUE, 0);	/* this one takes any extra space */

	gtk_container_add (GTK_CONTAINER (shell->_left_pane), left_pane);
	gtk_container_add (GTK_CONTAINER (shell->_right_pane), right_pane);
}
