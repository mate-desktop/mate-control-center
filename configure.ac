AC_INIT([mate-control-center], [1.9.0], [htts://www.mate-desktop.org/])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([shell])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.9 no-dist-gzip dist-xz tar-ustar])
AM_MAINTAINER_MODE
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_C_O
AC_STDC_HEADERS
AM_PROG_LIBTOOL

# Use the Yelp documentation framework
YELP_HELP_INIT

changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi
changequote([,])dnl

# Internationalization support

IT_PROG_INTLTOOL([0.37.1])

GETTEXT_PACKAGE=mate-control-center
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext package])

AM_GLIB_GNU_GETTEXT

MATE_COMMON_INIT
MATE_DEBUG_CHECK
MATE_COMPILE_WARNINGS([maximum])
MATE_MAINTAINER_MODE_DEFINES

AC_PATH_XTRA
x_libs="$X_PRE_LIBS $X_LIBS -lX11 $X_EXTRA_LIBS"

AC_CHECK_FUNC(XScreenSaverQueryInfo, [
  TYPING_BREAK="typing-break"], [
  save_LIBS=$LIBS
  LIBS="$LIBS $X_PRE_LIBS $X_LIBS -lXss -lXext -lX11"
  AC_CHECK_LIB(Xss, XScreenSaverQueryInfo, [
    TYPING_BREAK="typing-break"
    SCREENSAVER_LIBS="$X_PRE_LIBS $X_LIBS -lXss -lXext -lX11"], [
    LIBS=$save_LIBS
    LIBS="$LIBS $X_PRE_LIBS $X_LIBS -lXext -lX11"
    AC_CHECK_LIB(Xext, XScreenSaverQueryInfo, [
      TYPING_BREAK="typing-break"
      SCREENSAVER_LIBS="$X_PRE_LIBS $X_LIBS -lXext -lX11"],[])])
  LIBS=$save_LIBS])
AC_SUBST(TYPING_BREAK)
AC_SUBST(SCREENSAVER_LIBS)

dnl ==============================================
dnl Check GTK+ API version
dnl ==============================================

AC_MSG_CHECKING([which gtk+ version to compile against])
AC_ARG_WITH([gtk],
  [AS_HELP_STRING([--with-gtk=2.0|3.0],[which gtk+ version to compile against (default: 2.0)])],
  [case "$with_gtk" in
     2.0|3.0) ;;
     *) AC_MSG_ERROR([invalid gtk version specified]) ;;
   esac],
  [with_gtk=2.0])
AC_MSG_RESULT([$with_gtk])

case "$with_gtk" in
  2.0) GTK_API_VERSION=2.0
       GTK_REQUIRED=2.24.0
       MATE_DESKTOP_REQUIRED=1.7.3
       LIBCANBERRA_API_VERSION=
       LIBUNIQUE_API_VERSION=1.0
       ENGINES_FOLDER="engines"
       ;;
  3.0) GTK_API_VERSION=3.0
       GTK_REQUIRED=3.0.0
       MATE_DESKTOP_REQUIRED=1.9.3
       LIBCANBERRA_API_VERSION=3
       LIBUNIQUE_API_VERSION=3.0
       ENGINES_FOLDER="theming-engines"
       ;;
esac
AC_SUBST(GTK_API_VERSION)

PKG_CHECK_MODULES(LIBCANBERRA_GTK, libcanberra-gtk$LIBCANBERRA_API_VERSION, [
    CANBERRA_GTK=1
    AC_SUBST(LIBCANBERRA_GTK_CFLAGS)
    AC_SUBST(LIBCANBERRA_GTK_LIBS)
    ], [:])
AM_CONDITIONAL(HAVE_LIBCANBERRA_GTK, test "x$CANBERRA_GTK" = "x1")

AM_CONDITIONAL(HAVE_TYPING_BREAK, test "x$TYPING_BREAK" = xtyping-break)

dnl app indicator
APPINDICATOR_REQUIRED=0.0.7

AC_ARG_ENABLE(appindicator,
                        AS_HELP_STRING([--enable-appindicator[=@<:@no/auto/yes@:>@]],[Build support for application indicators ]),
                        [enable_appindicator=$enableval],
                        [enable_appindicator="auto"])

if test x$enable_appindicator = xauto ; then
        PKG_CHECK_EXISTS([appindicator-0.1 >= $APPINDICATOR_REQUIRED],
                         enable_appindicator="yes",
                         enable_appindicator="no")
