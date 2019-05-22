/*
 * This file is part of libtile.
 *
 * Copyright (c) 2006 Novell, Inc.
 *
 * Libtile is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Libtile is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libslab; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "tile.h"

#include <gdk/gdkkeysyms.h>
#include <string.h>

#include "double-click-detector.h"

typedef struct
{
	DoubleClickDetector *double_click_detector;

	gboolean is_dragging;
} TilePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (Tile, tile, GTK_TYPE_BUTTON)

static void tile_finalize (GObject *);
static void tile_dispose (GObject *);
static void tile_get_property (GObject *, guint, GValue *, GParamSpec *);
static void tile_set_property (GObject *, guint, const GValue *, GParamSpec *);
static GObject *tile_constructor (GType, guint, GObjectConstructParam *);

static void tile_setup (Tile *);

static void tile_enter (GtkButton * widget);
static void tile_leave (GtkButton * widget);
static void tile_clicked (GtkButton *widget);

static gboolean tile_focus_in (GtkWidget *, GdkEventFocus *);
static gboolean tile_focus_out (GtkWidget *, GdkEventFocus *);
static gboolean tile_draw (GtkWidget *, cairo_t *);
static gboolean tile_button_release (GtkWidget *, GdkEventButton *);
static gboolean tile_key_release (GtkWidget *, GdkEventKey *);
static gboolean tile_popup_menu (GtkWidget *);

static void tile_drag_begin (GtkWidget *, GdkDragContext *);
static void tile_drag_data_get (GtkWidget *, GdkDragContext *, GtkSelectionData *, guint,
guint);

static void tile_tile_action_triggered (Tile *, TileEvent *, TileAction *);
static void tile_action_triggered_event_marshal (GClosure *, GValue *, guint, const GValue *,
gpointer, gpointer);

typedef void (*marshal_func_VOID__POINTER_POINTER) (gpointer, gpointer, gpointer, gpointer);

enum
{
	TILE_ACTIVATED_SIGNAL,
	TILE_ACTION_TRIGGERED_SIGNAL,
	LAST_SIGNAL
};

static guint tile_signals[LAST_SIGNAL] = { 0 };

enum
{
	PROP_0,
	PROP_TILE_URI,
	PROP_TILE_CONTEXT_MENU,
	PROP_TILE_ACTIONS
};

static void
tile_class_init (TileClass * this_class)
{
	GObjectClass *g_obj_class = G_OBJECT_CLASS (this_class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (this_class);
	GtkButtonClass *button_class = GTK_BUTTON_CLASS (this_class);

	g_obj_class->constructor = tile_constructor;
	g_obj_class->get_property = tile_get_property;
	g_obj_class->set_property = tile_set_property;
	g_obj_class->finalize = tile_finalize;
	g_obj_class->dispose = tile_dispose;

	widget_class->focus_in_event = tile_focus_in;
	widget_class->focus_out_event = tile_focus_out;
	widget_class->draw = tile_draw;
	widget_class->button_release_event = tile_button_release;
	widget_class->key_release_event = tile_key_release;
	widget_class->drag_begin = tile_drag_begin;
	widget_class->drag_data_get = tile_drag_data_get;
	widget_class->popup_menu = tile_popup_menu;

	button_class->enter = tile_enter;
	button_class->leave = tile_leave;
	button_class->clicked = tile_clicked;

	this_class->tile_activated = NULL;
	this_class->tile_action_triggered = tile_tile_action_triggered;

	g_object_class_install_property (g_obj_class, PROP_TILE_URI,
		g_param_spec_string ("tile-uri", "tile-uri", "the uri of the tile", NULL,
			G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (g_obj_class, PROP_TILE_CONTEXT_MENU,
		g_param_spec_object ("context-menu", "context-menu",
			"the context menu for the tile", GTK_TYPE_MENU, G_PARAM_READWRITE));

	tile_signals[TILE_ACTIVATED_SIGNAL] = g_signal_new ("tile-activated",
		G_TYPE_FROM_CLASS (this_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET (TileClass, tile_activated),
		NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);

	tile_signals[TILE_ACTION_TRIGGERED_SIGNAL] = g_signal_new ("tile-action-triggered",
		G_TYPE_FROM_CLASS (this_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET (TileClass, tile_action_triggered),
		NULL, NULL, tile_action_triggered_event_marshal, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);
}

static GObject *
tile_constructor (GType type, guint n_param, GObjectConstructParam * param)
{
	GObject *g_obj;
	TilePrivate *priv;

	g_obj = (*G_OBJECT_CLASS (tile_parent_class)->constructor) (type, n_param, param);

	priv = tile_get_instance_private (TILE(g_obj));
	priv->double_click_detector = double_click_detector_new ();

	tile_setup (TILE (g_obj));

	return g_obj;
}

static void
tile_init (Tile * tile)
{
	TilePrivate *priv = tile_get_instance_private (tile);

	tile->uri = NULL;
	tile->context_menu = NULL;
	tile->entered = FALSE;
	tile->enabled = TRUE;

	tile->actions = NULL;
	tile->n_actions = 0;

	tile->default_action = NULL;

	priv->double_click_detector = NULL;
	priv->is_dragging = FALSE;
}

static void
tile_finalize (GObject * g_object)
{
	Tile *tile = TILE (g_object);
	TilePrivate *priv = tile_get_instance_private (TILE(tile));

	if (tile->n_actions)	/* this will also free "default_action" entry */
	{
		g_free (tile->actions);
	}

	if (tile->uri)
		g_free (tile->uri);

	g_object_unref (priv->double_click_detector);

	(*G_OBJECT_CLASS (tile_parent_class)->finalize) (g_object);
}

