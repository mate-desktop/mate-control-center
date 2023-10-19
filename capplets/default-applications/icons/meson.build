categories_icons = [
  ['16x16', 'instant-messaging.png'],
  ['22x22', 'instant-messaging.png'],
  ['24x24', 'instant-messaging.png'],
  ['32x32', 'instant-messaging.png'],
  ['48x48', 'instant-messaging.png'],
  ['256x256', 'instant-messaging.png'],
]

foreach icon: categories_icons
  install_data(
    join_paths(icon[0], 'categories', icon[1]),
    install_dir: join_paths(mcc_icondir, 'hicolor', icon[0], 'categories')
  )
endforeach