fi

if test x$enable_appindicator = xyes ; then
        PKG_CHECK_EXISTS([appindicator-0.1 >= $APPINDICATOR_REQUIRED],,
                         AC_MSG_ERROR([appindicator-0.1 is not installed]))
        PKG_CHECK_MODULES(APP_INDICATOR,
                        appindicator-0.1 >= $APPINDICATOR_REQUIRED)
        AC_SUBST(APP_INDICATOR_CFLAGS)
        AC_SUBST(APP_INDICATOR_LIBS)
        AC_DEFINE(HAVE_APP_INDICATOR, 1, [Have AppIndicator])
fi
AM_CONDITIONAL(HAVE_APP_INDICATOR, test x"$enable_appindicator" = xyes)

dnl keyboard-properties-capplet
savecppflags=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $X_CFLAGS"
AC_CHECK_HEADERS([X11/Xlib.h])
AC_CHECK_LIB(Xxf86misc, XF86MiscQueryExtension, [
  AC_CHECK_HEADERS([X11/extensions/xf86misc.h], [XF86MISC_LIBS="-lXxf86misc"],[],
[#if HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#endif
])])
AC_SUBST(XF86MISC_LIBS)
AC_CHECK_HEADERS(X11/extensions/XKB.h)
CPPFLAGS=$savecppflags

AC_CHECK_LIB(m, floor)

dnl ==============================================
dnl Check that we meet the  dependencies
dnl ==============================================

PKG_CHECK_MODULES(GMODULE,gmodule-2.0,[GMODULE_ADD="gmodule-2.0"],[GMODULE_ADD=""])
COMMON_MODULES="gtk+-$GTK_API_VERSION >= $GTK_REQUIRED dnl
 $GMODULE_ADD dnl
 glib-2.0 >= 2.36 dnl
 gthread-2.0 dnl
 gio-2.0 dnl
 gio-unix-2.0 dnl
 libxml-2.0 dnl
 mate-desktop-2.0 >= $MATE_DESKTOP_REQUIRED"
PKG_CHECK_MODULES(CAPPLET, $COMMON_MODULES)
PKG_CHECK_MODULES(MATECC, $COMMON_MODULES libmate-menu >= 1.1.0)
PKG_CHECK_MODULES(MATECC_SHELL, $COMMON_MODULES libmate-menu unique-$LIBUNIQUE_API_VERSION)
PKG_CHECK_MODULES(DBUS, dbus-1 dbus-glib-1)
PKG_CHECK_MODULES(MATE_DESKTOP, mate-desktop-2.0 >= MATE_DESKTOP_REQUIRED)
PKG_CHECK_MODULES(DEFAULT_APPLICATIONS_CAPPLET, libxml-2.0)
PKG_CHECK_MODULES(MARCO, libmarco-private >= 1.9.1)
PKG_CHECK_MODULES(TYPING, $GMODULE_ADD glib-2.0 > 2.36 gio-2.0 gtk+-$GTK_API_VERSION)
PKG_CHECK_MODULES(GSD_DBUS, mate-settings-daemon >= 1.5.2)
PKG_CHECK_MODULES(GIO, gio-2.0)

PKG_CHECK_MODULES([DCONF], [dconf >= 0.13.4],
   [AC_DEFINE([HAVE_DCONF_0_13], [1], [Use DCONF >= 0.13.4])],
   [PKG_CHECK_MODULES([DCONF], [dconf >= 0.10.0],
       [AC_DEFINE([HAVE_DCONF_0_10], [1], [Use DCONF 0.10.0])
    ])
])
AC_SUBST(DCONF_CFLAGS)
AC_SUBST(DCONF_LIBS)

gtk_lib_dir=`$PKG_CONFIG --variable libdir gtk+-$GTK_API_VERSION`
gtk_binary_version=`$PKG_CONFIG --variable gtk_binary_version gtk+-$GTK_API_VERSION`
GTK_ENGINE_DIR="$gtk_lib_dir/gtk-$GTK_API_VERSION/$gtk_binary_version/$ENGINES_FOLDER"
AC_SUBST(GTK_ENGINE_DIR)

PKG_CHECK_MODULES(GLIB, glib-2.0 $GMODULE_ADD)

dnl
dnl Check dependencies of libmate-slab
dnl
PKG_CHECK_MODULES(LIBSLAB, [
			   gio-2.0 mate-desktop-2.0 librsvg-2.0 libmate-menu pango
			   ],
			   have_libmateslab=yes,
			   have_libmateslab=no)
