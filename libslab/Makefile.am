pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mate-slab.pc

AM_CPPFLAGS =					\
	-DG_LOG_DOMAIN=\"libslab\"		\
	-I$(top_srcdir)				\
	$(LIBSLAB_CFLAGS)			\
	$(WARN_CFLAGS)

HEADER_FILES=					\
	$(BUILT_SOURCES)			\
	app-resizer.h				\
	app-shell.h				\
	application-tile.h			\
	bookmark-agent.h			\
	double-click-detector.h			\
	mate-utils.h				\
	libslab-utils.h				\
	nameplate-tile.h			\
	search-bar.h				\
	shell-window.h				\
	slab-mate-util.h			\
	slab-section.h				\
	slab.h					\
	tile.h

lib_LTLIBRARIES = libmate-slab.la

libmate_slab_la_SOURCES =			\
	$(MARSHAL_GENERATED)			\
	app-resizer.c				\
	app-shell.c				\
	application-tile.c			\
	bookmark-agent.c			\
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
	tile.c

libmate_slab_includedir = $(includedir)/libmate-slab
libmate_slab_include_HEADERS = $(HEADER_FILES)
libmate_slab_la_LIBADD = $(LIBSLAB_LIBS)

MARSHAL_GENERATED = nld-marshal.c nld-marshal.h

nld-marshal.h: nld-marshal.list
	( @GLIB_GENMARSHAL@ --prefix=nld_marshal $(srcdir)/nld-marshal.list --header > nld-marshal.tmp \
	&& mv nld-marshal.tmp nld-marshal.h ) \
	|| ( rm -f nld-marshal.tmp && exit 1 )

nld-marshal.c: nld-marshal.h
	( (echo '#include "nld-marshal.h"'; @GLIB_GENMARSHAL@ --prefix=nld_marshal $(srcdir)/nld-marshal.list --body) > nld-marshal.tmp \
	&& mv nld-marshal.tmp nld-marshal.c ) \
	|| ( rm -f nld-marshal.tmp && exit 1 )


BUILT_SOURCES = $(MARSHAL_GENERATED)

CLEANFILES = $(MARSHAL_GENERATED)

EXTRA_DIST = nld-marshal.list

-include $(top_srcdir)/git.mk
