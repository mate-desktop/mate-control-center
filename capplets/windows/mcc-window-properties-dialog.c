/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/* mcc-window-properties-dialog.c
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

#include "mcc-window-properties-dialog.h"

#include <stdlib.h>
#include <glib/gi18n.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <gdk/gdkx.h>

#include "wm-common.h"
#include "capplet-util.h"

#define MARCO_SCHEMA "org.mate.Marco.general"

#define MARCO_CENTER_NEW_WINDOWS_KEY "center-new-windows"
#define MARCO_ALLOW_TILING_KEY "allow-tiling"
#define MARCO_ALLOW_TOP_TILING_KEY "allow-top-tiling"
#define MARCO_SHOW_TAB_BORDER_KEY "show-tab-border"
#define MARCO_BUTTON_LAYOUT_KEY "button-layout"
#define MARCO_DOUBLE_CLICK_TITLEBAR_KEY "action-double-click-titlebar"
#define MARCO_FOCUS_KEY "focus-mode"
#define MARCO_AUTORAISE_KEY "auto-raise"
#define MARCO_AUTORAISE_DELAY_KEY "auto-raise-delay"
#define MARCO_MOUSE_MODIFIER_KEY "mouse-button-modifier"

#define MARCO_COMPOSITING_MANAGER_KEY "compositing-manager"
#define MARCO_COMPOSITING_FAST_ALT_TAB_KEY "compositing-fast-alt-tab"

#define MARCO_BUTTON_LAYOUT_RIGHT "menu:minimize,maximize,close"
#define MARCO_BUTTON_LAYOUT_LEFT "close,minimize,maximize:"

/* keep following enums in sync with marco */
enum
{
    ACTION_TITLEBAR_TOGGLE_SHADE,
    ACTION_TITLEBAR_TOGGLE_MAXIMIZE,
    ACTION_TITLEBAR_TOGGLE_MAXIMIZE_HORIZONTALLY,
    ACTION_TITLEBAR_TOGGLE_MAXIMIZE_VERTICALLY,
    ACTION_TITLEBAR_MINIMIZE,
    ACTION_TITLEBAR_NONE,
    ACTION_TITLEBAR_LOWER,
    ACTION_TITLEBAR_MENU
};

typedef enum
{
    FOCUS_MODE_CLICK = 0,
    FOCUS_MODE_SLOPPY,
    FOCUS_MODE_MOUSE
} FocusMode;

typedef struct
{
    int         number;
    char       *name;
    const char *value; /* machine-readable name for storing config */
    GtkWidget  *radio;
} MouseClickModifier;

struct _MccWindowPropertiesDialog
{
    GtkDialog  parent;

    /* Notebook */
    GtkWidget *nb;

    /* Behaviour */
    GtkWidget *show_tab_border_checkbutton;
    GtkWidget *compositing_fast_alt_tab_checkbutton;
    GtkWidget *double_click_titlebar_optionmenu;
    GtkWidget *focus_mode_checkbutton;
    GtkWidget *focus_mode_mouse_checkbutton;
    GtkWidget *autoraise_checkbutton;
    GtkWidget *autoraise_delay_spinbutton;
    GtkWidget *autoraise_delay_hbox;
    GtkWidget *alt_click_vbox;

    /* Placement */
    GtkWidget *center_new_windows_checkbutton;
    GtkWidget *enable_tiling_checkbutton;
    GtkWidget *allow_top_tiling_checkbutton;
    GtkWidget *titlebar_layout_optionmenu;

    /* Compositing Manager */
    GtkWidget *compositing_checkbutton;

    GSettings *marco_settings;

    MouseClickModifier *mouse_modifiers;
    int n_mouse_modifiers;
};

G_DEFINE_TYPE (MccWindowPropertiesDialog, mcc_window_properties_dialog, GTK_TYPE_DIALOG)

