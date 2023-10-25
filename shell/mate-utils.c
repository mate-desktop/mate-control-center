#include "mate-utils.h"

#include <string.h>

gboolean
load_image_by_id (GtkImage *image, GtkIconSize size, const gchar *image_id)
{
	cairo_surface_t *surface;
	gint width;
	gint height;
	gint scale_factor;

	GtkIconTheme *icon_theme;

	gchar *id;

	gboolean icon_exists;

	if (!image_id)
		return FALSE;

	id = g_strdup (image_id);
	scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (image));

	gtk_icon_size_lookup (size, &width, &height);
	gtk_image_set_pixel_size (image, width);

	if (g_path_is_absolute (id))
	{
		GdkPixbuf *pixbuf;

		pixbuf = gdk_pixbuf_new_from_file_at_size (id, width * scale_factor, height * scale_factor, NULL);

		icon_exists = (pixbuf != NULL);

		if (icon_exists)
		{
			surface = gdk_cairo_surface_create_from_pixbuf (pixbuf, scale_factor, NULL);
			gtk_image_set_from_surface (image, surface);

			cairo_surface_destroy (surface);
			g_object_unref (pixbuf);
		}
		else
			gtk_image_set_from_icon_name (image, "image-missing", size);
	}
	else
	{
		if (		/* file extensions are not copesetic with loading by "name" */
			g_str_has_suffix (id, ".png") ||
			g_str_has_suffix (id, ".svg") ||
			g_str_has_suffix (id, ".xpm")
		   )

			id[strlen (id) - 4] = '\0';

		if (gtk_widget_has_screen (GTK_WIDGET (image)))
			icon_theme =
				gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET
					(image)));
		else
			icon_theme = gtk_icon_theme_get_default ();

		surface = gtk_icon_theme_load_surface (icon_theme, id,
		                                       width, scale_factor,
		                                       NULL,
		                                       GTK_ICON_LOOKUP_FORCE_SIZE,
		                                       NULL);
		icon_exists = (surface != NULL);
		if (icon_exists) {
			gtk_image_set_from_surface (image, surface);
			cairo_surface_destroy (surface);
		}
		else
			gtk_image_set_from_icon_name (image, "image-missing", size);

	}

	g_free (id);

	return icon_exists;
}
