desktop = 'mate-keyboard.desktop'

i18n.merge_file(
  type: 'desktop',
  input: desktop + '.in',
  output: desktop,
  po_dir: po_dir,
  install: true,
  install_dir: mcc_desktopdir
)

sources = files(
  'mate-keyboard-properties.c',
  'mate-keyboard-properties-a11y.c',
  'mate-keyboard-properties-xkb.c',
  'mate-keyboard-properties-xkbmc.c',
  'mate-keyboard-properties-xkblt.c',
  'mate-keyboard-properties-xkbltadd.c',
  'mate-keyboard-properties-xkbot.c',
  'mate-keyboard-properties-xkbpv.c'
)

sources += gnome.compile_resources(
  'mate-keyboard-properties-resources',
  'org.mate.mcc.keyboard.gresource.xml',
  c_name: 'keyboard',
  export: true
)

cflags += [
  '-DMATECC_DATA_DIR="@0@"'.format(mcc_pkgdatadir),
]

executable(
  'mate-keyboard-properties',
  sources : sources,
  include_directories: config_inc,
  dependencies : [common_deps, libcommon_dep, xklavier_dep, matekbd_dep, matekbdui_dep],
  c_args : cflags,
  install : true,
  install_dir : get_option('bindir')
)
