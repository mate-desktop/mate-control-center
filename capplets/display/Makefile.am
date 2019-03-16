# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = display

uidir = $(pkgdatadir)/ui
dist_ui_DATA = display-capplet.ui

bin_PROGRAMS = mate-display-properties

sbin_PROGRAMS = mate-display-properties-install-systemwide

mate_display_properties_SOURCES =	\
	xrandr-capplet.c		\
	scrollarea.c			\
	foo-marshal.c			\
	scrollarea.h			\
	foo-marshal.h

mate_display_properties_LDFLAGS = -export-dynamic
mate_display_properties_LDADD = \
	$(top_builddir)/capplets/common/libcommon.la \
	$(DISPLAY_CAPPLET_LIBS)

mate_display_properties_install_systemwide_SOURCES =	\
	mate-display-properties-install-systemwide.c

mate_display_properties_install_systemwide_LDADD =	\
	$(GLIB_LIBS)

polkit_policydir = $(datadir)/polkit-1/actions
dist_polkit_policy_DATA =					\
	org.mate.randr.policy

# You will need a recent intltool or the patch from this bug
# http://bugzilla.gnome.org/show_bug.cgi?id=462312
@INTLTOOL_POLICY_RULE@

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
Desktop_in_files = mate-display-properties.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

AM_CPPFLAGS   = $(DISPLAY_CAPPLET_CFLAGS) \
             $(MATECC_CAPPLETS_CFLAGS) \
	     -DSBINDIR="\"$(sbindir)\"" \
	     -DUIDIR="\"$(uidir)\"" \
	     -DMATELOCALEDIR="\"$(datadir)/locale\"" \
	     -DMATECC_DATA_DIR="\"$(pkgdatadir)\""

CLEANFILES = $(MATECC_CAPPLETS_CLEANFILES) $(desktop_DATA)

EXTRA_DIST = org.mate.randr.policy.in $(Desktop_in_files)
DISTCLEANFILES = org.mate.randr.policy

-include $(top_srcdir)/git.mk
