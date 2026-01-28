#!/usr/bin/bash

set -e
set -o pipefail

NAME="marco"
TEMP_DIR=$(mktemp -d)
OS=$(cat /etc/os-release | grep ^ID | head -n 1 | awk -F= '{ print $2}')
TAG=$1
CACHE_DIR=$2

# Use grouped output messages
infobegin() {
	echo "::group::${1}"
}
infoend() {
	echo "::endgroup::"
}

# Required packages to build marco
arch_requires=(
	autoconf-archive
	gcc
	git
	glib2
	gtk3
	intltool
	libcanberra
	libgtop
	libxpresent
	libxres
	make
	mate-common
	mate-desktop
	which
	yelp-tools
	zenity
)

debian_requires=(
	autoconf-archive
	autopoint
	gcc
	git
	intltool
	libcanberra-gtk3-dev
	libglib2.0-dev
	libgtk-3-dev
	libgtop2-dev
	libice-dev
	libmate-desktop-dev
	libpango1.0-dev
	libsm-dev
	libstartup-notification0-dev
	libx11-dev
	libxcomposite-dev
	libxcursor-dev
	libxdamage-dev
	libxext-dev
	libxfixes-dev
	libxinerama-dev
	libxpresent-dev
	libxrandr-dev
	libxrender-dev
	libxres-dev
	libxt-dev
	make
	mate-common
	x11proto-present-dev
	yelp-tools
	zenity
)

fedora_requires=(
	desktop-file-utils
	gcc
	gtk3-devel
	libSM-devel
	libXdamage-devel
	libXpresent-devel
	libXres-devel
	libcanberra-devel
	libgtop2-devel
	libsoup-devel
	make
	mate-common
	mate-desktop-devel
	redhat-rpm-config
	startup-notification-devel
	yelp-tools
	zenity
)

ubuntu_requires=(
	autoconf-archive
	autopoint
	gcc
	git
	intltool
	libcanberra-gtk3-dev
	libglib2.0-dev
	libgtk-3-dev
	libgtop2-dev
	libice-dev
	libmate-desktop-dev
	libpango1.0-dev
	libsm-dev
	libstartup-notification0-dev
	libx11-dev
	libxcomposite-dev
	libxcursor-dev
	libxdamage-dev
	libxext-dev
	libxfixes-dev
	libxinerama-dev
	libxpresent-dev
	libxrandr-dev
	libxrender-dev
	libxres-dev
	libxt-dev
	make
	mate-common
	x11proto-present-dev
	yelp-tools
	zenity
)

requires=$(eval echo '${'"${OS}_requires[@]}")

infobegin "Install Depends for marco"
case ${OS} in
arch)
	pacman --noconfirm -Syu
	pacman --noconfirm -S ${requires[@]}
	;;
debian | ubuntu)
	apt-get update -qq
	env DEBIAN_FRONTEND=noninteractive \
		apt-get install --assume-yes --no-install-recommends ${requires[@]}
	;;
fedora)
	dnf update -y
	dnf install -y ${requires[@]}
	;;
esac
infoend

# Use cached packages first
if [ -f $CACHE_DIR/${NAME}-${TAG}.tar.xz ]; then
	echo "Found cache package, reuse it"
	tar -C / -Jxf $CACHE_DIR/${NAME}-${TAG}.tar.xz
else
	git clone --recurse-submodules https://github.com/mate-desktop/${NAME}

	# Foldable output information
	infobegin "Configure"
	cd ${NAME}
	git checkout v${TAG}
	if [[ ${OS} == "debian" || ${OS} == "ubuntu" ]]; then
		./autogen.sh --prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu --libexecdir=/usr/lib/x86_64-linux-gnu || {
			cat config.log
			exit 1
		}
	else
		./autogen.sh --prefix=/usr || {
			cat config.log
			exit 1
		}
	fi
	infoend

	infobegin "Build"
	make -j ${JOBS}
	infoend

	infobegin "Install"
	make install
	infoend

	# Cache this package version
	infobegin "Cache"
	[ -d ${CACHE_DIR} ] || mkdir -p ${CACHE_DIR}
	make install DESTDIR=${TEMP_DIR}
	cd $TEMP_DIR
	tar -J -cf $CACHE_DIR/${NAME}-${TAG}.tar.xz *
	infoend
fi
