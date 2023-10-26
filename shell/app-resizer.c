/*
 * This file is part of libslab.
 *
 * Copyright (c) 2006 Novell, Inc.
 *
 * Libslab is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Libslab is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libslab; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <gtk/gtk.h>

#include "app-shell.h"
#include "app-resizer.h"

static void app_resizer_size_allocate (GtkWidget * resizer, GtkAllocation * allocation);
static gboolean app_resizer_paint_window (GtkWidget * widget, cairo_t * cr, AppShellData * app_data);

G_DEFINE_TYPE (AppResizer, app_resizer, GTK_TYPE_LAYOUT);

static void
app_resizer_class_init (AppResizerClass * klass)
{
	GtkWidgetClass *widget_class;

	widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->size_allocate = app_resizer_size_allocate;
}

static void
app_resizer_init (AppResizer * window)
{
    gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (window)),
                                 GTK_STYLE_CLASS_VIEW);
}

void
remove_container_entries (GtkContainer * widget)
{
	GList *children, *l;

	children = gtk_container_get_children (widget);
	for (l = children; l; l = l->next)
	{
		GtkWidget *child = GTK_WIDGET (l->data);
		gtk_container_remove (GTK_CONTAINER (widget), GTK_WIDGET (child));
	}

	if (children)
		g_list_free (children);
}

static void
resize_table (AppResizer *widget, GtkGrid * table, gint columns)
{
	remove_container_entries (GTK_CONTAINER (table));
	widget->column = columns;
}

static void
relayout_table (AppResizer *widget, GtkGrid * table, GList * element_list)
{
	gint row = 0, col = 0;
	do
	{
		GtkWidget *element = GTK_WIDGET (element_list->data);
		gtk_grid_attach (table, element, col, row, 1, 1);
		col++;
		if (col == widget->column)
		{
			col = 0;
			row++;
		}
	}
	while (NULL != (element_list = g_list_next (element_list)));
}

void
app_resizer_layout_table_default (AppResizer * widget, GtkGrid * table, GList * element_list)
{
	resize_table (widget, table, widget->cur_num_cols);
	relayout_table (widget, table, element_list);
}

static void
relayout_tables (AppResizer * widget, gint num_cols)
{
	GtkGrid *table;
	GList *table_list, *launcher_list;

	for (table_list = widget->cached_tables_list; table_list != NULL;
		table_list = g_list_next (table_list))
	{
		table = GTK_GRID (table_list->data);
		launcher_list = gtk_container_get_children (GTK_CONTAINER (table));
		launcher_list = g_list_reverse (launcher_list);	/* Fixme - ugly hack because table stores prepend */
		resize_table (widget, table, num_cols);
		relayout_table (widget, table, launcher_list);
		g_list_free (launcher_list);
	}
}

static gint
calculate_num_cols (AppResizer * resizer, gint avail_width)
{
	if (resizer->table_elements_homogeneous)
	{
		gint num_cols;

		if (resizer->cached_element_width == -1)
		{
			GtkGrid *table = GTK_GRID (resizer->cached_tables_list->data);
			GList *children = gtk_container_get_children (GTK_CONTAINER (table));
			GtkWidget *table_element = GTK_WIDGET (children->data);
			gint natural_width;
			g_list_free (children);

			gtk_widget_get_preferred_width (table_element, NULL, &natural_width);
			resizer->cached_element_width = natural_width;
			resizer->cached_table_spacing = gtk_grid_get_column_spacing (table);
		}

		num_cols =
			(avail_width +
			resizer->cached_table_spacing) / (resizer->cached_element_width +
			resizer->cached_table_spacing);
		return num_cols;
	}
	else
		g_assert_not_reached ();	/* Fixme - implement... */
}

static gint
relayout_tables_if_needed (AppResizer * widget, gint avail_width, gint current_num_cols)
{
	gint num_cols = calculate_num_cols (widget, avail_width);
	if (num_cols < 1)
	{
		num_cols = 1;	/* just horiz scroll if avail_width is less than one column */
	}

	if (current_num_cols != num_cols)
	{
		relayout_tables (widget, num_cols);
		current_num_cols = num_cols;
	}
	return current_num_cols;
}

void
app_resizer_set_table_cache (AppResizer * widget, GList * cache_list)
{
	widget->cached_tables_list = cache_list;
}

