/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/* mate-window-properties.c
 * Copyright (C) 2002 Seth Nickell
 * Copyright (C) 2002 Red Hat, Inc.
 *
 * Written by: Seth Nickell <snickell@stanford.edu>
 *             Havoc Pennington <hp@redhat.com>
 *             Stefano Karapetsas <stefano@karapetsas.com>
 *             Friedrich Herbst <frimam@web.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <glib/gi18n.h>
#include <string.h>

#include <gdk/gdkx.h>

#include "mate-metacity-support.h"
#include "wm-common.h"
#include "capplet-util.h"

#define MARCO_SCHEMA "org.mate.Marco.general"
#define INTERFACE_SCHEMA "org.mate.interface"

#define GTK_BUTTON_LAYOUT_KEY "gtk-decoration-layout"

#define MARCO_CENTER_NEW_WINDOWS_KEY "center-new-windows"
#define MARCO_ALLOW_TILING_KEY "allow-tiling"
#define MARCO_ALLOW_TOP_TILING_KEY "allow-top-tiling"
#define MARCO_SHOW_TAB_BORDER_KEY "show-tab-border"
#define MARCO_BUTTON_LAYOUT_KEY "button-layout"
#define MARCO_DOUBLE_CLICK_TITLEBAR_KEY "action-double-click-titlebar"
#define MARCO_MIDDLE_CLICK_TITLEBAR_KEY "action-middle-click-titlebar"
#define MARCO_RIGHT_CLICK_TITLEBAR_KEY "action-right-click-titlebar"
#define MARCO_SCROLL_UP_TITLEBAR_KEY "action-scroll-up-titlebar"
#define MARCO_SCROLL_DOWN_TITLEBAR_KEY "action-scroll-down-titlebar"
#define MARCO_FOCUS_KEY "focus-mode"
#define MARCO_AUTORAISE_KEY "auto-raise"
#define MARCO_AUTORAISE_DELAY_KEY "auto-raise-delay"
#define MARCO_MOUSE_MODIFIER_KEY "mouse-button-modifier"

#define MARCO_COMPOSITING_MANAGER_KEY "compositing-manager"
#define MARCO_COMPOSITING_FAST_ALT_TAB_KEY "compositing-fast-alt-tab"

enum
{
    MARCO_BUTTON_LAYOUT_RIGHT_WITH_MENU = 0,
    MARCO_BUTTON_LAYOUT_LEFT_WITH_MENU,
    MARCO_BUTTON_LAYOUT_RIGHT,
    MARCO_BUTTON_LAYOUT_LEFT,
    MARCO_BUTTON_LAYOUT_COUNT
};

static const char *button_layout [MARCO_BUTTON_LAYOUT_COUNT] = {
   [MARCO_BUTTON_LAYOUT_RIGHT_WITH_MENU] = "menu:minimize,maximize,close",
   [MARCO_BUTTON_LAYOUT_LEFT_WITH_MENU]  = "close,minimize,maximize:menu",
   [MARCO_BUTTON_LAYOUT_RIGHT]           = ":minimize,maximize,close",
   [MARCO_BUTTON_LAYOUT_LEFT]            = "close,minimize,maximize:"
};

/* keep following enums in sync with marco */
enum
{
    ACTION_TITLEBAR_CLOSE,
    ACTION_TITLEBAR_MINIMIZE,
    ACTION_TITLEBAR_TOGGLE_MAXIMIZE,
    ACTION_TITLEBAR_TOGGLE_MAXIMIZE_HORIZONTALLY,
    ACTION_TITLEBAR_TOGGLE_MAXIMIZE_VERTICALLY,
    ACTION_TITLEBAR_TOGGLE_SHADE,
    ACTION_TITLEBAR_SHADE,
    ACTION_TITLEBAR_UNSHADE,
    ACTION_TITLEBAR_RAISE,
    ACTION_TITLEBAR_LOWER,
    ACTION_TITLEBAR_TOGGLE_STICK,
    ACTION_TITLEBAR_TOGGLE_ABOVE,
    ACTION_TITLEBAR_MENU,
    ACTION_TITLEBAR_NONE
};

