/* Monitor Settings for Wayland.  A preference panel for configuring monitors
 * that talks to the wlrandr plugin of mate-settings-daemon over D-Bus.
 *
 * Copyright (C) 2023  mate-desktop.org
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <config.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include "scrollarea.h"
#include <glib/gi18n.h>
#include <gio/gio.h>

#include "capplet-util.h"

#define MSD_WLRANDR_DBUS_NAME     "org.mate.SettingsDaemon.WLRANDR"
#define MSD_WLRANDR_DBUS_NAME_2   "org.mate.SettingsDaemon.WLRANDR_2"
#define MSD_WLRANDR_DBUS_PATH     "/org/mate/SettingsDaemon/WLRANDR"
#define MSD_WLRANDR_SCHEMA        "org.mate.SettingsDaemon.plugins.wlrandr"
#define SHOW_ICON_KEY             "show-notification-icon"

#define HEAD_INFO_TYPE  "a(ssbiiidbb(iiii)a(iiii))"
#define HEAD_SET_TYPE   "a(sbiiiiidib)"
#define HEAD_INFO_FORMAT  "(ssbiiidbb(iiii)a(iiii))"
#define HEAD_SET_FORMAT   "(sbiiiiidib)"
#define MODE_FORMAT       "(iiii)"

/* GVariant type strings cannot contain whitespace, so the compact forms
 * above are required on the wire.  These enums document what each field
 * means so the code below never has to be decoded from the string. */

/* One head as reported by GetConfiguration:
 * (name, description, enabled, x, y, transform, scale,
 *  adaptive_sync_supported, adaptive_sync,
 *  current_mode (width, height, refresh, preferred),
 *  modes (width, height, refresh, preferred) ...) */
typedef enum
{
    HEAD_INFO_FIELD_NAME,                    /* s      output name */
    HEAD_INFO_FIELD_DESCRIPTION,             /* s      human-readable description */
    HEAD_INFO_FIELD_ENABLED,                 /* b      output enabled */
    HEAD_INFO_FIELD_X,                       /* i      x position on the logical screen */
    HEAD_INFO_FIELD_Y,                       /* i      y position on the logical screen */
    HEAD_INFO_FIELD_TRANSFORM,               /* i      rotation (wl_output_transform) */
    HEAD_INFO_FIELD_SCALE,                   /* d      fractional scale factor */
    HEAD_INFO_FIELD_ADAPTIVE_SYNC_SUPPORTED, /* b      hardware supports adaptive sync */
    HEAD_INFO_FIELD_ADAPTIVE_SYNC,           /* b      adaptive sync currently active */
    HEAD_INFO_FIELD_CURRENT_MODE,            /* (iiii) the mode currently in use */
    HEAD_INFO_FIELD_MODES,                   /* a(iiii) all modes the output supports */
    HEAD_INFO_N_FIELDS
} HeadInfoField;

/* One entry of the modes array. */
typedef enum
{
    MODE_FIELD_WIDTH,                        /* i      width in pixels */
    MODE_FIELD_HEIGHT,                       /* i      height in pixels */
    MODE_FIELD_REFRESH,                      /* i      refresh rate in mHz */
    MODE_FIELD_PREFERRED,                    /* i      1 if this is the preferred mode */
    MODE_N_FIELDS
} ModeField;

/* One head as sent to SetConfiguration:
 * (name, enabled, width, height, refresh, x, y, scale, transform,
 *  adaptive_sync) */
typedef enum
{
    HEAD_SET_FIELD_NAME,                     /* s      output name */
    HEAD_SET_FIELD_ENABLED,                  /* b      output enabled */
    HEAD_SET_FIELD_WIDTH,                    /* i      desired width in pixels */
    HEAD_SET_FIELD_HEIGHT,                   /* i      desired height in pixels */
    HEAD_SET_FIELD_RATE,                     /* i      desired refresh rate in mHz */
    HEAD_SET_FIELD_X,                        /* i      desired x position */
    HEAD_SET_FIELD_Y,                        /* i      desired y position */
    HEAD_SET_FIELD_SCALE,                    /* d      desired fractional scale */
    HEAD_SET_FIELD_TRANSFORM,                /* i      desired rotation */
    HEAD_SET_FIELD_ADAPTIVE_SYNC,            /* b      desired adaptive sync state */
    HEAD_SET_N_FIELDS
} HeadSetField;

typedef struct App App;
typedef struct GrabInfo GrabInfo;

typedef struct
{
    int       width;
    int       height;
    int       rate;
    gboolean  preferred;
} Mode;

typedef struct
{
    char       *name;
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
} OutputHead;

struct GrabInfo
{
    int grab_x;
    int grab_y;
    int output_x;
    int output_y;
};

struct App
{
    GArray    *heads;                  /* OutputHead */
    OutputHead *current_head;

    GtkWidget *dialog;
    GtkWidget *current_monitor_event_box;
    GtkWidget *current_monitor_label;
    GtkWidget *monitor_on_radio;
    GtkWidget *monitor_off_radio;
    GtkWidget *resolution_combo;
    GtkWidget *refresh_combo;
    GtkWidget *rotation_combo;
    GtkWidget *panel_checkbox;
    GtkWidget *scale_vbox;
    GtkWidget *scale_bbox;
    GtkWidget *scale_combo;
    GtkWidget *show_icon_checkbox;
    GtkWidget *adaptive_sync_checkbox;
    GtkWidget *primary_button;
    GtkWidget *make_default_button;
    GtkWidget *detect_displays_button;

    GtkWidget *area;
    gboolean   ignore_gui_changes;
    gboolean   applying;
    GSettings *settings;

    GDBusConnection *connection;
    GDBusProxy *proxy;
    guint       signal_id;
};

/* Response codes for custom buttons in the main dialog */
enum {
    RESPONSE_MAKE_DEFAULT = 1
};

static void rebuild_gui (App *app);
static void fetch_configuration (App *app);
static void on_rate_changed (GtkComboBox *box, gpointer data);
static void monitor_on_off_toggled_cb (GtkToggleButton *toggle, gpointer data);
static gboolean mode_is_present (OutputHead *head, int width, int height, int rate);
static void select_default_head (App *app);
static void on_adaptive_sync_toggled (GtkToggleButton *toggle, gpointer data);
static void rebuild_adaptive_sync_checkbox (App *app);

