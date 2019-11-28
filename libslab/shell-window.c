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

static void shell_window_handle_size_request (GtkWidget * widget, GtkRequisition * requisition,
	AppShellData * data);

gboolean shell_window_paint_window (GtkWidget * widget, cairo_t * cr, gpointer data);

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

	g_signal_connect (G_OBJECT (window), "draw", G_CALLBACK (shell_window_paint_window),
		NULL);

/* FIXME add some replacement for GTK+3, or just remove this code? */
#if 0
	window->resize_handler_id =
		g_signal_connect (G_OBJECT (window), "size-request",
		G_CALLBACK (shell_window_handle_size_request), app_data);
#endif

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

/* We want the window to come up with proper runtime calculated width ( ie taking into account font size, locale, ...) so
   we can't hard code a size. But since ScrolledWindow returns basically zero for it's size request we need to
   grab the "real" desired width. Once it's shown though we want to allow the user to size down if they want too, so
   we unhook this function
*/
static void
shell_window_handle_size_request (GtkWidget * widget, GtkRequisition * requisition,
	AppShellData * app_data)
{
	gint height;
	GtkRequisition child_requisiton;

	gtk_widget_get_preferred_size (GTK_WIDGET (APP_RESIZER (app_data->category_layout)->child), &child_requisiton, NULL);

	requisition->width += child_requisiton.width;

	/* use the left side as a minimum height, if the right side is taller,
	   use it up to SIZING_HEIGHT_PERCENT of the screen height
	*/
	height = child_requisiton.height + 10;
	if (height > requisition->height)
	{
		requisition->height =
			MIN (((gfloat) HeightOfScreen (gdk_x11_screen_get_xscreen (gdk_screen_get_default ())) * SIZING_HEIGHT_PERCENT), height);
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

gboolean
shell_window_paint_window (GtkWidget * widget, cairo_t * cr, gpointer data)
{
	GtkWidget *left_pane;
	GtkAllocation allocation;

	left_pane = SHELL_WINDOW (widget)->_left_pane;

	gtk_widget_get_allocation (left_pane, &allocation);

	/* draw left pane background */
	gtk_paint_flat_box (
		gtk_widget_get_style (widget),
		cr,
		gtk_widget_get_state (widget),
		GTK_SHADOW_NONE,
		widget,
		"",
		allocation.x,
		allocation.y,
		allocation.width,
		allocation.height);

	return FALSE;
}
