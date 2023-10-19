cflags = [
  '-DIMAGEDIR="@0@"'.format(mcc_directorydir),
  '-DMATELOCALEDIR="@0@"'.format(mcc_pixmaps),
]

deps = [
  gtk_dep,
  glib_dep,
  mate_desktop_dep,
  x11_dep,
  xext_dep,
  xscrnsaver_dep,
  m_dep
]
deps += [appindicator_dep,ayatana_dep]
executable(
  'mate-typing-monitor',
  sources : [
    'drw-break-window.c',
    'drw-monitor.c',
    'drwright.c',
    'drw-selection.c',
    'drw-timer.c',
    'drw-utils.c',
    'main.c'
  ],
  include_directories : config_inc,
  dependencies : deps,
  c_args : cflags,
  install : true,
  install_dir : get_option('bindir')
)

install_data(
  'mate-typing-monitor.png',
  install_dir: join_paths(mcc_icondir, 'hicolor', '48x48', 'apps')
)

install_data(
  'mate-typing-monitor.svg',
  install_dir: join_paths(mcc_icondir, 'hicolor', 'scalable', 'apps')
)

icons = [
  'bar-disabled.png',
  'bar-green.png',
  'bar.png',
  'bar-red.png',
  'ocean-stripes.png'
]

foreach icon: icons
  install_data(
    icon,
    install_dir : mcc_pixmaps
  )
endforeach