static void
error_message (App *app, const char *primary_text, const char *secondary_text)
{
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new ((app && app->dialog) ? GTK_WINDOW (app->dialog) : NULL,
                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     "%s", primary_text);

    if (secondary_text)
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", secondary_text);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

/* ------------------------------------------------------------------ *
 *  OutputHead helpers                                                *
 * ------------------------------------------------------------------ */

static OutputHead *
get_nth_head (App *app, int i)
{
    return &g_array_index (app->heads, OutputHead, i);
}

static void
head_set_transform (OutputHead *head, int transform)
{
    /* wl_output.transform uses the values of the wl_output::transform
     * enum; keep only the rotation part since the capplet does not
     * expose reflection.
     */
    head->transform = transform & 0x3;
}

static void
head_get_preferred_size (OutputHead *head, int *width, int *height)
{
    guint i;
    int best_w = 0, best_h = 0;

    for (i = 0; i < head->modes->len; i++)
    {
        Mode *mode = &g_array_index (head->modes, Mode, i);

        if (mode->preferred)
        {
            *width = mode->width;
            *height = mode->height;
            return;
        }

        if (mode->width * mode->height > best_w * best_h)
        {
            best_w = mode->width;
            best_h = mode->height;
        }
    }

    *width = best_w;
    *height = best_h;
}

static void
get_geometry (OutputHead *head, int *w, int *h)
{
    if (head->enabled && head->width > 0 && head->height > 0)
    {
        *w = head->width;
        *h = head->height;
    }
    else
    {
        head_get_preferred_size (head, w, h);
    }

    if (head->transform & 1)
    {
        int tmp;

        tmp = *h;
        *h = *w;
        *w = tmp;
    }
}

static int
count_active_outputs (App *app)
{
    guint i;
    int count = 0;

    for (i = 0; i < app->heads->len; i++)
    {
        if (get_nth_head (app, i)->enabled)
            count++;
    }

    return count;
}

static GList *
list_heads (App *app, int *total_w, int *total_h)
{
    guint i;
    int dummy;
    GList *result = NULL;

    if (!total_w)
        total_w = &dummy;
    if (!total_h)
        total_h = &dummy;

    *total_w = 0;
    *total_h = 0;

    for (i = 0; i < app->heads->len; i++)
    {
        OutputHead *head = get_nth_head (app, i);
        int w, h;

        result = g_list_prepend (result, head);

        get_geometry (head, &w, &h);

        *total_w += w;
        *total_h += h;
    }

    return g_list_reverse (result);
}

static double
compute_scale (App *app)
{
    int available_w, available_h;
    int total_w, total_h;
    guint n_monitors;
    GdkRectangle viewport;
    GList *heads;
    const int SPACE = 15;
    const int MARGIN = 15;

    foo_scroll_area_get_viewport (FOO_SCROLL_AREA (app->area), &viewport);

    heads = list_heads (app, &total_w, &total_h);

    n_monitors = g_list_length (heads);

    g_list_free (heads);

    available_w = viewport.width  - 2 * MARGIN - ((int) n_monitors - 1) * SPACE;
    available_h = viewport.height - 2 * MARGIN - ((int) n_monitors - 1) * SPACE;

    if (total_w == 0 || total_h == 0)
        return 1.0;

    return MIN ((double) available_w / total_w, (double) available_h / total_h);
}

/* ------------------------------------------------------------------ *
 *  Combo box helpers                                                 *
 * ------------------------------------------------------------------ */

typedef struct
{
    const char *text;
    gboolean found;
    GtkTreeIter iter;
} ForeachInfo;

static gboolean
foreach (GtkTreeModel *model,
         GtkTreePath *path,
         GtkTreeIter *iter,
         gpointer data)
{
    ForeachInfo *info = data;
    char *text = NULL;

    gtk_tree_model_get (model, iter, 0, &text, -1);

    g_assert (text != NULL);

    if (strcmp (info->text, text) == 0)
    {
        info->found = TRUE;
        info->iter = *iter;
        return TRUE;
    }

    return FALSE;
}

static void
clear_combo (GtkWidget *widget)
{
    GtkComboBox *box = GTK_COMBO_BOX (widget);
    GtkTreeModel *model = gtk_combo_box_get_model (box);
    GtkListStore *store = GTK_LIST_STORE (model);

    gtk_list_store_clear (store);
}

static void
make_text_combo (GtkWidget *widget, int sort_column)
{
    GtkComboBox *box = GTK_COMBO_BOX (widget);
    GtkListStore *store = gtk_list_store_new (
        6,
        G_TYPE_STRING,      /* Text */
        G_TYPE_INT,         /* Width */
        G_TYPE_INT,         /* Height */
        G_TYPE_INT,         /* Frequency */
        G_TYPE_INT,         /* Width * Height */
        G_TYPE_INT);        /* Rotation */

    GtkCellRenderer *cell;

    gtk_cell_layout_clear (GTK_CELL_LAYOUT (widget));

    gtk_combo_box_set_model (box, GTK_TREE_MODEL (store));

    cell = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (box), cell, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (box), cell,
                                    "text", 0,
                                    NULL);

    if (sort_column != -1)
    {
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
                                              sort_column,
                                              GTK_SORT_DESCENDING);
    }
}

static void
add_key (GtkWidget *widget,
         const char *text,
         guint       width,
         guint       height,
         int         rate,
         int         rotation)
{
    ForeachInfo info;
    GtkComboBox *box = GTK_COMBO_BOX (widget);
    GtkTreeModel *model = gtk_combo_box_get_model (box);
    GtkListStore *store = GTK_LIST_STORE (model);

    info.text = text;
    info.found = FALSE;

    gtk_tree_model_foreach (model, foreach, &info);

    if (!info.found)
    {
        GtkTreeIter iter;
        gtk_list_store_insert_with_values (store, &iter, -1,
                                           0, text,
                                           1, width,
                                           2, height,
                                           3, rate,
                                           4, width * height,
                                           5, rotation,
                                           -1);
    }
}

static gboolean
combo_select (GtkWidget *widget, const char *text)
{
    GtkComboBox *box = GTK_COMBO_BOX (widget);
    GtkTreeModel *model = gtk_combo_box_get_model (box);
    ForeachInfo info;

    info.text = text;
    info.found = FALSE;

    gtk_tree_model_foreach (model, foreach, &info);

    if (!info.found)
        return FALSE;

    gtk_combo_box_set_active_iter (box, &info.iter);
    return TRUE;
}

static gboolean
get_mode (GtkWidget *widget, int *width, int *height, int *freq, int *rot)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    GtkComboBox *box = GTK_COMBO_BOX (widget);
    int dummy;

    if (!gtk_combo_box_get_active_iter (box, &iter))
        return FALSE;

    if (!width)
        width = &dummy;

    if (!height)
        height = &dummy;

    if (!freq)
        freq = &dummy;

    if (!rot)
        rot = &dummy;

    model = gtk_combo_box_get_model (box);
    gtk_tree_model_get (model, &iter,
                        1, width,
                        2, height,
                        3, freq,
                        5, rot,
                        -1);

    return TRUE;
}

static char *
make_resolution_string (guint width, guint height)
{
    return g_strdup_printf (_("%u x %u"), width, height);
}

static char *
make_rate_string (int hz)
{
    return g_strdup_printf (_("%d Hz"), hz);
}

/* ------------------------------------------------------------------ *
 *  D-Bus                                                             *
 * ------------------------------------------------------------------ */

