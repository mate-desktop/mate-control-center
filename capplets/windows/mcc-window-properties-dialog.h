/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/* mcc-window-properties-dialog.h
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

#ifndef __MCC_WINDOW_PROPERTIES_DIALOG_H
#define __MCC_WINDOW_PROPERTIES_DIALOG_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MCC_TYPE_WINDOW_PROPERTIES_DIALOG (mcc_window_properties_dialog_get_type ())
G_DECLARE_FINAL_TYPE (MccWindowPropertiesDialog, mcc_window_properties_dialog, MCC, WINDOW_PROPERTIES_DIALOG, GtkDialog)

GtkWidget * mcc_window_properties_dialog_new (void);

G_END_DECLS

#endif /* __MCC_WINDOW_PROPERTIES_DIALOG_H */
