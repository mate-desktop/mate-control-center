#include "mate-utils.h"

#include <string.h>

gboolean
load_image_by_id (GtkImage * image, GtkIconSize size, const gchar * image_id)
{
	GdkPixbuf *pixbuf;
	gint width;
	gint height;

	GtkIconTheme *icon_theme;

	gchar *id;

	gboolean icon_exists;

	if (!image_id)
		return FALSE;

	id = g_strdup (image_id);

	gtk_icon_size_lookup (size, &width, &height);

	if (g_path_is_absolute (id))
	{
		pixbuf = gdk_pixbuf_new_from_file_at_size (id, width, height, NULL);

		icon_exists = (pixbuf != NULL);

		if (icon_exists)
		{
			gtk_image_set_from_pixbuf (image, pixbuf);

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

		icon_exists = gtk_icon_theme_has_icon (icon_theme, id);

		if (icon_exists)
			gtk_image_set_from_icon_name (image, id, size);
		else
			gtk_image_set_from_icon_name (image, "image-missing", size);

	}

	g_free (id);

	return icon_exists;
}

void
handle_g_error (GError ** error, const gchar * msg_format, ...)
{
	gchar *msg;
	va_list args;

	va_start (args, msg_format);
	msg = g_strdup_vprintf (msg_format, args);
	va_end (args);

	if (*error)
	{
		g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
			"\nGError raised: [%s]\nuser_message: [%s]\n", (*error)->message, msg);

		g_error_free (*error);

		*error = NULL;
	}
	else
		g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "\nerror raised: [%s]\n", msg);

	g_free (msg);
}