static void
tile_dispose (GObject * g_object)
{
	Tile *tile = TILE (g_object);

	/* free the TileAction object */
	if (tile->n_actions)
	{
		gint x;
		for (x = 0; x < tile->n_actions; x++)
		{
			if (tile->actions[x] != NULL) {
				g_object_unref (tile->actions[x]);
				tile->actions[x] = NULL;
			}
		}
	}

	/* free the GtkMenu object */
	if (tile->context_menu != NULL) {
		gtk_widget_destroy (GTK_WIDGET (tile->context_menu));
		tile->context_menu = NULL;
	}

	(*G_OBJECT_CLASS (tile_parent_class)->dispose) (g_object);
}

static void
tile_get_property (GObject * g_obj, guint prop_id, GValue * value, GParamSpec * param_spec)
{
	if (!IS_TILE (g_obj))
		return;

	switch (prop_id)
	{
	case PROP_TILE_URI:
		g_value_set_string (value, TILE (g_obj)->uri);
		break;

	case PROP_TILE_CONTEXT_MENU:
		g_value_set_object (value, TILE (g_obj)->context_menu);
		break;

	default:
		break;
	}
}

static void
tile_set_property (GObject * g_obj, guint prop_id, const GValue * value, GParamSpec * param_spec)
{
	Tile *tile;
	GtkMenu *menu;

	if (!IS_TILE (g_obj))
		return;

	tile = TILE (g_obj);

	switch (prop_id)
	{
	case PROP_TILE_URI:
		tile->uri = g_strdup (g_value_get_string (value));
		break;

	case PROP_TILE_CONTEXT_MENU:
		menu = g_value_get_object (value);

		if (menu == tile->context_menu)
			break;

		if (tile->context_menu)
			gtk_menu_detach (tile->context_menu);

		tile->context_menu = menu;

		if (tile->context_menu)
			gtk_menu_attach_to_widget (tile->context_menu, GTK_WIDGET (tile), NULL);

		break;

	default:
		break;
	}
}

static void
tile_setup (Tile * tile)
{
	gtk_button_set_relief (GTK_BUTTON (tile), GTK_RELIEF_NONE);

	if (tile->uri)
	{
		gtk_drag_source_set (GTK_WIDGET (tile), GDK_BUTTON1_MASK, NULL, 0,
			GDK_ACTION_COPY | GDK_ACTION_MOVE);

		gtk_drag_source_add_uri_targets (GTK_WIDGET (tile));
	}
}

static void
tile_enter (GtkButton * widget)
{
	gtk_widget_set_state_flags (GTK_WIDGET (widget), TILE_STATE_ENTERED, TRUE);

	TILE (widget)->entered = TRUE;
}

static void
tile_leave (GtkButton * widget)
{
	if (gtk_widget_has_focus (GTK_WIDGET (widget)))
		gtk_widget_set_state_flags (GTK_WIDGET (widget), TILE_STATE_FOCUSED, TRUE);
	else
		gtk_widget_set_state_flags (GTK_WIDGET (widget), GTK_STATE_FLAG_NORMAL, TRUE);

	TILE (widget)->entered = FALSE;
}

static void
tile_clicked (GtkButton * widget)
{
	TileEvent *tile_event;

	tile_event = g_new0 (TileEvent, 1);
	tile_event->type = TILE_EVENT_ACTIVATED_DOUBLE_CLICK;
	tile_event->time = gtk_get_current_event_time ();

	g_signal_emit (widget, tile_signals[TILE_ACTIVATED_SIGNAL], 0, tile_event);

	gtk_button_released (widget);
	g_free (tile_event);
}

static gboolean
tile_focus_in (GtkWidget * widget, GdkEventFocus * event)
{
	gtk_widget_set_state_flags (widget, TILE_STATE_FOCUSED, TRUE);

	return FALSE;
}

static gboolean
tile_focus_out (GtkWidget * widget, GdkEventFocus * event)
{
	if (TILE (widget)->entered)
		gtk_widget_set_state_flags (widget, TILE_STATE_ENTERED, TRUE);
	else
		gtk_widget_set_state_flags (widget, GTK_STATE_FLAG_NORMAL, TRUE);

	return FALSE;
}

