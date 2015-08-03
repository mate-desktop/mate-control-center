/*
 * Copyright (C) 2007 The GNOME Foundation
 * Written by Jonathan Blandford <jrb@gnome.org>
 *            Jens Granseuer <jensgr@gmx.net>
 * All Rights Reserved
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "appearance.h"

#include <string.h>
#include <stdarg.h>
#include <math.h>

#ifdef HAVE_XFT2
	#include <gdk/gdkx.h>
	#include <X11/Xft/Xft.h>
#endif /* HAVE_XFT2 */

#include <glib/gi18n.h>
#include <gio/gio.h>

#include "capplet-util.h"

#ifdef HAVE_XFT2
/* X servers sometimes lie about the screen's physical dimensions, so we cannot
 * compute an accurate DPI value.  When this happens, the user gets fonts that
 * are too huge or too tiny.  So, we see what the server returns:  if it reports
 * something outside of the range [DPI_LOW_REASONABLE_VALUE,
 * DPI_HIGH_REASONABLE_VALUE], then we assume that it is lying and we use
 * DPI_FALLBACK instead.
 *
 * See get_dpi_from_mate_conf_or_server() below, and also
 * https://bugzilla.novell.com/show_bug.cgi?id=217790
 */
#define DPI_FALLBACK 96
#define DPI_LOW_REASONABLE_VALUE 50
#define DPI_HIGH_REASONABLE_VALUE 500
#endif /* HAVE_XFT2 */

static gboolean in_change = FALSE;

#define MAX_FONT_POINT_WITHOUT_WARNING 32
#define MAX_FONT_SIZE_WITHOUT_WARNING MAX_FONT_POINT_WITHOUT_WARNING * 1024

#ifdef HAVE_XFT2

#if !GTK_CHECK_VERSION (3, 0, 0)
/*
 * Code for displaying previews of font rendering with various Xft options
 */

static void sample_size_request(GtkWidget* darea, GtkRequisition* requisition)
{
	GdkPixbuf* pixbuf = g_object_get_data(G_OBJECT(darea), "sample-pixbuf");

	requisition->width = gdk_pixbuf_get_width(pixbuf) + 2;
	requisition->height = gdk_pixbuf_get_height(pixbuf) + 2;
}
#endif

#if GTK_CHECK_VERSION (3, 0, 0)
static void sample_draw(GtkWidget* darea, cairo_t* cr)
#else
static void sample_expose(GtkWidget* darea, GdkEventExpose* expose)
#endif
{
	GtkAllocation allocation;
	GdkPixbuf* pixbuf = g_object_get_data(G_OBJECT(darea), "sample-pixbuf");
	int width = gdk_pixbuf_get_width(pixbuf);
	int height = gdk_pixbuf_get_height(pixbuf);

	gtk_widget_get_allocation (darea, &allocation);
	int x = (allocation.width - width) / 2;
	int y = (allocation.height - height) / 2;

	GdkColor black, white;
	gdk_color_parse ("black", &black);
	gdk_color_parse ("white", &white);

#if !GTK_CHECK_VERSION (3, 0, 0)
	cairo_t *cr = gdk_cairo_create (expose->window);
#endif
	cairo_set_line_width (cr, 1);
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);

	gdk_cairo_set_source_color (cr, &white);
	cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
	cairo_fill_preserve (cr);
	gdk_cairo_set_source_color (cr, &black);
	cairo_stroke (cr);

	gdk_cairo_set_source_pixbuf(cr, pixbuf, x, y);
	cairo_paint(cr);

#if !GTK_CHECK_VERSION (3, 0, 0)
	cairo_destroy (cr);
#endif
}

typedef enum {
	ANTIALIAS_NONE,
	ANTIALIAS_GRAYSCALE,
	ANTIALIAS_RGBA
} Antialiasing;

typedef enum {
	HINT_NONE,
	HINT_SLIGHT,
	HINT_MEDIUM,
	HINT_FULL
} Hinting;

typedef enum {
	RGBA_RGB,
	RGBA_BGR,
	RGBA_VRGB,
	RGBA_VBGR
} RgbaOrder;

