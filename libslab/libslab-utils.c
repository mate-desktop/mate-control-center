#include "libslab-utils.h"

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <gtk/gtk.h>

#define DESKTOP_ITEM_TERMINAL_EMULATOR_FLAG "TerminalEmulator"
#define ALTERNATE_DOCPATH_KEY               "DocPath"

static FILE *checkpoint_file;

MateDesktopItem *
libslab_mate_desktop_item_new_from_unknown_id (const gchar *id)
{
	MateDesktopItem *item;
	gchar            *basename;

	GError *error = NULL;


	if (! id)
		return NULL;

	item = mate_desktop_item_new_from_uri (id, 0, & error);

	if (! error)
		return item;
	else {
		g_error_free (error);
		error = NULL;
	}

	item = mate_desktop_item_new_from_file (id, 0, & error);

	if (! error)
		return item;
	else {
		g_error_free (error);
		error = NULL;
	}

	item = mate_desktop_item_new_from_basename (id, 0, & error);

	if (! error)
		return item;
	else {
		g_error_free (error);
		error = NULL;
	}

	basename = g_strrstr (id, "/");

	if (basename) {
		basename++;

		item = mate_desktop_item_new_from_basename (basename, 0, &error);

		if (! error)
			return item;
		else {
			g_error_free (error);
			error = NULL;
		}
	}

	return NULL;
}

/* Ugh, here we don't have knowledge of the screen that is being used.  So, do
 * what we can to find it.
 */
GdkScreen *
libslab_get_current_screen (void)
{
	GdkEvent *event;
	GdkScreen *screen = NULL;

	event = gtk_get_current_event ();
	if (event) {
		if (event->any.window)
			screen = gtk_window_get_screen (GTK_WINDOW (event->any.window));

		gdk_event_free (event);
	}

	if (!screen)
		screen = gdk_screen_get_default ();

	return screen;
}

guint32
libslab_get_current_time_millis ()
{
	GTimeVal t_curr;

	g_get_current_time (& t_curr);

	return 1000L * t_curr.tv_sec + t_curr.tv_usec / 1000L;
}

gint
libslab_strcmp (const gchar *a, const gchar *b)
{
	if (! a && ! b)
		return 0;

	if (! a)
		return strcmp ("", b);

	if (! b)
		return strcmp (a, "");

	return strcmp (a, b);
}

void
libslab_handle_g_error (GError **error, const gchar *msg_format, ...)
{
	gchar   *msg;
	va_list  args;


	va_start (args, msg_format);
	msg = g_strdup_vprintf (msg_format, args);
	va_end (args);

	if (*error) {
		g_log (
			G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
			"\nGError raised: [%s]\nuser_message: [%s]\n", (*error)->message, msg);

		g_error_free (*error);

		*error = NULL;
	}
	else
		g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "\nerror raised: [%s]\n", msg);

	g_free (msg);
}

static guint thumbnail_factory_idle_id;
static MateDesktopThumbnailFactory *thumbnail_factory;

static void
create_thumbnail_factory (void)
{
	/* The thumbnail_factory may already have been created by an applet
	 * instance that was launched before the current one.
	 */
	if (thumbnail_factory != NULL)
		return;

	libslab_checkpoint ("create_thumbnail_factory(): start");

	thumbnail_factory = mate_desktop_thumbnail_factory_new (MATE_DESKTOP_THUMBNAIL_SIZE_NORMAL);

	libslab_checkpoint ("create_thumbnail_factory(): end");
}

static gboolean
init_thumbnail_factory_idle_cb (gpointer data)
{
	create_thumbnail_factory ();
	thumbnail_factory_idle_id = 0;
	return FALSE;
}

void
libslab_checkpoint (const char *format, ...)
{
	va_list args;
	struct timeval tv;
	struct tm tm;
	struct rusage rusage;

	if (!checkpoint_file)
		return;

	gettimeofday (&tv, NULL);
	tm = *localtime (&tv.tv_sec);

	getrusage (RUSAGE_SELF, &rusage);

	fprintf (checkpoint_file,
		 "%02d:%02d:%02d.%04d (user:%d.%04d, sys:%d.%04d) - ",
		 (int) tm.tm_hour,
		 (int) tm.tm_min,
		 (int) tm.tm_sec,
		 (int) (tv.tv_usec / 100),
		 (int) rusage.ru_utime.tv_sec,
		 (int) (rusage.ru_utime.tv_usec / 100),
		 (int) rusage.ru_stime.tv_sec,
		 (int) (rusage.ru_stime.tv_usec / 100));

	va_start (args, format);
	vfprintf (checkpoint_file, format, args);
	va_end (args);

	fputs ("\n", checkpoint_file);
	fflush (checkpoint_file);
}
