REAL_LIBSLAB_CFLAGS = -I$(top_srcdir)/libslab
REAL_LIBSLAB_LIBS = $(top_builddir)/libslab/libslab.la

INCLUDES =					\
	-I$(top_srcdir)				\
	$(REAL_LIBSLAB_CFLAGS)			\
	$(MATECC_SHELL_CFLAGS)

bin_PROGRAMS = mate-control-center 

mate_control_center_SOURCES =		\
	control-center.c

mate_control_center_LDADD =						\
	$(MATECC_SHELL_LIBS)						\
	$(REAL_LIBSLAB_LIBS)

AM_CPPFLAGS =							\
	-DMATELOCALEDIR="\"$(datadir)/locale\""

sysdir = $(datadir)/applications
sys_in_files = matecc.desktop.in
sys_DATA = $(sys_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

directorydir = $(datadir)/desktop-directories
directory_in_files = matecc.directory.in
directory_DATA = $(directory_in_files:.directory.in=.directory)
@INTLTOOL_DIRECTORY_RULE@

menudir = $(sysconfdir)/xdg/menus
menu_DATA = matecc.menu

EXTRA_DIST = matecc.desktop.in.in matecc.directory.in matecc.menu

DISTCLEANFILES = matecc.desktop matecc.desktop.in matecc.directory

-include $(top_srcdir)/git.mk
