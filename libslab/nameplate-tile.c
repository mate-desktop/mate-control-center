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

#include "nameplate-tile.h"

static void nameplate_tile_class_init (NameplateTileClass *);
static void nameplate_tile_init (NameplateTile *);
static void nameplate_tile_get_property (GObject *, guint, GValue *, GParamSpec *);
static void nameplate_tile_set_property (GObject *, guint, const GValue *, GParamSpec *);
static GObject *nameplate_tile_constructor (GType, guint, GObjectConstructParam *);

static void nameplate_tile_drag_begin (GtkWidget *, GdkDragContext *);

static void nameplate_tile_setup (NameplateTile *);

typedef struct
{
	GtkContainer *image_ctnr;
	GtkContainer *header_ctnr;
	GtkContainer *subheader_ctnr;
} NameplateTilePrivate;

enum
{
	PROP_0,
	PROP_NAMEPLATE_IMAGE,
	PROP_NAMEPLATE_HEADER,
	PROP_NAMEPLATE_SUBHEADER,
};

G_DEFINE_TYPE_WITH_PRIVATE (NameplateTile, nameplate_tile, TILE_TYPE)

GtkWidget *nameplate_tile_new (const gchar * uri, GtkWidget * image, GtkWidget * header,
	GtkWidget * subheader)
{
	return GTK_WIDGET (
		g_object_new (NAMEPLATE_TILE_TYPE,
		"tile-uri",            uri,
		"nameplate-image",     image,
		"nameplate-header",    header,
		"nameplate-subheader", subheader,
		NULL));
}

