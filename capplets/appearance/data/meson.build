desktops = [
  'mate-appearance-properties.desktop',
  'mate-theme-installer.desktop'
]

foreach desktop: desktops
  i18n.merge_file(
    type: 'desktop',
    input: desktop + '.in',
    output: desktop,
    po_dir: po_dir,
    install: true,
    install_dir: mcc_desktopdir
  )
endforeach

icons = [
  'gtk-theme-thumbnailing.png',
  'icon-theme-thumbnailing.png',
  'mouse-cursor-normal-large.png',
  'mouse-cursor-normal.png',
  'mouse-cursor-white-large.png',
  'mouse-cursor-white.png',
  'subpixel-bgr.png',
  'subpixel-rgb.png',
  'subpixel-vbgr.png',
  'subpixel-vrgb.png',
  'theme-thumbnailing.png',
  'window-theme-thumbnailing.png'
]

foreach icon: icons
  install_data (
    icon,
    install_dir : mcc_pixmaps
  )
endforeach

 i18n.merge_file(
   input: 'mate-theme-package.xml.in',
   output: 'mate-theme-package.xml',
   type: 'xml',
   po_dir: po_dir,
   install: true,
   install_dir: mcc_mime,
 )
