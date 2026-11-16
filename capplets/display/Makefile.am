# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = display

BUILT_SOURCES = \
	foo-marshal.h \
	foo-marshal.c \
	mate-display-properties-resources.h \
	mate-display-properties-resources.c

if HAVE_WAYLAND_PROTOCOLS
BUILT_SOURCES += \
	wlr-output-management-unstable-v1-client-protocol.h \
	wlr-output-management-unstable-v1-protocol.c \
	mate-output-v1-client-protocol.h \
	mate-output-v1-protocol.c
endif

ui_files = display-capplet.ui

include $(top_srcdir)/gla11y.mk

bin_PROGRAMS = mate-display-properties

sbin_PROGRAMS = mate-display-properties-install-systemwide

nodist_mate_display_properties_SOURCES = \
	$(BUILT_SOURCES)
mate_display_properties_SOURCES =	\
	xrandr-capplet.c		\
	wlrandr-capplet.c		\
	display-backend.c		\
	display-backend.h		\
	display-backend-wlrandr.c	\
	display-head.c			\
	display-head.h			\
	scrollarea.c			\
	scrollarea.h

if HAVE_WAYLAND_PROTOCOLS
mate_display_properties_SOURCES += display-backend-protocol.c
endif

mate_display_properties_LDFLAGS = -export-dynamic
mate_display_properties_LDADD = \
	$(top_builddir)/capplets/common/libcommon.la \
	$(DISPLAY_CAPPLET_LIBS)

check_PROGRAMS = test-display-head
TESTS = $(check_PROGRAMS)
test_display_head_SOURCES = test-display-head.c display-head.c display-head.h
test_display_head_LDADD = $(DISPLAY_CAPPLET_LIBS)

mate_display_properties_install_systemwide_SOURCES =	\
	mate-display-properties-install-systemwide.c

mate_display_properties_install_systemwide_LDADD =	\
	$(GLIB_LIBS)

polkit_policydir = $(datadir)/polkit-1/actions
dist_polkit_policy_DATA =					\
	org.mate.randr.policy

%.policy: %.policy.in
if USE_NLS
	$(AM_V_GEN) $(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@
else
	$(AM_V_GEN) cp -f $< $@
endif

desktopdir = $(datadir)/applications
Desktop_in_files = mate-display-properties.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

$(desktop_DATA): $(Desktop_in_files)
if USE_NLS
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword= --keyword=Name --keyword=Comment --keyword=Keywords --template $< -d $(top_srcdir)/po -o $@
else
	$(AM_V_GEN) sed '/^# Translators/d' < $< > $@
endif

AM_CPPFLAGS =					\
	$(WARN_CFLAGS)				\
	$(DISPLAY_CAPPLET_CFLAGS)		\
	$(MATECC_CAPPLETS_CFLAGS)		\
	-DSBINDIR="\"$(sbindir)\""		\
	-DMATELOCALEDIR="\"$(datadir)/locale\""	\
	-DMATECC_DATA_DIR="\"$(pkgdatadir)\""

%-client-protocol.h: $(srcdir)/protocols/%.xml
	$(AM_V_GEN) $(WAYLAND_SCANNER) client-header $< $@
%-protocol.c: $(srcdir)/protocols/%.xml
	$(AM_V_GEN) $(WAYLAND_SCANNER) private-code $< $@
mate-output-v1-client-protocol.h: $(MATE_WAYLAND_PROTOCOLS_DIR)/mate-output-v1.xml
	$(AM_V_GEN) $(WAYLAND_SCANNER) client-header $< $@
mate-output-v1-protocol.c: $(MATE_WAYLAND_PROTOCOLS_DIR)/mate-output-v1.xml
	$(AM_V_GEN) $(WAYLAND_SCANNER) private-code $< $@

foo-marshal.c: foo-marshal.list
	@GLIB_GENMARSHAL@ --prefix=foo_marshal $< --header > $@
foo-marshal.h: foo-marshal.list
	@GLIB_GENMARSHAL@ --prefix=foo_marshal $< --body --prototypes > $@
mate-display-properties-resources.h mate-display-properties-resources.c: org.mate.mcc.display.gresource.xml Makefile $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir) $(srcdir)/org.mate.mcc.display.gresource.xml)
	$(AM_V_GEN) XMLLINT=$(XMLLINT) $(GLIB_COMPILE_RESOURCES) --target $@ --sourcedir $(srcdir) --generate --c-name display $<

CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(desktop_DATA) $(BUILT_SOURCES)

EXTRA_DIST = $(ui_files) protocols/wlr-output-management-unstable-v1.xml foo-marshal.list $(Desktop_in_files) org.mate.mcc.display.gresource.xml org.mate.randr.policy.in ui-a11y.suppr
DISTCLEANFILES = org.mate.randr.policy

-include $(top_srcdir)/git.mk
