/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/* main.c
 * Copyright (C) 2002 Seth Nickell
 * Copyright (C) 2002 Red Hat, Inc.
 *
 * Written by: Seth Nickell <snickell@stanford.edu>
 *             Havoc Pennington <hp@redhat.com>
 *             Stefano Karapetsas <stefano@karapetsas.com>
 *             Friedrich Herbst <frimam@web.de>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <glib/gi18n.h>
#include <string.h>

#include <gdk/gdkx.h>

#include "mcc-window-properties-dialog.h"
#include "mate-metacity-support.h"
#include "wm-common.h"
#include "capplet-util.h"

static void
wm_unsupported (void)
{
    GtkWidget *no_tool_dialog;

    no_tool_dialog = gtk_message_dialog_new (NULL,
                                             GTK_DIALOG_DESTROY_WITH_PARENT,
                                             GTK_MESSAGE_ERROR,
                                             GTK_BUTTONS_CLOSE,
                                             " ");
    gtk_window_set_title (GTK_WINDOW (no_tool_dialog), "");
    gtk_window_set_resizable (GTK_WINDOW (no_tool_dialog), FALSE);

    gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (no_tool_dialog), _("The current window manager is unsupported"));

    gtk_dialog_run (GTK_DIALOG (no_tool_dialog));

    gtk_widget_destroy (no_tool_dialog);
}

int
main (int argc, char **argv)
{
    GtkWidget  *dialog;
    GdkScreen  *screen;
    const char *current_wm;

    capplet_init (NULL, &argc, &argv);

    screen = gdk_display_get_default_screen (gdk_display_get_default ());
    current_wm = gdk_x11_screen_get_window_manager_name (screen);

    if (strcmp (current_wm, WM_COMMON_METACITY) == 0) {
        mate_metacity_config_tool ();
        return EXIT_SUCCESS;
    }

    if (strcmp (current_wm, WM_COMMON_MARCO) != 0) {
        wm_unsupported ();
        return EXIT_FAILURE;
    }

    dialog = mcc_window_properties_dialog_new ();
    gtk_widget_show (dialog);

    gtk_main ();

    return EXIT_SUCCESS;
}
