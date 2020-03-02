# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = display

BUILT_SOURCES = \
	foo-marshal.h \
	foo-marshal.c \
	mate-display-properties-resources.h \
	mate-display-properties-resources.c

ui_files = display-capplet.ui

include $(top_srcdir)/gla11y.mk

bin_PROGRAMS = mate-display-properties

sbin_PROGRAMS = mate-display-properties-install-systemwide

nodist_mate_display_properties_SOURCES = \
	$(BUILT_SOURCES)
mate_display_properties_SOURCES =	\
	xrandr-capplet.c		\
	scrollarea.c			\
	scrollarea.h

mate_display_properties_LDFLAGS = -export-dynamic
mate_display_properties_LDADD = \
	$(top_builddir)/capplets/common/libcommon.la \
	$(DISPLAY_CAPPLET_LIBS)

mate_display_properties_install_systemwide_SOURCES =	\
	mate-display-properties-install-systemwide.c

mate_display_properties_install_systemwide_LDADD =	\
	$(GLIB_LIBS)

polkit_policydir = $(datadir)/polkit-1/actions
dist_polkit_policy_DATA =					\
	org.mate.randr.policy

%.policy: %.policy.in
	$(AM_V_GEN) $(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

desktopdir = $(datadir)/applications
Desktop_in_files = mate-display-properties.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

$(desktop_DATA): $(Desktop_in_files)
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword= --keyword=Name --keyword=Comment --keyword=Keywords --template $< -d $(top_srcdir)/po -o $@

AM_CPPFLAGS =					\
	$(WARN_CFLAGS)				\
	$(DISPLAY_CAPPLET_CFLAGS)		\
	$(MATECC_CAPPLETS_CFLAGS)		\
	-DSBINDIR="\"$(sbindir)\""		\
	-DMATELOCALEDIR="\"$(datadir)/locale\""	\
	-DMATECC_DATA_DIR="\"$(pkgdatadir)\""

foo-marshal.c: foo-marshal.list
	@GLIB_GENMARSHAL@ --prefix=foo_marshal $< --header > $@
foo-marshal.h: foo-marshal.list
	@GLIB_GENMARSHAL@ --prefix=foo_marshal $< --body --prototypes > $@
mate-display-properties-resources.h mate-display-properties-resources.c: org.mate.mcc.display.gresource.xml Makefile $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir) $(srcdir)/org.mate.mcc.display.gresource.xml)
	$(AM_V_GEN) XMLLINT=$(XMLLINT) $(GLIB_COMPILE_RESOURCES) --target $@ --sourcedir $(srcdir) --generate --c-name display $<

CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(desktop_DATA) $(BUILT_SOURCES)

EXTRA_DIST = $(ui_files) foo-marshal.list $(Desktop_in_files) org.mate.mcc.display.gresource.xml org.mate.randr.policy.in ui-a11y.suppr
DISTCLEANFILES = org.mate.randr.policy

-include $(top_srcdir)/git.mk
