# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = about-me

ui_files = mate-about-me-dialog.ui mate-about-me-password.ui mate-about-me-fingerprint.ui
Desktop_in_files = mate-about-me.desktop.in

BUILT_SOURCES = marshal.c marshal.h mate-about-me-resources.h mate-about-me-resources.c
nodist_mate_about_me_SOURCES = \
	$(BUILT_SOURCES)
mate_about_me_SOURCES =	 	\
	mate-about-me-password.c	\
	mate-about-me-password.h	\
	e-image-chooser.c		\
	e-image-chooser.h		\
	mate-about-me-fingerprint.c	\
	mate-about-me-fingerprint.h	\
	fingerprint-strings.h		\
	mate-about-me.c

marshal.h: fprintd-marshal.list
	@GLIB_GENMARSHAL@ --prefix=fprintd_marshal $< --header > $@
marshal.c: fprintd-marshal.list
	@GLIB_GENMARSHAL@ --prefix=fprintd_marshal $< --body --prototypes > $@
mate-about-me-resources.h mate-about-me-resources.c: org.mate.mcc.am.gresource.xml Makefile $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir) $(srcdir)/org.mate.mcc.am.gresource.xml)
	$(AM_V_GEN) XMLLINT=$(XMLLINT) $(GLIB_COMPILE_RESOURCES) --target $@ --sourcedir $(srcdir) --generate --c-name about_me $<

bin_PROGRAMS = mate-about-me

mate_about_me_LDADD = $(MATECC_CAPPLETS_LIBS)
if HAVE_ACCOUNTSSERVICE
mate_about_me_LDADD += $(ACCOUNTSSERVICE_LIBS)
endif
mate_about_me_LDFLAGS = -export-dynamic

desktopdir = $(datadir)/applications
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)
$(desktop_DATA): $(Desktop_in_files)
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword= --keyword=Name --keyword=Comment --keyword=Keywords --template $< -d $(top_srcdir)/po -o $@

include $(top_srcdir)/gla11y.mk

AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(MATECC_CAPPLETS_CFLAGS) \
	$(LIBEBOOK_CFLAGS) \
	-DDATADIR="\"$(datadir)\"" \
	-DMATECC_DATA_DIR="\"$(pkgdatadir)\"" \
	-DMATECC_PIXMAP_DIR="\"$(pkgdatadir)/pixmaps\"" \
	-DMATELOCALEDIR="\"$(datadir)/locale\""

if HAVE_ACCOUNTSSERVICE
AM_CPPFLAGS += \
	$(ACCOUNTSSERVICE_CFLAGS)
endif

CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(desktop_DATA) $(BUILT_SOURCES)
EXTRA_DIST = \
	$(ui_files) \
	fprintd-marshal.list \
	$(Desktop_in_files) \
	org.mate.mcc.am.gresource.xml \
	ui-a11y.suppr

-include $(top_srcdir)/git.mk
