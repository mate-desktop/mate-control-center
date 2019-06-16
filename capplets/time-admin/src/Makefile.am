cappletname = time-admin

bin_PROGRAMS = mate-time-admin

mate_time_admin_SOURCES =	 	\
	main.c       time-map.c  time-share.c  time-tool.c  time-zone.c \
	time-map.h  time-share.h  time-tool.h  time-zone.h

mate_time_admin_LDADD = $(MATECC_CAPPLETS_LIBS)
mate_time_admin_LDFLAGS = -export-dynamic

AM_CPPFLAGS = \
	$(MATECC_CAPPLETS_CFLAGS) \
	-DDATADIR="\"$(datadir)/mate-time-admin/icon/\"" \
	-DMATELOCALEDIR="\"$(datadir)/locale\""\
	-DTIMPZONEDIR="\"$(datadir)/mate-time-admin/map/\""


CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) 

EXTRA_DIST = time-zones.h

-include $(top_srcdir)/git.mk
