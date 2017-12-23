#ifndef __LIBSLAB_UTILS_H__
#define __LIBSLAB_UTILS_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <libmate-desktop/mate-desktop-item.h>

#ifdef __cplusplus
extern "C" {
#endif

MateDesktopItem *libslab_mate_desktop_item_new_from_unknown_id (const gchar *id);
guint32           libslab_get_current_time_millis (void);
gint              libslab_strcmp (const gchar *a, const gchar *b);
void              libslab_handle_g_error (GError **error, const gchar *msg_format, ...);

GdkScreen *libslab_get_current_screen (void);

void libslab_checkpoint (const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