static XftFont* open_pattern(FcPattern* pattern, Antialiasing antialiasing, Hinting hinting)
{
	#ifdef FC_HINT_STYLE
		static const int hintstyles[] = {
			FC_HINT_NONE, FC_HINT_SLIGHT, FC_HINT_MEDIUM, FC_HINT_FULL
		};
	#endif /* FC_HINT_STYLE */

	FcPattern* res_pattern;
	FcResult result;
	XftFont* font;

	Display* xdisplay = gdk_x11_get_default_xdisplay();
	int screen = gdk_x11_get_default_screen();

	res_pattern = XftFontMatch(xdisplay, screen, pattern, &result);
	
	if (res_pattern == NULL)
	{
		return NULL;
	}

	FcPatternDel(res_pattern, FC_HINTING);
	FcPatternAddBool(res_pattern, FC_HINTING, hinting != HINT_NONE);

	#ifdef FC_HINT_STYLE
		FcPatternDel(res_pattern, FC_HINT_STYLE);
		FcPatternAddInteger(res_pattern, FC_HINT_STYLE, hintstyles[hinting]);
	#endif /* FC_HINT_STYLE */

	FcPatternDel(res_pattern, FC_ANTIALIAS);
	FcPatternAddBool(res_pattern, FC_ANTIALIAS, antialiasing != ANTIALIAS_NONE);

	FcPatternDel(res_pattern, FC_RGBA);
	FcPatternAddInteger(res_pattern, FC_RGBA, antialiasing == ANTIALIAS_RGBA ? FC_RGBA_RGB : FC_RGBA_NONE);

	FcPatternDel(res_pattern, FC_DPI);
	FcPatternAddInteger(res_pattern, FC_DPI, 96);

	font = XftFontOpenPattern(xdisplay, res_pattern);
	
	if (!font)
	{
		FcPatternDestroy(res_pattern);
	}

	return font;
}

static void setup_font_sample(GtkWidget* darea, Antialiasing antialiasing, Hinting hinting)
{
	const char* string1 = "abcfgop AO ";
	const char* string2 = "abcfgop";

	XftColor black, white;
	XRenderColor rendcolor;

	Display* xdisplay = gdk_x11_get_default_xdisplay();

#if GTK_CHECK_VERSION (3, 0, 0)
	Colormap xcolormap = DefaultColormap(xdisplay, 0);
#else
	GdkColormap* colormap = gdk_rgb_get_colormap();
	Colormap xcolormap = GDK_COLORMAP_XCOLORMAP(colormap);
#endif

#if GTK_CHECK_VERSION (3, 0, 0)
	GdkVisual* visual = gdk_visual_get_system ();
#else
	GdkVisual* visual = gdk_colormap_get_visual(colormap);
#endif
	Visual* xvisual = GDK_VISUAL_XVISUAL(visual);

	FcPattern* pattern;
	XftFont* font1;
	XftFont* font2;
	XGlyphInfo extents1 = { 0 };
	XGlyphInfo extents2 = { 0 };
#if !GTK_CHECK_VERSION (3, 0, 0)
	GdkPixmap* pixmap;
#endif
	XftDraw* draw;
	GdkPixbuf* tmp_pixbuf;
	GdkPixbuf* pixbuf;

	int width, height;
	int ascent, descent;

	pattern = FcPatternBuild (NULL,
		FC_FAMILY, FcTypeString, "Serif",
		FC_SLANT, FcTypeInteger, FC_SLANT_ROMAN,
		FC_SIZE, FcTypeDouble, 18.,
		NULL);
	font1 = open_pattern (pattern, antialiasing, hinting);
	FcPatternDestroy (pattern);

	pattern = FcPatternBuild (NULL,
		FC_FAMILY, FcTypeString, "Serif",
		FC_SLANT, FcTypeInteger, FC_SLANT_ITALIC,
		FC_SIZE, FcTypeDouble, 20.,
		NULL);
	font2 = open_pattern (pattern, antialiasing, hinting);
	FcPatternDestroy (pattern);

	ascent = 0;
	descent = 0;
	
	if (font1)
	{
		XftTextExtentsUtf8 (xdisplay, font1, (unsigned char*) string1,
		strlen (string1), &extents1);
		ascent = MAX (ascent, font1->ascent);
		descent = MAX (descent, font1->descent);
	}

	if (font2)
	{
		XftTextExtentsUtf8 (xdisplay, font2, (unsigned char*) string2, strlen (string2), &extents2);
		ascent = MAX (ascent, font2->ascent);
		descent = MAX (descent, font2->descent);
	}

	width = extents1.xOff + extents2.xOff + 4;
	height = ascent + descent + 2;

#if !GTK_CHECK_VERSION (3, 0, 0)
	pixmap = gdk_pixmap_new (NULL, width, height, gdk_visual_get_depth (visual));
#endif

#if GTK_CHECK_VERSION (3, 0, 0)
	draw = XftDrawCreate (xdisplay, GDK_WINDOW_XID (gdk_screen_get_root_window (gdk_screen_get_default ())), xvisual, xcolormap);
#else
	draw = XftDrawCreate (xdisplay, GDK_DRAWABLE_XID (pixmap), xvisual, xcolormap);
#endif

	rendcolor.red = 0;
	rendcolor.green = 0;
	rendcolor.blue = 0;
	rendcolor.alpha = 0xffff;
	
	XftColorAllocValue(xdisplay, xvisual, xcolormap, &rendcolor, &black);

	rendcolor.red = 0xffff;
	rendcolor.green = 0xffff;
	rendcolor.blue = 0xffff;
	rendcolor.alpha = 0xffff;
	
	XftColorAllocValue(xdisplay, xvisual, xcolormap, &rendcolor, &white);
	XftDrawRect(draw, &white, 0, 0, width, height);
	
	if (font1)
	{
		XftDrawStringUtf8(draw, &black, font1, 2, 2 + ascent, (unsigned char*) string1, strlen(string1));
	}
	
	if (font2)
	{
		XftDrawStringUtf8(draw, &black, font2, 2 + extents1.xOff, 2 + ascent, (unsigned char*) string2, strlen(string2));
	}

	XftDrawDestroy(draw);

	if (font1)
	{
		XftFontClose(xdisplay, font1);
	}
	
	if (font2)
	{
		XftFontClose(xdisplay, font2);
	}

#if GTK_CHECK_VERSION (3, 0, 0)
	tmp_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE,8, width, height);
