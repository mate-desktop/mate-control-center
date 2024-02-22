project(
  'mate-control-center', 'c',
        version : '1.28.0',
  meson_version : '>= 0.57.0'
)

mcc_prefix = get_option('prefix')
mcc_bindir = join_paths(mcc_prefix, get_option('bindir'))
mcc_sbindir = join_paths(mcc_prefix, get_option('sbindir'))
mcc_datadir = join_paths(mcc_prefix, get_option('datadir'))
mcc_libexecdir = join_paths(mcc_prefix, get_option('libexecdir'))
mcc_libdir = join_paths(mcc_prefix, get_option('libdir'))
mcc_localedir = join_paths(mcc_prefix, get_option('localedir'))
mcc_mandir = join_paths(mcc_prefix, get_option('mandir'))
mcc_sysconfdir = join_paths(mcc_prefix, get_option('sysconfdir'))

mcc_pkgdatadir = join_paths(mcc_datadir, meson.project_name())
mcc_pixmaps = join_paths(mcc_pkgdatadir, 'pixmaps')
mcc_background = join_paths(mcc_datadir, 'backgrounds')
mcc_wallpaperdir = join_paths(mcc_datadir, 'mate-background-properties')
mcc_desktopdir = join_paths(mcc_datadir, 'applications')
mcc_icondir = join_paths(mcc_datadir, 'icons')
mcc_schemadir = join_paths (mcc_datadir, 'glib-2.0', 'schemas')
mcc_policydir = join_paths (mcc_datadir, 'polkit-1', 'actions')
mcc_timezonedir = join_paths (mcc_datadir, 'mate-time-admin', 'map')
mcc_directorydir = join_paths(mcc_datadir, 'desktop-directories')
mcc_mime = join_paths(mcc_datadir, 'mime', 'packages')
mcc_gettext = meson.project_name() + '-2.0'

cc = meson.get_compiler('c')

config_h = configuration_data()

# defines
set_defines = [
  # package
  ['PACKAGE', meson.project_name()],
  ['PACKAGE_VERSION', meson.project_version()],
  ['VERSION', meson.project_version()],
  ['XCURSOR_ICONDIR', mcc_icondir],
  # i18n
  ['GETTEXT_PACKAGE', mcc_gettext]
]

foreach define: set_defines
  config_h.set_quoted(define[0], define[1])
endforeach

common_flags = ['-DHAVE_CONFIG_H']

if get_option('buildtype').contains('debug')
  test_cflags = [
    '-Wcast-align',
    '-Wmissing-field-initializers',
    '-Wmissing-declarations',
    '-Wmissing-prototypes',
    '-Wnested-externs',
    '-Wno-strict-aliasing',
    '-Wno-sign-compare'
  ]

  common_flags += cc.get_supported_arguments(test_cflags)
endif

add_project_arguments(common_flags, language: 'c')

gtk_dep = dependency('gtk+-3.0', version: '>= 3.22.0')
gio_dep = dependency('gio-2.0')
glib_dep = dependency('glib-2.0', version: '>= 2.64.0')
mate_desktop_dep = dependency('mate-desktop-2.0', version: '>= 1.27.1')
marco_dep = dependency('libmarco-private', version: '>= 1.17.0')
menu_dep = dependency('libmate-menu', version: '>= 1.21.0')
libxml_dep = dependency('libxml-2.0')
pango_dep = dependency('pango')
xcursor_dep = dependency('xcursor')
dconf_dep = dependency('dconf', version: '>= 0.13.4')
fontconfig_dep = dependency('fontconfig')
gtop_dep = dependency('libgtop-2.0')
udisks2_dep = dependency('udisks2')
polkit_dep = dependency('polkit-gobject-1')
matekbd_dep = dependency('libmatekbd', version: '>=1.17.0')
matekbdui_dep = dependency('libmatekbdui', version: '>=1.17.0')
xklavier_dep = dependency('libxklavier', version: '>= 5.2')
x11_dep = dependency('x11', version: '>= 1.8')
xi_dep = dependency('xi', version: '>= 1.5')
xext_dep = dependency('xext')
xscrnsaver_dep = dependency('xscrnsaver')
m_dep = cc.find_library('m')

enable_accountsservice = get_option('accountsservice')
accounts_dep = dependency('accountsservice', version: '>= 0.6.39', required: enable_accountsservice)
config_h.set10('HAVE_ACCOUNTSSERVICE', accounts_dep.found())

common_deps = [
  gio_dep,
  xcursor_dep,
  x11_dep,
  glib_dep,
  marco_dep,
  mate_desktop_dep,
  accounts_dep,
  dependency('gio-unix-2.0'),
  dependency('gthread-2.0'),
  gtk_dep,
]

enable_libappindicator = get_option('libappindicator')
if enable_libappindicator == 'yes'
  appindicator = true
  ayatana = false
elif enable_libappindicator == 'auto'
  if dependency('ayatana-appindicator3-0.1', required : false).found()
    appindicator = false
    ayatana = true
  elif dependency('appindicator3-0.1', required : false).found()
    appindicator = true
    ayatana = false
  else
    error ('Neither Ayatana AppIndicator nor Ubuntu AppIndicator library is present, but you enabled AppIndicator support.')
  endif
else
  appindicator = false
  ayatana = true
endif
appindicator_dep = dependency('appindicator3-0.1', version: '>= 0.0.13', required: appindicator)
ayatana_dep = dependency('ayatana-appindicator3-0.1', version: '>= 0.0.13', required: ayatana)
config_h.set('HAVE_UBUNTU_APPINDICATOR', appindicator)
gnome = import('gnome')
i18n = import('i18n')
pkg = import('pkgconfig')

po_dir = join_paths(meson.project_source_root(), 'po')
its_dir = join_paths(meson.project_source_root())
config_inc = include_directories('.')

schema_data = configuration_data()
schema_data.set('GETTEXT_PACKAGE', meson.project_name())
configure_file(
  input : 'org.mate.control-center.keybinding.gschema.xml.in',
  output : 'org.mate.control-center.keybinding.gschema.xml',
  configuration : schema_data,
  install : true,
  install_dir : join_paths(mcc_datadir, 'glib-2.0', 'schemas')
)
configure_file(
  input : 'org.mate.control-center.gschema.xml.in',
  output : 'org.mate.control-center.gschema.xml',
  configuration : schema_data,
  install : true,
  install_dir : join_paths(mcc_datadir, 'glib-2.0', 'schemas')
)

subdir('po')
subdir('man')
subdir('help')
subdir('font-viewer')
subdir('capplets')
subdir('typing-break')
subdir('shell')

gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
)

configure_file(
  output: 'config.h',
  configuration: config_h
)

meson.add_install_script(
  'meson_post_install.py',
  mcc_datadir
)

summary = [
  'configuration summary:',
  '',
  '                 project: @0@ @1@'.format(meson.project_name(), meson.project_version()),
  '                  prefix: @0@'.format(mcc_prefix),
  'accountsservice supports: @0@'.format(accounts_dep.found()),
  '    Ayatana AppIndicator: @0@'.format(ayatana),
  '     Ubuntu AppIndicator: @0@'.format(appindicator),
  ''
]
message('\n'.join(summary))
