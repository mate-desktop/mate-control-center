# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = panel

bin_PROGRAMS = mate-panel-properties

mate_panel_properties_LDADD = $(MATECC_CAPPLETS_LIBS)

mate_panel_properties_SOURCES = \
	mate-panel-properties.c \
	pp-config.c \
	pp-config.h \
	pp-panel.c \
	pp-panel.h \
	pp-window.c \
	pp-window.h

desktopdir = $(datadir)/applications
Desktop_in_files = mate-panel-properties.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

$(desktop_DATA): $(Desktop_in_files)
if USE_NLS
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword= --keyword=Name --keyword=Comment --keyword=Keywords --template $< -d $(top_srcdir)/po -o $@
else
	$(AM_V_GEN) sed '/^# Translators/d' < $< > $@
endif

AM_CPPFLAGS =				\
	$(WARN_CFLAGS)			\
	$(MATECC_CAPPLETS_CFLAGS)

CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(desktop_DATA)
EXTRA_DIST = $(Desktop_in_files)

-include $(top_srcdir)/git.mk
