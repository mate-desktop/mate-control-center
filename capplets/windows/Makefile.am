# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = window

bin_PROGRAMS = mate-window-properties

mate_window_properties_LDADD = $(MATECC_CAPPLETS_LIBS)

mate_window_properties_SOURCES = \
	mate-metacity-support.h \
	mate-metacity-support.c \
	mate-window-properties.c

desktopdir = $(datadir)/applications
Desktop_in_files = mate-window-properties.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

$(desktop_DATA): $(Desktop_in_files)
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

AM_CPPFLAGS   = $(MATECC_CAPPLETS_CFLAGS)					\
	     -DUIDIR=\""$(uidir)"\"  	\
	     -DPIXMAPDIR=\""$(pixmapdir)"\"

CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(desktop_DATA)
EXTRA_DIST = $(Desktop_in_files)

-include $(top_srcdir)/git.mk
