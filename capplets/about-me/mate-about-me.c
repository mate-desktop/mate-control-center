/* mate-about-me.c
 * Copyright (C) 2002 Diego Gonzalez
 *
 * Written by: Diego Gonzalez <diego@pemas.net>
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
#  include <config.h>
#endif

#include <glib/gstdio.h>
#include <gio/gio.h>
#include <unistd.h>
#include <dbus/dbus-glib-bindings.h>

#if HAVE_ACCOUNTSSERVICE
#include <act/act.h>
#endif

#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmate-desktop/mate-desktop-thumbnail.h>

#include "e-image-chooser.h"
#include "mate-about-me-password.h"
#include "mate-about-me-fingerprint.h"
#include "marshal.h"

#include "capplet-util.h"

#define MAX_HEIGHT 100
#define MAX_WIDTH  100

typedef struct {

	GtkBuilder 	*dialog;
	GtkWidget	*enable_fingerprint_button;
	GtkWidget	*disable_fingerprint_button;
	GtkWidget   	*image_chooser;
	GdkPixbuf       *image;
#if HAVE_ACCOUNTSSERVICE
	ActUser         *user;
#endif

	GdkScreen    	*screen;
	GtkIconTheme 	*theme;
	MateDesktopThumbnailFactory *thumbs;

	gboolean      	 have_image;
	gboolean      	 image_changed;
	gboolean      	 create_self;

	gchar        	*person;
	gchar 		*login;
	gchar 		*username;

	guint	      	 commit_timeout_id;
} MateAboutMe;

static MateAboutMe *me = NULL;

static void
about_me_destroy (void)
{
	if (me->dialog)
		g_object_unref (me->dialog);
	if (me->image)
		g_object_unref (me->image);

	g_free (me->person);
	g_free (me->login);
	g_free (me->username);
	g_free (me);
	me = NULL;
}

static void
about_me_load_photo (MateAboutMe *me)
{
	gchar         *file = NULL;
	GError        *error = NULL;
#if HAVE_ACCOUNTSSERVICE
	const gchar   *act_file;

	if (act_user_is_loaded (me->user)) {
		act_file = act_user_get_icon_file (me->user);
		if ( act_file != NULL && strlen (act_file) > 1) {
			file = g_strdup (act_file);
		}
	}
#endif
	if (file == NULL) {
		file = g_build_filename (g_get_home_dir (), ".face", NULL);
	}

	me->image = gdk_pixbuf_new_from_file(file, &error);

	if (me->image != NULL) {
		e_image_chooser_set_from_file (E_IMAGE_CHOOSER (me->image_chooser), file);
		me->have_image = TRUE;
	} else {
		me->have_image = FALSE;
		g_warning ("Could not load %s: %s", file, error->message);
		e_image_chooser_set_from_file (E_IMAGE_CHOOSER (me->image_chooser), me->person);
		g_error_free (error);
	}
	g_free (file);
}

static void
about_me_update_photo (MateAboutMe *me)
{
	gchar         *file;
	GError        *error = NULL;

	guchar 	      *data;
	gsize 	       length;

	if (me->image_changed && me->have_image) {
		GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
		GdkPixbuf *pixbuf = NULL, *scaled = NULL;
		int height, width;
		gboolean do_scale = FALSE;
		float scale = 1.0;
		float scalex = 1.0, scaley = 1.0;

		e_image_chooser_get_image_data (E_IMAGE_CHOOSER (me->image_chooser), (char **) &data, &length);

		/* Before updating the image in EDS scale it to a reasonable size
		   so that the user doesn't get an application that does not respond
		   or that takes 100% CPU */
		gdk_pixbuf_loader_write (loader, data, length, NULL);
		gdk_pixbuf_loader_close (loader, NULL);

		pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

		if (pixbuf)
			g_object_ref (pixbuf);

		g_object_unref (loader);

		height = gdk_pixbuf_get_height (pixbuf);
		width = gdk_pixbuf_get_width (pixbuf);

		if (width > MAX_WIDTH) {
			scalex = (float)MAX_WIDTH/width;
			if (scalex < scale) {
				scale = scalex;
			}
			do_scale = TRUE;
		}

		if (height > MAX_HEIGHT) {
			scaley = (float)MAX_HEIGHT/height;
			if (scaley < scale) {
				scale = scaley;
			}
			do_scale = TRUE;
		}

		if (do_scale) {
			char *scaled_data = NULL;
			gsize scaled_length;

			scaled = gdk_pixbuf_scale_simple (pixbuf, width*scale, height*scale, GDK_INTERP_BILINEAR);
			gdk_pixbuf_save_to_buffer (scaled, &scaled_data, &scaled_length, "png", NULL,
						   "compression", "9", NULL);

			g_free (data);
			data = (guchar *) scaled_data;
			length = scaled_length;
		}

		/* Save the image for MDM */
		/* FIXME: I would have to read the default used by the mdmgreeter program */
		error = NULL;
		file = g_build_filename (g_get_home_dir (), ".face", NULL);
		if (g_file_set_contents (file, (gchar *)data, length, &error) == TRUE) {
			g_chmod (file, 0644);
#if HAVE_ACCOUNTSSERVICE
			act_user_set_icon_file (me->user, file);
#endif
		} else {
			g_warning ("Could not create %s: %s", file, error->message);
			g_error_free (error);
		}

		g_free (file);
		g_object_unref (pixbuf);
		g_free (data);
	} else if (me->image_changed && !me->have_image) {
		/* Update the image in the card */
		file = g_build_filename (g_get_home_dir (), ".face", NULL);

		g_unlink (file);

		g_free (file);
#if HAVE_ACCOUNTSSERVICE
		act_user_set_icon_file (me->user, "");
#endif
	}
}