#else
	tmp_pixbuf = gdk_pixbuf_get_from_drawable(NULL, pixmap, colormap, 0, 0, 0, 0, width, height);
#endif
	pixbuf = gdk_pixbuf_scale_simple(tmp_pixbuf, 1 * width, 1 * height, GDK_INTERP_TILES);

#if !GTK_CHECK_VERSION (3, 0, 0)
	g_object_unref(pixmap);
#endif
	g_object_unref(tmp_pixbuf);

	g_object_set_data_full(G_OBJECT(darea), "sample-pixbuf", pixbuf, (GDestroyNotify) g_object_unref);

#if GTK_CHECK_VERSION (3, 0, 0)
	gtk_widget_set_size_request  (GTK_WIDGET(darea), width + 2, height + 2);
	g_signal_connect(darea, "draw", G_CALLBACK(sample_draw), NULL);
#else
	g_signal_connect(darea, "size_request", G_CALLBACK(sample_size_request), NULL);
	g_signal_connect(darea, "expose_event", G_CALLBACK(sample_expose), NULL);
#endif
}

/*
 * Code implementing a group of radio buttons with different Xft option combinations.
 * If one of the buttons is matched by the GSettings key, we pick it. Otherwise we
 * show the group as inconsistent.
 */

typedef struct {
  Antialiasing antialiasing;
  Hinting hinting;
  GtkToggleButton *radio;
} FontPair;

static GSList *font_pairs = NULL;

static void
font_render_load (GSettings *settings)
{
  Antialiasing antialiasing;
  Hinting hinting;
  gboolean inconsistent = TRUE;
  GSList *tmp_list;

  antialiasing = g_settings_get_enum (settings, FONT_ANTIALIASING_KEY);
  hinting = g_settings_get_enum (settings, FONT_HINTING_KEY);

  in_change = TRUE;

  for (tmp_list = font_pairs; tmp_list; tmp_list = tmp_list->next) {
    FontPair *pair = tmp_list->data;

    if (antialiasing == pair->antialiasing && hinting == pair->hinting) {
      gtk_toggle_button_set_active (pair->radio, TRUE);
      inconsistent = FALSE;
      break;
    }
  }

  for (tmp_list = font_pairs; tmp_list; tmp_list = tmp_list->next) {
    FontPair *pair = tmp_list->data;

    gtk_toggle_button_set_inconsistent (pair->radio, inconsistent);
  }

  in_change = FALSE;
}