static void
update_sensitivity (MccWindowPropertiesDialog *dialog)
{
    gchar *str;
    FocusMode focus_mode;

    gtk_widget_set_sensitive (dialog->compositing_fast_alt_tab_checkbutton,
                              g_settings_get_boolean (dialog->marco_settings, MARCO_COMPOSITING_MANAGER_KEY));
    gtk_widget_set_sensitive (dialog->allow_top_tiling_checkbutton,
                              g_settings_get_boolean (dialog->marco_settings, MARCO_ALLOW_TILING_KEY));

    focus_mode = g_settings_get_enum (dialog->marco_settings, MARCO_FOCUS_KEY);
    gtk_widget_set_sensitive (dialog->focus_mode_mouse_checkbutton,
                              focus_mode != FOCUS_MODE_CLICK);
    gtk_widget_set_sensitive (dialog->autoraise_delay_hbox,
                              focus_mode != FOCUS_MODE_CLICK);
    gtk_widget_set_sensitive (dialog->autoraise_delay_spinbutton,
                              focus_mode != FOCUS_MODE_CLICK &&
                              g_settings_get_boolean (dialog->marco_settings, MARCO_AUTORAISE_KEY));

    str = g_settings_get_string (dialog->marco_settings, MARCO_BUTTON_LAYOUT_KEY);
    gtk_widget_set_sensitive (dialog->titlebar_layout_optionmenu,
                              g_strcmp0 (str, MARCO_BUTTON_LAYOUT_LEFT) == 0 ||
                              g_strcmp0 (str, MARCO_BUTTON_LAYOUT_RIGHT) == 0);
    g_free (str);
}

static void
marco_settings_changed_callback (GSettings   *settings,
                                 const gchar *key,
                                 gpointer     data)
{
    MccWindowPropertiesDialog *dialog = data;

    update_sensitivity (dialog);
}

static void
mouse_focus_toggled_callback (GtkWidget *button,
                              gpointer   data)
{
    MccWindowPropertiesDialog *dialog = data;
    FocusMode focus_mode;

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->focus_mode_checkbutton))) {
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->focus_mode_mouse_checkbutton)))
            focus_mode = FOCUS_MODE_MOUSE;
        else
            focus_mode = FOCUS_MODE_SLOPPY;
    }
    else {
        focus_mode = FOCUS_MODE_CLICK;
    }
    g_settings_set_enum (dialog->marco_settings, MARCO_FOCUS_KEY, focus_mode);
}

static void
mouse_focus_changed_callback (GSettings   *settings,
                              const gchar *key,
                              gpointer     data)
{
    MccWindowPropertiesDialog *dialog = data;
    FocusMode focus_mode;
    gboolean active_focus_mode;
    gboolean active_focus_mode_mouse;

    focus_mode = g_settings_get_enum (settings, key);
    switch (focus_mode) {
        case FOCUS_MODE_MOUSE:
            active_focus_mode       = TRUE;
            active_focus_mode_mouse = TRUE;
            break;
        case FOCUS_MODE_SLOPPY:
            active_focus_mode       = TRUE;
            active_focus_mode_mouse = FALSE;
            break;
        default:
            active_focus_mode       = FALSE;
            active_focus_mode_mouse = FALSE;
    }

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->focus_mode_checkbutton),
                                  active_focus_mode);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->focus_mode_mouse_checkbutton),
                                  active_focus_mode_mouse);
}

static void
on_autoraise_delay_spinbutton_value_changed (GtkWidget *spinbutton,
                                             gpointer   data)
{
    MccWindowPropertiesDialog *dialog = data;

    g_settings_set_int (dialog->marco_settings, MARCO_AUTORAISE_DELAY_KEY,
                        gtk_spin_button_get_value (GTK_SPIN_BUTTON (spinbutton)) * 1000);
}

static void
on_double_click_titlebar_optionmenu_changed (GtkWidget *optionmenu,
                                             gpointer   data)
{
    MccWindowPropertiesDialog *dialog = data;

    g_settings_set_enum (dialog->marco_settings, MARCO_DOUBLE_CLICK_TITLEBAR_KEY,
                         gtk_combo_box_get_active (GTK_COMBO_BOX (optionmenu)));
}

static void
on_titlebar_layout_optionmenu_changed (GtkWidget *optionmenu,
                                       gpointer   data)
{
    MccWindowPropertiesDialog *dialog = data;
    gint value;
    const char* button_layout = NULL;

    value = gtk_combo_box_get_active (GTK_COMBO_BOX (optionmenu));
    switch (value) {
        case 0:
            button_layout = MARCO_BUTTON_LAYOUT_RIGHT;
            break;
        case 1:
            button_layout = MARCO_BUTTON_LAYOUT_LEFT;
    }

    if (button_layout != NULL)
        g_settings_set_string (dialog->marco_settings, MARCO_BUTTON_LAYOUT_KEY, button_layout);
}

