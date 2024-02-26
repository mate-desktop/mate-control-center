if USE_NLS
PO_SUBDIR = po
endif

SUBDIRS = $(PO_SUBDIR) capplets font-viewer help man
DIST_SUBDIRS = po capplets font-viewer help shell typing-break man

SUBDIRS += shell

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

EXTRA_DIST += \
	meson.build					\
	meson_options.txt				\
	meson_post_install.py				\
	capplets/meson.build				\
	capplets/about-me/meson.build			\
	capplets/accessibility/at-properties/meson.build \
	capplets/appearance/meson.build			\
	capplets/appearance/data/meson.build		\
	capplets/common/meson.build		\
	capplets/default-applications/meson.build	\
	capplets/default-applications/icons/meson.build \
	capplets/display/meson.build			\
	capplets/display/meson.build			\
	capplets/keyboard/meson.build			\
	capplets/keybindings/meson.build		\
	capplets/mouse/meson.build			\
	capplets/network/meson.build			\
	capplets/system-info/meson.build		\
	capplets/time-admin/meson.build			\
	capplets/time-admin/data/meson.build		\
	capplets/time-admin/src/meson.build		\
	capplets/windows/meson.build			\
	font-viewer/meson.build				\
	help/meson.build				\
	help/LINGUAS					\
	man/meson.build					\
	po/meson.build					\
	shell/meson.build				\
	typing-break/meson.build

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