static void
font_render_changed (GSettings *settings,
                     gchar     *key,
                     gpointer   user_data)
{
  font_render_load (settings);
}

static void
font_radio_toggled (GtkToggleButton *toggle_button,
		    FontPair        *pair)
{
  if (!in_change) {
    GSettings *settings = g_settings_new (FONT_RENDER_SCHEMA);

    g_settings_set_enum (settings, FONT_ANTIALIASING_KEY, pair->antialiasing);
    g_settings_set_enum (settings, FONT_HINTING_KEY, pair->hinting);

    /* Restore back to the previous state until we get notification */
    font_render_load (settings);
    g_object_unref (settings);
  }
}

static void
setup_font_pair (GtkWidget    *radio,
		 GtkWidget    *darea,
		 Antialiasing  antialiasing,
		 Hinting       hinting)
{
  FontPair *pair = g_new (FontPair, 1);

  pair->antialiasing = antialiasing;
  pair->hinting = hinting;
  pair->radio = GTK_TOGGLE_BUTTON (radio);

  setup_font_sample (darea, antialiasing, hinting);
  font_pairs = g_slist_prepend (font_pairs, pair);

  g_signal_connect (radio, "toggled",
		    G_CALLBACK (font_radio_toggled), pair);
}
#endif /* HAVE_XFT2 */

static void
marco_titlebar_load_sensitivity (AppearanceData *data)
{
  gtk_widget_set_sensitive (appearance_capplet_get_widget (data, "window_title_font"),
			    !g_settings_get_boolean (data->marco_settings,
						    WINDOW_TITLE_USES_SYSTEM_KEY));
}

static void
marco_changed (GSettings *settings,
	       gchar     *entry,
	       gpointer   user_data)
{
  marco_titlebar_load_sensitivity (user_data);
}

#ifdef HAVE_XFT2
/*
 * EnumGroup - a group of radio buttons for a gsettings enum
 */
typedef struct
{
  GSettings *settings;
  GSList *items;
  gchar *settings_key;
  gulong settings_signal_id;
} EnumGroup;

typedef struct
{
  EnumGroup *group;
  GtkToggleButton *widget;
  int value;
} EnumItem;

static void
enum_group_load (EnumGroup *group)
{
  gint val = g_settings_get_enum (group->settings, group->settings_key);
  GSList *tmp_list;

  in_change = TRUE;

  for (tmp_list = group->items; tmp_list; tmp_list = tmp_list->next) {
    EnumItem *item = tmp_list->data;

    if (val == item->value)
      gtk_toggle_button_set_active (item->widget, TRUE);
  }

  in_change = FALSE;
}

static void
enum_group_changed (GSettings *settings,
		    gchar     *key,
		    gpointer  user_data)
{
  enum_group_load (user_data);
}

static void
enum_item_toggled (GtkToggleButton *toggle_button,
		   EnumItem        *item)
{
  EnumGroup *group = item->group;

  if (!in_change) {
    g_settings_set_enum (group->settings, group->settings_key, item->value);
  }

  /* Restore back to the previous state until we get notification */
  enum_group_load (group);
}

static EnumGroup *
enum_group_create (GSettings           *settings,
		   const gchar         *settings_key,
		   GtkWidget           *first_widget,
		   ...)
{
  EnumGroup *group;
  GtkWidget *widget;
  va_list args;

  group = g_new (EnumGroup, 1);

  group->settings = g_object_ref (settings);
  group->settings_key = g_strdup (settings_key);
  group->items = NULL;

  va_start (args, first_widget);

  widget = first_widget;
  while (widget) {
    EnumItem *item;

    item = g_new (EnumItem, 1);
    item->group = group;
    item->widget = GTK_TOGGLE_BUTTON (widget);
    item->value = va_arg (args, int);

    g_signal_connect (item->widget, "toggled",
		      G_CALLBACK (enum_item_toggled), item);

    group->items = g_slist_prepend (group->items, item);

    widget = va_arg (args, GtkWidget *);
  }

  va_end (args);

  enum_group_load (group);

  gchar *signal_name = g_strdup_printf("changed::%s", settings_key);
  group->settings_signal_id = g_signal_connect (settings, signal_name,
                                                G_CALLBACK (enum_group_changed), group);
  g_free (signal_name);

  return group;
}