static void
parse_heads_from_variant (App *app, GVariant *configuration)
{
    GVariantIter iter;

    g_variant_iter_init (&iter, configuration);

    if (app->heads)
    {
        g_array_free (app->heads, TRUE);
        app->heads = NULL;
    }

    app->heads = g_array_new (FALSE, FALSE, sizeof (OutputHead));

    while (TRUE)
    {
        GVariant *head_var;
        const char *name;
        const char *description;
        gboolean enabled;
        gboolean adaptive_sync_supported;
        gboolean adaptive_sync;
        int x, y, transform;
        double scale;
        int cur_w, cur_h, cur_rate, cur_pref;
        g_autoptr (GVariant) modes_var = NULL;
        OutputHead head;
        GVariantIter modes_iter;

        head_var = g_variant_iter_next_value (&iter);
        if (head_var == NULL)
            break;

        g_variant_get_child (head_var, HEAD_INFO_FIELD_NAME,
                             "&s", &name);
        g_variant_get_child (head_var, HEAD_INFO_FIELD_DESCRIPTION,
                             "&s", &description);
        g_variant_get_child (head_var, HEAD_INFO_FIELD_ENABLED,
                             "b", &enabled);
        g_variant_get_child (head_var, HEAD_INFO_FIELD_X,
                             "i", &x);
        g_variant_get_child (head_var, HEAD_INFO_FIELD_Y,
                             "i", &y);
        g_variant_get_child (head_var, HEAD_INFO_FIELD_TRANSFORM,
                             "i", &transform);
        g_variant_get_child (head_var, HEAD_INFO_FIELD_SCALE,
                             "d", &scale);
        g_variant_get_child (head_var, HEAD_INFO_FIELD_ADAPTIVE_SYNC_SUPPORTED,
                             "b", &adaptive_sync_supported);
        g_variant_get_child (head_var, HEAD_INFO_FIELD_ADAPTIVE_SYNC,
                             "b", &adaptive_sync);
        g_variant_get_child (head_var, HEAD_INFO_FIELD_CURRENT_MODE,
                             MODE_FORMAT, &cur_w, &cur_h, &cur_rate, &cur_pref);
        g_variant_get_child (head_var, HEAD_INFO_FIELD_MODES,
                             "@a" MODE_FORMAT, &modes_var);

        memset (&head, 0, sizeof (OutputHead));
        head.name = g_strdup (name);
        head.description = g_strdup (description);
        head.enabled = enabled;
        head.x = x;
        head.y = y;
        head_set_transform (&head, transform);
        head.scale = scale;
        head.adaptive_sync_supported = adaptive_sync_supported ? 1 : 0;
        head.adaptive_sync = adaptive_sync;
        head.width = cur_w;
        head.height = cur_h;
        head.rate = cur_rate;
        head.preferred = cur_pref;
        head.modes = g_array_new (FALSE, FALSE, sizeof (Mode));

        if (cur_w > 0 && cur_h > 0)
        {
            Mode m;

            m.width = cur_w;
            m.height = cur_h;
            m.rate = cur_rate;
            m.preferred = cur_pref;
            g_array_append_val (head.modes, m);
        }

        g_variant_iter_init (&modes_iter, modes_var);
        while (TRUE)
        {
            int w, h, rate;
            int pref;
            Mode mode;

            if (!g_variant_iter_next (&modes_iter, MODE_FORMAT,
                                      &w, &h, &rate, &pref))
                break;

            if (mode_is_present (&head, w, h, rate))
                continue;

            mode.width = w;
            mode.height = h;
            mode.rate = rate;
            mode.preferred = pref;
            g_array_append_val (head.modes, mode);
        }

        g_variant_unref (head_var);

        g_array_append_val (app->heads, head);
    }

    if (app->heads->len == 0)
    {
        g_array_free (app->heads, TRUE);
        app->heads = NULL;
    }
}

static gboolean
mode_is_present (OutputHead *head, int width, int height, int rate)
{
    guint i;

    for (i = 0; i < head->modes->len; i++)
    {
        Mode *mode = &g_array_index (head->modes, Mode, i);

        if (mode->width == width && mode->height == height && mode->rate == rate)
            return TRUE;
    }

    return FALSE;
}

static void
free_heads (App *app)
{
    guint i;

    if (!app->heads)
        return;

    for (i = 0; i < app->heads->len; i++)
    {
        OutputHead *head = get_nth_head (app, i);

        g_free (head->name);
        g_free (head->description);
        g_free (head->grab_info);
        if (head->modes)
            g_array_free (head->modes, TRUE);
    }

    g_array_free (app->heads, TRUE);
    app->heads = NULL;
}

static void
on_configuration_received (GObject *source_object, GAsyncResult *res, gpointer data)
{
    App *app = data;
    GError *error = NULL;
    g_autoptr (GVariant) result = NULL;
    g_autoptr (GVariant) configuration = NULL;

    result = g_dbus_proxy_call_finish (app->proxy, res, &error);
    if (result == NULL)
    {
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            error_message (app, _("Could not get display configuration"), error->message);
        g_error_free (error);
        return;
    }

    g_variant_get (result, "(@a" HEAD_INFO_FORMAT ")", &configuration);

    free_heads (app);
    app->current_head = NULL;

    parse_heads_from_variant (app, configuration);

    select_default_head (app);

    rebuild_gui (app);
    foo_scroll_area_invalidate (FOO_SCROLL_AREA (app->area));
}

static void
fetch_configuration (App *app)
{
    if (app->proxy == NULL)
        return;

    g_dbus_proxy_call (app->proxy,
                       "GetConfiguration",
                       NULL,
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       (GAsyncReadyCallback) on_configuration_received,
                       app);
}

static void
on_set_configuration_returned (GObject *source_object, GAsyncResult *res, gpointer data)
{
    App *app = data;
    GError *error = NULL;
    g_autoptr (GVariant) result = NULL;

    app->applying = FALSE;

    result = g_dbus_proxy_call_finish (app->proxy, res, &error);
    if (result == NULL)
    {
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            error_message (app, _("Could not apply the display configuration"), error->message);
        g_error_free (error);
    }

    gtk_widget_set_sensitive (app->dialog, TRUE);
}

static void
apply_configuration (App *app)
{
    GVariantBuilder builder;
    guint i;

    g_assert (app->proxy != NULL);
    g_assert (app->applying == FALSE);

    app->applying = TRUE;

    g_variant_builder_init (&builder, G_VARIANT_TYPE (HEAD_SET_TYPE));

    for (i = 0; i < app->heads->len; i++)
    {
        OutputHead *head = get_nth_head (app, i);
        int width, height;

        if (!head->enabled)
        {
            width = 0;
            height = 0;
        }
        else
        {
            get_geometry (head, &width, &height);
        }

        g_variant_builder_add (&builder, HEAD_SET_FORMAT,
                               head->name,
                               head->enabled,
                               width,
                               height,
                               head->rate,
                               head->x,
                               head->y,
                               head->scale,
                               head->transform,
                               head->adaptive_sync);
    }

    gtk_widget_set_sensitive (app->dialog, FALSE);

    g_dbus_proxy_call (app->proxy,
                       "SetConfiguration",
                       g_variant_new ("(@a" HEAD_SET_FORMAT ")", g_variant_builder_end (&builder)),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       (GAsyncReadyCallback) on_set_configuration_returned,
                       app);
}

static void
on_configuration_changed (GDBusConnection *connection,
                          const gchar     *sender_name,
                          const gchar     *object_path,
                          const gchar     *interface_name,
                          const gchar     *signal_name,
                          GVariant        *parameters,
                          gpointer         user_data)
{
    App *app = user_data;

    if (!app->applying)
    {
        fetch_configuration (app);
        return;
    }

    /* When we applied a configuration ourselves, fetch the new state once
     * the daemon reports it so that the edited values stay in sync with
     * what was actually applied.
     */
    fetch_configuration (app);
}

/* ------------------------------------------------------------------ *
 *  GUI rebuilding                                                    *
 * ------------------------------------------------------------------ */

