desktop = 'mate-default-applications-properties.desktop'

i18n.merge_file(
  type: 'desktop',
  input: desktop + '.in',
  output: desktop,
  po_dir: po_dir,
  install: true,
  install_dir: mcc_desktopdir
)

sources = files(
  'mate-da-capplet.c'
)

sources += gnome.compile_resources(
  'mate-default-applications-properties-resources',
  'org.mate.mcc.da.gresource.xml',
  c_name: 'default_applications',
  export: true
)

cflags += [
  '-DAPPLICATIONSDIR="@0@"'.format(mcc_desktopdir),
]

executable(
  'mate-default-applications-properties',
  sources : sources,
  include_directories: config_inc,
  dependencies : [common_deps, libcommon_dep],
  c_args : cflags,
  install : true,
  install_dir : get_option('bindir')
)

pc_conf = configuration_data()
pc_conf.set('prefix', mcc_prefix)
pc_conf.set('datarootdir', mcc_datadir)
pc_conf.set('datadir', mcc_datadir)
pc_conf.set('PACKAGE', meson.project_name())
pc_conf.set('VERSION', meson.project_version())

pc = 'mate-default-applications.pc'

configure_file(
  input: pc + '.in',
  output: pc,
  install: true,
  install_dir: join_paths(mcc_libdir, 'pkgconfig'),
  configuration: pc_conf
)

subdir('icons')
