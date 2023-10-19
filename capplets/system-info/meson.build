desktop = 'mate-system-info.desktop'

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
  'mate-system-info.c',
  'info-cleanup.c'
)

sources += gnome.compile_resources(
  'mate-system-info-resources',
  'system-info.gresource.xml',
  c_name: 'mate_system_info',
  export: true
)

cflags += [
  '-DMATECC_DATA_DIR="@0@"'.format(mcc_pkgdatadir),
]

executable(
  'mate-system-info',
  sources : sources,
  include_directories: config_inc,
  dependencies : [common_deps, libcommon_dep, gtop_dep, udisks2_dep],
  c_args : cflags,
  install : true,
  install_dir : get_option('bindir')
)