enum
{
    FOCUS_MODE_CLICK,
    FOCUS_MODE_SLOPPY,
    FOCUS_MODE_MOUSE
};

typedef struct
{
    int number;
    char *name;
    const char *value; /* machine-readable name for storing config */
    GtkWidget *radio;
} MouseClickModifier;

static GtkWidget *dialog_win;

/* Behaviour */
static GtkWidget *show_tab_border_checkbutton;
static GtkWidget *compositing_fast_alt_tab_checkbutton;
static GtkWidget *double_click_titlebar_optionmenu;
static GtkWidget *middle_click_titlebar_optionmenu;
static GtkWidget *right_click_titlebar_optionmenu;
static GtkWidget *scroll_up_titlebar_optionmenu;
static GtkWidget *scroll_down_titlebar_optionmenu;
static GtkWidget *focus_mode_checkbutton;
static GtkWidget *focus_mode_mouse_checkbutton;
static GtkWidget *autoraise_checkbutton;
static GtkWidget *autoraise_delay_spinbutton;
static GtkWidget *autoraise_delay_hbox;
static GtkWidget *movement_description_label;
static GtkWidget *alt_click_vbox;

/* Placement */
static GtkWidget *center_new_windows_checkbutton;
static GtkWidget *enable_tiling_checkbutton;
static GtkWidget *allow_top_tiling_checkbutton;
static GtkWidget *titlebar_layout_optionmenu;

/* Compositing Manager */
static GtkWidget *compositing_checkbutton;

static GSettings *marco_settings;
static GSettings *interface_settings;

static MouseClickModifier *mouse_modifiers = NULL;
static int n_mouse_modifiers = 0;

static void reload_mouse_modifiers (void);

static void
update_sensitivity (void)
{
    gtk_widget_set_sensitive (compositing_fast_alt_tab_checkbutton,
                              g_settings_get_boolean (marco_settings, MARCO_COMPOSITING_MANAGER_KEY));
    gtk_widget_set_sensitive (allow_top_tiling_checkbutton,
                              g_settings_get_boolean (marco_settings, MARCO_ALLOW_TILING_KEY));
    gtk_widget_set_sensitive (focus_mode_mouse_checkbutton,
                              g_settings_get_enum (marco_settings, MARCO_FOCUS_KEY) != FOCUS_MODE_CLICK);
    gtk_widget_set_sensitive (autoraise_delay_hbox,
                              g_settings_get_enum (marco_settings, MARCO_FOCUS_KEY) != FOCUS_MODE_CLICK);
    gtk_widget_set_sensitive (autoraise_delay_spinbutton,
                              g_settings_get_enum (marco_settings, MARCO_FOCUS_KEY) != FOCUS_MODE_CLICK &&
                              g_settings_get_boolean (marco_settings, MARCO_AUTORAISE_KEY));
}

static void
marco_settings_changed_callback (GSettings *settings,
                                 const gchar *key,
                                 gpointer user_data)
{
    update_sensitivity ();
}

static void
mouse_focus_toggled_callback (GtkWidget *button,
                              void      *data)
{
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (focus_mode_checkbutton))) {
        g_settings_set_enum (marco_settings,
                             MARCO_FOCUS_KEY,
                             gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (focus_mode_mouse_checkbutton)) ?
                             FOCUS_MODE_MOUSE : FOCUS_MODE_SLOPPY);
    }
    else {
        g_settings_set_enum (marco_settings, MARCO_FOCUS_KEY, FOCUS_MODE_CLICK);
    }
}

static void
mouse_focus_changed_callback (GSettings *settings,
                              const gchar *key,
                              gpointer user_data)
{
    if (g_settings_get_enum (settings, key) == FOCUS_MODE_MOUSE) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (focus_mode_checkbutton), TRUE);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (focus_mode_mouse_checkbutton), TRUE);
    }
    else if (g_settings_get_enum (settings, key) == FOCUS_MODE_SLOPPY) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (focus_mode_checkbutton), TRUE);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (focus_mode_mouse_checkbutton), FALSE);
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (focus_mode_checkbutton), FALSE);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (focus_mode_mouse_checkbutton), FALSE);
    }
}

