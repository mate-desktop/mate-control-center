sources = files(
  'main.c',
  'time-map.c',
  'time-share.c',
  'time-tool.c',
  'time-zone.c'
)

sources += gnome.compile_resources(
  'mate-time-admin-resources',
  'org.mate.mcc.ta.gresource.xml',
  c_name: 'time_admin',
  export: true
)

cflags += [
  '-DMATECC_DATA_DIR="@0@"'.format(mcc_pkgdatadir),
  '-DTIMPZONEDIR="@0@"'.format(mcc_timezonedir),
]

executable(
  'mate-time-admin',
  sources : sources,
  include_directories: config_inc,
  dependencies : [common_deps, libcommon_dep, polkit_dep, m_dep],
  c_args : cflags,
  install : true,
  install_dir : get_option('bindir')
)