static void
about_me_load_info (MateAboutMe *me)
{
	set_fingerprint_label (me->enable_fingerprint_button,
			       me->disable_fingerprint_button);
}

static void
about_me_update_preview (GtkFileChooser *chooser,
			 MateAboutMe   *me)
{
	gchar *uri;

	uri = gtk_file_chooser_get_preview_uri (chooser);

	if (uri) {
		GtkWidget *image;
		GdkPixbuf *pixbuf = NULL;
		GFile *file;
		GFileInfo *file_info;

		if (!me->thumbs)
			me->thumbs = mate_desktop_thumbnail_factory_new (MATE_DESKTOP_THUMBNAIL_SIZE_NORMAL);

		file = g_file_new_for_uri (uri);
		file_info = g_file_query_info (file,
					       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
					       G_FILE_QUERY_INFO_NONE,
					       NULL, NULL);
		g_object_unref (file);

		if (file_info != NULL) {
			const gchar *content_type;

			content_type = g_file_info_get_content_type (file_info);
			if (content_type) {
				gchar *mime_type;

				mime_type = g_content_type_get_mime_type (content_type);

				pixbuf = mate_desktop_thumbnail_factory_generate_thumbnail (me->thumbs,
										     uri,
										     mime_type);
				g_free (mime_type);
			}
			g_object_unref (file_info);
		}

		image = gtk_file_chooser_get_preview_widget (chooser);

		if (pixbuf != NULL) {
			gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
			g_object_unref (pixbuf);
		} else {
			gtk_image_set_from_icon_name (GTK_IMAGE (image),
						  "dialog-question",
						  GTK_ICON_SIZE_DIALOG);
		}
	}
	gtk_file_chooser_set_preview_widget_active (chooser, TRUE);
}

