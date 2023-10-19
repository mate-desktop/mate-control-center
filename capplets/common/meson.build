common_inc = include_directories('.')
cflags = [
  '-DMATECC_DATA_DIR="@0@"'.format(mcc_pkgdatadir),
  '-DMATEDATADIR="@0@"'.format(mcc_datadir),
  '-DMATELOCALEDIR="@0@"'.format(mcc_localedir),
  '-DGTK_ENGINE_DIR="/usr/lib/gtk-3.0/3.0.0/theming-engines"',
  '-DG_LOG_DOMAIN="capplet-common"',
  '-DINSTALL_PREFIX="@0@"'.format(mcc_prefix),
  '-DPIXMAP_DIR="@0@"'.format(mcc_pixmaps),
]

sources = [
  'activate-settings-daemon.c',
  'capplet-util.c',
  'dconf-util.c',
  'file-transfer-dialog.c',
  'mate-theme-apply.c',
  'mate-theme-info.c',
  'gtkrc-utils.c',
  'theme-thumbnail.c',
  'wm-common.c',
]
gmodule_dep = cc.find_library('gmodule-2.0')
libcommon = static_library(
  'common',
  sources: sources,
  include_directories: config_inc,
  dependencies: [common_deps,gmodule_dep],
  c_args: cflags,
  #link_args: ldflags,
  install: false,
)

libcommon_dep = declare_dependency(
  include_directories: common_inc,
  link_with: libcommon
)

executable(
  'mate-theme-test',
  sources : 'mate-theme-test.c',
  dependencies : [common_deps, libcommon_dep],
  include_directories: config_inc,
  c_args : cflags,
  install : false,
)
