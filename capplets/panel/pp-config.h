/*
 * pp-config.h: GSettings backed model of the MATE panel configuration
 *
 * Copyright (C) 2026 MATE Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

#ifndef PP_CONFIG_H
#define PP_CONFIG_H

#include <gio/gio.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum {
	PP_ALIGN_START = 0,
	PP_ALIGN_CENTER = 1,
	PP_ALIGN_END = 2
} PpAlign;

typedef struct {
	char     *id;
	char     *name;
	GIcon    *icon;
	char     *type;
	char     *launcher;
	PpAlign   align;
	int       index;
	int       position;
	gboolean  locked;
} PpObject;

gboolean    pp_config_available          (void);
gboolean    pp_config_has_pack_keys      (void);
gboolean    pp_config_locked_down        (void);

GSettings  *pp_config_general            (void);
GSettings  *pp_config_toplevel           (const char *toplevel_id);
GSettings  *pp_config_background         (const char *toplevel_id);
GSettings  *pp_config_object             (const char *object_id);

char      **pp_config_toplevel_ids       (void);
char      **pp_config_object_ids         (void);
char       *pp_config_toplevel_label     (const char *toplevel_id);

char       *pp_config_toplevel_create    (GdkDisplay *display);
gboolean    pp_config_toplevel_delete    (const char *toplevel_id);
void        pp_config_reset_all          (void);

GPtrArray  *pp_config_objects            (const char *toplevel_id);
void        pp_object_free               (PpObject   *object);
gboolean    pp_config_object_move        (const char *toplevel_id,
                                          const char *object_id,
                                          int         direction,
                                          GError    **error);
void        pp_config_object_set_locked  (const char *object_id,
                                          gboolean    locked);
void        pp_config_object_remove      (const char *object_id);
gboolean    pp_config_object_can_edit    (PpObject   *object);
gboolean    pp_config_edit_object        (PpObject   *object,
                                          GError    **error);

G_END_DECLS

#endif
