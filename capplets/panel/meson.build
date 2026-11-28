desktop = 'mate-panel-properties.desktop'

i18n.merge_file(
  type: 'desktop',
  input: desktop + '.in',
  output: desktop,
  po_dir: po_dir,
  install: true,
  install_dir: mcc_desktopdir
)

sources = files(
  'mate-panel-properties.c',
  'pp-config.c',
  'pp-monitors.c',
  'pp-panel.c',
  'pp-window.c'
)

executable(
  'mate-panel-properties',
  sources : sources,
  include_directories: config_inc,
  dependencies : [common_deps, libcommon_dep],
  c_args : cflags,
  install : true,
  install_dir : get_option('bindir')
)

test('panel-monitors', executable(
  'test-panel-monitors',
  sources : files('test-panel-monitors.c', 'pp-config.c', 'pp-monitors.c', 'pp-panel.c', 'pp-window.c'),
  include_directories: config_inc,
  dependencies : [common_deps, libcommon_dep],
  c_args : cflags,
))
