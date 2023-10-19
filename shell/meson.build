directory_desktop = 'matecc.directory'
desktop = 'matecc.desktop'

i18n.merge_file(
  type: 'desktop',
  input: desktop + '.in',
  output: desktop,
  po_dir: po_dir,
  install: true,
  install_dir: mcc_desktopdir
)

i18n.merge_file(
  type: 'desktop',
  input: directory_desktop + '.desktop.in',
  output: directory_desktop,
  po_dir: po_dir,
  install: true,
  install_dir: join_paths(mcc_datadir, 'desktop-directories')
)

sources = files(
  'application-tile.c',
  'app-resizer.c',
  'app-shell.c',
  'bookmark-agent.c',
  'control-center.c',
  'double-click-detector.c',
  'libslab-utils.c',
  'mate-utils.c',
  'nameplate-tile.c',
  'search-bar.c',
  'shell-window.c',
  'slab-mate-util.c',
  'slab-section.c',
  'themed-icon.c',
  'tile-action.c',
  'tile.c'
)

sources += gnome.genmarshal('nld-marshal', prefix : 'nld_marshal', sources : 'nld-marshal.list' )
cflags = [
  '-DDATADIR="@0@"'.format(mcc_datadir),
  '-DMATELOCALEDIR="@0@"'.format(mcc_localedir),
]

executable(
  'mate-control-center',
  sources : sources,
  include_directories: config_inc,
  dependencies : [common_deps, menu_dep],
  c_args : cflags,
  install : true,
  install_dir : get_option('bindir')
)

install_data (
  'matecc.menu',
  install_dir : join_paths(get_option('sysconfdir'), 'xdg', 'menus')
)