static void
autoraise_delay_spinbutton_value_callback (GtkWidget *spinbutton,
                                           void      *data)
{
    g_settings_set_int (marco_settings,
                        MARCO_AUTORAISE_DELAY_KEY,
                        gtk_spin_button_get_value (GTK_SPIN_BUTTON (spinbutton)) * 1000);
}

static void
double_click_titlebar_changed_callback (GtkWidget *optionmenu,
                                        void      *data)
{
    g_settings_set_enum (marco_settings, MARCO_DOUBLE_CLICK_TITLEBAR_KEY,
                         gtk_combo_box_get_active (GTK_COMBO_BOX (optionmenu)));
}

static void
middle_click_titlebar_changed_callback (GtkWidget *optionmenu,
                                        void      *data)
{
    g_settings_set_enum (marco_settings, MARCO_MIDDLE_CLICK_TITLEBAR_KEY,
                         gtk_combo_box_get_active (GTK_COMBO_BOX (optionmenu)));
}

static void
right_click_titlebar_changed_callback (GtkWidget *optionmenu,
                                       void      *data)
{
    g_settings_set_enum (marco_settings, MARCO_RIGHT_CLICK_TITLEBAR_KEY,
                         gtk_combo_box_get_active (GTK_COMBO_BOX (optionmenu)));
}

static void
scroll_up_titlebar_changed_callback (GtkWidget *optionmenu,
                                     void      *data)
{
    g_settings_set_enum (marco_settings, MARCO_SCROLL_UP_TITLEBAR_KEY,
                         gtk_combo_box_get_active (GTK_COMBO_BOX (optionmenu)));
}

static void
scroll_down_titlebar_changed_callback (GtkWidget *optionmenu,
                                       void      *data)
{
    g_settings_set_enum (marco_settings, MARCO_SCROLL_DOWN_TITLEBAR_KEY,
                         gtk_combo_box_get_active (GTK_COMBO_BOX (optionmenu)));
}

static gchar *custom_titlebar_button_layout = NULL;

static void
titlebar_layout_changed_callback (GtkWidget *optionmenu)
{
    gint value = gtk_combo_box_get_active (GTK_COMBO_BOX (optionmenu));

    if (value < MARCO_BUTTON_LAYOUT_COUNT) {
        g_settings_set_string (marco_settings, MARCO_BUTTON_LAYOUT_KEY, button_layout[value]);
        g_settings_set_string (interface_settings, GTK_BUTTON_LAYOUT_KEY, button_layout[value]);
    }
    /* Custom Layout */
    else if ((value == MARCO_BUTTON_LAYOUT_COUNT) && custom_titlebar_button_layout) {
        g_settings_set_string (marco_settings, MARCO_BUTTON_LAYOUT_KEY, custom_titlebar_button_layout);
        g_settings_set_string (interface_settings, GTK_BUTTON_LAYOUT_KEY, custom_titlebar_button_layout);
    }
}

static void
set_titlebar_button_layout(void)
{
    gchar    *str;
    gboolean  found = FALSE;

    str = g_settings_get_string (marco_settings, MARCO_BUTTON_LAYOUT_KEY);
    for (gint i = 0; i < MARCO_BUTTON_LAYOUT_COUNT; i++) {
        if (g_strcmp0 (str, button_layout[i]) == 0) {
            gtk_combo_box_set_active (GTK_COMBO_BOX (titlebar_layout_optionmenu), i);
            found = TRUE;
            break;
        }
    }
    /* A custom value is found in MARCO_BUTTON_LAYOUT_KEY
     * (maybe the user changed the value in dconf-editor) */
    if (!found) {
        g_free (custom_titlebar_button_layout);
        custom_titlebar_button_layout = g_strdup(str);
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (titlebar_layout_optionmenu), _("Custom"));
        gtk_combo_box_set_active (GTK_COMBO_BOX (titlebar_layout_optionmenu), 4);
    }

    g_free(str);
}

static void
alt_click_radio_toggled_callback (GtkWidget *radio,
                                  void      *data)
{
    MouseClickModifier *modifier = data;
    gboolean active;
    gchar *value;

    active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio));

    if (active) {
        value = g_strdup_printf ("<%s>", modifier->value);
        g_settings_set_string (marco_settings, MARCO_MOUSE_MODIFIER_KEY, value);
        g_free (value);
    }
}

