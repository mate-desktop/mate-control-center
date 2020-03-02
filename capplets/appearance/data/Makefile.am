ui_files = appearance.ui

include $(top_srcdir)/gla11y.mk

pixmapdir = $(pkgdatadir)/pixmaps
dist_pixmap_DATA = \
        subpixel-bgr.png \
        subpixel-rgb.png \
        subpixel-vbgr.png \
        subpixel-vrgb.png \
        theme-thumbnailing.png \
        gtk-theme-thumbnailing.png \
        window-theme-thumbnailing.png \
        icon-theme-thumbnailing.png \
        mouse-cursor-normal.png \
        mouse-cursor-normal-large.png \
        mouse-cursor-white.png \
        mouse-cursor-white-large.png

desktopdir = $(datadir)/applications
desktop_in_files = \
	mate-appearance-properties.desktop.in \
	mate-theme-installer.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
%.desktop: %.desktop.in
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword= --keyword=Name --keyword=Comment --keyword=Keywords --template $< -d $(top_srcdir)/po -o $@

xml_in_files = \
	mate-theme-package.xml.in

mimedir = $(datadir)/mime/packages
mime_DATA = $(xml_in_files:.xml.in=.xml)

$(mime_DATA): $(xml_in_files)
	$(AM_V_GEN) GETTEXTDATADIR=$(top_srcdir) $(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

install-data-hook:
if ENABLE_UPDATE_MIMEDB
	$(UPDATE_MIME_DATABASE) "$(DESTDIR)$(datadir)/mime"
endif

uninstall-hook:
if ENABLE_UPDATE_MIMEDB
	$(UPDATE_MIME_DATABASE) "$(DESTDIR)$(datadir)/mime"
endif

EXTRA_DIST = \
	$(ui_files) \
	$(xml_in_files) \
	$(desktop_in_files) \
	ui-a11y.suppr

CLEANFILES = $(desktop_DATA) $(mime_DATA)

-include $(top_srcdir)/git.mk
