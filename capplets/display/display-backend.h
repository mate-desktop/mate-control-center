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

G_BEGIN_DECLS

typedef struct
{
    int       width;
    int       height;
    int       rate;                    /* mHz */
    gboolean  preferred;
} Mode;

typedef struct GrabInfo GrabInfo;

typedef struct
{
    char       *name;                  /* connector */
    char       *description;
    gboolean    enabled;
    int         x;
    int         y;
    int         transform;             /* wl_output.transform value */
    double      scale;
    int         adaptive_sync_supported;  /* -1 unknown, 0 no, 1 yes */
    gboolean    adaptive_sync;
    int         width;                 /* current mode */
    int         height;
    int         rate;
    gboolean    preferred;
    GArray     *modes;                 /* Mode */
    GrabInfo   *grab_info;

    /* Only with DISPLAY_BACKEND_IDENTITY (else NULL or FALSE). */
    char       *id;
    char       *display_name;
    gboolean    internal;
    /* With DISPLAY_BACKEND_PRIMARY: read, and applied by display_backend_apply (). */
    gboolean    primary;
    /* With DISPLAY_BACKEND_MIRROR: the id of the head whose picture this one shows. */
    char       *mirror_of;
} OutputHead;

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

void            display_heads_free           (GArray                    *heads);
void            display_head_get_preferred_size (OutputHead             *head,
                                                 int                    *width,
                                                 int                    *height);
gboolean        display_head_has_mode        (OutputHead                *head,
                                              int                        width,
                                              int                        height,
                                              int                        rate);

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
