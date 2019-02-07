# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = mate-default-applications-properties

bin_PROGRAMS = mate-default-applications-properties

mate_default_applications_properties_LDADD = $(MATECC_CAPPLETS_LIBS)
mate_default_applications_properties_SOURCES = \
	mate-da-capplet.c mate-da-capplet.h

@INTLTOOL_DESKTOP_RULE@

uidir = $(pkgdatadir)/ui
dist_ui_DATA = mate-default-applications-properties.ui

desktopdir = $(datadir)/applications
Desktop_in_files = mate-default-applications-properties.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mate-default-applications.pc

AM_CPPFLAGS = \
	$(MATECC_CAPPLETS_CFLAGS) \
	$(DEFAULT_APPLICATIONS_CAPPLET_CFLAGS) \
	-DAPPLICATIONSDIR=\""$(datadir)/applications"\"\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"\
	-DMATECC_UI_DIR=\""$(uidir)"\"

CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(desktop_DATA) $(autostart_DATA)
EXTRA_DIST = mate-default-applications.pc.in $(Desktop_in_files)

-include $(top_srcdir)/git.mk
