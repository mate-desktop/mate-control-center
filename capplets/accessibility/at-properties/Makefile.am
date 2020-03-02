bin_PROGRAMS = mate-at-properties

ui_files = at-enable-dialog.ui

BUILT_SOURCES = \
	mate-at-properties-resources.c \
	mate-at-properties-resources.h
nodist_mate_at_properties_SOURCES= $(BUILT_SOURCES)
mate_at_properties_LDADD = $(AT_CAPPLET_LIBS) $(MATECC_CAPPLETS_LIBS) $(top_builddir)/capplets/common/libcommon.la
mate_at_properties_SOURCES = \
	dm-util.h	\
	dm-util.c	\
	main.c
mate_at_properties_LDFLAGS = -export-dynamic

desktopdir = $(datadir)/applications
Desktop_in_files = mate-at-properties.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

$(desktop_DATA): $(Desktop_in_files)
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword= --keyword=Name --keyword=Comment --keyword=Keywords --template $< -d $(top_srcdir)/po -o $@

pixmapdir  = $(pkgdatadir)/pixmaps
pixmap_DATA =					\
	at-startup.png				\
	at-support.png

include $(top_srcdir)/gla11y.mk

AM_CPPFLAGS =					\
	$(WARN_CFLAGS)				\
	$(AT_CAPPLET_CFLAGS)			\
	$(MATECC_CAPPLETS_CFLAGS)		\
	-DPIXMAPDIR=\""$(pixmapdir)"\"		\
	-DMATECC_DATA_DIR="\"$(pkgdatadir)\""
CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(desktop_DATA) $(BUILT_SOURCES)
EXTRA_DIST = \
	$(ui_files) \
	$(pixmap_DATA) \
	$(Desktop_in_files) \
	org.mate.mcc.accessibility.at.gresource.xml \
	ui-a11y.suppr

# Generate resources
mate-at-properties-resources.h mate-at-properties-resources.c: org.mate.mcc.accessibility.at.gresource.xml Makefile $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir) $(srcdir)/org.mate.mcc.accessibility.at.gresource.xml)
	$(AM_V_GEN) XMLLINT=$(XMLLINT) $(GLIB_COMPILE_RESOURCES) --target $@ --sourcedir $(srcdir) --generate --c-name accessibility $<

-include $(top_srcdir)/git.mk