static void
about_me_image_clicked_cb (GtkWidget *button, MateAboutMe *me)
{
	GtkFileChooser *chooser_dialog;
	gint response;
	GtkBuilder *dialog;
	GtkWidget  *image;
	const gchar *chooser_dir = DATADIR"/pixmaps/faces";
	const gchar *pics_dir;
	GtkFileFilter *filter;

	dialog = me->dialog;

	chooser_dialog = GTK_FILE_CHOOSER (
			 gtk_file_chooser_dialog_new (_("Select Image"), GTK_WINDOW (WID ("about-me-dialog")),
							GTK_FILE_CHOOSER_ACTION_OPEN,
							_("No Image"), GTK_RESPONSE_NO,
							"gtk-cancel", GTK_RESPONSE_CANCEL,
							"gtk-open", GTK_RESPONSE_ACCEPT,
							NULL));
	gtk_window_set_modal (GTK_WINDOW (chooser_dialog), TRUE);
	gtk_dialog_set_default_response (GTK_DIALOG (chooser_dialog), GTK_RESPONSE_ACCEPT);

	gtk_file_chooser_add_shortcut_folder (chooser_dialog, chooser_dir, NULL);
	pics_dir = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
	if (pics_dir != NULL)
		gtk_file_chooser_add_shortcut_folder (chooser_dialog, pics_dir, NULL);

	if (!g_file_test (chooser_dir, G_FILE_TEST_IS_DIR))
		chooser_dir = g_get_home_dir ();

	gtk_file_chooser_set_current_folder (chooser_dialog, chooser_dir);
	gtk_file_chooser_set_use_preview_label (chooser_dialog,	FALSE);

	image = gtk_image_new ();
	gtk_file_chooser_set_preview_widget (chooser_dialog, image);
	gtk_widget_set_size_request (image, 128, -1);

	gtk_widget_show (image);

	g_signal_connect (chooser_dialog, "update-preview",
			  G_CALLBACK (about_me_update_preview), me);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Images"));
	gtk_file_filter_add_pixbuf_formats (filter);
	gtk_file_chooser_add_filter (chooser_dialog, filter);
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All Files"));
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter (chooser_dialog, filter);

	response = gtk_dialog_run (GTK_DIALOG (chooser_dialog));

	if (response == GTK_RESPONSE_ACCEPT) {
		gchar* filename;

		filename = gtk_file_chooser_get_filename (chooser_dialog);
		me->have_image = TRUE;
		me->image_changed = TRUE;

		e_image_chooser_set_from_file (E_IMAGE_CHOOSER (me->image_chooser), filename);
		g_free (filename);
		about_me_update_photo (me);
	} else if (response == GTK_RESPONSE_NO) {
		me->have_image = FALSE;
		me->image_changed = TRUE;
		e_image_chooser_set_from_file (E_IMAGE_CHOOSER (me->image_chooser), me->person);
		about_me_update_photo (me);
	}

	gtk_widget_destroy (GTK_WIDGET (chooser_dialog));
}

static void
about_me_image_changed_cb (GtkWidget *widget, MateAboutMe *me)
{
	me->have_image = TRUE;
	me->image_changed = TRUE;
	about_me_update_photo (me);
}

/* About Me Dialog Callbacks */

static void
about_me_icon_theme_changed (GtkWindow    *window,
			     GtkIconTheme *theme)
{
	GtkIconInfo *icon;

	icon = gtk_icon_theme_lookup_icon (me->theme, "avatar-default", 80, 0);
	if (icon != NULL) {
		g_free (me->person);
		me->person = g_strdup (gtk_icon_info_get_filename (icon));
		g_object_unref (icon);
	}

	if (me->have_image) {
#if HAVE_ACCOUNTSSERVICE
		act_user_set_icon_file (me->user, me->person);
#endif
		e_image_chooser_set_from_file (E_IMAGE_CHOOSER (me->image_chooser), me->person);
	}
}

static void
about_me_button_clicked_cb (GtkDialog *dialog, gint response_id, MateAboutMe *me)
{
	if (me->commit_timeout_id) {
		g_source_remove (me->commit_timeout_id);
	}

	about_me_destroy ();
	gtk_main_quit ();
}

static void
about_me_passwd_clicked_cb (GtkWidget *button, MateAboutMe *me)
{
	GtkBuilder *dialog;

	dialog = me->dialog;
	mate_about_me_password (GTK_WINDOW (WID ("about-me-dialog")));
}

static void
about_me_fingerprint_button_clicked_cb (GtkWidget *button, MateAboutMe *me)
{
	fingerprint_button_clicked (me->dialog,
				    me->enable_fingerprint_button,
				    me->disable_fingerprint_button);
}

#if HAVE_ACCOUNTSSERVICE
static void on_user_is_loaded_changed (ActUser *user, GParamSpec *pspec, MateAboutMe* me)
{
	if (act_user_is_loaded (user)) {
		about_me_load_photo (me);
		g_signal_handlers_disconnect_by_func (G_OBJECT (user),
				G_CALLBACK (on_user_is_loaded_changed),
				me);
	}
}
#endif