static void
set_alt_click_value (void)
{
    gboolean match_found = FALSE;
    gchar *mouse_move_modifier;
    gchar *value;
    int i;

    mouse_move_modifier = g_settings_get_string (marco_settings, MARCO_MOUSE_MODIFIER_KEY);

    /* We look for a matching modifier and set it. */
    if (mouse_move_modifier != NULL) {
        for (i = 0; i < n_mouse_modifiers; i ++) {
            value = g_strdup_printf ("<%s>", mouse_modifiers[i].value);
            if (strcmp (value, mouse_move_modifier) == 0) {
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mouse_modifiers[i].radio), TRUE);
                match_found = TRUE;
                break;
            }
            g_free (value);
        }
        g_free (mouse_move_modifier);
    }

    /* No matching modifier was found; we set all the toggle buttons to be
     * insensitive. */
    for (i = 0; i < n_mouse_modifiers; i++) {
        gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (mouse_modifiers[i].radio), ! match_found);
    }
}

static void
wm_unsupported (void)
{
    GtkWidget *no_tool_dialog;

    no_tool_dialog = gtk_message_dialog_new (NULL,
                                             GTK_DIALOG_DESTROY_WITH_PARENT,
                                             GTK_MESSAGE_ERROR,
                                             GTK_BUTTONS_CLOSE,
                                             " ");
    gtk_window_set_title (GTK_WINDOW (no_tool_dialog), "");
    gtk_window_set_resizable (GTK_WINDOW (no_tool_dialog), FALSE);

    gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (no_tool_dialog), _("The current window manager is unsupported"));

    gtk_dialog_run (GTK_DIALOG (no_tool_dialog));

    gtk_widget_destroy (no_tool_dialog);
}

static void
wm_changed_callback (GdkScreen *screen,
                     void      *data)
{
    const char *current_wm;

    current_wm = gdk_x11_screen_get_window_manager_name (screen);

    gtk_widget_set_sensitive (dialog_win, g_strcmp0 (current_wm, WM_COMMON_MARCO) == 0);
}

static void
response_cb (GtkWidget *dialog_win,
             int    response_id,
             void      *data)
{

    if (response_id == GTK_RESPONSE_HELP) {
        capplet_help (GTK_WINDOW (dialog_win), "goscustdesk-58");
    } else {
        gtk_widget_destroy (dialog_win);
    }
}

