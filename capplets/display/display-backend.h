/* Where the Wayland display capplet gets and applies the monitor layout.
 *
 * Copyright (C) 2026  mate-desktop.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef DISPLAY_BACKEND_H
#define DISPLAY_BACKEND_H

#include <glib.h>

#include "display-head.h"

G_BEGIN_DECLS

typedef enum
{
    DISPLAY_BACKEND_PRIMARY   = 1 << 0,
    DISPLAY_BACKEND_IDENTITY  = 1 << 1,
    DISPLAY_BACKEND_MIRROR    = 1 << 2,
    DISPLAY_BACKEND_TEMPORARY = 1 << 3,
} DisplayBackendFeatures;

typedef struct DisplayBackend DisplayBackend;

/* @heads (OutputHead, owned by the callee; free with display_heads_free ()),
 * or NULL with @error set when the layout could not be read. */
typedef void (*DisplayBackendChangedFunc) (GArray *heads, const char *error, gpointer data);
/* @error is NULL on success. */
typedef void (*DisplayBackendAppliedFunc) (const char *error, gpointer data);

/* The protocol backend when the compositor offers zwlr_output_manager_v1 and
 * mate_output_manager_v1, else mate-settings-daemon's WLRANDR D-Bus service.
 * NULL, with @error set, when neither is there. */
DisplayBackend *display_backend_new          (DisplayBackendChangedFunc  changed,
                                              gpointer                   data,
                                              GError                   **error);
guint           display_backend_get_features (DisplayBackend            *backend);
const char     *display_backend_get_name     (DisplayBackend            *backend);
void            display_backend_refresh      (DisplayBackend            *backend);
/* @temporary needs DISPLAY_BACKEND_TEMPORARY: apply without saving. */
void            display_backend_apply        (DisplayBackend            *backend,
                                              GArray                    *heads,
                                              gboolean                   temporary,
                                              DisplayBackendAppliedFunc  done,
                                              gpointer                   data);
void            display_backend_free         (DisplayBackend            *backend);

/* Backend implementations, behind display_backend_new (). */
struct DisplayBackend
{
    const char  *name;
    guint        features;
    DisplayBackendChangedFunc changed;
    gpointer     changed_data;
    void       (*refresh) (DisplayBackend *backend);
    void       (*apply)   (DisplayBackend *backend, GArray *heads, gboolean temporary,
                           DisplayBackendAppliedFunc done, gpointer data);
    void       (*free)    (DisplayBackend *backend);
};

DisplayBackend *display_backend_protocol_new (DisplayBackendChangedFunc changed, gpointer data);
DisplayBackend *display_backend_wlrandr_new  (DisplayBackendChangedFunc changed, gpointer data,
                                              GError **error);

G_END_DECLS

#endif
