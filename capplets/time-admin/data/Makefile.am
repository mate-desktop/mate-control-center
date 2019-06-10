mapdir = $(datadir)/mate-time-admin/map
dist_map_DATA = \
map/backward                map/timezone_12.png        map/timezone_4.5_dim.png   map/timezone_6.png  \
map/bg_dim.png              map/timezone_13_dim.png    map/timezone_-4.5.png      map/timezone_-7_dim.png \
map/bg.png                  map/timezone_13.png        map/timezone_4.5.png       map/timezone_7_dim.png \
map/cc.png                  map/timezone_14_dim.png    map/timezone_-4_dim.png    map/timezone_-7.png \
map/pin.png                 map/timezone_14.png        map/timezone_4_dim.png     map/timezone_7.png \
map/timezone_0_dim.png      map/timezone_-1_dim.png    map/timezone_-4.png        map/timezone_8.5_dim.png \
map/timezone_0.png          map/timezone_1_dim.png     map/timezone_4.png         map/timezone_8.5.png \
map/timezone_10.5_dim.png   map/timezone_-1.png        map/timezone_-5.5_dim.png  map/timezone_8.75_dim.png \
map/timezone_10.5.png       map/timezone_1.png         map/timezone_5.5_dim.png   map/timezone_8.75.png \
map/timezone_-10_dim.png    map/timezone_-2_dim.png    map/timezone_-5.5.png      map/timezone_-8_dim.png \
map/timezone_10_dim.png     map/timezone_2_dim.png     map/timezone_5.5.png       map/timezone_8_dim.png \
map/timezone_-10.png        map/timezone_-2.png        map/timezone_5.75_dim.png  map/timezone_-8.png \
map/timezone_10.png         map/timezone_2.png         map/timezone_5.75.png      map/timezone_8.png \
map/timezone_11.5_dim.png   map/timezone_-3.5_dim.png  map/timezone_-5_dim.png    map/timezone_-9.5_dim.png \
map/timezone_11.5.png       map/timezone_3.5_dim.png   map/timezone_5_dim.png     map/timezone_9.5_dim.png \
map/timezone_-11_dim.png    map/timezone_-3.5.png      map/timezone_-5.png        map/timezone_-9.5.png \
map/timezone_11_dim.png     map/timezone_3.5.png       map/timezone_5.png         map/timezone_9.5.png \
map/timezone_-11.png        map/timezone_-3_dim.png    map/timezone_6.5_dim.png   map/timezone_-9_dim.png \
map/timezone_11.png         map/timezone_3_dim.png     map/timezone_6.5.png       map/timezone_9_dim.png \
map/timezone_12.75_dim.png  map/timezone_-3.png        map/timezone_-6_dim.png    map/timezone_-9.png \
map/timezone_12.75.png      map/timezone_3.png         map/timezone_6_dim.png     map/timezone_9.png \
map/timezone_12_dim.png     map/timezone_-4.5_dim.png  map/timezone_-6.png

icons16dir = $(datadir)/icons/hicolor/16x16/apps
dist_icons16_DATA = hicolor/16x16/apps/mate-times-admin.png
icons22dir = $(datadir)/icons/hicolor/22x22/apps
dist_icons22_DATA = hicolor/22x22/apps/mate-times-admin.png
icons24dir = $(datadir)/icons/hicolor/24x24/apps
dist_icons24_DATA = hicolor/24x24/apps/mate-times-admin.png
icons32dir = $(datadir)/icons/hicolor/32x32/apps
dist_icons32_DATA = hicolor/32x32/apps/mate-times-admin.png
icons48dir = $(datadir)/icons/hicolor/48x48/apps
dist_icons48_DATA = hicolor/48x48/apps/mate-times-admin.png

icondir = $(datadir)/mate-time-admin/icon
dist_icon_DATA = icon/mate-times-admin.png

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files = mate-time-admin.desktop.in 
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

CLEANFILES = $(desktop_DATA)
EXTRA_DIST = \
    $(desktop_in_files)
-include $(top_srcdir)/git.mk
