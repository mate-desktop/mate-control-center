# This is used in MATECC_CAPPLETS_CFLAGS
cappletname = keybinding

bin_PROGRAMS = mate-keybinding-properties

mate_keybinding_properties_LDADD = $(MATECC_CAPPLETS_LIBS)
BUILT_SOURCES = mate-keybinding-properties-resources.h mate-keybinding-properties-resources.c
nodist_mate_keybinding_properties_SOURCES = \
	$(BUILT_SOURCES)
mate_keybinding_properties_SOURCES = \
	mate-keybinding-properties.c \
	eggcellrendererkeys.c \
	eggcellrendererkeys.h \
	eggaccelerators.c \
	eggaccelerators.h

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
Desktop_in_files = mate-keybinding.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_XML_NOMERGE_RULE@

xmldir = $(pkgdatadir)/keybindings
xml_in_files = 00-multimedia-key.xml.in 01-desktop-key.xml.in
xml_DATA = $(xml_in_files:.xml.in=.xml)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mate-keybindings.pc

mate-keybinding-properties-resources.h mate-keybinding-properties-resources.c: org.mate.mcc.keybindings.gresource.xml Makefile $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir) $(srcdir)/org.mate.mcc.keybindings.gresource.xml)
	$(AM_V_GEN) XMLLINT=$(XMLLINT) $(GLIB_COMPILE_RESOURCES) --target $@ --sourcedir $(srcdir) --generate --c-name keybindings $<

AM_CPPFLAGS = \
	$(MATECC_CAPPLETS_CFLAGS) \
	-DMATELOCALEDIR="\"$(datadir)/locale\"" \
	-DMATECC_DATA_DIR="\"$(pkgdatadir)\""
CLEANFILES = \
	$(BUILT_SOURCES) \
	$(MATECC_CAPPLETS_CLEANFILES) \
	$(desktop_DATA) \
	$(xml_DATA)
EXTRA_DIST = $(xml_in_files) \
	mate-keybindings.pc.in \
	mate-keybinding-properties.ui \
	$(Desktop_in_files) \
	org.mate.mcc.keybindings.gresource.xml

-include $(top_srcdir)/git.mk
