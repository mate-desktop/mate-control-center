/*
 * pp-window.h: main window of the MATE panel properties tool
 *
 * Copyright (C) 2026 MATE Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

#ifndef PP_WINDOW_H
#define PP_WINDOW_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

GtkWidget *pp_window_new (GtkApplication *application);

G_END_DECLS

#endif