static void
alt_click_radio_toggled_callback (GtkWidget *radio,
                                  gpointer   data)
{
    MouseClickModifier *modifier = data;
    gboolean active;

    active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio));

    if (active) {
        GSettings *marco_settings;
        gchar *value;

        marco_settings = g_settings_new (MARCO_SCHEMA);
        value = g_strdup_printf ("<%s>", modifier->value);
        g_settings_set_string (marco_settings, MARCO_MOUSE_MODIFIER_KEY, value);
        g_free (value);
        g_object_unref (marco_settings);
    }
}

static void
set_alt_click_value (MccWindowPropertiesDialog *dialog)
{
    gboolean match_found = FALSE;
    gchar *mouse_move_modifier;
    gchar *value;
    int i;

    mouse_move_modifier = g_settings_get_string (dialog->marco_settings, MARCO_MOUSE_MODIFIER_KEY);

    /* We look for a matching modifier and set it. */
    if (mouse_move_modifier != NULL) {
        for (i = 0; i < dialog->n_mouse_modifiers; i ++) {
            value = g_strdup_printf ("<%s>", dialog->mouse_modifiers[i].value);
            if (strcmp (value, mouse_move_modifier) == 0) {
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->mouse_modifiers[i].radio), TRUE);
                match_found = TRUE;
                break;
            }
            g_free (value);
        }
        g_free (mouse_move_modifier);
    }

    /* No matching modifier was found; we set all the toggle buttons to be
     * insensitive. */
    for (i = 0; i < dialog->n_mouse_modifiers; i++) {
        gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (dialog->mouse_modifiers[i].radio), ! match_found);
    }
}

static void
wm_changed_callback (GdkScreen *screen,
                     void      *data)
{
    const char *current_wm;
    MccWindowPropertiesDialog *dialog = data;

    current_wm = gdk_x11_screen_get_window_manager_name (screen);
    gtk_widget_set_sensitive (GTK_WIDGET (dialog), g_strcmp0 (current_wm, WM_COMMON_MARCO) == 0);
}

static void
mcc_window_properties_dialog_response (GtkDialog *dialog,
                                       int        response_id)
{
    if (response_id == GTK_RESPONSE_HELP) {
        capplet_help (GTK_WINDOW (dialog), "goscustdesk-58");
    } else {
        gtk_widget_destroy (GTK_WIDGET (dialog));
        gtk_main_quit ();
    }
}

static void
mcc_window_properties_dialog_dispose (GObject *object)
{
    MccWindowPropertiesDialog *dialog;
    int i;

    g_return_if_fail (object != NULL);
    g_return_if_fail (MCC_IS_WINDOW_PROPERTIES_DIALOG (object));

    dialog = MCC_WINDOW_PROPERTIES_DIALOG (object);

    if (dialog->marco_settings != NULL) {
        g_object_unref (dialog->marco_settings);
        dialog->marco_settings = NULL;
    }

    i = 0;
    while (i < dialog->n_mouse_modifiers) {
        g_free (dialog->mouse_modifiers[i].name);
        if (dialog->mouse_modifiers[i].radio)
            gtk_widget_destroy (dialog->mouse_modifiers[i].radio);
        ++i;
    }
    g_free (dialog->mouse_modifiers);
    dialog->mouse_modifiers = NULL;
    dialog->n_mouse_modifiers = 0;

    G_OBJECT_CLASS (mcc_window_properties_dialog_parent_class)->dispose (object);
}

static void
mcc_window_properties_dialog_finalize (GObject *object)
{
        g_return_if_fail (object != NULL);
        g_return_if_fail (MCC_IS_WINDOW_PROPERTIES_DIALOG (object));

        G_OBJECT_CLASS (mcc_window_properties_dialog_parent_class)->finalize (object);
}

static void
fill_radio (GtkRadioButton            *group,
            MouseClickModifier        *modifier,
            GtkWidget                 *alt_click_vbox)
{
    modifier->radio = gtk_radio_button_new_with_mnemonic_from_widget (group, modifier->name);
    gtk_box_pack_start (GTK_BOX (alt_click_vbox), modifier->radio, FALSE, FALSE, 0);
    gtk_widget_show (modifier->radio);
}

