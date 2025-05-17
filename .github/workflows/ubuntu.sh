#!/usr/bin/bash

# Use grouped output messages
infobegin() {
	echo "::group::${1}"
}
infoend() {
	echo "::endgroup::"
}

# Required packages on Ubuntu
requires=(
	autoconf-archive
	autopoint
	ccache
	clang
	clang-tools
	cppcheck
	curl
	desktop-file-utils
	gettext
	git
	gobject-introspection
	intltool
	make
	mate-common
	meson
	pkexec
	polkitd
	python3-lxml
	shared-mime-info
	xsltproc
)

dev_requires=(
	libayatana-appindicator3-dev
	libcanberra-gtk3-dev
	libdconf-dev
	libglib2.0-dev
	libgtk-3-dev
	libgtop2-dev
	libmarco-dev
	libmate-desktop-dev
	libmate-menu-dev
	libmatekbd-dev
	libpango1.0-dev
	libpolkit-gobject-1-dev
	librsvg2-bin
	librsvg2-dev
	libstartup-notification0-dev
	libsystemd-dev
	libudisks2-dev
	libx11-dev
	libxcursor-dev
	libxi-dev
	libxklavier-dev
	libxml2-dev
	libxrandr-dev
	libxss-dev
	libxt-dev
)

infobegin "Update system"
apt-get update -y
infoend

infobegin "Install dependency packages"
env DEBIAN_FRONTEND=noninteractive \
	apt-get install -y \
	${requires[@]} \
	${dev_requires[@]}
infoend
