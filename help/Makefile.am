@YELP_HELP_RULES@

HELP_ID = mate-control-center
HELP_FILES = \
	index.docbook \
	config-accessibility-keyboard.xml \
	config-background.xml \
	config-behavior.xml \
	config-cds.xml \
	config-default-apps.xml \
	config-file-type.xml \
	config-hints.xml \
	config-html.xml \
	config-kbd.xml \
	config-keybindings.xml \
	config-mouse.xml \
	config-screensaver.xml \
	config-themes.xml \
	legal.xml

# Add linguas to be ignored, e.g. IGNORE_HELP_LINGUAS = ca de es fr
IGNORE_HELP_LINGUAS =
HELP_LINGUAS = $(if $(IGNORE_HELP_LINGUAS), \
	$(filter-out $(IGNORE_HELP_LINGUAS),$(subst /,,$(dir $(wildcard */*.po)))), \
	$(subst /,,$(dir $(wildcard */*.po))) )

-include $(top_srcdir)/git.mk
