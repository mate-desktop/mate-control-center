EXTRA_DIST =

AM_CPPFLAGS = \
	-DMATECC_DATA_DIR="\"$(pkgdatadir)\""				\
	-DMATELOCALEDIR="\"$(datadir)/locale\""				\
	-DMATEDATADIR="\"$(datadir)\""				\
	-DGTK_ENGINE_DIR="\"$(GTK_ENGINE_DIR)\"" 			\
	-DG_LOG_DOMAIN=\"capplet-common\"				\
	-DINSTALL_PREFIX=\"$(prefix)\"					\
	-I$(top_srcdir)							\
	-DPIXMAP_DIR=\""$(datadir)/mate-control-center/pixmaps"\"	\
	$(WARN_CFLAGS)							\
	$(CAPPLET_CFLAGS)						\
	$(MATE_DESKTOP_CFLAGS)						\
	$(MARCO_CFLAGS)							\
	$(GIO_CFLAGS)							\
	$(DCONF_CFLAGS)


noinst_LTLIBRARIES = libcommon.la

libcommon_la_SOURCES = \
	activate-settings-daemon.c	\
	activate-settings-daemon.h	\
	capplet-util.c			\
	capplet-util.h			\
	dconf-util.c			\
	dconf-util.h			\
	file-transfer-dialog.c		\
	file-transfer-dialog.h		\
	mate-theme-apply.c		\
	mate-theme-apply.h 		\
	mate-theme-info.c		\
	mate-theme-info.h		\
	gtkrc-utils.c			\
	gtkrc-utils.h			\
	theme-thumbnail.c		\
	theme-thumbnail.h		\
	wm-common.c			\
	wm-common.h

libcommon_la_LIBADD =							\
	$(MARCO_LIBS)						\
	$(MATE_DESKTOP_LIBS)						\
	$(GIO_LIBS)									\
	$(DCONF_LIBS)

mate_theme_test_SOURCES = \
	mate-theme-test.c

mate_theme_test_LDADD = 						\
	libcommon.la							\
	$(MATECC_CAPPLETS_LIBS)						\
	$(MATECC_LIBS)

noinst_PROGRAMS = \
	mate-theme-test

-include $(top_srcdir)/git.mk
