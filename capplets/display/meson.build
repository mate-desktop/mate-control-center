sources = files(
  'xrandr-capplet.c',
  'wlrandr-capplet.c',
  'display-backend.c',
  'display-backend-wlrandr.c',
  'display-head.c',
  'display-layout.c',
  'scrollarea.c',
)

display_deps = [common_deps, libcommon_dep]

if have_wayland_protocols
  mate_wayland_protocols_dir = mate_wayland_protocols_dep.get_variable(pkgconfig: 'pkgdatadir')
  display_protocols = [
    ['wlr-output-management-unstable-v1', files('protocols/wlr-output-management-unstable-v1.xml')],
    ['mate-output-v1', mate_wayland_protocols_dir / 'mate-output-v1.xml'],
  ]
  foreach protocol : display_protocols
    sources += custom_target(protocol[0] + '-client-header',
      input: protocol[1],
      output: protocol[0] + '-client-protocol.h',
      command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
    )
    sources += custom_target(protocol[0] + '-protocol-code',
      input: protocol[1],
      output: protocol[0] + '-protocol.c',
      command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
    )
  endforeach
  sources += files('display-backend-protocol.c')
  display_deps += wayland_client_dep
endif

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
  dependencies : display_deps,
  c_args : cflags,
  install : true,
  install_dir : get_option('bindir')
)

test('display-head', executable('test-display-head',
  sources : ['test-display-head.c', 'display-head.c'],
  include_directories: config_inc,
  dependencies : glib_dep,
))

test('display-layout', executable('test-display-layout',
  sources : ['test-display-layout.c', 'display-layout.c', 'display-head.c'],
  include_directories: config_inc,
  dependencies : glib_dep,
))

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
