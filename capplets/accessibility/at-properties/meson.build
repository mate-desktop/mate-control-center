desktop = 'mate-at-properties.desktop'

i18n.merge_file(
  type: 'desktop',
  input: desktop + '.in',
  output: desktop,
  po_dir: po_dir,
  install: true,
  install_dir: mcc_desktopdir
)

sources = files(
  'main.c',
  'dm-util.c'
)

sources += gnome.compile_resources(
  'mate-at-properties-resources',
  'org.mate.mcc.accessibility.at.gresource.xml',
  c_name: 'accessibility',
  export: true
)

cflags += [
  '-DDATADIR="@0@"'.format(mcc_datadir),
  '-DMATECC_DATA_DIR="@0@"'.format(mcc_pkgdatadir),
  '-DPIXMAPDIR="@0@"'.format(mcc_pixmaps),
  '-DMATELOCALEDIR="@0@"'.format(mcc_localedir),
]

executable(
  'mate-at-properties',
  sources : sources,
  include_directories: config_inc,
  dependencies : [common_deps, libcommon_dep],
  c_args : cflags,
  install : true,
  install_dir : get_option('bindir')
)

install_data (
  'at-startup.png',
  install_dir : mcc_pixmaps
)

install_data (
  'at-support.png',
  install_dir : mcc_pixmaps
)
