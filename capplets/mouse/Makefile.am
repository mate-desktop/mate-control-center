# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = mouse

bin_PROGRAMS = mate-mouse-properties

mate_mouse_properties_LDADD = $(MATECC_CAPPLETS_LIBS)
mate_mouse_properties_SOURCES =	\
	mate-mouse-properties.c	\
	msd-input-helper.c	\
	msd-input-helper.h

@INTLTOOL_DESKTOP_RULE@

pixmapdir  = $(pkgdatadir)/pixmaps
pixmap_DATA = \
	double-click-on.svg		\
	double-click-off.svg		\
	double-click-maybe.svg

uidir   = $(pkgdatadir)/ui
ui_DATA = mate-mouse-properties.ui

desktopdir = $(datadir)/applications
Desktop_in_files = mate-settings-mouse.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

AM_CPPFLAGS = \
	$(MATECC_CAPPLETS_CFLAGS) \
	-DMATELOCALEDIR="\"$(datadir)/locale\"" \
	-DMATECC_DATA_DIR="\"$(pkgdatadir)\"" \
	-DMATECC_UI_DIR="\"$(uidir)\""
CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(desktop_DATA)
EXTRA_DIST = $(ui_DATA) $(pixmap_DATA) $(Desktop_in_files)

-include $(top_srcdir)/git.mk