static gint
about_me_setup_dialog (void)
{
	GtkWidget    *widget;
	GtkWidget    *main_dialog;
	GtkIconInfo  *icon;
	GtkBuilder   *dialog;
	GError       *error = NULL;
	gchar        *str;
#if HAVE_ACCOUNTSSERVICE
	ActUserManager* manager;
#endif

	me = g_new0 (MateAboutMe, 1);
	me->image = NULL;

	dialog = gtk_builder_new ();
	if (gtk_builder_add_from_resource (dialog, "/org/mate/mcc/am/mate-about-me-dialog.ui", &error) == 0)
        {
                g_warning ("Could not parse UI definition: %s", error->message);
                g_error_free (error);
        }

	me->image_chooser = e_image_chooser_new_with_size (MAX_WIDTH, MAX_HEIGHT);
	gtk_container_add (GTK_CONTAINER (WID ("button-image")), me->image_chooser);

	if (dialog == NULL) {
		about_me_destroy ();
		return -1;
	}

	me->dialog = dialog;

	/* Connect the close button signal */
	main_dialog = WID ("about-me-dialog");
	g_signal_connect (main_dialog, "response",
			  G_CALLBACK (about_me_button_clicked_cb), me);

	gtk_window_set_resizable (GTK_WINDOW (main_dialog), FALSE);
	capplet_set_icon (main_dialog, "user-info");

	/* Setup theme details */
	me->screen = gtk_window_get_screen (GTK_WINDOW (main_dialog));
	me->theme = gtk_icon_theme_get_for_screen (me->screen);

	icon = gtk_icon_theme_lookup_icon (me->theme, "avatar-default", 80, 0);
	if (icon != NULL) {
		me->person = g_strdup (gtk_icon_info_get_filename (icon));
		g_object_unref (icon);
	}

	g_signal_connect_object (me->theme, "changed",
				 G_CALLBACK (about_me_icon_theme_changed),
				 main_dialog,
				 G_CONNECT_SWAPPED);

	me->login = g_strdup (g_get_user_name ());
	me->username = g_strdup (g_get_real_name ());

#if HAVE_ACCOUNTSSERVICE
	manager = act_user_manager_get_default ();
	me->user = act_user_manager_get_user (manager, me->login);
	g_signal_connect (me->user, "notify::is-loaded", G_CALLBACK (on_user_is_loaded_changed), me);
#endif
	/* Contact Tab */
	about_me_load_photo (me);

	widget = WID ("fullname");
	str = g_strdup_printf ("<b><span size=\"xx-large\">%s</span></b>", me->username);

	gtk_label_set_markup (GTK_LABEL (widget), str);
	g_free (str);

	widget = WID ("login");
	gtk_label_set_text (GTK_LABEL (widget), me->login);

	str = g_strdup_printf (_("About %s"), me->username);
	gtk_window_set_title (GTK_WINDOW (main_dialog), str);
	g_free (str);

	widget = WID ("password");
	g_signal_connect (widget, "clicked",
			  G_CALLBACK (about_me_passwd_clicked_cb), me);

	widget = WID ("button-image");
	g_signal_connect (widget, "clicked",
			  G_CALLBACK (about_me_image_clicked_cb), me);

	me->enable_fingerprint_button = WID ("enable_fingerprint_button");
	me->disable_fingerprint_button = WID ("disable_fingerprint_button");

	g_signal_connect (me->enable_fingerprint_button, "clicked",
			  G_CALLBACK (about_me_fingerprint_button_clicked_cb), me);
	g_signal_connect (me->disable_fingerprint_button, "clicked",
			  G_CALLBACK (about_me_fingerprint_button_clicked_cb), me);

	g_signal_connect (me->image_chooser, "changed",
			  G_CALLBACK (about_me_image_changed_cb), me);

	about_me_load_info (me);

	gtk_widget_show_all (main_dialog);

	return 0;
}

int
main (int argc, char **argv)
{
	int rc = 0;

	capplet_init (NULL, &argc, &argv);

	dbus_g_object_register_marshaller (fprintd_marshal_VOID__STRING_BOOLEAN,
					   G_TYPE_NONE, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_INVALID);

	rc = about_me_setup_dialog ();

	if (rc != -1) {
		gtk_main ();
	}

	return rc;
}
