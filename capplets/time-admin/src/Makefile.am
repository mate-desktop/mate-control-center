cappletname = time-admin

bin_PROGRAMS = mate-time-admin

BUILT_SOURCES = mate-time-admin-resources.h mate-time-admin-resources.c
nodist_mate_time_admin_SOURCES = $(BUILT_SOURCES)
mate_time_admin_SOURCES =		\
	main.c       time-map.c  time-share.c  time-tool.c  time-zone.c \
	time-map.h  time-share.h  time-tool.h  time-zone.h

mate-time-admin-resources.h mate-time-admin-resources.c: org.mate.mcc.ta.gresource.xml Makefile $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir) $(srcdir)/org.mate.mcc.ta.gresource.xml)
	$(AM_V_GEN) XMLLINT=$(XMLLINT) $(GLIB_COMPILE_RESOURCES) --target $@ --sourcedir $(srcdir) --generate --c-name time_admin $<

mate_time_admin_LDADD = $(MATECC_CAPPLETS_LIBS)
mate_time_admin_LDFLAGS = -export-dynamic

AM_CPPFLAGS =				\
	$(WARN_CFLAGS)			\
	$(MATECC_CAPPLETS_CFLAGS)	\
	-DTIMPZONEDIR="\"$(datadir)/mate-time-admin/map/\""


CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(BUILT_SOURCES)

EXTRA_DIST = time-admin.ui time-zones.h org.mate.mcc.ta.gresource.xml

-include $(top_srcdir)/git.mk