static gboolean
tile_draw (GtkWidget * widget, cairo_t * cr)
{
	/* FIXME: there ought to be a better way to prevent the focus from being rendered. */

	gboolean has_focus;
	gboolean retval;

	if ((has_focus = gtk_widget_has_focus (widget)))
		gtk_widget_unset_state_flags (widget, GTK_STATE_FLAG_FOCUSED);

	retval = (*GTK_WIDGET_CLASS (tile_parent_class)->draw) (widget, cr);

	if (has_focus)
		gtk_widget_set_state_flags (widget, GTK_STATE_FLAG_FOCUSED, TRUE);

	return retval;
}

static gboolean
tile_button_release (GtkWidget * widget, GdkEventButton * event)
{
	Tile *tile = TILE (widget);
	TilePrivate *priv = tile_get_instance_private (tile);

	TileEvent *tile_event;

	if (priv->is_dragging)
	{
		priv->is_dragging = FALSE;

		return TRUE;
	}

	switch (event->button)
	{
	case 1:
		tile_event = g_new0 (TileEvent, 1);
		tile_event->time = event->time;

		if (double_click_detector_is_double_click (priv->double_click_detector, event->time,
				TRUE))
			tile_event->type = TILE_EVENT_ACTIVATED_DOUBLE_CLICK;
		else
			tile_event->type = TILE_EVENT_ACTIVATED_SINGLE_CLICK;

		g_signal_emit (tile, tile_signals[TILE_ACTIVATED_SIGNAL], 0, tile_event);

		gtk_button_released (GTK_BUTTON (widget));
		g_free (tile_event);

		break;

	case 3:
		if (GTK_IS_MENU (tile->context_menu))
		    gtk_menu_popup_at_widget (GTK_MENU (tile->context_menu),
		                              widget,
		                              GDK_GRAVITY_SOUTH_WEST,
		                              GDK_GRAVITY_NORTH_WEST,
		                              (const GdkEvent*) event);

		break;

	default:
		break;
	}

	return TRUE;
}

static gboolean
tile_key_release (GtkWidget * widget, GdkEventKey * event)
{
	TileEvent *tile_event;

	if (event->keyval == GDK_KEY_Return)
	{
		tile_event = g_new0 (TileEvent, 1);
		tile_event->type = TILE_EVENT_ACTIVATED_KEYBOARD;
		tile_event->time = event->time;

		g_signal_emit (widget, tile_signals[TILE_ACTIVATED_SIGNAL], 0, tile_event);

		return TRUE;
	}

	return FALSE;
}

static gboolean
tile_popup_menu (GtkWidget * widget)
{
	Tile *tile = TILE (widget);

	if (GTK_IS_MENU (tile->context_menu))
	{
		gtk_menu_popup_at_widget (GTK_MENU (tile->context_menu),
		                          widget,
		                          GDK_GRAVITY_SOUTH_WEST,
		                          GDK_GRAVITY_NORTH_WEST,
		                          NULL);

		return TRUE;
	}

	else
		return FALSE;
}

static void
tile_drag_begin (GtkWidget * widget, GdkDragContext * context)
{
	TilePrivate *priv;

	priv = tile_get_instance_private (TILE(widget));
        priv->is_dragging = TRUE;
}

static void
tile_drag_data_get (GtkWidget * widget, GdkDragContext * context, GtkSelectionData * data,
	guint info, guint time)
{
	gchar *uris[2];

	if (TILE (widget)->uri)
	{
		uris[0] = TILE (widget)->uri;
		uris[1] = NULL;

		gtk_selection_data_set_uris (data, uris);
	}
}

static void
tile_tile_action_triggered (Tile * tile, TileEvent * event, TileAction * action)
{
	if (action && action->func)
		(*action->func) (tile, event, action);
}

void
tile_trigger_action (Tile * tile, TileAction * action)
{
	tile_trigger_action_with_time (tile, action, GDK_CURRENT_TIME);
}

void
tile_trigger_action_with_time (Tile * tile, TileAction * action, guint32 time)
{
	TileEvent *event = g_new0 (TileEvent, 1);

	event->type = TILE_EVENT_ACTION_TRIGGERED;
	event->time = time;

	g_signal_emit (tile, tile_signals[TILE_ACTION_TRIGGERED_SIGNAL], 0, event, action);
	g_free (event);
}

static void
tile_action_triggered_event_marshal (GClosure * closure, GValue * retval, guint n_param,
	const GValue * param, gpointer invocation_hint, gpointer marshal_data)
{
	marshal_func_VOID__POINTER_POINTER callback;
	GCClosure *cc = (GCClosure *) closure;
	gpointer data_0, data_1;

	g_return_if_fail (n_param == 3);

	if (G_CCLOSURE_SWAP_DATA (closure))
	{
		data_0 = closure->data;
		data_1 = g_value_peek_pointer (param);
	}
	else
	{
		data_0 = g_value_peek_pointer (param);
		data_1 = closure->data;
	}

	if (marshal_data)
		callback = (marshal_func_VOID__POINTER_POINTER) marshal_data;
	else
		callback = (marshal_func_VOID__POINTER_POINTER) cc->callback;

	callback (data_0, g_value_peek_pointer (param + 1), g_value_peek_pointer (param + 2),
		data_1);
}
