# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = mouse

ui_files = mate-mouse-properties.ui

bin_PROGRAMS = mate-mouse-properties

mate_mouse_properties_LDADD = $(MATECC_CAPPLETS_LIBS)
BUILT_SOURCES = mate-mouse-properties-resources.h mate-mouse-properties-resources.c
if HAVE_WAYLAND_PROTOCOLS
BUILT_SOURCES += \
	mate-input-device-v1-client-protocol.h \
	mate-input-device-v1-protocol.c
mate_mouse_properties_LDADD += $(MOUSE_WAYLAND_LIBS)
endif
nodist_mate_mouse_properties_SOURCES = \
	$(BUILT_SOURCES)
mate_mouse_properties_SOURCES =	\
	mate-mouse-properties.c	\
	msd-input-helper.c	\
	msd-input-helper.h	\
	msd-input-wayland.c	\
	msd-input-wayland.h

mate-input-device-v1-client-protocol.h: $(MATE_WAYLAND_PROTOCOLS_DIR)/mate-input-device-v1.xml
	$(AM_V_GEN) $(WAYLAND_SCANNER) client-header $< $@
mate-input-device-v1-protocol.c: $(MATE_WAYLAND_PROTOCOLS_DIR)/mate-input-device-v1.xml
	$(AM_V_GEN) $(WAYLAND_SCANNER) private-code $< $@

include $(top_srcdir)/gla11y.mk

desktopdir = $(datadir)/applications
Desktop_in_files = mate-settings-mouse.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

$(desktop_DATA): $(Desktop_in_files)
if USE_NLS
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword= --keyword=Name --keyword=Comment --keyword=Keywords --template $< -d $(top_srcdir)/po -o $@
else
	$(AM_V_GEN) sed '/^# Translators/d' < $< > $@
endif

mate-mouse-properties-resources.h mate-mouse-properties-resources.c: org.mate.mcc.mouse.gresource.xml Makefile $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir) $(srcdir)/org.mate.mcc.mouse.gresource.xml)
	$(AM_V_GEN) XMLLINT=$(XMLLINT) $(GLIB_COMPILE_RESOURCES) --target $@ --sourcedir $(srcdir) --generate --c-name mouse $<

AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(MATECC_CAPPLETS_CFLAGS) \
	$(MOUSE_WAYLAND_CFLAGS) \
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
