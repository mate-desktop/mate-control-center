# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = system-info

ui_files = mate-system-info.ui

bin_PROGRAMS = mate-system-info

mate_system_info_LDADD = $(MATECC_CAPPLETS_LIBS)
BUILT_SOURCES = mate-system-info-resources.h mate-system-info-resources.c
nodist_mate_system_info_SOURCES = \
	$(BUILT_SOURCES)
mate_system_info_SOURCES =	\
	mate-system-info.c	\
	mate-system-info.h	\
	info-cleanup.c  \
	info-cleanup.h  \
	main.c

include $(top_srcdir)/gla11y.mk

desktopdir = $(datadir)/applications
Desktop_in_files = mate-system-info.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

$(desktop_DATA): $(Desktop_in_files)
if USE_NLS
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword= --keyword=Name --keyword=Comment --keyword=Keywords --template $< -d $(top_srcdir)/po -o $@
else
	$(AM_V_GEN) sed '/^# Translators/d' < $< > $@
endif

mate-system-info-resources.h mate-system-info-resources.c: system-info.gresource.xml Makefile $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir) $(srcdir)/system-info.gresource.xml)
	$(AM_V_GEN) XMLLINT=$(XMLLINT) $(GLIB_COMPILE_RESOURCES) --target $@ --sourcedir $(srcdir) --generate --c-name mate_system_info $<

AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(MATECC_CAPPLETS_CFLAGS) \
	-DMATECC_DATA_DIR="\"$(pkgdatadir)\""
CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(desktop_DATA) $(BUILT_SOURCES)
EXTRA_DIST = \
	$(ui_files) \
	$(Desktop_in_files) \
	system-info.gresource.xml

-include $(top_srcdir)/git.mk
