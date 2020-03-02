# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = window

bin_PROGRAMS = mate-window-properties

mate_window_properties_LDADD = $(MATECC_CAPPLETS_LIBS)

BUILT_SOURCES = mate-window-properties-resources.h mate-window-properties-resources.c
nodist_mate_window_properties_SOURCES = $(BUILT_SOURCES)
mate_window_properties_SOURCES = \
	mate-metacity-support.h \
	mate-metacity-support.c \
	mate-window-properties.c

desktopdir = $(datadir)/applications
Desktop_in_files = mate-window-properties.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

mate-window-properties-resources.h mate-window-properties-resources.c: org.mate.mcc.windows.gresource.xml Makefile $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir) $(srcdir)/org.mate.mcc.windows.gresource.xml)
	$(AM_V_GEN) XMLLINT=$(XMLLINT) $(GLIB_COMPILE_RESOURCES) --target $@ --sourcedir $(srcdir) --generate --c-name window_properties $<

$(desktop_DATA): $(Desktop_in_files)
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword= --keyword=Name --keyword=Comment --keyword=Keywords --template $< -d $(top_srcdir)/po -o $@

AM_CPPFLAGS =				\
	$(WARN_CFLAGS)			\
	$(MATECC_CAPPLETS_CFLAGS)	\
	-DUIDIR=\""$(uidir)"\"		\
	-DPIXMAPDIR=\""$(pixmapdir)"\"

CLEANFILES = $(BUILT_SOURCES) $(MATECC_CAPPLETS_CLEANFILES) $(desktop_DATA)
EXTRA_DIST = $(Desktop_in_files) org.mate.mcc.windows.gresource.xml window-properties.ui

-include $(top_srcdir)/git.mk
