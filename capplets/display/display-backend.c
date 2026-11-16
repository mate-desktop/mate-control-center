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

#include <config.h>

#include "display-backend.h"

DisplayBackend *
display_backend_new (DisplayBackendChangedFunc changed, gpointer data, GError **error)
{
    DisplayBackend *backend = NULL;

#ifdef HAVE_WAYLAND_PROTOCOLS
    backend = display_backend_protocol_new (changed, data);
#endif
    if (backend == NULL)
        backend = display_backend_wlrandr_new (changed, data, error);
    return backend;
}

guint
display_backend_get_features (DisplayBackend *backend)
{
    return backend->features;
}

const char *
display_backend_get_name (DisplayBackend *backend)
{
    return backend->name;
}

void
display_backend_refresh (DisplayBackend *backend)
{
    backend->refresh (backend);
}

void
display_backend_apply (DisplayBackend            *backend,
                       GArray                    *heads,
                       gboolean                   temporary,
                       DisplayBackendAppliedFunc  done,
                       gpointer                   data)
{
    backend->apply (backend, heads,
                    temporary && (backend->features & DISPLAY_BACKEND_TEMPORARY),
                    done, data);
}

void
display_backend_free (DisplayBackend *backend)
{
    if (backend)
        backend->free (backend);
}