WARN_CFLAGS="-Wall"

AC_SUBST(LIBMATESLAB_CFLAGS)
AC_SUBST(LIBMATESLAB_LIBS)
AC_SUBST(WARN_CFLAGS)

AM_CONDITIONAL(HAVE_LIBMATESLAB, [test $have_libmateslab = yes])

dnl
dnl Check for Xft version 2; we build in extra functionality to the font capplet
dnl when we have it.
dnl
xft_modules=
if $PKG_CONFIG --exists xft ; then
  xft_modules="xft"
  AC_DEFINE(HAVE_XFT2,,[Define if Xft functionality is available])
fi

PKG_CHECK_MODULES(FONT_CAPPLET, $COMMON_MODULES $xft_modules fontconfig)
PKG_CHECK_MODULES(FONT_VIEWER, $COMMON_MODULES $xft_modules freetype2)

PKG_CHECK_MODULES(AT_CAPPLET, $COMMON_MODULES)

PKG_CHECK_MODULES(DISPLAY_CAPPLET, $COMMON_MODULES)
DISPLAY_CAPPLET_LIBS="$DISPLAY_CAPPLET_LIBS"

CAPPLET_LIBS="$CAPPLET_LIBS $x_libs"
MATECC_LIBS="$MATECC_LIBS $x_libs"

dnl
dnl Check for XCursor support.  If it exists, then we compile the
dnl mouse capplet with support for it turned on
dnl
have_xcursor=no
PKG_CHECK_MODULES(XCURSOR, [
	xcursor
	],
	have_xcursor=yes
	AC_DEFINE(HAVE_XCURSOR, 1, [Define if the Xcursor extension is available]),
	have_xcursor=no)
AM_CONDITIONAL(HAVE_XCURSOR, [test $have_xcursor=yes])
AC_SUBST(XCURSOR_CFLAGS)
AC_SUBST(XCURSOR_LIBS)

if test x$have_xcursor = xyes; then
  AC_MSG_CHECKING([for XCURSOR cursor location])
  XCURSOR_ICONDIR=$($PKG_CONFIG --variable=icondir xcursor)
  if test "x$XCURSOR_ICONDIR" != x; then
    AC_DEFINE_UNQUOTED(XCURSOR_ICONDIR, "${XCURSOR_ICONDIR}", [Define to the xcursor icon path])
  else
    XCURSOR_ICONDIR=none
  fi
  AC_MSG_RESULT([${XCURSOR_ICONDIR}])
fi

CAPPLET_LIBS="$CAPPLET_LIBS $XCURSOR_LIBS"

dnl =============================================
dnl X Input library >= 1.2 with property support
dnl =============================================
have_xinput=no
PKG_CHECK_MODULES(XINPUT, [xi >= 1.2],
	have_xinput=yes
	AC_DEFINE(HAVE_XINPUT, 1, [Define if the XInput extension is available]),
	have_xinput=no)
AM_CONDITIONAL(HAVE_XINPUT, [test $have_xinput=yes])
AC_SUBST(XINPUT_CFLAGS)
AC_SUBST(XINPUT_LIBS)

CAPPLET_LIBS="$CAPPLET_LIBS $XINPUT_LIBS"

dnl ==============
dnl gswitchit
dnl ==============
PKG_CHECK_MODULES(LIBMATEKBD, [libmatekbd >= 1.1.0 libxklavier >= 4.0])
AC_SUBST(LIBMATEKBD_CFLAGS)
AC_SUBST(LIBMATEKBD_LIBS)

PKG_CHECK_MODULES(LIBMATEKBDUI, [libmatekbdui >= 1.1.0])
AC_SUBST(LIBMATEKBDUI_CFLAGS)
AC_SUBST(LIBMATEKBDUI_LIBS)

dnl ==============================================
dnl End: Check that we meet the  dependencies
dnl ==============================================

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal, no)

if test x"$GLIB_GENMARSHAL" = xno; then
  AC_MSG_ERROR([glib-genmarshal executable not found in your path - should be installed with glib])
fi

AC_SUBST(GLIB_GENMARSHAL)

dnl ==============================================
dnl Special GSettings section
dnl ==============================================

GLIB_GSETTINGS

dnl ==============================================
dnl Define the main variables
dnl ==============================================
COMMON_CFLAGS="-I\$(top_srcdir)/capplets/common"
COMMON_LIBS="\$(top_builddir)/capplets/common/libcommon.la"

