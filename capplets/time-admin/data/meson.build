desktop = 'mate-time-admin.desktop'

i18n.merge_file(
  type: 'desktop',
  input: desktop + '.in',
  output: desktop,
  po_dir: po_dir,
  install: true,
  install_dir: mcc_desktopdir
)

install_subdir(
  'map',
  install_dir: join_paths(mcc_datadir, 'mate-time-admin')
)
