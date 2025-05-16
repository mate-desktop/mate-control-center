#!/usr/bin/bash

# Use grouped output messages
infobegin() {
	echo "::group::${1}"
}
infoend() {
	echo "::endgroup::"
}

# Required packages on Fedora
requires=(
	ccache # Use ccache to speed up build
	meson  # Used for meson build
)

requires+=(
	autoconf-archive
	accountsservice-devel
	cairo-gobject-devel
	cppcheck-htmlreport
	dconf-devel
	desktop-file-utils
	gcc
	git
	gobject-introspection-devel
	gtk3-devel
	iso-codes-devel
	itstool
	libappindicator-gtk3-devel
	libSM-devel
	libXScrnSaver-devel
	libcanberra-devel
	libmatekbd-devel
	libgtop2-devel
	librsvg2-devel
	librsvg2-tools
	libudisks2-devel
	make
	marco-devel
	mate-menus-devel
	polkit-devel
	python3-lxml
	mate-common
	redhat-rpm-config
	startup-notification-devel
	systemd-devel
	which
)

infobegin "Update system"
dnf update -y
infoend

infobegin "Install dependency packages"
dnf install -y ${requires[@]}
infoend