static void
enum_group_destroy (EnumGroup *group)
{
  g_signal_handler_disconnect (group->settings, group->settings_signal_id);
  g_clear_object (&group->settings);
  group->settings_signal_id = 0;
  g_free (group->settings_key);

  g_slist_foreach (group->items, (GFunc) g_free, NULL);
  g_slist_free (group->items);

  g_free (group);
}

static double
dpi_from_pixels_and_mm (int pixels, int mm)
{
  double dpi;

  if (mm >= 1)
    dpi = pixels / (mm / 25.4);
  else
    dpi = 0;

  return dpi;
}

static double
get_dpi_from_x_server (void)
{
  GdkScreen *screen;
  double dpi;

  screen = gdk_screen_get_default ();
  if (screen) {
    double width_dpi, height_dpi;

    width_dpi = dpi_from_pixels_and_mm (gdk_screen_get_width (screen),
					gdk_screen_get_width_mm (screen));
    height_dpi = dpi_from_pixels_and_mm (gdk_screen_get_height (screen),
					 gdk_screen_get_height_mm (screen));

    if (width_dpi < DPI_LOW_REASONABLE_VALUE || width_dpi > DPI_HIGH_REASONABLE_VALUE ||
        height_dpi < DPI_LOW_REASONABLE_VALUE || height_dpi > DPI_HIGH_REASONABLE_VALUE)
      dpi = DPI_FALLBACK;
    else
      dpi = (width_dpi + height_dpi) / 2.0;
  } else {
    /* Huh!?  No screen? */
    dpi = DPI_FALLBACK;
  }

  return dpi;
}

/*
 * The font rendering details dialog
 */
static void
dpi_load (GSettings     *settings,
	  GtkSpinButton *spinner)
{
  gdouble value = g_settings_get_double (settings, FONT_DPI_KEY);
  gdouble dpi;

  if (value != 0)
    dpi = value;
  else
    dpi = get_dpi_from_x_server ();

  if (dpi < DPI_LOW_REASONABLE_VALUE)
    dpi = DPI_LOW_REASONABLE_VALUE;

  in_change = TRUE;
  gtk_spin_button_set_value (spinner, dpi);
  in_change = FALSE;
}

static void
dpi_changed (GSettings *settings,
	     gchar     *key,
	     gpointer   user_data)
{
  dpi_load (settings, user_data);
}

static void
dpi_value_changed (GtkSpinButton *spinner,
		   GSettings     *settings)
{
  /* Like any time when using a spin button with GSettings, there is
   * a race condition here. When we change, we send the new
   * value to GSettings, then restore to the old value until
   * we get a response to emulate the proper model/view behavior.
   *
   * If the user changes the value faster than responses are
   * received from GSettings, this may cause mildly strange effects.
   */
  if (!in_change) {
    gdouble new_dpi = gtk_spin_button_get_value (spinner);

    g_settings_set_double (settings, FONT_DPI_KEY, new_dpi);

    dpi_load (settings, spinner);
  }
}

static void
cb_details_response (GtkDialog *dialog, gint response_id)
{
  if (response_id == GTK_RESPONSE_HELP) {
    capplet_help (GTK_WINDOW (dialog),
		  "goscustdesk-38");
  } else
    gtk_widget_hide (GTK_WIDGET (dialog));
}

