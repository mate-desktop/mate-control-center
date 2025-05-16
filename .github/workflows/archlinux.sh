#!/usr/bin/bash

# Use grouped output messages
infobegin() {
	echo "::group::${1}"
}
infoend() {
	echo "::endgroup::"
}

# Required packages on Archlinux
requires=(
	ccache # Use ccache to speed up build
	clang  # Build with clang on Archlinux
	meson  # Used for meson build
)

requires+=(
	autoconf-archive
	file
	gcc
	git
	glib2-devel
	gobject-introspection
	intltool
	itstool
	libayatana-appindicator
	libxss
	libgtop
	libmatekbd
	make
	marco
	mate-common
	mate-menus
	polkit
	udisks2
	which
	yelp-tools
)

infobegin "Update system"
pacman --noconfirm -Syu
infoend

infobegin "Install dependency packages"
pacman --noconfirm -S ${requires[@]}
infoend
