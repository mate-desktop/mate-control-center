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

sources = [
  'drw-break-window.c',
  'drw-monitor.c',
  'drwright.c',
  'drw-selection.c',
  'drw-timer.c',
  'drw-utils.c',
  'main.c'
]

# Typing activity on Wayland comes from the compositor (ext-idle-notify-v1)
wayland_client_dep = dependency('wayland-client', required : false)
gtk_wayland_dep = dependency('gtk+-wayland-3.0', required : false)
wayland_protocols_dep = dependency('wayland-protocols', version : '>= 1.27', required : false)
wayland_scanner = find_program('wayland-scanner', native : true, required : false)

if wayland_client_dep.found() and gtk_wayland_dep.found() and wayland_protocols_dep.found() and wayland_scanner.found()
  idle_notify_xml = join_paths(wayland_protocols_dep.get_variable(pkgconfig : 'pkgdatadir'),
                               'staging', 'ext-idle-notify', 'ext-idle-notify-v1.xml')
  sources += custom_target('ext-idle-notify-v1-client-protocol.h',
    input : idle_notify_xml,
    output : 'ext-idle-notify-v1-client-protocol.h',
    command : [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'])
  sources += custom_target('ext-idle-notify-v1-protocol.c',
    input : idle_notify_xml,
    output : 'ext-idle-notify-v1-protocol.c',
    command : [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'])
  sources += 'drw-idle-wayland.c'
  deps += wayland_client_dep
  cflags += '-DHAVE_WAYLAND_IDLE'
endif

executable(
  'mate-typing-monitor',
  sources : sources,
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
