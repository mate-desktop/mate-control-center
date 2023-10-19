sources = files(
  'appearance-desktop.c',
  'appearance-font.c',
  'appearance-main.c',
  'appearance-style.c',
  'appearance-support.c',
  'appearance-themes.c',
  'appearance-ui.c',
  'mate-wp-info.c',
  'mate-wp-item.c',
  'mate-wp-xml.c',
  'theme-installer.c',
  'theme-save.c',
  'theme-util.c'
)

sources += gnome.compile_resources(
  'mate-appearance-properties-resources',
  'org.mate.mcc.appearance.gresource.xml',
  c_name: 'appearance',
  export: true
)

cflags += [
  '-DDATADIR="@0@"'.format(mcc_datadir),
  '-DMATECC_DATA_DIR="@0@"'.format(mcc_pkgdatadir),
  '-DMATECC_PIXMAP_DIR="@0@"'.format(mcc_pixmaps),
  '-DMATELOCALEDIR="@0@"'.format(mcc_localedir),
  '-DBACKGROUND_DATADIR="@0@"'.format(mcc_background),
  '-DWALLPAPER_DATADIR="@0@"'.format(mcc_wallpaperdir),
  '-DMATE_DESKTOP_USE_UNSTABLE_API',
]

deps = [
  common_deps,
  libxml_dep,
  accounts_dep
]

executable(
  'mate-appearance-properties',
  sources : sources,
  include_directories: config_inc,
  dependencies : [deps, libcommon_dep],
  c_args : cflags,
  install : true,
  install_dir : get_option('bindir')
)

subdir('data')