static void
cb_show_details (GtkWidget *button,
		 AppearanceData *data)
{
  if (!data->font_details) {
    GtkAdjustment *adjustment;
    GtkWidget *widget;
    EnumGroup *group;

    data->font_details = appearance_capplet_get_widget (data, "render_details");

    gtk_window_set_transient_for (GTK_WINDOW (data->font_details),
                                  GTK_WINDOW (appearance_capplet_get_widget (data, "appearance_window")));

    widget = appearance_capplet_get_widget (data, "dpi_spinner");

    /* pick a sensible maximum dpi */
    adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget));
    gtk_adjustment_set_lower (adjustment, DPI_LOW_REASONABLE_VALUE);
    gtk_adjustment_set_upper (adjustment, DPI_HIGH_REASONABLE_VALUE);
    gtk_adjustment_set_step_increment (adjustment, 1);

    dpi_load (data->font_settings, GTK_SPIN_BUTTON (widget));
    g_signal_connect (widget, "value_changed",
		      G_CALLBACK (dpi_value_changed), data->font_settings);

    g_signal_connect (data->font_settings, "changed::" FONT_DPI_KEY, G_CALLBACK (dpi_changed), widget);

    setup_font_sample (appearance_capplet_get_widget (data, "antialias_none_sample"),      ANTIALIAS_NONE,      HINT_FULL);
    setup_font_sample (appearance_capplet_get_widget (data, "antialias_grayscale_sample"), ANTIALIAS_GRAYSCALE, HINT_FULL);
    setup_font_sample (appearance_capplet_get_widget (data, "antialias_subpixel_sample"),  ANTIALIAS_RGBA,      HINT_FULL);

    group = enum_group_create (
    	data->font_settings, FONT_ANTIALIASING_KEY,
	appearance_capplet_get_widget (data, "antialias_none_radio"),      ANTIALIAS_NONE,
	appearance_capplet_get_widget (data, "antialias_grayscale_radio"), ANTIALIAS_GRAYSCALE,
	appearance_capplet_get_widget (data, "antialias_subpixel_radio"),  ANTIALIAS_RGBA,
	NULL);
    data->font_groups = g_slist_prepend (data->font_groups, group);

    setup_font_sample (appearance_capplet_get_widget (data, "hint_none_sample"),   ANTIALIAS_GRAYSCALE, HINT_NONE);
    setup_font_sample (appearance_capplet_get_widget (data, "hint_slight_sample"), ANTIALIAS_GRAYSCALE, HINT_SLIGHT);
    setup_font_sample (appearance_capplet_get_widget (data, "hint_medium_sample"), ANTIALIAS_GRAYSCALE, HINT_MEDIUM);
    setup_font_sample (appearance_capplet_get_widget (data, "hint_full_sample"),   ANTIALIAS_GRAYSCALE, HINT_FULL);

    group = enum_group_create (data->font_settings, FONT_HINTING_KEY,
                               appearance_capplet_get_widget (data, "hint_none_radio"),   HINT_NONE,
                               appearance_capplet_get_widget (data, "hint_slight_radio"), HINT_SLIGHT,
                               appearance_capplet_get_widget (data, "hint_medium_radio"), HINT_MEDIUM,
                               appearance_capplet_get_widget (data, "hint_full_radio"),   HINT_FULL,
                               NULL);
    data->font_groups = g_slist_prepend (data->font_groups, group);

    gtk_image_set_from_file (GTK_IMAGE (appearance_capplet_get_widget (data, "subpixel_rgb_image")),
                             MATECC_PIXMAP_DIR "/subpixel-rgb.png");
    gtk_image_set_from_file (GTK_IMAGE (appearance_capplet_get_widget (data, "subpixel_bgr_image")),
                             MATECC_PIXMAP_DIR "/subpixel-bgr.png");
    gtk_image_set_from_file (GTK_IMAGE (appearance_capplet_get_widget (data, "subpixel_vrgb_image")),
                             MATECC_PIXMAP_DIR "/subpixel-vrgb.png");
    gtk_image_set_from_file (GTK_IMAGE (appearance_capplet_get_widget (data, "subpixel_vbgr_image")),
                             MATECC_PIXMAP_DIR "/subpixel-vbgr.png");

    group = enum_group_create (data->font_settings, FONT_RGBA_ORDER_KEY,
                               appearance_capplet_get_widget (data, "subpixel_rgb_radio"),  RGBA_RGB,
                               appearance_capplet_get_widget (data, "subpixel_bgr_radio"),  RGBA_BGR,
                               appearance_capplet_get_widget (data, "subpixel_vrgb_radio"), RGBA_VRGB,
                               appearance_capplet_get_widget (data, "subpixel_vbgr_radio"), RGBA_VBGR,
                               NULL);
    data->font_groups = g_slist_prepend (data->font_groups, group);

    g_signal_connect (G_OBJECT (data->font_details),
		      "response",
		      G_CALLBACK (cb_details_response), NULL);
    g_signal_connect (G_OBJECT (data->font_details),
		      "delete_event",
		      G_CALLBACK (gtk_true), NULL);
  }

  gtk_window_present (GTK_WINDOW (data->font_details));
}
#endif /* HAVE_XFT2 */

