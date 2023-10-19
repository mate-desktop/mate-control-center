desktop = 'mate-network-properties.desktop'

i18n.merge_file(
  type: 'desktop',
  input: desktop + '.in',
  output: desktop,
  po_dir: po_dir,
  install: true,
  install_dir: mcc_desktopdir
)

sources = files(
  'mate-network-properties.c',
)

sources += gnome.compile_resources(
  'mate-network-properties-resources',
  'org.mate.mcc.network.gresource.xml',
  c_name: 'network',
  export: true
)

cflags += [
  '-DMATECC_DATA_DIR="@0@"'.format(mcc_pkgdatadir),
]

executable(
  'mate-network-properties',
  sources : sources,
  include_directories: config_inc,
  dependencies : [common_deps, libcommon_dep],
  c_args : cflags,
  install : true,
  install_dir : get_option('bindir')
)
