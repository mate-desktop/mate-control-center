desktop = 'mate-settings-mouse.desktop'

i18n.merge_file(
  type: 'desktop',
  input: desktop + '.in',
  output: desktop,
  po_dir: po_dir,
  install: true,
  install_dir: mcc_desktopdir
)

sources = files(
  'mate-mouse-properties.c',
  'msd-input-helper.c',
  'msd-input-wayland.c',
)

mouse_deps = [common_deps, libcommon_dep, xi_dep]

if have_wayland_protocols
  input_device_xml = mate_wayland_protocols_dep.get_variable(pkgconfig: 'pkgdatadir') / 'mate-input-device-v1.xml'
  sources += custom_target('mate-input-device-v1-client-header',
    input: input_device_xml,
    output: 'mate-input-device-v1-client-protocol.h',
    command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
  )
  sources += custom_target('mate-input-device-v1-protocol-code',
    input: input_device_xml,
    output: 'mate-input-device-v1-protocol.c',
    command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
  )
  mouse_deps += wayland_client_dep
endif

sources += gnome.compile_resources(
  'mate-mouse-properties-resources',
  'org.mate.mcc.mouse.gresource.xml',
  c_name: 'mouse',
  export: true
)

cflags = [
  '-DMATECC_DATA_DIR="@0@"'.format(mcc_pkgdatadir),
]

executable(
  'mate-mouse-properties',
  sources : sources,
  include_directories: config_inc,
  dependencies : mouse_deps,
  c_args : cflags,
  install : true,
  install_dir : get_option('bindir')
)
