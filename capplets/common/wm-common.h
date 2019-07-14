#ifndef WM_COMMON_H
#define WM_COMMON_H

#define WM_COMMON_MARCO       "Metacity (Marco)"
#define WM_COMMON_SAWFISH     "Sawfish"
#define WM_COMMON_METACITY    "Metacity"
#define WM_COMMON_COMPIZ      "Compiz"
#define WM_COMMON_COMPIZ_OLD  "compiz"
#define WM_COMMON_UNKNOWN     "Unknown"

gchar *wm_common_get_current_window_manager (void);
/* Returns a strv of keybinding names for the window manager;
 * using _MATE_WM_KEYBINDINGS if available, _NET_WM_NAME otherwise. */
char **wm_common_get_current_keybindings (void);

void   wm_common_register_window_manager_change (GFunc    func,
                                                 gpointer data);
void   wm_common_update_window (void);

#endif /* WM_COMMON_H */

