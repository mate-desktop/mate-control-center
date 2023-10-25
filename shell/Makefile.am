AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	$(WARN_CFLAGS)				\
	$(MATECC_SHELL_CFLAGS)			\
	-DMATELOCALEDIR="\"$(datadir)/locale\""

bin_PROGRAMS = mate-control-center

mate_control_center_SOURCES =		\
	$(MARSHAL_GENERATED)			\
	app-resizer.c				\
	app-shell.c				\
	application-tile.c			\
	bookmark-agent.c			\
	control-center.c			\
	double-click-detector.c			\
	mate-utils.c				\
	libslab-utils.c				\
	nameplate-tile.c			\
	search-bar.c				\
	shell-window.c				\
	slab-mate-util.c			\
	slab-section.c				\
	themed-icon.c				\
	themed-icon.h				\
	tile-action.c				\
	tile.c					\
	application-tile.h			\
	app-resizer.h				\
	app-shell.h				\
	bookmark-agent.h			\
	double-click-detector.h			\
	libslab-utils.h				\
	mate-utils.h				\
	nameplate-tile.h			\
	search-bar.h				\
	shell-window.h				\
	slab.h					\
	slab-mate-util.h			\
	slab-section.h				\
	themed-icon.h				\
	tile.h

mate_control_center_LDADD =						\
	$(MATECC_SHELL_LIBS)

sysdir = $(datadir)/applications
sys_in_files = matecc.desktop.in
sys_DATA = $(sys_in_files:.desktop.in=.desktop)
$(sys_DATA): $(sys_in_files)
if USE_NLS
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword= --keyword=Name --keyword=Comment --keyword=Keywords --template $< -d $(top_srcdir)/po -o $@
else
	$(AM_V_GEN) sed '/^# Translators/d' < $< > $@
endif

directorydir = $(datadir)/desktop-directories
directory_in_files = matecc.directory.desktop.in
directory_DATA = $(directory_in_files:.directory.desktop.in=.directory)
$(directory_DATA): $(directory_in_files)
if USE_NLS
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@
else
	$(AM_V_GEN) sed '/^# Translators/d' < $< > $@
endif

MARSHAL_GENERATED = nld-marshal.c nld-marshal.h

nld-marshal.h: nld-marshal.list
	( @GLIB_GENMARSHAL@ --prefix=nld_marshal $(srcdir)/nld-marshal.list --header > nld-marshal.tmp \
	&& mv nld-marshal.tmp nld-marshal.h ) \
	|| ( rm -f nld-marshal.tmp && exit 1 )

nld-marshal.c: nld-marshal.h
	( (echo '#include "nld-marshal.h"'; @GLIB_GENMARSHAL@ --prefix=nld_marshal $(srcdir)/nld-marshal.list --body) > nld-marshal.tmp \
	&& mv nld-marshal.tmp nld-marshal.c ) \
	|| ( rm -f nld-marshal.tmp && exit 1 )

menudir = $(sysconfdir)/xdg/menus
menu_DATA = matecc.menu

EXTRA_DIST = $(sys_in_files) matecc.directory.desktop.in matecc.menu nld-marshal.list

DISTCLEANFILES = $(sys_DATA) matecc.directory

-include $(top_srcdir)/git.mk
