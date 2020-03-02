SUBDIRS = icons

# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = mate-default-applications-properties

ui_files = mate-default-applications-properties.ui

bin_PROGRAMS = mate-default-applications-properties

BUILT_SOURCES = \
	mate-default-applications-properties-resources.h \
	mate-default-applications-properties-resources.c

mate_default_applications_properties_LDADD = $(MATECC_CAPPLETS_LIBS)
nodist_mate_default_applications_properties_SOURCES = \
	$(BUILT_SOURCES)
mate_default_applications_properties_SOURCES = \
	mate-da-capplet.c mate-da-capplet.h

include $(top_srcdir)/gla11y.mk

desktopdir = $(datadir)/applications
Desktop_in_files = mate-default-applications-properties.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)
$(desktop_DATA): $(Desktop_in_files)
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword= --keyword=Name --keyword=Comment --keyword=Keywords --template $< -d $(top_srcdir)/po -o $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mate-default-applications.pc

AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(MATECC_CAPPLETS_CFLAGS) \
	$(DEFAULT_APPLICATIONS_CAPPLET_CFLAGS) \
	-DAPPLICATIONSDIR=\""$(datadir)/applications"\"

mate-default-applications-properties-resources.h mate-default-applications-properties-resources.c: org.mate.mcc.da.gresource.xml Makefile $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir) $(srcdir)/org.mate.mcc.da.gresource.xml)
	$(AM_V_GEN) XMLLINT=$(XMLLINT) $(GLIB_COMPILE_RESOURCES) --target $@ --sourcedir $(srcdir) --generate --c-name default_applications $<

CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(desktop_DATA) $(autostart_DATA) $(BUILT_SOURCES)
EXTRA_DIST = \
	mate-default-applications.pc.in \
	mate-default-applications-properties.ui \
	$(Desktop_in_files) \
	org.mate.mcc.da.gresource.xml

-include $(top_srcdir)/git.mk