static void
reload_mouse_modifiers (MccWindowPropertiesDialog *dialog)
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
    while (i < dialog->n_mouse_modifiers) {
        g_free (dialog->mouse_modifiers[i].name);
        if (dialog->mouse_modifiers[i].radio)
            gtk_widget_destroy (dialog->mouse_modifiers[i].radio);
        ++i;
    }
    g_free (dialog->mouse_modifiers);
    dialog->mouse_modifiers = NULL;

    dialog->n_mouse_modifiers = 1; /* alt */
    if (have_super)
        ++dialog->n_mouse_modifiers;
    if (have_hyper)
        ++dialog->n_mouse_modifiers;
    if (have_meta)
        ++dialog->n_mouse_modifiers;

    dialog->mouse_modifiers = g_new0 (MouseClickModifier, dialog->n_mouse_modifiers);

    i = 0;

    dialog->mouse_modifiers[i].number = i;
    dialog->mouse_modifiers[i].name = g_strdup (_("_Alt"));
    dialog->mouse_modifiers[i].value = "Alt";
    ++i;

    if (have_hyper) {
        dialog->mouse_modifiers[i].number = i;
        dialog->mouse_modifiers[i].name = g_strdup (_("H_yper"));
        dialog->mouse_modifiers[i].value = "Hyper";
        ++i;
    }

    if (have_super) {
        dialog->mouse_modifiers[i].number = i;
        dialog->mouse_modifiers[i].name = g_strdup (_("S_uper (or \"Windows logo\")"));
        dialog->mouse_modifiers[i].value = "Super";
        ++i;
    }

    if (have_meta) {
        dialog->mouse_modifiers[i].number = i;
        dialog->mouse_modifiers[i].name = g_strdup (_("_Meta"));
        dialog->mouse_modifiers[i].value = "Meta";
        ++i;
    }

    g_assert (i == dialog->n_mouse_modifiers);

    i = 0;
    while (i < dialog->n_mouse_modifiers) {
        fill_radio (i == 0 ? NULL : GTK_RADIO_BUTTON (dialog->mouse_modifiers[i-1].radio),
                    &dialog->mouse_modifiers[i],
                    dialog->alt_click_vbox);
        ++i;
    }
}

static void
mcc_window_properties_dialog_class_init (MccWindowPropertiesDialogClass *klass)
{
    const gchar    *resource = "/org/mate/mcc/windows/window-properties.ui";
    GObjectClass   *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
    GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

    object_class->dispose = mcc_window_properties_dialog_dispose;
    object_class->finalize = mcc_window_properties_dialog_finalize;

    dialog_class->response = mcc_window_properties_dialog_response;

    gtk_widget_class_set_template_from_resource (widget_class, resource);

    gtk_widget_class_bind_template_child (widget_class, MccWindowPropertiesDialog, nb);

    /* Behaviour */
    gtk_widget_class_bind_template_child (widget_class, MccWindowPropertiesDialog, show_tab_border_checkbutton);
    gtk_widget_class_bind_template_child (widget_class, MccWindowPropertiesDialog, compositing_fast_alt_tab_checkbutton);
    gtk_widget_class_bind_template_child (widget_class, MccWindowPropertiesDialog, double_click_titlebar_optionmenu);
    gtk_widget_class_bind_template_child (widget_class, MccWindowPropertiesDialog, focus_mode_checkbutton);
    gtk_widget_class_bind_template_child (widget_class, MccWindowPropertiesDialog, focus_mode_mouse_checkbutton);
    gtk_widget_class_bind_template_child (widget_class, MccWindowPropertiesDialog, autoraise_delay_hbox);
    gtk_widget_class_bind_template_child (widget_class, MccWindowPropertiesDialog, autoraise_checkbutton);
    gtk_widget_class_bind_template_child (widget_class, MccWindowPropertiesDialog, autoraise_delay_spinbutton);
    gtk_widget_class_bind_template_child (widget_class, MccWindowPropertiesDialog, alt_click_vbox);

    /* Placement */
    gtk_widget_class_bind_template_child (widget_class, MccWindowPropertiesDialog, center_new_windows_checkbutton);
    gtk_widget_class_bind_template_child (widget_class, MccWindowPropertiesDialog, enable_tiling_checkbutton);
    gtk_widget_class_bind_template_child (widget_class, MccWindowPropertiesDialog, allow_top_tiling_checkbutton);
    gtk_widget_class_bind_template_child (widget_class, MccWindowPropertiesDialog, titlebar_layout_optionmenu);

    /* Compositing Manager */
    gtk_widget_class_bind_template_child (widget_class, MccWindowPropertiesDialog, compositing_checkbutton);

    /* Callbacks */
    gtk_widget_class_bind_template_callback (widget_class, on_double_click_titlebar_optionmenu_changed);
    gtk_widget_class_bind_template_callback (widget_class, on_autoraise_delay_spinbutton_value_changed);
    gtk_widget_class_bind_template_callback (widget_class, on_titlebar_layout_optionmenu_changed);
    gtk_widget_class_bind_template_callback (widget_class, mouse_focus_toggled_callback);
}