static void
set_override_color (GtkWidget  *widget,
                    const char *style,
                    GdkRGBA    *rgba)
{
    gchar          *css;
    GtkCssProvider *provider;

    provider = gtk_css_provider_new ();

    css = g_strdup_printf ("* { %s: %s;}", style, gdk_rgba_to_string (rgba));
    gtk_css_provider_load_from_data (provider, css, -1, NULL);
    g_free (css);

    gtk_style_context_add_provider (gtk_widget_get_style_context (widget),
                                    GTK_STYLE_PROVIDER (provider),
                                    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref (provider);
}

static GdkRGBA
get_head_color (guint index)
{
    static const GdkRGBA palette[] = {
        { 0.38, 0.55, 0.79, 1.0 },
        { 0.55, 0.79, 0.38, 1.0 },
        { 0.79, 0.38, 0.55, 1.0 },
        { 0.79, 0.70, 0.38, 1.0 },
        { 0.38, 0.79, 0.70, 1.0 },
        { 0.70, 0.38, 0.79, 1.0 },
    };

    return palette[index % G_N_ELEMENTS (palette)];
}

static void
rebuild_current_monitor_label (App *app)
{
    char *str, *tmp;
    GdkRGBA color;
    gboolean use_color;

    if (app->current_head)
    {
        const char *name = app->current_head->description && app->current_head->description[0] ?
                           app->current_head->description : app->current_head->name;

        tmp = g_strdup_printf (_("Monitor: %s"), name);

        str = g_strdup_printf ("<b>%s</b>", tmp);
        use_color = TRUE;
        g_free (tmp);
    }
    else
    {
        str = g_strdup_printf ("<b>%s</b>", _("Monitor"));
        use_color = FALSE;
    }

    gtk_label_set_markup (GTK_LABEL (app->current_monitor_label), str);
    g_free (str);

    if (use_color)
    {
        guint i;
        color = get_head_color (0);

        for (i = 0; i < app->heads->len; i++)
        {
            if (get_nth_head (app, i) == app->current_head)
            {
                color = get_head_color (i);
                break;
            }
        }

        set_override_color (app->current_monitor_event_box, "background-color", &color);
        color.red = 1.0;
        color.green = 1.0;
        color.blue = 1.0;
        set_override_color (app->current_monitor_label, "color", &color);
    }

    gtk_event_box_set_visible_window (GTK_EVENT_BOX (app->current_monitor_event_box), use_color);
}

static void
rebuild_on_off_radios (App *app)
{
    gboolean sensitive;
    gboolean on_active;
    gboolean off_active;

    g_signal_handlers_block_by_func (app->monitor_on_radio, G_CALLBACK (monitor_on_off_toggled_cb), app);
    g_signal_handlers_block_by_func (app->monitor_off_radio, G_CALLBACK (monitor_on_off_toggled_cb), app);

    sensitive = FALSE;
    on_active = FALSE;
    off_active = FALSE;

    if (app->current_head)
    {
        if (count_active_outputs (app) > 1 || !app->current_head->enabled)
            sensitive = TRUE;
        else
            sensitive = FALSE;

        on_active = app->current_head->enabled;
        off_active = !on_active;
    }

    gtk_widget_set_sensitive (app->monitor_on_radio, sensitive);
    gtk_widget_set_sensitive (app->monitor_off_radio, sensitive);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (app->monitor_on_radio), on_active);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (app->monitor_off_radio), off_active);

    g_signal_handlers_unblock_by_func (app->monitor_on_radio, G_CALLBACK (monitor_on_off_toggled_cb), app);
    g_signal_handlers_unblock_by_func (app->monitor_off_radio, G_CALLBACK (monitor_on_off_toggled_cb), app);
}

static void
rebuild_resolution_combo (App *app)
{
    guint i;
    int width, height;

    clear_combo (app->resolution_combo);

    if (!app->current_head || !app->current_head->enabled)
    {
        gtk_widget_set_sensitive (app->resolution_combo, FALSE);
        return;
    }

    gtk_widget_set_sensitive (app->resolution_combo, TRUE);

    for (i = 0; i < app->current_head->modes->len; i++)
    {
        Mode *mode = &g_array_index (app->current_head->modes, Mode, i);

        add_key (app->resolution_combo,
                 make_resolution_string (mode->width, mode->height),
                 mode->width, mode->height, 0, -1);
    }

    get_geometry (app->current_head, &width, &height);

    if (!combo_select (app->resolution_combo, make_resolution_string (width, height)))
    {
        int best_w = 0, best_h = 0;

        for (i = 0; i < app->current_head->modes->len; i++)
        {
            Mode *mode = &g_array_index (app->current_head->modes, Mode, i);

            if (mode->width * mode->height > best_w * best_h)
            {
                best_w = mode->width;
                best_h = mode->height;
            }
        }

        combo_select (app->resolution_combo, make_resolution_string (best_w, best_h));
    }
}

static void
rebuild_rate_combo (App *app)
{
    int output_width, output_height;
    guint i;
    int best = -1;

    clear_combo (app->refresh_combo);

    if (!app->current_head || !app->current_head->enabled)
    {
        gtk_widget_set_sensitive (app->refresh_combo, FALSE);
        return;
    }

    gtk_widget_set_sensitive (app->refresh_combo, TRUE);

    get_geometry (app->current_head, &output_width, &output_height);

    for (i = 0; i < app->current_head->modes->len; i++)
    {
        Mode *mode = &g_array_index (app->current_head->modes, Mode, i);

        if (mode->width == output_width && mode->height == output_height)
        {
            add_key (app->refresh_combo,
                     make_rate_string (mode->rate),
                     0, 0, mode->rate, -1);

            if (mode->rate > best)
                best = mode->rate;
        }
    }

    if (!combo_select (app->refresh_combo, make_rate_string (app->current_head->rate)))
        combo_select (app->refresh_combo, make_rate_string (best));
}

static void
rebuild_rotation_combo (App *app)
{
    typedef struct
    {
        int       transform;
        const char *name;
    } RotationInfo;
    static const RotationInfo rotations[] = {
        { 0, N_("Normal") },
        { 1, N_("Left") },
        { 3, N_("Right") },
        { 2, N_("Upside Down") },
    };
    const char *selection;
    int current;
    unsigned int i;

    clear_combo (app->rotation_combo);

    gtk_widget_set_sensitive (app->rotation_combo,
                              app->current_head && app->current_head->enabled);

    if (!app->current_head)
        return;

    current = app->current_head->transform;

    selection = NULL;
    for (i = 0; i < G_N_ELEMENTS (rotations); ++i)
    {
        const RotationInfo *info = &(rotations[i]);

        add_key (app->rotation_combo, _(info->name), 0, 0, 0, info->transform);

        if (info->transform == current)
            selection = _(info->name);
    }

    if (!(selection && combo_select (app->rotation_combo, selection)))
        combo_select (app->rotation_combo, _("Normal"));
}

static void
rebuild_scale_combo (App *app)
{
    static const double scales[] = { 1.0, 1.25, 1.5, 1.75, 2.0 };
    guint i;

    clear_combo (app->scale_combo);

    gtk_widget_set_sensitive (app->scale_combo,
                              app->current_head && app->current_head->enabled);

    if (!app->current_head)
        return;

    for (i = 0; i < G_N_ELEMENTS (scales); ++i)
        add_key (app->scale_combo, g_strdup_printf ("%.2f", scales[i]), 0, 0, 0, -1);

    combo_select (app->scale_combo, g_strdup_printf ("%.2f", app->current_head->scale));
}

static void
rebuild_adaptive_sync_checkbox (App *app)
{
    gboolean sensitive;
    gboolean active;

    g_signal_handlers_block_by_func (app->adaptive_sync_checkbox,
                                     G_CALLBACK (on_adaptive_sync_toggled), app);

    sensitive = FALSE;
    active = FALSE;

    if (app->current_head && app->current_head->enabled)
    {
        if (app->current_head->adaptive_sync_supported == 1)
            sensitive = TRUE;

        active = app->current_head->adaptive_sync;
    }

    gtk_widget_set_sensitive (app->adaptive_sync_checkbox, sensitive);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (app->adaptive_sync_checkbox), active);

    g_signal_handlers_unblock_by_func (app->adaptive_sync_checkbox,
                                       G_CALLBACK (on_adaptive_sync_toggled), app);
}

static void
rebuild_gui (App *app)
{
    g_assert (app->ignore_gui_changes == FALSE);

    app->ignore_gui_changes = TRUE;

    rebuild_current_monitor_label (app);
    rebuild_on_off_radios (app);
    rebuild_resolution_combo (app);
    rebuild_rate_combo (app);
    rebuild_rotation_combo (app);
    rebuild_scale_combo (app);
    rebuild_adaptive_sync_checkbox (app);

    /* Features not supported by the wlrandr backend: panel inclusion,
     * primary output and system-wide installation.
     */
    gtk_widget_set_sensitive (app->panel_checkbox, FALSE);
    gtk_widget_set_sensitive (app->primary_button, FALSE);
    gtk_widget_set_sensitive (app->make_default_button, FALSE);

    app->ignore_gui_changes = FALSE;
}

/* ------------------------------------------------------------------ *
 *  Widget callbacks                                                  *
 * ------------------------------------------------------------------ */

