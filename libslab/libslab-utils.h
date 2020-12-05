#ifndef __LIBSLAB_UTILS_H__
#define __LIBSLAB_UTILS_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <libmate-desktop/mate-desktop-item.h>

G_BEGIN_DECLS

MateDesktopItem *libslab_mate_desktop_item_new_from_unknown_id (const gchar *id);

G_END_DECLS

#endif /* __LIBSLAB_UTILS_H__ */
