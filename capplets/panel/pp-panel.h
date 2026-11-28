/*
 * pp-panel.h: client for the org.mate.Panel.Shell D-Bus interface
 *
 * Copyright (C) 2026 MATE Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

#ifndef PP_PANEL_H
#define PP_PANEL_H

#include <gio/gio.h>

G_BEGIN_DECLS

#define PP_PANEL_BUS_NAME "org.mate.Panel"

gboolean   pp_panel_is_running      (void);
char     **pp_panel_get_objects     (const char  *toplevel_id,
                                     GError     **error);
gboolean   pp_panel_move_object     (const char  *object_id,
                                     int          direction,
                                     GError     **error);
gboolean   pp_panel_add_to_panel    (const char  *toplevel_id,
                                     GError     **error);
gboolean   pp_panel_show_properties (const char  *object_id,
                                     GError     **error);
GVariant  *pp_panel_get_monitors    (GError     **error);

G_END_DECLS

#endif