static void
select_resolution_for_current_output (App *app)
{
    int width, height;
    guint i;
    int best_w = 0, best_h = 0;

    head_get_preferred_size (app->current_head, &width, &height);

    if (width != 0 && height != 0)
    {
        app->current_head->width = width;
        app->current_head->height = height;
        return;
    }

    for (i = 0; i < app->current_head->modes->len; i++)
    {
        Mode *mode = &g_array_index (app->current_head->modes, Mode, i);

        if (mode->width * mode->height > best_w * best_h)
        {
            best_w = mode->width;
            best_h = mode->height;
        }
    }

    app->current_head->width = best_w;
    app->current_head->height = best_h;
}

static void
monitor_on_off_toggled_cb (GtkToggleButton *toggle, gpointer data)
{
    App *app = data;
    gboolean is_on;

    if (!app->current_head)
        return;

    if (!gtk_toggle_button_get_active (toggle))
        return;

    if (GTK_WIDGET (toggle) == app->monitor_on_radio)
        is_on = TRUE;
    else if (GTK_WIDGET (toggle) == app->monitor_off_radio)
        is_on = FALSE;
    else
    {
        g_assert_not_reached ();
        return;
    }

    app->current_head->enabled = is_on;

    if (is_on)
        select_resolution_for_current_output (app); /* The refresh rate will be picked in rebuild_rate_combo() */

    rebuild_gui (app);
    foo_scroll_area_invalidate (FOO_SCROLL_AREA (app->area));
}

static void
realign_outputs_after_resolution_change (App *app, OutputHead *head_that_changed, int old_width, int old_height)
{
    int i;
    int old_right_edge, old_bottom_edge;
    int dx, dy;
    int x, y, width, height;

    g_assert (app->heads != NULL);

    x = head_that_changed->x;
    y = head_that_changed->y;
    get_geometry (head_that_changed, &width, &height);
    if (width == old_width && height == old_height)
        return;

    old_right_edge = x + old_width;
    old_bottom_edge = y + old_height;

    dx = width - old_width;
    dy = height - old_height;

    for (i = 0; i < (int) app->heads->len; i++)
    {
        OutputHead *head = get_nth_head (app, i);
        int output_x, output_y;
        int output_width, output_height;

        if (head == head_that_changed)
            continue;

        output_x = head->x;
        output_y = head->y;
        get_geometry (head, &output_width, &output_height);

        if (output_x >= old_right_edge)
            output_x += dx;
        else if (output_x + output_width == old_right_edge)
            output_x = x + width - output_width;

        if (output_y >= old_bottom_edge)
            output_y += dy;
        else if (output_y + output_height == old_bottom_edge)
            output_y = y + height - output_height;

        head->x = output_x;
        head->y = output_y;
    }
}

static void
on_resolution_changed (GtkComboBox *box, gpointer data)
{
    App *app = data;
    int old_width, old_height;
    int width;
    int height;

    if (!app->current_head)
        return;

    get_geometry (app->current_head, &old_width, &old_height);

    if (get_mode (app->resolution_combo, &width, &height, NULL, NULL))
    {
        app->current_head->width = width;
        app->current_head->height = height;

        if (width == 0 || height == 0)
            app->current_head->enabled = FALSE;
        else
            app->current_head->enabled = TRUE;
    }

    realign_outputs_after_resolution_change (app, app->current_head, old_width, old_height);

    rebuild_rate_combo (app);

    foo_scroll_area_invalidate (FOO_SCROLL_AREA (app->area));
}

static void
on_rate_changed (GtkComboBox *box, gpointer data)
{
    App *app = data;
    int rate;

    if (!app->current_head)
        return;

    if (get_mode (app->refresh_combo, NULL, NULL, &rate, NULL))
        app->current_head->rate = rate;

    foo_scroll_area_invalidate (FOO_SCROLL_AREA (app->area));
}

static void
on_rotation_changed (GtkComboBox *box, gpointer data)
{
    App *app = data;
    int transform;

    if (!app->current_head)
        return;

    if (get_mode (app->rotation_combo, NULL, NULL, NULL, &transform))
        head_set_transform (app->current_head, transform);

    foo_scroll_area_invalidate (FOO_SCROLL_AREA (app->area));
}

static void
on_scale_changed (GtkComboBox *box, gpointer data)
{
    App *app = data;
    GtkTreeIter iter;
    GtkTreeModel *model;
    gchar *text = NULL;

    if (!app->current_head)
        return;

    if (!gtk_combo_box_get_active_iter (box, &iter))
        return;

    model = gtk_combo_box_get_model (box);
    gtk_tree_model_get (model, &iter, 0, &text, -1);

    if (text)
    {
        app->current_head->scale = g_ascii_strtod (text, NULL);
        g_free (text);
    }

    foo_scroll_area_invalidate (FOO_SCROLL_AREA (app->area));
}

static void
on_detect_displays (GtkWidget *widget, gpointer data)
{
    App *app = data;

    /* Wayland outputs are hotplugged automatically; just refetch the
     * current state from the daemon.
     */
    fetch_configuration (app);
}

static void
set_primary (GtkWidget *widget, gpointer data)
{
    /* The wlr-output-management protocol has no primary output concept. */
}

static void
on_show_icon_toggled (GtkWidget *widget, gpointer data)
{
    GtkToggleButton *tb = GTK_TOGGLE_BUTTON (widget);
    App *app = data;

    g_settings_set_boolean (app->settings, SHOW_ICON_KEY,
                            gtk_toggle_button_get_active (tb));
}

static void
on_adaptive_sync_toggled (GtkToggleButton *toggle, gpointer data)
{
    App *app = data;

    if (app->ignore_gui_changes)
        return;

    if (!app->current_head)
        return;

    app->current_head->adaptive_sync = gtk_toggle_button_get_active (toggle);
}

/* ------------------------------------------------------------------ *
 *  Scroll area                                                       *
 * ------------------------------------------------------------------ */

#define SPACE 15
#define MARGIN 15

static PangoLayout *
get_display_name (App *app, OutputHead *head)
{
    char *text;
    PangoLayout *layout;

    text = g_strdup_printf ("<b>%s</b>\n<small>%s</small>",
                            head->description && head->description[0] ? head->description : head->name,
                            head->name);
    layout = gtk_widget_create_pango_layout (GTK_WIDGET (app->area), text);
    pango_layout_set_markup (layout, text, -1);
    g_free (text);
    pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
    return layout;
}

static void
layout_set_font (PangoLayout *layout, const char *font)
{
    PangoFontDescription *desc =
        pango_font_description_from_string (font);

    if (desc)
    {
        pango_layout_set_font_description (layout, desc);
        pango_font_description_free (desc);
    }
}

static void
set_cursor (GtkWidget *widget, GdkCursorType type)
{
    GdkCursor *cursor;
    GdkWindow *window;

    if (type == GDK_BLANK_CURSOR)
        cursor = NULL;
    else
        cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget), type);

    window = gtk_widget_get_window (widget);

    if (window)
        gdk_window_set_cursor (window, cursor);

    if (cursor)
        g_object_unref (cursor);
}

static void
set_monitors_tooltip (App *app, gboolean is_dragging)
{
    const char *text;

    if (is_dragging)
        text = NULL;
    else
        text = _("Select a monitor to change its properties; drag it to rearrange its placement.");

    gtk_widget_set_tooltip_text (app->area, text);
}

typedef struct Edge
{
    OutputHead *head;
    int x1, y1;
    int x2, y2;
} Edge;

typedef struct Snap
{
    Edge *snapper;              /* Edge that should be snapped */
    Edge *snappee;
    int dy, dx;
} Snap;

static void
add_edge (OutputHead *head, int x1, int y1, int x2, int y2, GArray *edges)
{
    Edge e;

    e.x1 = x1;
    e.x2 = x2;
    e.y1 = y1;
    e.y2 = y2;
    e.head = head;

    g_array_append_val (edges, e);
}