static void
nameplate_tile_class_init (NameplateTileClass * this_class)
{
	GObjectClass *g_obj_class = G_OBJECT_CLASS (this_class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (this_class);

	g_obj_class->constructor = nameplate_tile_constructor;
	g_obj_class->get_property = nameplate_tile_get_property;
	g_obj_class->set_property = nameplate_tile_set_property;

	widget_class->drag_begin = nameplate_tile_drag_begin;

	g_object_class_install_property (g_obj_class, PROP_NAMEPLATE_IMAGE,
		g_param_spec_object ("nameplate-image", "nameplate-image", "nameplate image",
			GTK_TYPE_WIDGET, G_PARAM_READWRITE));

	g_object_class_install_property (g_obj_class, PROP_NAMEPLATE_HEADER,
		g_param_spec_object ("nameplate-header", "nameplate-header", "nameplate header",
			GTK_TYPE_WIDGET, G_PARAM_READWRITE));

	g_object_class_install_property (g_obj_class, PROP_NAMEPLATE_SUBHEADER,
		g_param_spec_object ("nameplate-subheader", "nameplate-subheader",
			"nameplate subheader", GTK_TYPE_WIDGET, G_PARAM_READWRITE));
}

static void
nameplate_tile_init (NameplateTile * this)
{
}

static GObject *
nameplate_tile_constructor (GType type, guint n_param, GObjectConstructParam * param)
{
	GObject *g_obj =
		(*G_OBJECT_CLASS (nameplate_tile_parent_class)->constructor) (type, n_param, param);

	nameplate_tile_setup (NAMEPLATE_TILE (g_obj));

	return g_obj;
}

static void
nameplate_tile_get_property (GObject * g_object, guint prop_id, GValue * value,
	GParamSpec * param_spec)
{
	NameplateTile *np_tile = NAMEPLATE_TILE (g_object);

	switch (prop_id)
	{
	case PROP_NAMEPLATE_IMAGE:
		g_value_set_object (value, np_tile->image);
		break;

	case PROP_NAMEPLATE_HEADER:
		g_value_set_object (value, np_tile->header);
		break;

	case PROP_NAMEPLATE_SUBHEADER:
		g_value_set_object (value, np_tile->subheader);
		break;
	default:
		break;
	}
}

static void
nameplate_tile_set_property (GObject * g_object, guint prop_id, const GValue * value,
	GParamSpec * param_spec)
{
	NameplateTile *this = NAMEPLATE_TILE (g_object);
	NameplateTilePrivate *priv = nameplate_tile_get_instance_private (this);

	GObject *widget_obj = NULL;

	switch (prop_id) {
		case PROP_NAMEPLATE_IMAGE:
		case PROP_NAMEPLATE_HEADER:
		case PROP_NAMEPLATE_SUBHEADER:
			widget_obj = g_value_get_object (value);
			break;
		default:
			break;
	}

	switch (prop_id)
	{
	case PROP_NAMEPLATE_IMAGE:
		if (GTK_IS_WIDGET (widget_obj))
		{
			if (GTK_IS_WIDGET (this->image))
				gtk_widget_destroy (this->image);

			this->image = GTK_WIDGET (widget_obj);

			gtk_container_add (priv->image_ctnr, this->image);

			gtk_widget_show_all (this->image);
		}
		else if (GTK_IS_WIDGET (this->image))
			gtk_widget_destroy (this->image);

		break;

	case PROP_NAMEPLATE_HEADER:
		if (GTK_IS_WIDGET (widget_obj))
		{
			if (GTK_IS_WIDGET (this->header))
				gtk_widget_destroy (this->header);

			this->header = GTK_WIDGET (widget_obj);

			gtk_container_add (priv->header_ctnr, this->header);

			gtk_widget_show_all (this->header);
		}
		else if (GTK_IS_WIDGET (this->header))
			gtk_widget_destroy (this->header);

		break;

	case PROP_NAMEPLATE_SUBHEADER:
		if (GTK_IS_WIDGET (widget_obj))
		{
			if (GTK_IS_WIDGET (this->subheader))
				gtk_widget_destroy (this->subheader);

			this->subheader = GTK_WIDGET (widget_obj);

			gtk_container_add (priv->subheader_ctnr, this->subheader);

			gtk_widget_show_all (this->subheader);
		}
		else if (GTK_IS_WIDGET (this->subheader))
			gtk_widget_destroy (this->subheader);

		break;

	default:
		break;
	}
}

static void
nameplate_tile_setup (NameplateTile *this)
{
	NameplateTilePrivate *priv = nameplate_tile_get_instance_private (this);

	GtkWidget *hbox;
	GtkWidget *vbox;

	priv->image_ctnr = GTK_CONTAINER (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
	gtk_widget_set_valign (GTK_WIDGET (priv->image_ctnr), GTK_ALIGN_CENTER);

	priv->header_ctnr = GTK_CONTAINER (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));

	priv->subheader_ctnr = GTK_CONTAINER (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
	gtk_widget_set_halign (GTK_WIDGET (priv->subheader_ctnr), GTK_ALIGN_START);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_halign (vbox, GTK_ALIGN_FILL);
	gtk_widget_set_valign (vbox, GTK_ALIGN_CENTER);

	gtk_container_add (GTK_CONTAINER (this), hbox);
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (priv->image_ctnr), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (priv->header_ctnr), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (priv->subheader_ctnr), FALSE, FALSE, 0);

	if (GTK_IS_WIDGET (this->image))
		gtk_container_add (priv->image_ctnr, this->image);

	if (GTK_IS_WIDGET (this->header))
		gtk_container_add (priv->header_ctnr, this->header);

	if (GTK_IS_WIDGET (this->subheader))
		gtk_container_add (priv->subheader_ctnr, this->subheader);

	gtk_widget_set_focus_on_click (GTK_WIDGET (this), FALSE);
}

static void
nameplate_tile_drag_begin (GtkWidget * widget, GdkDragContext * context)
{
	NameplateTile *this = NAMEPLATE_TILE (widget);
	GtkImage *image;
	const gchar *name;

	(*GTK_WIDGET_CLASS (nameplate_tile_parent_class)->drag_begin) (widget, context);

	if (!this->image || !GTK_IS_IMAGE (this->image))
		return;

	image = GTK_IMAGE (this->image);

	switch (gtk_image_get_storage_type (image))
	{
	case GTK_IMAGE_PIXBUF:
		if (gtk_image_get_pixbuf (image))
			gtk_drag_set_icon_pixbuf (context, gtk_image_get_pixbuf (image), 0, 0);

		break;

	case GTK_IMAGE_ICON_NAME:
		gtk_image_get_icon_name (image, &name, NULL);
		if (name)
			gtk_drag_set_icon_name (context, name, 0, 0);

		break;

	default:
		break;
	}
}
