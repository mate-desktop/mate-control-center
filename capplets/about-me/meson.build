desktop = 'mate-about-me.desktop'

i18n.merge_file(
  type: 'desktop',
  input: desktop + '.in',
  output: desktop,
  po_dir: po_dir,
  install: true,
  install_dir: mcc_desktopdir
)

sources = files(
  'e-image-chooser.c',
  'mate-about-me.c',
  'mate-about-me-fingerprint.c',
  'mate-about-me-password.c'
)

sources += gnome.compile_resources(
  'mate-about-me-resources',
  'org.mate.mcc.am.gresource.xml',
  c_name: 'about_me',
  export: true
)

cflags += [
  '-DDATADIR="@0@"'.format(mcc_datadir),
  '-DMATECC_DATA_DIR="@0@"'.format(mcc_pkgdatadir),
  '-DMATECC_PIXMAP_DIR="@0@"'.format(mcc_pixmaps),
  '-DMATELOCALEDIR="@0@"'.format(mcc_localedir),
]

executable(
  'mate-about-me',
  sources : sources,
  include_directories: config_inc,
  dependencies : [common_deps, libcommon_dep],
  c_args : cflags,
  install : true,
  install_dir : get_option('bindir')
)
