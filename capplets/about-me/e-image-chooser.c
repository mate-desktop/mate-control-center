/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* e-image-chooser.c
 * Copyright (C) 2004  Novell, Inc.
 * Author: Chris Toshok <toshok@ximian.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <config.h>

#include <string.h>

#include <glib-object.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "e-image-chooser.h"

typedef struct _EImageChooserPrivate EImageChooserPrivate;
struct _EImageChooserPrivate {

	GtkWidget *image;

	char *image_buf;
	int   image_buf_size;
	int   width;
	int   height;

	gboolean editable;
	gboolean scaleable;
};

enum {
	CHANGED,
	LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_WIDTH,
    PROP_HEIGHT,
    NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };
static guint image_chooser_signals [LAST_SIGNAL] = { 0 };

static void e_image_chooser_dispose      (GObject *object);

static gboolean image_drag_motion_cb (GtkWidget *widget,
				      GdkDragContext *context,
				      gint x, gint y, guint time, EImageChooser *chooser);
static gboolean image_drag_drop_cb (GtkWidget *widget,
				    GdkDragContext *context,
				    gint x, gint y, guint time, EImageChooser *chooser);
static void image_drag_data_received_cb (GtkWidget *widget,
					 GdkDragContext *context,
					 gint x, gint y,
					 GtkSelectionData *selection_data,
					 guint info, guint time, EImageChooser *chooser);

G_DEFINE_TYPE_WITH_PRIVATE (EImageChooser, e_image_chooser, GTK_TYPE_BOX);

enum DndTargetType {
	DND_TARGET_TYPE_URI_LIST
};
#define URI_LIST_TYPE "text/uri-list"

static GtkTargetEntry image_drag_types[] = {
	{ URI_LIST_TYPE, 0, DND_TARGET_TYPE_URI_LIST },
};
static const int num_image_drag_types = sizeof (image_drag_types) / sizeof (image_drag_types[0]);

GtkWidget *
e_image_chooser_new (void)
{
	return g_object_new (E_TYPE_IMAGE_CHOOSER, "orientation", GTK_ORIENTATION_VERTICAL, NULL);
}

GtkWidget *e_image_chooser_new_with_size (int width, int height)
{
	return g_object_new (E_TYPE_IMAGE_CHOOSER,
			"width", width,
			"height", height,
			"orientation", GTK_ORIENTATION_VERTICAL, NULL);
}