static void
list_edges_for_output (OutputHead *head, GArray *edges)
{
    int x, y, w, h;

    x = head->x;
    y = head->y;
    get_geometry (head, &w, &h);

    /* Top, Bottom, Left, Right */
    add_edge (head, x, y, x + w, y, edges);
    add_edge (head, x, y + h, x + w, y + h, edges);
    add_edge (head, x, y, x, y + h, edges);
    add_edge (head, x + w, y, x + w, y + h, edges);
}

static void
list_edges (App *app, GArray *edges)
{
    guint i;

    for (i = 0; i < app->heads->len; i++)
        list_edges_for_output (get_nth_head (app, i), edges);
}

static gboolean
overlap (int s1, int e1, int s2, int e2)
{
    return (!(e1 < s2 || s1 >= e2));
}

static gboolean
horizontal_overlap (Edge *snapper, Edge *snappee)
{
    if (snapper->y1 != snapper->y2 || snappee->y1 != snappee->y2)
        return FALSE;

    return overlap (snapper->x1, snapper->x2, snappee->x1, snappee->x2);
}

static gboolean
vertical_overlap (Edge *snapper, Edge *snappee)
{
    if (snapper->x1 != snapper->x2 || snappee->x1 != snappee->x2)
        return FALSE;

    return overlap (snapper->y1, snapper->y2, snappee->y1, snappee->y2);
}

static void
add_snap (GArray *snaps, Snap snap)
{
    if (ABS (snap.dx) <= 200 || ABS (snap.dy) <= 200)
        g_array_append_val (snaps, snap);
}

static void
add_edge_snaps (Edge *snapper, Edge *snappee, GArray *snaps)
{
    Snap snap;

    snap.snapper = snapper;
    snap.snappee = snappee;

    if (horizontal_overlap (snapper, snappee))
    {
        snap.dx = 0;
        snap.dy = snappee->y1 - snapper->y1;

        add_snap (snaps, snap);
    }
    else if (vertical_overlap (snapper, snappee))
    {
        snap.dy = 0;
        snap.dx = snappee->x1 - snapper->x1;

        add_snap (snaps, snap);
    }

    /* Corner snaps */
    /* 1->1 */
    snap.dx = snappee->x1 - snapper->x1;
    snap.dy = snappee->y1 - snapper->y1;

    add_snap (snaps, snap);

    /* 1->2 */
    snap.dx = snappee->x2 - snapper->x1;
    snap.dy = snappee->y2 - snapper->y1;

    add_snap (snaps, snap);

    /* 2->2 */
    snap.dx = snappee->x2 - snapper->x2;
    snap.dy = snappee->y2 - snapper->y2;

    add_snap (snaps, snap);

    /* 2->1 */
    snap.dx = snappee->x1 - snapper->x2;
    snap.dy = snappee->y1 - snapper->y2;

    add_snap (snaps, snap);
}

static void
list_snaps (OutputHead *head, GArray *edges, GArray *snaps)
{
    guint i;

    for (i = 0; i < edges->len; ++i)
    {
        Edge *output_edge = &(g_array_index (edges, Edge, i));

        if (output_edge->head == head)
        {
            guint j;

            for (j = 0; j < edges->len; ++j)
            {
                Edge *edge = &(g_array_index (edges, Edge, j));

                if (edge->head != head)
                    add_edge_snaps (output_edge, edge, snaps);
            }
        }
    }
}

static gboolean
is_corner_snap (const Snap *s)
{
    return s->dx != 0 && s->dy != 0;
}

static int
compare_snaps (gconstpointer v1, gconstpointer v2)
{
    const Snap *s1 = v1;
    const Snap *s2 = v2;
    int sv1 = MAX (ABS (s1->dx), ABS (s1->dy));
    int sv2 = MAX (ABS (s2->dx), ABS (s2->dy));
    int d;

    d = sv1 - sv2;

    if (d == 0)
    {
        if (is_corner_snap (s1) && !is_corner_snap (s2))
            return -1;
        else if (is_corner_snap (s2) && !is_corner_snap (s1))
            return 1;
        else
            return 0;
    }
    else
    {
        return d;
    }
}

static gboolean
corner_on_edge (int x, int y, Edge *e)
{
    if (x == e->x1 && x == e->x2 && y >= e->y1 && y <= e->y2)
        return TRUE;

    if (y == e->y1 && y == e->y2 && x >= e->x1 && x <= e->x2)
        return TRUE;

    return FALSE;
}

static gboolean
edges_align (Edge *e1, Edge *e2)
{
    if (corner_on_edge (e1->x1, e1->y1, e2))
        return TRUE;

    if (corner_on_edge (e2->x1, e2->y1, e1))
        return TRUE;

    return FALSE;
}

static gboolean
head_is_aligned (OutputHead *head, GArray *edges)
{
    gboolean result = FALSE;
    guint i;

    for (i = 0; i < edges->len; ++i)
    {
        Edge *output_edge = &(g_array_index (edges, Edge, i));

        if (output_edge->head == head)
        {
            guint j;

            for (j = 0; j < edges->len; ++j)
            {
                Edge *edge = &(g_array_index (edges, Edge, j));

                if ((edge->head != output_edge->head) &&
                    edges_align (output_edge, edge))
                {
                    result = TRUE;
                    goto done;
                }
            }
        }
    }
done:

    return result;
}

static gboolean
config_is_aligned (App *app, GArray *edges)
{
    guint i;
    gboolean result = TRUE;

    for (i = 0; i < app->heads->len; i++)
    {
        if (!head_is_aligned (get_nth_head (app, i), edges))
            return FALSE;
    }

    return result;
}

static void
on_output_event (FooScrollArea      *area,
                 FooScrollAreaEvent *event,
                 gpointer            data)
{
    OutputHead *head = data;
    App *app = g_object_get_data (G_OBJECT (area), "app");

    if (app->heads && app->heads->len > 1)
        set_cursor (GTK_WIDGET (area), GDK_FLEUR);

    if (event->type == FOO_BUTTON_PRESS)
    {
        GrabInfo *info;

        app->current_head = head;

        rebuild_gui (app);
        set_monitors_tooltip (app, TRUE);

        if (app->heads && app->heads->len > 1)
        {
            foo_scroll_area_begin_grab (area, on_output_event, data);

            info = g_new0 (GrabInfo, 1);
            info->grab_x = event->x;
            info->grab_y = event->y;
            info->output_x = head->x;
            info->output_y = head->y;

            head->grab_info = info;
        }

        foo_scroll_area_invalidate (area);
    }
    else
    {
        if (foo_scroll_area_is_grabbed (area))
        {
            GrabInfo *info = head->grab_info;
            double scale = compute_scale (app);
            int width, height;
            int new_x, new_y;
            guint i;
            GArray *edges, *snaps;

            get_geometry (head, &width, &height);
            new_x = info->output_x + (int) ((double)(event->x - info->grab_x) / scale);
            new_y = info->output_y + (int) ((double)(event->y - info->grab_y) / scale);

            head->x = new_x;
            head->y = new_y;

            edges = g_array_new (TRUE, TRUE, sizeof (Edge));
            snaps = g_array_new (TRUE, TRUE, sizeof (Snap));

            list_edges (app, edges);
            list_snaps (head, edges, snaps);

            g_array_sort (snaps, compare_snaps);

            head->x = new_x;
            head->y = new_y;

            for (i = 0; i < snaps->len; ++i)
            {
                Snap *snap = &(g_array_index (snaps, Snap, i));

                head->x = new_x + snap->dx;
                head->y = new_y + snap->dy;

                if (config_is_aligned (app, edges))
                    break;
                else
                {
                    head->x = info->output_x;
                    head->y = info->output_y;
                }
            }

            g_array_free (snaps, TRUE);
            g_array_free (edges, TRUE);

            if (event->type == FOO_BUTTON_RELEASE)
            {
                foo_scroll_area_end_grab (area);
                set_monitors_tooltip (app, FALSE);

                g_free (head->grab_info);
                head->grab_info = NULL;
            }

            foo_scroll_area_invalidate (area);
        }
    }
}

