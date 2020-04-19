SUBDIRS = po libwindow-settings capplets font-viewer help man
DIST_SUBDIRS = po libwindow-settings capplets font-viewer help libslab shell typing-break man

if HAVE_LIBMATESLAB
SUBDIRS += libslab
endif

if HAVE_LIBMATESLAB
SUBDIRS += shell
endif

if HAVE_TYPING_BREAK
SUBDIRS += typing-break
endif

gsettingsschema_in_files = \
	org.mate.control-center.gschema.xml.in \
	org.mate.control-center.keybinding.gschema.xml.in

gsettings_SCHEMAS = $(gsettingsschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

CLEANFILES = \
	$(gsettings_SCHEMAS) \
	*.gschema.valid

MAINTAINERCLEANFILES = \
	$(srcdir)/INSTALL \
	$(srcdir)/aclocal.m4 \
	$(srcdir)/autoscan.log \
	$(srcdir)/compile \
	$(srcdir)/config.guess \
	$(srcdir)/config.h.in \
	$(srcdir)/config.sub \
	$(srcdir)/configure.scan \
	$(srcdir)/depcomp \
	$(srcdir)/install-sh \
	$(srcdir)/ltmain.sh \
	$(srcdir)/missing \
	$(srcdir)/mkinstalldirs \
	$(srcdir)/omf.make \
	$(srcdir)/xmldocs.make \
	$(srcdir)/gtk-doc.make \
	`find "$(srcdir)" -type f -name Makefile.in -print`

EXTRA_DIST = \
	autogen.sh \
	its	\
	$(gsettingsschema_in_files)

DISTCHECK_CONFIGURE_FLAGS = \
	--disable-update-mimedb \
	--enable-compile-warnings=no \
	CFLAGS='-Wno-deprecated-declarations'

GITIGNOREFILES=m4

# Build ChangeLog from GIT  history
ChangeLog:
	$(AM_V_GEN) if test -d $(top_srcdir)/.git; then \
		GIT_DIR="$(top_srcdir)/.git" git log --stat > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog

-include $(top_srcdir)/git.mk