void font_init(AppearanceData* data)
{
	GtkWidget* widget;

	data->font_details = NULL;
	data->font_groups = NULL;

	widget = appearance_capplet_get_widget(data, "application_font");
	g_settings_bind (data->interface_settings,
			 GTK_FONT_KEY,
			 G_OBJECT (widget),
			 "font-name",
			 G_SETTINGS_BIND_DEFAULT);

	widget = appearance_capplet_get_widget (data, "document_font");
	g_settings_bind (data->interface_settings,
			 DOCUMENT_FONT_KEY,
			 G_OBJECT (widget),
			 "font-name",
			 G_SETTINGS_BIND_DEFAULT);

	widget = appearance_capplet_get_widget (data, "desktop_font");

	if (data->caja_settings)
		g_settings_bind (data->caja_settings,
				 DESKTOP_FONT_KEY,
				 G_OBJECT (widget),
				 "font-name",
				 G_SETTINGS_BIND_DEFAULT);
	else
		gtk_widget_set_sensitive (widget, FALSE);

	widget = appearance_capplet_get_widget (data, "window_title_font");
	g_settings_bind (data->marco_settings,
			 WINDOW_TITLE_FONT_KEY,
			 G_OBJECT (widget),
			 "font-name",
			 G_SETTINGS_BIND_DEFAULT);

	widget = appearance_capplet_get_widget (data, "monospace_font");
	g_settings_bind (data->interface_settings,
			 MONOSPACE_FONT_KEY,
			 G_OBJECT (widget),
			 "font-name",
			 G_SETTINGS_BIND_DEFAULT);

	g_signal_connect (data->marco_settings,
			  "changed::" WINDOW_TITLE_USES_SYSTEM_KEY,
			  G_CALLBACK (marco_changed),
			  data);

	marco_titlebar_load_sensitivity(data);

	#ifdef HAVE_XFT2
		setup_font_pair(appearance_capplet_get_widget(data, "monochrome_radio"), appearance_capplet_get_widget (data, "monochrome_sample"), ANTIALIAS_NONE, HINT_FULL);
		setup_font_pair(appearance_capplet_get_widget(data, "best_shapes_radio"), appearance_capplet_get_widget (data, "best_shapes_sample"), ANTIALIAS_GRAYSCALE, HINT_MEDIUM);
		setup_font_pair(appearance_capplet_get_widget(data, "best_contrast_radio"), appearance_capplet_get_widget (data, "best_contrast_sample"), ANTIALIAS_GRAYSCALE, HINT_FULL);
		setup_font_pair(appearance_capplet_get_widget(data, "subpixel_radio"), appearance_capplet_get_widget (data, "subpixel_sample"), ANTIALIAS_RGBA, HINT_FULL);

		font_render_load (data->font_settings);

		g_signal_connect (data->font_settings, "changed", G_CALLBACK (font_render_changed), NULL);

		g_signal_connect (appearance_capplet_get_widget (data, "details_button"), "clicked", G_CALLBACK (cb_show_details), data);
	#else /* !HAVE_XFT2 */
		gtk_widget_hide (appearance_capplet_get_widget (data, "font_render_frame"));
	#endif /* HAVE_XFT2 */
}

void font_shutdown(AppearanceData* data)
{
	g_slist_foreach(data->font_groups, (GFunc) enum_group_destroy, NULL);
	g_slist_free(data->font_groups);
	g_slist_foreach(font_pairs, (GFunc) g_free, NULL);
	g_slist_free(font_pairs);
}