static void
setup_dialog (MccWindowPropertiesDialog *dialog)
{
    GdkScreen *screen;
    gchar *str;
    int i;

    /* Notebook */
    gtk_widget_add_events (dialog->nb, GDK_SCROLL_MASK);
    g_signal_connect (dialog->nb, "scroll-event",
                      G_CALLBACK (capplet_notebook_scroll_event_cb),
                      NULL);

    /* Load settings */
    dialog->marco_settings = g_settings_new (MARCO_SCHEMA);
    reload_mouse_modifiers (dialog);

    str = g_settings_get_string (dialog->marco_settings, MARCO_BUTTON_LAYOUT_KEY);
    gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->titlebar_layout_optionmenu),
                              g_strcmp0 (str, MARCO_BUTTON_LAYOUT_RIGHT) == 0 ? 0 : 1);
    g_free (str);

    gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->double_click_titlebar_optionmenu),
                              g_settings_get_enum (dialog->marco_settings, MARCO_DOUBLE_CLICK_TITLEBAR_KEY));

    set_alt_click_value (dialog);

    gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->double_click_titlebar_optionmenu),
                              g_settings_get_enum (dialog->marco_settings, MARCO_DOUBLE_CLICK_TITLEBAR_KEY));

    /* Behaviour */
    g_settings_bind (dialog->marco_settings, MARCO_SHOW_TAB_BORDER_KEY,
                     dialog->show_tab_border_checkbutton, "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (dialog->marco_settings, MARCO_COMPOSITING_FAST_ALT_TAB_KEY,
                     dialog->compositing_fast_alt_tab_checkbutton, "active",
                     G_SETTINGS_BIND_DEFAULT | G_SETTINGS_BIND_INVERT_BOOLEAN);

    /* Placement */
    g_settings_bind (dialog->marco_settings, MARCO_CENTER_NEW_WINDOWS_KEY,
                     dialog->center_new_windows_checkbutton, "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (dialog->marco_settings, MARCO_ALLOW_TILING_KEY,
                     dialog->enable_tiling_checkbutton, "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (dialog->marco_settings, MARCO_ALLOW_TOP_TILING_KEY,
                     dialog->allow_top_tiling_checkbutton, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Compositing Manager */
    g_settings_bind (dialog->marco_settings, MARCO_COMPOSITING_MANAGER_KEY,
                     dialog->compositing_checkbutton, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Initialize the checkbox state appropriately */
    mouse_focus_changed_callback (dialog->marco_settings, MARCO_FOCUS_KEY, dialog);

    g_settings_bind (dialog->marco_settings, MARCO_AUTORAISE_KEY,
                     dialog->autoraise_checkbutton, "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_signal_connect (dialog->marco_settings, "changed",
                      G_CALLBACK (marco_settings_changed_callback),
                      dialog);

    g_signal_connect (dialog->marco_settings, "changed::" MARCO_FOCUS_KEY,
                      G_CALLBACK (mouse_focus_changed_callback),
                      dialog);

    screen = gdk_display_get_default_screen (gdk_display_get_default ());
    g_signal_connect (screen, "window_manager_changed",
                      G_CALLBACK (wm_changed_callback),
                      dialog);

    i = 0;
    while (i < dialog->n_mouse_modifiers) {
        g_signal_connect (dialog->mouse_modifiers[i].radio, "toggled",
                          G_CALLBACK (alt_click_radio_toggled_callback),
                          &dialog->mouse_modifiers[i]);
        ++i;
    }

    /* update sensitivity */
    update_sensitivity (dialog);
}

static void
mcc_window_properties_dialog_init (MccWindowPropertiesDialog *dialog)
{
    gtk_widget_init_template (GTK_WIDGET (dialog));
    setup_dialog (dialog);
}

GtkWidget *
mcc_window_properties_dialog_new (void)
{
    GObject *object;

    object = g_object_new (MCC_TYPE_WINDOW_PROPERTIES_DIALOG,
                           NULL);

    return GTK_WIDGET (object);
}
