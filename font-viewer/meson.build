cflags = [
  '-DDIRECTORY_DIR="@0@"'.format(mcc_directorydir),
  '-DMATELOCALEDIR="@0@"'.format(mcc_localedir),
]

libm = cc.find_library('libm', required: false)
deps = [
  common_deps,
  pango_dep,
  fontconfig_dep,
  libm,
]
executable(
  'mate-font-viewer',
  sources : [
    'sushi-font-loader.c',
    'font-model.c',
    'font-utils.c',
    'gd-main-toolbar.c',
    'sushi-font-widget.c',
    'font-view.c'
  ],
  include_directories : config_inc,
  dependencies : deps,
  c_args : cflags,
  install : true,
  install_dir : get_option('bindir')
)

executable(
  'mate-thumbnail-font',
  sources : [
    'sushi-font-loader.c',
    'font-thumbnailer.c',
    'totem-resources.c',
  ],
  include_directories : config_inc,
  dependencies : deps,
  c_args : cflags,
  install : true,
)

i18n.merge_file(
  input: 'mate-font-viewer.desktop.in',
  output: 'mate-font-viewer.desktop',
  type: 'desktop',
  po_dir: join_paths(meson.project_source_root(), 'po'),
  install: true,
  install_dir: mcc_desktopdir, 
)

install_data('mate-font-viewer.thumbnailer',
  install_dir : join_paths(mcc_datadir, 'thumbnailers'),
)
