REAL_LIBSLAB_CFLAGS = -I$(top_srcdir)/libslab
REAL_LIBSLAB_LIBS = $(top_builddir)/libslab/libmate-slab.la

AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	$(REAL_LIBSLAB_CFLAGS)		\
	$(MATECC_SHELL_CFLAGS)  	\
	-DMATELOCALEDIR="\"$(datadir)/locale\""

bin_PROGRAMS = mate-control-center 

mate_control_center_SOURCES =		\
	control-center.c

mate_control_center_LDADD =						\
	$(MATECC_SHELL_LIBS)						\
	$(REAL_LIBSLAB_LIBS)

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

EXTRA_DIST = $(sys_in_files) matecc.directory.in matecc.menu

DISTCLEANFILES = $(sys_DATA) matecc.directory

-include $(top_srcdir)/git.mk
