# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = mouse

bin_PROGRAMS = mate-mouse-properties

mate_mouse_properties_LDADD = $(MATECC_CAPPLETS_LIBS)
BUILT_SOURCES = mate-mouse-properties-resources.h mate-mouse-properties-resources.c
nodist_mate_mouse_properties_SOURCES = \
	$(BUILT_SOURCES)
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

desktopdir = $(datadir)/applications
Desktop_in_files = mate-settings-mouse.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

mate-mouse-properties-resources.h mate-mouse-properties-resources.c: org.mate.mcc.mouse.gresource.xml Makefile $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir) $(srcdir)/org.mate.mcc.mouse.gresource.xml)
	$(AM_V_GEN) XMLLINT=$(XMLLINT) $(GLIB_COMPILE_RESOURCES) --target $@ --sourcedir $(srcdir) --generate --c-name mouse $<

AM_CPPFLAGS = \
	$(MATECC_CAPPLETS_CFLAGS) \
	-DMATELOCALEDIR="\"$(datadir)/locale\"" \
	-DMATECC_DATA_DIR="\"$(pkgdatadir)\""
CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(desktop_DATA) $(BUILT_SOURCES)
EXTRA_DIST = mate-mouse-properties.ui $(pixmap_DATA) $(Desktop_in_files) org.mate.mcc.mouse.gresource.xml

-include $(top_srcdir)/git.mk