static void
app_resizer_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
	AppResizer *resizer = APP_RESIZER (widget);
	GtkWidget *child = GTK_WIDGET (APP_RESIZER (resizer)->child);
	GtkAllocation widget_allocation;
	GtkRequisition child_requisition;

	static gboolean first_time = TRUE;
	gint new_num_cols;

	if (first_time)
	{
		/* we are letting the first show be the "natural" size of the child widget so do nothing. */
		if (GTK_WIDGET_CLASS (app_resizer_parent_class)->size_allocate)
			(*GTK_WIDGET_CLASS (app_resizer_parent_class)->size_allocate) (widget, allocation);

		first_time = FALSE;
		gtk_widget_get_allocation (child, &widget_allocation);
		gtk_layout_set_size (GTK_LAYOUT (resizer), widget_allocation.width,
			widget_allocation.height);
		return;
	}

	gtk_widget_get_preferred_size (child, &child_requisition, NULL);

	if (!resizer->cached_tables_list)	/* if everthing is currently filtered out - just return */
	{
		GtkAllocation child_allocation;

		if (GTK_WIDGET_CLASS (app_resizer_parent_class)->size_allocate)
			(*GTK_WIDGET_CLASS (app_resizer_parent_class)->size_allocate) (widget, allocation);

		/* We want the message to center itself and only scroll if it's bigger than the available real size. */
		child_allocation.x = 0;
		child_allocation.y = 0;
		child_allocation.width = MAX (allocation->width, child_requisition.width);
		child_allocation.height = MAX (allocation->height, child_requisition.height);

		gtk_widget_size_allocate (child, &child_allocation);
		gtk_layout_set_size (GTK_LAYOUT (resizer), child_allocation.width,
			child_allocation.height);
		return;
	}
	GtkRequisition other_requisiton;
	gtk_widget_get_preferred_size (GTK_WIDGET (resizer->cached_tables_list->data), &other_requisiton, NULL);

	new_num_cols =
		relayout_tables_if_needed (APP_RESIZER (resizer), allocation->width,
		resizer->cur_num_cols);
	if (resizer->cur_num_cols != new_num_cols)
	{
		GtkRequisition req;

		/* Have to do this so that it requests, and thus gets allocated, new amount */
		gtk_widget_get_preferred_size (child, &req, NULL);

		resizer->cur_num_cols = new_num_cols;
	}

	if (GTK_WIDGET_CLASS (app_resizer_parent_class)->size_allocate)
		(*GTK_WIDGET_CLASS (app_resizer_parent_class)->size_allocate) (widget, allocation);
	gtk_widget_get_allocation (child, &widget_allocation);
	gtk_layout_set_size (GTK_LAYOUT (resizer), widget_allocation.width,
		widget_allocation.height);
}

GtkWidget *
app_resizer_new (GtkBox * child, gint initial_num_columns, gboolean homogeneous,
	AppShellData * app_data)
{
	AppResizer *widget;

	g_assert (child != NULL);

	widget = g_object_new (APP_RESIZER_TYPE, NULL);
	widget->cached_element_width = -1;
	widget->cur_num_cols = initial_num_columns;
	widget->table_elements_homogeneous = homogeneous;
	widget->setting_style = FALSE;
	widget->app_data = app_data;

	g_signal_connect (widget, "draw",
	                  G_CALLBACK (app_resizer_paint_window),
	                  app_data);

	gtk_container_add (GTK_CONTAINER (widget), GTK_WIDGET (child));
	widget->child = child;

	return GTK_WIDGET (widget);
}

void
app_resizer_set_vadjustment_value (GtkWidget * widget, gdouble value)
{
	GtkAdjustment *adjust;

	adjust = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (widget));

	gdouble upper = gtk_adjustment_get_upper (adjust);
	gdouble page_size = gtk_adjustment_get_page_size (adjust);
	if (value > upper - page_size)
	{
		value = upper - page_size;
	}
	gtk_adjustment_set_value (adjust, value);
}

static gboolean
app_resizer_paint_window (GtkWidget * widget, cairo_t * cr, AppShellData * app_data)
{
	cairo_save(cr);
	GtkStyleContext *context;
	GdkRGBA *bg_rgba = NULL;

	GtkAllocation widget_allocation;
	gtk_widget_get_allocation (widget, &widget_allocation);

	context = gtk_widget_get_style_context (widget);
	gtk_style_context_get (context,
	                       GTK_STATE_FLAG_NORMAL,
	                       "background-color", &bg_rgba,
	                       NULL);

	gdk_cairo_set_source_rgba (cr, bg_rgba);
	cairo_set_line_width(cr, 1);

	cairo_rectangle(cr, widget_allocation.x, widget_allocation.y, widget_allocation.width, widget_allocation.height);
	cairo_stroke_preserve(cr);
	cairo_fill(cr);

	if (app_data->selected_group)
	{
		GtkWidget *selected_widget = GTK_WIDGET (app_data->selected_group);
		GdkRGBA *rgba;
		GtkAllocation selected_widget_allocation;
		gtk_widget_get_allocation (selected_widget, &selected_widget_allocation);

		gtk_style_context_get (context,
		                       GTK_STATE_FLAG_PRELIGHT,
		                       "background-color", &rgba,
		                       NULL);

		gdk_cairo_set_source_rgba (cr, rgba);
		cairo_set_line_width(cr, 1);
		cairo_rectangle(cr, selected_widget_allocation.x, selected_widget_allocation.y, selected_widget_allocation.width, selected_widget_allocation.height);
		cairo_stroke_preserve(cr);
		cairo_fill(cr);
		gdk_rgba_free (rgba);
	}

	cairo_restore(cr);
	gdk_rgba_free (bg_rgba);

	return FALSE;
}
