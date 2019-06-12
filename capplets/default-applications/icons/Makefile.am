iconsdir = $(datadir)/icons/hicolor

nobase_dist_icons_DATA = \
	16x16/categories/instant-messaging.png \
	22x22/categories/instant-messaging.png \
	24x24/categories/instant-messaging.png \
	32x32/categories/instant-messaging.png \
	48x48/categories/instant-messaging.png \
	256x256/categories/instant-messaging.png

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(iconsdir)

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

-include $(top_srcdir)/git.mk