static void
on_canvas_event (FooScrollArea *area,
                 FooScrollAreaEvent *event,
                 gpointer data)
{
    set_cursor (GTK_WIDGET (area), GDK_BLANK_CURSOR);
}

static void
paint_background (FooScrollArea *area,
                  cairo_t       *cr)
{
    GdkRectangle viewport;
    GtkWidget *widget;
    GtkStyleContext *widget_style;
    GdkRGBA *base_color = NULL;
    GdkRGBA dark_color;

    widget = GTK_WIDGET (area);

    foo_scroll_area_get_viewport (area, &viewport);

    widget_style = gtk_widget_get_style_context (widget);

    gtk_style_context_save (widget_style);
    gtk_style_context_set_state (widget_style, GTK_STATE_FLAG_SELECTED);
    gtk_style_context_get (widget_style,
                           gtk_style_context_get_state (widget_style),
                           GTK_STYLE_PROPERTY_BACKGROUND_COLOR, &base_color,
                           NULL);
    gtk_style_context_restore (widget_style);
    gdk_cairo_set_source_rgba (cr, base_color);
    gdk_rgba_free (base_color);

    cairo_rectangle (cr,
                     viewport.x, viewport.y,
                     viewport.width, viewport.height);

    cairo_fill_preserve (cr);

    foo_scroll_area_add_input_from_fill (area, cr, on_canvas_event, NULL);

    gtk_style_context_save (widget_style);
    gtk_style_context_set_state (widget_style, GTK_STATE_FLAG_SELECTED);
    gtk_style_context_get (widget_style,
                           gtk_style_context_get_state (widget_style),
                           GTK_STYLE_PROPERTY_COLOR, &dark_color,
                           NULL);
    gtk_style_context_restore (widget_style);
    gdk_cairo_set_source_rgba (cr, &dark_color);

    cairo_stroke (cr);
}

static void
paint_head (App     *app,
            cairo_t *cr,
            guint    i)
{
    int w, h;
    double scale = compute_scale (app);
    double x, y;
    int total_w, total_h;
    GList *heads = list_heads (app, &total_w, &total_h);
    OutputHead *head = g_list_nth_data (heads, i);
    PangoLayout *layout = get_display_name (app, head);
    PangoRectangle ink_extent, log_extent;
    GdkRectangle viewport;
    GdkRGBA output_color;
    double r, g, b;
    double available_w;
    double factor;

    cairo_save (cr);

    foo_scroll_area_get_viewport (FOO_SCROLL_AREA (app->area), &viewport);

    get_geometry (head, &w, &h);

    viewport.height -= 2 * MARGIN;
    viewport.width -= 2 * MARGIN;

    x = head->x * scale + MARGIN + (viewport.width - total_w * scale) / 2.0;
    y = head->y * scale + MARGIN + (viewport.height - total_h * scale) / 2.0;

    cairo_translate (cr,
                     x + (w * scale + 0.5) / 2,
                     y + (h * scale + 0.5) / 2);

    cairo_translate (cr,
                     - x - (w * scale + 0.5) / 2,
                     - y - (h * scale + 0.5) / 2);

    cairo_rectangle (cr, x, y, w * scale + 0.5, h * scale + 0.5);
    cairo_clip_preserve (cr);

    output_color = get_head_color (i);
    r = output_color.red;
    g = output_color.green;
    b = output_color.blue;

    if (!head->enabled)
    {
        /* If the output is turned off, just darken the selected color */
        r *= 0.2;
        g *= 0.2;
        b *= 0.2;
    }

    cairo_set_source_rgba (cr, r, g, b, 1.0);

    foo_scroll_area_add_input_from_fill (FOO_SCROLL_AREA (app->area),
                                         cr, on_output_event, head);
    cairo_fill (cr);

    if (head == app->current_head)
    {
        cairo_rectangle (cr, x + 2, y + 2, w * scale + 0.5 - 4, h * scale + 0.5 - 4);

        cairo_set_line_width (cr, 4);
        cairo_set_source_rgba (cr, 0.33, 0.43, 0.57, 1.0);
        cairo_stroke (cr);
    }

    cairo_rectangle (cr, x + 0.5, y + 0.5, w * scale + 0.5 - 1, h * scale + 0.5 - 1);

    cairo_set_line_width (cr, 1);
    cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);

    cairo_stroke (cr);
    cairo_set_line_width (cr, 2);

    layout_set_font (layout, "Sans 12");
    pango_layout_get_pixel_extents (layout, &ink_extent, &log_extent);

    available_w = w * scale + 0.5 - 6; /* Same as the inner rectangle's width, minus 1 pixel of padding on each side */
    if (available_w < ink_extent.width)
        factor = available_w / ink_extent.width;
    else
        factor = 1.0;

    cairo_move_to (cr,
                   x + ((w * scale + 0.5) - factor * log_extent.width) / 2,
                   y + ((h * scale + 0.5) - factor * log_extent.height) / 2);

    cairo_scale (cr, factor, factor);

    if (head->enabled)
        cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
    else
        cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);

    pango_cairo_show_layout (cr, layout);

    cairo_restore (cr);

    g_object_unref (layout);

    g_list_free (heads);
}

static void
on_area_paint (FooScrollArea *area,
               cairo_t       *cr,
               gpointer      data)
{
    App *app = data;
    GList *heads;
    GList *list;

    paint_background (area, cr);

    if (!app->heads)
        return;

    heads = list_heads (app, NULL, NULL);

    for (list = heads; list != NULL; list = list->next)
    {
        int pos;

        if ((pos = g_list_position (heads, list)) != -1)
            paint_head (app, cr, (guint) pos);
    }

    g_list_free (heads);
}

static void
on_viewport_changed (FooScrollArea *scroll_area,
                     GdkRectangle  *old_viewport,
                     GdkRectangle  *new_viewport)
{
    foo_scroll_area_set_size (scroll_area,
                              new_viewport->width,
                              new_viewport->height);

    foo_scroll_area_invalidate (scroll_area);
}

/* ------------------------------------------------------------------ *
 *  Application                                                       *
 * ------------------------------------------------------------------ */

static GtkWidget*
_gtk_builder_get_widget (GtkBuilder *builder, const gchar *name)
{
    return GTK_WIDGET (gtk_builder_get_object (builder, name));
}

static void
select_default_head (App *app)
{
    guint i;

    if (app->current_head)
        return;

    for (i = 0; i < app->heads->len; i++)
    {
        if (get_nth_head (app, i)->enabled)
        {
            app->current_head = get_nth_head (app, i);
            break;
        }
    }
}

static void
on_dialog_map (GtkWidget *widget, gpointer data)
{
    App *app = data;

    /* Wayland has no notion of the monitor showing the dialog, so just
     * select the first enabled head.
     */
    if (!app->current_head && app->heads)
    {
        select_default_head (app);

        rebuild_gui (app);
        foo_scroll_area_invalidate (FOO_SCROLL_AREA (app->area));
    }
}

