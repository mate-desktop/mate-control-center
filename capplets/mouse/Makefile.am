# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = mouse

ui_files = mate-mouse-properties.ui

bin_PROGRAMS = mate-mouse-properties

mate_mouse_properties_LDADD = $(MATECC_CAPPLETS_LIBS)
BUILT_SOURCES = mate-mouse-properties-resources.h mate-mouse-properties-resources.c
nodist_mate_mouse_properties_SOURCES = \
	$(BUILT_SOURCES)
mate_mouse_properties_SOURCES =	\
	mate-mouse-properties.c	\
	msd-input-helper.c	\
	msd-input-helper.h

include $(top_srcdir)/gla11y.mk

desktopdir = $(datadir)/applications
Desktop_in_files = mate-settings-mouse.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

$(desktop_DATA): $(Desktop_in_files)
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword= --keyword=Name --keyword=Comment --keyword=Keywords --template $< -d $(top_srcdir)/po -o $@

mate-mouse-properties-resources.h mate-mouse-properties-resources.c: org.mate.mcc.mouse.gresource.xml Makefile $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir) $(srcdir)/org.mate.mcc.mouse.gresource.xml)
	$(AM_V_GEN) XMLLINT=$(XMLLINT) $(GLIB_COMPILE_RESOURCES) --target $@ --sourcedir $(srcdir) --generate --c-name mouse $<

AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(MATECC_CAPPLETS_CFLAGS) \
	-DMATECC_DATA_DIR="\"$(pkgdatadir)\""
CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(desktop_DATA) $(BUILT_SOURCES)
EXTRA_DIST = \
	$(ui_files) \
	double-click-on.svg \
	double-click-off.svg \
	double-click-maybe.svg \
	$(Desktop_in_files) \
	org.mate.mcc.mouse.gresource.xml \
	ui-a11y.suppr

-include $(top_srcdir)/git.mk
