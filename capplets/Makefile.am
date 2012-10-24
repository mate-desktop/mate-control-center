SUBDIRS = \
	common \
	accessibility \
	appearance \
	default-applications \
	display \
	keyboard \
	mouse \
	network \
	windows

# Enable these applets when migrated to gsettings
GSETTINS_DISABLED_SUBDIRS = \
	keybindings

DIST_SUBDIRS = \
	common \
	accessibility \
	appearance \
	default-applications \
	keybindings \
	keyboard \
	mouse \
	network \
	windows \
	display \
	about-me


if BUILD_ABOUTME
SUBDIRS += about-me
endif

-include $(top_srcdir)/git.mk