static void
e_image_chooser_set_property (GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
	EImageChooserPrivate *priv;

	priv = e_image_chooser_get_instance_private (E_IMAGE_CHOOSER (object));

	switch (prop_id)
	{
		case PROP_WIDTH:
			priv->width = g_value_get_int (value);
			priv->scaleable = FALSE;
			break;
		case PROP_HEIGHT:
			priv->height = g_value_get_int (value);
			priv->scaleable = FALSE;
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
e_image_chooser_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	EImageChooserPrivate *priv;

	priv = e_image_chooser_get_instance_private (E_IMAGE_CHOOSER (object));

	switch (prop_id)
	{
		case PROP_WIDTH:
			g_value_set_int (value, priv->width);
			break;
		case PROP_HEIGHT:
			g_value_set_int (value, priv->height);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
e_image_chooser_class_init (EImageChooserClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = e_image_chooser_dispose;
	object_class->set_property = e_image_chooser_set_property;
	object_class->get_property = e_image_chooser_get_property;

	image_chooser_signals [CHANGED] =
		g_signal_new ("changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (EImageChooserClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	properties[PROP_WIDTH] =
		g_param_spec_int ("width",
				"Chooser width",
				"Chooser width to show image",
				0, G_MAXINT,
				32,
				G_PARAM_READWRITE);
	properties[PROP_HEIGHT] =
		g_param_spec_int ("height",
				"Chooser height",
				"Chooser height to show image",
				0, G_MAXINT,
				32,
				G_PARAM_READWRITE);

	g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);
}

static void
e_image_chooser_init (EImageChooser *chooser)
{
	EImageChooserPrivate *priv;

	priv = e_image_chooser_get_instance_private (chooser);

	priv->image = gtk_image_new ();
	priv->scaleable = TRUE;

	gtk_box_set_homogeneous (GTK_BOX (chooser), FALSE);
	gtk_box_pack_start (GTK_BOX (chooser), priv->image, TRUE, TRUE, 0);

	gtk_drag_dest_set (priv->image, 0, image_drag_types, num_image_drag_types, GDK_ACTION_COPY);
	g_signal_connect (priv->image,
			  "drag_motion", G_CALLBACK (image_drag_motion_cb), chooser);
	g_signal_connect (priv->image,
			  "drag_drop", G_CALLBACK (image_drag_drop_cb), chooser);
	g_signal_connect (priv->image,
			  "drag_data_received", G_CALLBACK (image_drag_data_received_cb), chooser);

	gtk_widget_show_all (priv->image);

	/* we default to being editable */
	priv->editable = TRUE;
}

static void
e_image_chooser_dispose (GObject *object)
{
	EImageChooserPrivate *priv;

	priv = e_image_chooser_get_instance_private (E_IMAGE_CHOOSER (object));


	if (priv->image_buf) {
		g_free (priv->image_buf);
		priv->image_buf = NULL;
	}

	if (G_OBJECT_CLASS (e_image_chooser_parent_class)->dispose)
		(* G_OBJECT_CLASS (e_image_chooser_parent_class)->dispose) (object);
}


static gboolean
set_image_from_data (EImageChooser *chooser,
		     char *data, int length)
{
	gboolean rv = FALSE;
	GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
	GdkPixbuf *pixbuf;
	EImageChooserPrivate *priv;

	priv = e_image_chooser_get_instance_private (chooser);

	gdk_pixbuf_loader_write (loader, (guchar *) data, length, NULL);
	gdk_pixbuf_loader_close (loader, NULL);

	pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
	if (pixbuf)
		g_object_ref (pixbuf);
	g_object_unref (loader);

	if (pixbuf) {
		GdkPixbuf *scaled;
		if (priv->scaleable) {
			gtk_image_set_from_pixbuf (GTK_IMAGE (priv->image), pixbuf);
		} else {
			scaled = gdk_pixbuf_scale_simple (pixbuf,
							  priv->width, priv->height,
							  GDK_INTERP_BILINEAR);

			gtk_image_set_from_pixbuf (GTK_IMAGE (priv->image), scaled);
			g_object_unref (scaled);
		}

		g_object_unref (pixbuf);

		g_free (priv->image_buf);
		priv->image_buf = data;
		priv->image_buf_size = length;

		g_signal_emit (chooser,
			       image_chooser_signals [CHANGED], 0);

		rv = TRUE;
	}

	return rv;
}

static gboolean
image_drag_motion_cb (GtkWidget *widget,
		      GdkDragContext *context,
		      gint x, gint y, guint time, EImageChooser *chooser)
{
	GList *p;
	EImageChooserPrivate *priv;

	priv = e_image_chooser_get_instance_private (chooser);

	if (!priv->editable)
		return FALSE;

	for (p = gdk_drag_context_list_targets (context); p; p = p->next) {
		char *possible_type;

		possible_type = gdk_atom_name (GDK_POINTER_TO_ATOM (p->data));
		if (!strcmp (possible_type, URI_LIST_TYPE)) {
			g_free (possible_type);
			gdk_drag_status (context, GDK_ACTION_COPY, time);
			return TRUE;
		}

		g_free (possible_type);
	}

	return FALSE;
}

static gboolean
image_drag_drop_cb (GtkWidget *widget,
		    GdkDragContext *context,
		    gint x, gint y, guint time, EImageChooser *chooser)
{
	GList *p;
	EImageChooserPrivate *priv;

	priv = e_image_chooser_get_instance_private (chooser);

	if (!priv->editable)
		return FALSE;

	if (gdk_drag_context_list_targets (context) == NULL) {
		return FALSE;
	}

	for (p = gdk_drag_context_list_targets (context); p; p = p->next) {
		char *possible_type;

		possible_type = gdk_atom_name (GDK_POINTER_TO_ATOM (p->data));
		if (!strcmp (possible_type, URI_LIST_TYPE)) {
			g_free (possible_type);
			gtk_drag_get_data (widget, context,
					   GDK_POINTER_TO_ATOM (p->data),
					   time);
			return TRUE;
		}

		g_free (possible_type);
	}

	return FALSE;
}

static void
image_drag_data_received_cb (GtkWidget *widget,
			     GdkDragContext *context,
			     gint x, gint y,
			     GtkSelectionData *selection_data,
			     guint info, guint time, EImageChooser *chooser)
{
	char *target_type;
	gboolean handled = FALSE;

	target_type = gdk_atom_name (gtk_selection_data_get_target (selection_data));

	if (!strcmp (target_type, URI_LIST_TYPE)) {
		const char *data = (const char *) gtk_selection_data_get_data (selection_data);
		char *uri;
		GFile *file;
		GInputStream *istream;
		char *nl = strstr (data, "\r\n");

		if (nl)
			uri = g_strndup (data, nl - (char *) data);
		else
			uri = g_strdup (data);

		file = g_file_new_for_uri (uri);
		istream = G_INPUT_STREAM (g_file_read (file, NULL, NULL));

		if (istream != NULL) {
			GFileInfo *info;

			info = g_file_query_info (file,
						  G_FILE_ATTRIBUTE_STANDARD_SIZE,
						  G_FILE_QUERY_INFO_NONE,
						  NULL, NULL);

			if (info != NULL) {
				gsize size;
				gboolean success;
				gchar *buf;

				size = g_file_info_get_size (info);
				g_object_unref (info);

				buf = g_malloc (size);

				success = g_input_stream_read_all (istream,
								   buf,
								   size,
								   &size,
								   NULL,
								   NULL);
				g_input_stream_close (istream, NULL, NULL);

				if (success &&
						set_image_from_data (chooser, buf, size))
					handled = TRUE;
				else
					g_free (buf);
			}

			g_object_unref (istream);
		}

		g_object_unref (file);
		g_free (uri);
	}

	gtk_drag_finish (context, handled, FALSE, time);
}

gboolean
e_image_chooser_set_from_file (EImageChooser *chooser, const char *filename)
{
	gchar *data;
	gsize data_length;

	g_return_val_if_fail (E_IS_IMAGE_CHOOSER (chooser), FALSE);
	g_return_val_if_fail (filename, FALSE);

	if (!g_file_get_contents (filename, &data, &data_length, NULL)) {
		return FALSE;
	}

	if (!set_image_from_data (chooser, data, data_length))
		g_free (data);

	return TRUE;
}

void
e_image_chooser_set_editable (EImageChooser *chooser, gboolean editable)
{
	g_return_if_fail (E_IS_IMAGE_CHOOSER (chooser));
	EImageChooserPrivate *priv;

	priv = e_image_chooser_get_instance_private (chooser);

	priv->editable = editable;
}

void
e_image_chooser_set_scaleable  (EImageChooser *chooser, gboolean scaleable)
{
	g_return_if_fail (E_IS_IMAGE_CHOOSER (chooser));
	EImageChooserPrivate *priv;

	priv = e_image_chooser_get_instance_private (chooser);

	priv->scaleable = scaleable;
}

gboolean
e_image_chooser_get_image_data (EImageChooser *chooser, char **data, gsize *data_length)
{
	g_return_val_if_fail (E_IS_IMAGE_CHOOSER (chooser), FALSE);
	g_return_val_if_fail (data != NULL, FALSE);
	g_return_val_if_fail (data_length != NULL, FALSE);
	EImageChooserPrivate *priv;

	priv = e_image_chooser_get_instance_private (chooser);

	*data_length = priv->image_buf_size;
	*data = g_malloc (*data_length);
	memcpy (*data, priv->image_buf, *data_length);

	return TRUE;
}

gboolean
e_image_chooser_set_image_data (EImageChooser *chooser, char *data, gsize data_length)
{
	char *buf;

	g_return_val_if_fail (E_IS_IMAGE_CHOOSER (chooser), FALSE);
	g_return_val_if_fail (data != NULL, FALSE);

	/* yuck, a copy... */
	buf = g_malloc (data_length);
	memcpy (buf, data, data_length);

	if (!set_image_from_data (chooser, buf, data_length)) {
		g_free (buf);
		return FALSE;
	}

	return TRUE;
}