static void
run_application (App *app)
{
    GtkBuilder *builder;
    GtkWidget *align;
    GError *error = NULL;

    builder = gtk_builder_new_from_resource ("/org/mate/mcc/display/display-capplet.ui");

    app->settings = g_settings_new (MSD_WLRANDR_SCHEMA);

    app->dialog = _gtk_builder_get_widget (builder, "dialog");
    g_signal_connect (app->dialog, "map",
                      G_CALLBACK (on_dialog_map), app);

    gtk_window_set_default_icon_name ("preferences-desktop-display");
    gtk_window_set_icon_name (GTK_WINDOW (app->dialog),
                              "preferences-desktop-display");

    app->current_monitor_event_box = _gtk_builder_get_widget (builder,
                                                              "current_monitor_event_box");
    app->current_monitor_label = _gtk_builder_get_widget (builder,
                                                          "current_monitor_label");

    app->monitor_on_radio = _gtk_builder_get_widget (builder,
                                                     "monitor_on_radio");
    app->monitor_off_radio = _gtk_builder_get_widget (builder,
                                                      "monitor_off_radio");

    g_signal_connect (app->monitor_on_radio, "toggled",
                      G_CALLBACK (monitor_on_off_toggled_cb), app);
    g_signal_connect (app->monitor_off_radio, "toggled",
                      G_CALLBACK (monitor_on_off_toggled_cb), app);

    app->resolution_combo = _gtk_builder_get_widget (builder,
                                                     "resolution_combo");
    g_signal_connect (app->resolution_combo, "changed",
                      G_CALLBACK (on_resolution_changed), app);

    app->refresh_combo = _gtk_builder_get_widget (builder, "refresh_combo");
    g_signal_connect (app->refresh_combo, "changed",
                      G_CALLBACK (on_rate_changed), app);

    app->rotation_combo = _gtk_builder_get_widget (builder, "rotation_combo");
    g_signal_connect (app->rotation_combo, "changed",
                      G_CALLBACK (on_rotation_changed), app);

    app->scale_vbox = _gtk_builder_get_widget (builder, "scale_vbox");

    /* Per-output scaling (a feature of the wlr-output-management protocol) */
    app->scale_bbox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_container_set_border_width (GTK_CONTAINER (app->scale_bbox), 6);
    gtk_container_add (GTK_CONTAINER (app->scale_vbox), app->scale_bbox);

    app->scale_combo = gtk_combo_box_new ();
    make_text_combo (app->scale_combo, -1);
    g_signal_connect (app->scale_combo, "changed",
                      G_CALLBACK (on_scale_changed), app);
    gtk_container_add (GTK_CONTAINER (app->scale_bbox), app->scale_combo);
    gtk_widget_show (app->scale_combo);
    gtk_widget_show (app->scale_bbox);

    /* Adaptive sync (variable refresh rate) is a wlr-output-management
     * feature; this widget is only ever created from the Wayland capplet.
     */
    app->adaptive_sync_checkbox = gtk_check_button_new_with_mnemonic (_("Adaptive _sync"));
    gtk_widget_set_tooltip_text (app->adaptive_sync_checkbox,
                                 _("Dynamically adjust the refresh rate to match the content"));
    gtk_container_add (GTK_CONTAINER (app->scale_vbox), app->adaptive_sync_checkbox);
    gtk_widget_show (app->adaptive_sync_checkbox);
    g_signal_connect (app->adaptive_sync_checkbox, "toggled",
                      G_CALLBACK (on_adaptive_sync_toggled), app);

    app->detect_displays_button = _gtk_builder_get_widget (builder, "detect_displays_button");
    g_signal_connect (app->detect_displays_button, "clicked",
                      G_CALLBACK (on_detect_displays), app);

    /* Mirroring is not supported by the wlr-output-management protocol. */
    gtk_widget_hide (_gtk_builder_get_widget (builder, "clone_checkbox"));

    app->primary_button = _gtk_builder_get_widget (builder, "primary_button");
    g_signal_connect (app->primary_button, "clicked", G_CALLBACK (set_primary), app);

    app->show_icon_checkbox = _gtk_builder_get_widget (builder,
                                                       "show_notification_icon");

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (app->show_icon_checkbox),
                                  g_settings_get_boolean (app->settings, SHOW_ICON_KEY));

    g_signal_connect (app->show_icon_checkbox, "toggled", G_CALLBACK (on_show_icon_toggled), app);

    app->panel_checkbox = _gtk_builder_get_widget (builder, "panel_checkbox");

    app->make_default_button = _gtk_builder_get_widget (builder, "make_default_button");

    make_text_combo (app->resolution_combo, 4);
    make_text_combo (app->refresh_combo, 3);
    make_text_combo (app->rotation_combo, -1);

    /* Scroll Area */
    app->area = (GtkWidget *) foo_scroll_area_new ();

    g_object_set_data (G_OBJECT (app->area), "app", app);

    set_monitors_tooltip (app, FALSE);

    /* FIXME: this should be computed dynamically */
    foo_scroll_area_set_min_size (FOO_SCROLL_AREA (app->area), -1, 200);
    gtk_widget_show (app->area);
    g_signal_connect (app->area, "paint",
                      G_CALLBACK (on_area_paint), app);
    g_signal_connect (app->area, "viewport_changed",
                      G_CALLBACK (on_viewport_changed), app);

    align = _gtk_builder_get_widget (builder, "align");

    gtk_container_add (GTK_CONTAINER (align), app->area);

    g_object_unref (builder);

    app->connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
    if (app->connection == NULL)
    {
        error_message (app, _("Could not get session bus"), error->message);
        g_error_free (error);
        gtk_widget_destroy (app->dialog);
        app->dialog = NULL;
        return;
    }

    app->proxy = g_dbus_proxy_new_sync (app->connection,
                                        G_DBUS_PROXY_FLAGS_NONE,
                                        NULL,
                                        "org.mate.SettingsDaemon",
                                        MSD_WLRANDR_DBUS_PATH,
                                        MSD_WLRANDR_DBUS_NAME,
                                        NULL,
                                        &error);
    if (app->proxy == NULL)
    {
        error_message (app, _("Could not connect to the display settings daemon"),
                       _("The wlrandr plugin of mate-settings-daemon does not seem to be active."));
        g_error_free (error);
        gtk_widget_destroy (app->dialog);
        app->dialog = NULL;
        return;
    }

    app->signal_id = g_dbus_connection_signal_subscribe (app->connection,
                                                         NULL,
                                                         MSD_WLRANDR_DBUS_NAME,
                                                         "ConfigurationChanged",
                                                         MSD_WLRANDR_DBUS_PATH,
                                                         NULL,
                                                         G_DBUS_SIGNAL_FLAGS_NONE,
                                                         on_configuration_changed,
                                                         app,
                                                         NULL);

    fetch_configuration (app);

restart:
    switch (gtk_dialog_run (GTK_DIALOG (app->dialog)))
    {
    default:
        /* Fall Through */
    case GTK_RESPONSE_DELETE_EVENT:
    case GTK_RESPONSE_CLOSE:
        break;

    case GTK_RESPONSE_HELP:
        gtk_show_uri_on_window (GTK_WINDOW (app->dialog),
                                "help:mate-user-guide/goscustdesk-70",
                                gtk_get_current_event_time (),
                                &error);
        if (error)
        {
            error_message (app, _("Could not open help content"), error->message);
            g_error_free (error);
        }
        goto restart;
        break;

    case GTK_RESPONSE_APPLY:
        apply_configuration (app);
        goto restart;
        break;

    case RESPONSE_MAKE_DEFAULT:
        goto restart;
        break;
    }
}

/* Entry point used by mate-display-properties' main() when the session
 * runs under Wayland.  GTK is initialized by main() before this is called.
 */
int
wlrandr_capplet_main (int argc, char **argv)
{
    App *app;

    app = g_new0 (App, 1);

    run_application (app);

    if (app->signal_id != 0)
        g_dbus_connection_signal_unsubscribe (app->connection, app->signal_id);

    if (app->proxy)
        g_object_unref (app->proxy);
    if (app->connection)
        g_object_unref (app->connection);

    free_heads (app);

    if (app->dialog)
        gtk_widget_destroy (app->dialog);

    g_object_unref (app->settings);

    g_free (app);

    return 0;
}
