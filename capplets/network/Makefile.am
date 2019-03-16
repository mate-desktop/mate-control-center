# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = network

bin_PROGRAMS = mate-network-properties

mate_network_properties_SOURCES = mate-network-properties.c
mate_network_properties_LDADD = $(MATECC_CAPPLETS_LIBS)

@INTLTOOL_DESKTOP_RULE@

uidir = $(pkgdatadir)/ui
dist_ui_DATA = mate-network-properties.ui

desktopdir = $(datadir)/applications
desktop_in_files = mate-network-properties.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

AM_CPPFLAGS = \
	$(MATECC_CAPPLETS_CFLAGS) \
	-DMATELOCALEDIR="\"$(datadir)/locale\"" \
	-DMATECC_UI_DIR="\"$(uidir)\""

CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(desktop_DATA)
EXTRA_DIST = $(desktop_in_files)

-include $(top_srcdir)/git.mk