EXTRA_CFLAGS="-I\$(top_srcdir)/ -DG_LOG_DOMAIN=\"\\\"\$(cappletname)-properties\\\"\""

MATECC_CAPPLETS_CFLAGS="${COMMON_CFLAGS} ${CAPPLET_CFLAGS} ${EXTRA_CFLAGS} ${DBUS_CFLAGS}"
MATECC_CAPPLETS_LIBS="${COMMON_LIBS} ${CAPPLET_LIBS} ${DBUS_LIBS}"

MATECC_CAPPLETS_CLEANFILES="\$(desktop) \$(desktop).in"
MATECC_CAPPLETS_EXTRA_DIST="ChangeLog \$(desktop).in.in \$(cappletname)-capplet.png \$(pixmaps_DATA)"

AC_SUBST(MATECC_CAPPLETS_EXTRA_DIST)
AC_SUBST(MATECC_CAPPLETS_CLEANFILES)
AC_SUBST(MATECC_CAPPLETS_CFLAGS)
AC_SUBST(MATECC_CAPPLETS_LIBS)

dnl =======================================
dnl Update Mime Database
dnl =======================================

AC_PATH_PROG(UPDATE_MIME_DATABASE, update-mime-database, no)

AC_ARG_ENABLE(update-mimedb,
   AC_HELP_STRING([--disable-update-mimedb],
                   [do not update mime database after installation]),,
                   enable_update_mimedb=yes)
AM_CONDITIONAL(ENABLE_UPDATE_MIMEDB, test x$enable_update_mimedb = xyes)

dnl ****************************************************************************
dnl * using Ubuntu hacks
dnl ****************************************************************************
AC_ARG_ENABLE(ubuntu,
	AS_HELP_STRING([--enable-ubuntu], [Enable Ubuntu Counter Hacks]), enable_ubuntu=yes, enable_ubuntu=no)

if test "x$enable_ubuntu" = "xyes" ; then
	# we define a UBUNTU on code
	MATE_DESKTOP_CFLAGS="$MATE_DESKTOP_CFLAGS -DUBUNTU"

	ENABLE_UBUNTU=yes
else
	ENABLE_UBUNTU=no
fi

AC_CONFIG_FILES([
Makefile
capplets/Makefile
capplets/about-me/Makefile
capplets/about-me/mate-about-me.desktop.in
capplets/about-me/icons/Makefile
capplets/accessibility/Makefile
capplets/accessibility/at-properties/mate-at-properties.desktop.in
capplets/accessibility/at-properties/Makefile
capplets/appearance/Makefile
capplets/appearance/data/Makefile
capplets/appearance/data/mate-appearance-properties.desktop.in
capplets/appearance/data/mate-theme-installer.desktop.in
capplets/common/Makefile
capplets/default-applications/Makefile
capplets/default-applications/mate-default-applications-properties.desktop.in
capplets/default-applications/mate-default-applications.pc
capplets/display/Makefile
capplets/display/mate-display-properties.desktop.in
capplets/keybindings/Makefile
capplets/keybindings/mate-keybindings.pc
capplets/keybindings/mate-keybinding.desktop.in
capplets/keyboard/Makefile
capplets/keyboard/mate-keyboard.desktop.in
capplets/mouse/Makefile
capplets/mouse/mate-settings-mouse.desktop.in
capplets/network/Makefile
capplets/network/mate-network-properties.desktop.in
capplets/windows/Makefile
capplets/windows/mate-window-properties.desktop.in
font-viewer/Makefile
font-viewer/mate-font-viewer.desktop.in
help/Makefile
libwindow-settings/Makefile
libwindow-settings/mate-window-settings-2.0.pc
man/Makefile
po/Makefile.in
libslab/mate-slab.pc
libslab/Makefile
shell/Makefile
shell/matecc.desktop.in
typing-break/Makefile
])

dnl due to a bug in intltool we need to expand something from the root last control-center.spec
AC_OUTPUT

dnl ---------------------------------------------------------------------------
dnl - Show summary
dnl ---------------------------------------------------------------------------

echo "
              mate-control-center

        GTK+ API version:      ${GTK_API_VERSION}
        Appindicator:          ${enable_appindicator}
        Libmate-slab:          ${have_libmateslab}
        XCursor support:       ${have_xcursor}
        XInput support:        ${have_xinput}
        Ubuntu hacks:          ${enable_ubuntu}

"
