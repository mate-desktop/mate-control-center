SUBDIRS = data

BUILT_SOURCES = mate-appearance-properties-resources.h mate-appearance-properties-resources.c

# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = appearance

bin_PROGRAMS = mate-appearance-properties

nodist_mate_appearance_properties_SOURCES = \
	$(BUILT_SOURCES)
mate_appearance_properties_SOURCES = \
	appearance.h \
	appearance-desktop.c \
	appearance-desktop.h \
	appearance-font.c \
	appearance-font.h \
	appearance-main.c \
	appearance-themes.c \
	appearance-themes.h \
	appearance-style.c \
	appearance-style.h \
	appearance-ui.c \
	appearance-ui.h \
	appearance-support.c \
	appearance-support.h \
	mate-wp-info.c \
	mate-wp-info.h \
	mate-wp-item.c \
	mate-wp-item.h \
	mate-wp-xml.c \
	mate-wp-xml.h \
	theme-installer.c \
	theme-installer.h \
	theme-save.c \
	theme-save.h \
	theme-util.c \
	theme-util.h

AM_CFLAGS = -DMATE_DESKTOP_USE_UNSTABLE_API

mate_appearance_properties_LDADD = \
	$(top_builddir)/libwindow-settings/libmate-window-settings.la \
	$(top_builddir)/capplets/common/libcommon.la \
	$(MATECC_CAPPLETS_LIBS) \
	$(FONT_CAPPLET_LIBS) \
	$(MARCO_LIBS)
mate_appearance_properties_LDFLAGS = -export-dynamic

pixmapdir = $(pkgdatadir)/pixmaps
wallpaperdir = $(datadir)/mate-background-properties
backgrounddir = $(datadir)/backgrounds

AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(MARCO_CFLAGS) \
	$(MATECC_CAPPLETS_CFLAGS) \
	$(FONT_CAPPLET_CFLAGS) \
	-DMATECC_DATA_DIR="\"$(pkgdatadir)\"" \
	-DMATECC_PIXMAP_DIR="\"$(pixmapdir)\"" \
	-DWALLPAPER_DATADIR="\"$(wallpaperdir)\"" \
	-DBACKGROUND_DATADIR="\"$(backgrounddir)\""

mate-appearance-properties-resources.h mate-appearance-properties-resources.c: org.mate.mcc.appearance.gresource.xml Makefile $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir) $(srcdir)/org.mate.mcc.appearance.gresource.xml)
	$(AM_V_GEN) XMLLINT=$(XMLLINT) $(GLIB_COMPILE_RESOURCES) --target $@ --sourcedir $(srcdir) --generate --c-name appearance $<

CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(BUILT_SOURCES)
EXTRA_DIST = org.mate.mcc.appearance.gresource.xml

-include $(top_srcdir)/git.mk
