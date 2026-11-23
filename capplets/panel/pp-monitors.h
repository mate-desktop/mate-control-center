/*
 * pp-monitors.h: the monitors as the panel numbers them
 *
 * Copyright (C) 2026 MATE Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

#ifndef PP_MONITORS_H
#define PP_MONITORS_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct {
	int           index;
	char         *id;
	char         *connector;
	char         *name;
	GdkRectangle  geometry;
	gboolean      primary;
} PpMonitor;

GPtrArray       *pp_monitors_new     (GdkDisplay      *display);
const PpMonitor *pp_monitors_lookup  (GPtrArray       *monitors,
                                      int              index);
char            *pp_monitor_label    (const PpMonitor *monitor);
void             pp_monitors_assign  (GSettings       *toplevel,
                                      const PpMonitor *monitor);

G_END_DECLS

#endif