int
main (int argc, char **argv)
{
    GtkBuilder *builder;
    GdkScreen  *screen;
    GtkWidget  *nb;
    const char *current_wm;
    int i;

    capplet_init (NULL, &argc, &argv);

    screen = gdk_display_get_default_screen (gdk_display_get_default ());
    current_wm = gdk_x11_screen_get_window_manager_name (screen);

    if (g_strcmp0 (current_wm, WM_COMMON_METACITY) == 0) {
        mate_metacity_config_tool ();
        return 0;
    }

    if (g_strcmp0 (current_wm, WM_COMMON_MARCO) != 0) {
        wm_unsupported ();
        return 1;
    }

    builder = gtk_builder_new_from_resource ("/org/mate/mcc/windows/window-properties.ui");

    gtk_builder_add_callback_symbols (builder,
                                      "on_dialog_win_response",                        G_CALLBACK (response_cb),
                                      "on_double_click_titlebar_optionmenu_changed",   G_CALLBACK (double_click_titlebar_changed_callback),
                                      "on_middle_click_titlebar_optionmenu_changed",   G_CALLBACK (middle_click_titlebar_changed_callback),
                                      "on_right_click_titlebar_optionmenu_changed",    G_CALLBACK (right_click_titlebar_changed_callback),
                                      "on_scroll_up_titlebar_optionmenu_changed",      G_CALLBACK (scroll_up_titlebar_changed_callback),
                                      "on_scroll_down_titlebar_optionmenu_changed",    G_CALLBACK (scroll_down_titlebar_changed_callback),
                                      "on_autoraise_delay_spinbutton_value_changed",   G_CALLBACK (autoraise_delay_spinbutton_value_callback),
                                      "on_titlebar_layout_optionmenu_changed",         G_CALLBACK (titlebar_layout_changed_callback),
                                      "on_focus_mode_checkbutton_toggled",             G_CALLBACK (mouse_focus_toggled_callback),
                                      "on_focus_mode_mouse_checkbutton_toggled",       G_CALLBACK (mouse_focus_toggled_callback),
                                      NULL);

    gtk_builder_connect_signals (builder, NULL);

    #define GET_WIDGET(x) GTK_WIDGET(gtk_builder_get_object(builder, x))

    /* Window */
    dialog_win = GET_WIDGET ("dialog_win");

    /* Notebook */
    nb = GET_WIDGET ("nb");
    gtk_widget_add_events (nb, GDK_SCROLL_MASK);
    g_signal_connect (nb,
                      "scroll-event",
                      G_CALLBACK (capplet_notebook_scroll_event_cb),
                      NULL);

    /* Behaviour */
    show_tab_border_checkbutton = GET_WIDGET ("show_tab_border_checkbutton");
    compositing_fast_alt_tab_checkbutton = GET_WIDGET ("compositing_fast_alt_tab_checkbutton");
    double_click_titlebar_optionmenu = GET_WIDGET ("double_click_titlebar_optionmenu");
    middle_click_titlebar_optionmenu = GET_WIDGET ("middle_click_titlebar_optionmenu");
    right_click_titlebar_optionmenu = GET_WIDGET ("right_click_titlebar_optionmenu");
    scroll_up_titlebar_optionmenu = GET_WIDGET ("scroll_up_titlebar_optionmenu");
    scroll_down_titlebar_optionmenu = GET_WIDGET ("scroll_down_titlebar_optionmenu");
    focus_mode_checkbutton = GET_WIDGET ("focus_mode_checkbutton");
    focus_mode_mouse_checkbutton = GET_WIDGET ("focus_mode_mouse_checkbutton");
    autoraise_delay_hbox = GET_WIDGET ("autoraise_delay_hbox");
    autoraise_checkbutton = GET_WIDGET ("autoraise_checkbutton");
    autoraise_delay_spinbutton = GET_WIDGET ("autoraise_delay_spinbutton");
    movement_description_label = GET_WIDGET ("movement_description_label");
    alt_click_vbox = GET_WIDGET ("alt_click_vbox");

    /* Placement */
    center_new_windows_checkbutton = GET_WIDGET ("center_new_windows_checkbutton");
    enable_tiling_checkbutton = GET_WIDGET ("enable_tiling_checkbutton");
    allow_top_tiling_checkbutton = GET_WIDGET ("allow_top_tiling_checkbutton");
    titlebar_layout_optionmenu = GET_WIDGET ("titlebar_layout_optionmenu");

    /* Compositing Manager */
    compositing_checkbutton = GET_WIDGET ("compositing_checkbutton");

    g_object_unref (builder);

    #undef GET_WIDGET

    /* Load settings */
    marco_settings = g_settings_new (MARCO_SCHEMA);
    interface_settings = g_settings_new (INTERFACE_SCHEMA);

    reload_mouse_modifiers ();

    gtk_combo_box_set_active (GTK_COMBO_BOX (double_click_titlebar_optionmenu),
                              g_settings_get_enum (marco_settings, MARCO_DOUBLE_CLICK_TITLEBAR_KEY));
    gtk_combo_box_set_active (GTK_COMBO_BOX (middle_click_titlebar_optionmenu),
                              g_settings_get_enum (marco_settings, MARCO_MIDDLE_CLICK_TITLEBAR_KEY));
    gtk_combo_box_set_active (GTK_COMBO_BOX (right_click_titlebar_optionmenu),
                              g_settings_get_enum (marco_settings, MARCO_RIGHT_CLICK_TITLEBAR_KEY));
    gtk_combo_box_set_active (GTK_COMBO_BOX (scroll_up_titlebar_optionmenu),
                              g_settings_get_enum (marco_settings, MARCO_SCROLL_UP_TITLEBAR_KEY));
    gtk_combo_box_set_active (GTK_COMBO_BOX (scroll_down_titlebar_optionmenu),
                              g_settings_get_enum (marco_settings, MARCO_SCROLL_DOWN_TITLEBAR_KEY));

    set_titlebar_button_layout ();

    set_alt_click_value ();

    /* Behaviour */
    g_settings_bind (marco_settings,
                     MARCO_SHOW_TAB_BORDER_KEY,
                     show_tab_border_checkbutton,
                     "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (marco_settings,
                     MARCO_COMPOSITING_FAST_ALT_TAB_KEY,
                     compositing_fast_alt_tab_checkbutton,
                     "active",
                     G_SETTINGS_BIND_DEFAULT | G_SETTINGS_BIND_INVERT_BOOLEAN);

    /* Placement */
    g_settings_bind (marco_settings,
                     MARCO_CENTER_NEW_WINDOWS_KEY,
                     center_new_windows_checkbutton,
                     "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (marco_settings,
                     MARCO_ALLOW_TILING_KEY,
                     enable_tiling_checkbutton,
                     "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (marco_settings,
                     MARCO_ALLOW_TOP_TILING_KEY,
                     allow_top_tiling_checkbutton,
                     "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Compositing Manager */
    g_settings_bind (marco_settings,
                     MARCO_COMPOSITING_MANAGER_KEY,
                     compositing_checkbutton,
                     "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Initialize the checkbox state appropriately */
    mouse_focus_changed_callback(marco_settings, MARCO_FOCUS_KEY, NULL);

    g_settings_bind (marco_settings,
                     MARCO_AUTORAISE_KEY,
                     autoraise_checkbutton,
                     "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_signal_connect (dialog_win, "destroy",
                      G_CALLBACK (gtk_main_quit), NULL);

    g_signal_connect (marco_settings, "changed",
                      G_CALLBACK (marco_settings_changed_callback), NULL);

    g_signal_connect (marco_settings, "changed::" MARCO_FOCUS_KEY,
                      G_CALLBACK (mouse_focus_changed_callback), NULL);

    g_signal_connect (screen, "window_manager_changed",
                      G_CALLBACK (wm_changed_callback), NULL);

    i = 0;
    while (i < n_mouse_modifiers) {
        g_signal_connect (mouse_modifiers[i].radio, "toggled",
                          G_CALLBACK (alt_click_radio_toggled_callback),
                          &mouse_modifiers[i]);
        ++i;
    }

    /* update sensitivity */
    update_sensitivity ();

    gtk_widget_show_all (dialog_win);

    gtk_main ();

    g_object_unref (marco_settings);
    g_object_unref (interface_settings);

    g_free (custom_titlebar_button_layout);

    return 0;
}

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <gdk/gdkx.h>

static void
fill_radio (GtkRadioButton     *group,
        MouseClickModifier *modifier)
{
    modifier->radio = gtk_radio_button_new_with_mnemonic_from_widget (group, modifier->name);
    gtk_box_pack_start (GTK_BOX (alt_click_vbox), modifier->radio, FALSE, FALSE, 0);

    gtk_widget_show (modifier->radio);
}

static void
reload_mouse_modifiers (void)
{
    XModifierKeymap *modmap;
    KeySym *keymap;
    int keysyms_per_keycode;
    int map_size;
    int i;
    gboolean have_meta;
    gboolean have_hyper;
    gboolean have_super;
    int min_keycode, max_keycode;
    int mod_meta, mod_super, mod_hyper;
    AtkObject *label_accessible;

    XDisplayKeycodes (gdk_x11_display_get_xdisplay(gdk_display_get_default()),
                      &min_keycode,
                      &max_keycode);

    keymap = XGetKeyboardMapping (gdk_x11_display_get_xdisplay(gdk_display_get_default()),
                                  min_keycode,
                                  max_keycode - min_keycode,
                                  &keysyms_per_keycode);

    modmap = XGetModifierMapping (gdk_x11_display_get_xdisplay(gdk_display_get_default()));

    have_super = FALSE;
    have_meta = FALSE;
    have_hyper = FALSE;

    /* there are 8 modifiers, and the first 3 are shift, shift lock,
     * and control
     */
    map_size = 8 * modmap->max_keypermod;
    i = 3 * modmap->max_keypermod;
    mod_meta = mod_super = mod_hyper = 0;
    while (i < map_size) {
        /* get the key code at this point in the map,
         * see if its keysym is one we're interested in
         */
        int keycode = modmap->modifiermap[i];

        if (keycode >= min_keycode &&
            keycode <= max_keycode) {
            int j = 0;
            KeySym *syms = keymap + (keycode - min_keycode) * keysyms_per_keycode;

            while (j < keysyms_per_keycode) {
                if (syms[j] == XK_Super_L || syms[j] == XK_Super_R)
                    mod_super = i / modmap->max_keypermod;
                else if (syms[j] == XK_Hyper_L || syms[j] == XK_Hyper_R)
                    mod_hyper = i / modmap->max_keypermod;
                else if ((syms[j] == XK_Meta_L || syms[j] == XK_Meta_R))
                    mod_meta = i / modmap->max_keypermod;
                ++j;
            }
        }

        ++i;
    }

    if ((1 << mod_meta) != Mod1Mask)
        have_meta = TRUE;
    if (mod_super != 0 && mod_super != mod_meta)
        have_super = TRUE;
    if (mod_hyper != 0 && mod_hyper != mod_meta && mod_hyper != mod_super)
        have_hyper = TRUE;

    XFreeModifiermap (modmap);
    XFree (keymap);

    i = 0;
    while (i < n_mouse_modifiers) {
        g_free (mouse_modifiers[i].name);
        if (mouse_modifiers[i].radio)
            gtk_widget_destroy (mouse_modifiers[i].radio);
        ++i;
    }
    g_free (mouse_modifiers);
    mouse_modifiers = NULL;

    n_mouse_modifiers = 1; /* alt */
    if (have_super)
        ++n_mouse_modifiers;
    if (have_hyper)
        ++n_mouse_modifiers;
    if (have_meta)
        ++n_mouse_modifiers;

    mouse_modifiers = g_new0 (MouseClickModifier, n_mouse_modifiers);

    i = 0;

    mouse_modifiers[i].number = i;
    mouse_modifiers[i].name = g_strdup (_("_Alt"));
    mouse_modifiers[i].value = "Alt";
    ++i;

    if (have_hyper) {
        mouse_modifiers[i].number = i;
        mouse_modifiers[i].name = g_strdup (_("H_yper"));
        mouse_modifiers[i].value = "Hyper";
        ++i;
    }

    if (have_super) {
        mouse_modifiers[i].number = i;
        mouse_modifiers[i].name = g_strdup (_("S_uper (or \"Windows logo\")"));
        mouse_modifiers[i].value = "Super";
        ++i;
    }

    if (have_meta) {
        mouse_modifiers[i].number = i;
        mouse_modifiers[i].name = g_strdup (_("_Meta"));
        mouse_modifiers[i].value = "Meta";
        ++i;
    }

    g_assert (i == n_mouse_modifiers);

    i = 0;
    while (i < n_mouse_modifiers) {
        fill_radio (i == 0 ? NULL : GTK_RADIO_BUTTON (mouse_modifiers[i-1].radio), &mouse_modifiers[i]);
        ++i;
    }

    /* set up accessibility relationships between the main label and each
     * radio button, because GTK doesn't do it for us (usually, it expects the
     * label of the radio button to be enough, but our case is better served
     * with associating the main label as well). */
    label_accessible = gtk_widget_get_accessible (movement_description_label);
    if (ATK_IS_OBJECT (label_accessible)) {
        for (i = 0; i < n_mouse_modifiers; i++) {
            AtkObject *radio_accessible = gtk_widget_get_accessible (mouse_modifiers[i].radio);
            if (ATK_IS_OBJECT (radio_accessible)) {
                atk_object_add_relationship (label_accessible,
                                             ATK_RELATION_LABEL_FOR,
                                             radio_accessible);
                atk_object_add_relationship (radio_accessible,
                                             ATK_RELATION_LABELLED_BY,
                                             label_accessible);
            }
        }
    }
}
