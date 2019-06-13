# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = mate-desktop-icons-properties

bin_PROGRAMS = mate-desktop-icons-properties

BUILT_SOURCES = \
	mate-desktop-icons-resources.h \
	mate-desktop-icons-resources.c

mate_desktop_icons_properties_LDADD = $(MATECC_CAPPLETS_LIBS)
nodist_mate_desktop_icons_properties_SOURCES = \
	$(BUILT_SOURCES)
mate_desktop_icons_properties_SOURCES = \
	mate-di-capplet.h mate-di-capplet.c

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
Desktop_in_files = mate-desktop-icons-properties.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mate-desktop-icons.pc

AM_CPPFLAGS = \
	$(MATECC_CAPPLETS_CFLAGS) \
	$(DEFAULT_APPLICATIONS_CAPPLET_CFLAGS) \
	-DAPPLICATIONSDIR=\""$(datadir)/applications"\"\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"

mate-desktop-icons-resources.h mate-desktop-icons-resources.c: org.mate.mcc.di.gresource.xml Makefile $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir) $(srcdir)/org.mate.mcc.di.gresource.xml)
	$(AM_V_GEN) XMLLINT=$(XMLLINT) $(GLIB_COMPILE_RESOURCES) --target $@ --sourcedir $(srcdir) --generate --c-name desktop_icons $<

CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(desktop_DATA) $(autostart_DATA) $(BUILT_SOURCES)
EXTRA_DIST = \
	mate-desktop-icons.pc.in \
	mate-desktop-icons-properties.ui \
	$(Desktop_in_files) \
	org.mate.mcc.di.gresource.xml

-include $(top_srcdir)/git.mk
