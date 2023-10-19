sources = files(
  'xrandr-capplet.c',
  'scrollarea.c',
)

sources += gnome.compile_resources(
  'mate-display-properties-resources',
  'org.mate.mcc.display.gresource.xml',
  c_name: 'display',
  export: true
)

sources += gnome.genmarshal('foo-marshal', prefix : 'foo_marshal', sources : 'foo-marshal.list')
cflags = [
  '-DDATADIR="@0@"'.format(mcc_datadir),
  '-DMATECC_DATA_DIR="@0@"'.format(mcc_pkgdatadir),
  '-DSBINDIR="@0@"'.format(mcc_sbindir),
  '-DMATELOCALEDIR="@0@"'.format(mcc_localedir),
]

executable(
  'mate-display-properties',
  sources : sources,
  include_directories: config_inc,
  dependencies : [common_deps, libcommon_dep],
  c_args : cflags,
  install : true,
  install_dir : get_option('bindir')
)

executable(
  'mate-display-properties-install-systemwide',
  sources : 'mate-display-properties-install-systemwide.c',
  include_directories: config_inc,
  dependencies : [common_deps, libcommon_dep],
  c_args : cflags,
  install : true,
  install_dir : get_option('sbindir')
)

desktop = 'mate-display-properties.desktop'

i18n.merge_file(
  type: 'desktop',
  input: desktop + '.in',
  output: desktop,
  po_dir: po_dir,
  install: true,
  install_dir: mcc_desktopdir
)

policy = 'org.mate.randr.policy'

i18n.merge_file(
  input: policy + '.in',
  output: policy,
  po_dir: po_dir,
  install: true,
  install_dir: mcc_policydir,
)
