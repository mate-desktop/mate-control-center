desktop = 'mate-keybinding.desktop'

i18n.merge_file(
  type: 'desktop',
  input: desktop + '.in',
  output: desktop,
  po_dir: po_dir,
  install: true,
  install_dir: mcc_desktopdir
)

sources = files(
  'mate-keybinding-properties.c',
  'eggcellrendererkeys.c',
  'eggaccelerators.c'
)

sources += gnome.compile_resources(
  'mate-keybinding-properties-resources',
  'org.mate.mcc.keybindings.gresource.xml',
  c_name: 'keybindings',
  export: true
)

cflags += [
  '-DMATECC_DATA_DIR="@0@"'.format(mcc_pkgdatadir),
]

executable(
  'mate-keybinding-properties',
  sources : sources,
  include_directories: config_inc,
  dependencies : [common_deps, libcommon_dep, dconf_dep],
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

pc = 'mate-keybindings.pc'

configure_file(
  input: pc + '.in',
  output: pc,
  install: true,
  install_dir: join_paths(mcc_libdir, 'pkgconfig'),
  configuration: pc_conf
)

xml_files = [
  '00-multimedia-key.xml',
  '01-desktop-key.xml'
]

foreach file: xml_files
  i18n.merge_file(
    input: file + '.in',
    output: file,
    po_dir: po_dir,
    data_dirs: its_dir,
    install: true,
    install_dir: join_paths(mcc_pkgdatadir, 'keybindings')
  )
endforeach
