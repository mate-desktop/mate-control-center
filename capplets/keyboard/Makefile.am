# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = keyboard

ui_files = mate-keyboard-properties-a11y-notifications.ui \
           mate-keyboard-properties-dialog.ui \
           mate-keyboard-properties-layout-chooser.ui \
           mate-keyboard-properties-model-chooser.ui \
           mate-keyboard-properties-options-dialog.ui


bin_PROGRAMS = mate-keyboard-properties

BUILT_SOURCES = mate-keyboard-properties-resources.h mate-keyboard-properties-resources.c
nodist_mate_keyboard_properties_SOURCES = \
	$(BUILT_SOURCES)
mate_keyboard_properties_SOURCES = \
	mate-keyboard-properties.c \
	mate-keyboard-properties-a11y.c \
	mate-keyboard-properties-a11y.h \
	mate-keyboard-properties-xkb.c \
	mate-keyboard-properties-xkbmc.c \
	mate-keyboard-properties-xkblt.c \
	mate-keyboard-properties-xkbltadd.c \
	mate-keyboard-properties-xkbot.c \
	mate-keyboard-properties-xkbpv.c \
	mate-keyboard-properties-xkb.h

mate_keyboard_properties_LDADD = $(MATECC_CAPPLETS_LIBS) $(LIBMATEKBDUI_LIBS)

include $(top_srcdir)/gla11y.mk

desktopdir = $(datadir)/applications
Desktop_in_files = mate-keyboard.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

$(desktop_DATA): $(Desktop_in_files)
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword= --keyword=Name --keyword=Comment --keyword=Keywords --template $< -d $(top_srcdir)/po -o $@

mate-keyboard-properties-resources.h mate-keyboard-properties-resources.c: org.mate.mcc.keyboard.gresource.xml Makefile $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir) $(srcdir)/org.mate.mcc.keyboard.gresource.xml)
	$(AM_V_GEN) XMLLINT=$(XMLLINT) $(GLIB_COMPILE_RESOURCES) --target $@ --sourcedir $(srcdir) --generate --c-name keyboard $<

AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(MATECC_CAPPLETS_CFLAGS) \
	$(LIBMATEKBDUI_CFLAGS) \
	-DMATECC_DATA_DIR="\"$(pkgdatadir)\""
CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(desktop_DATA) $(BUILT_SOURCES)
EXTRA_DIST = \
	$(ui_files) \
	$(Desktop_in_files) \
	org.mate.mcc.keyboard.gresource.xml \
	ui-a11y.suppr

-include $(top_srcdir)/git.mk
