autodesktop_sources = [
  'main.c',
  'autodesktop-window.c',
]

autodesktop_deps = [
  dependency('gio-2.0', version: '>= 2.50'),
  dependency('gtk+-3.0', version: '>= 3.22'),
]

gnome = import('gnome')

autodesktop_sources += gnome.compile_resources('autodesktop-resources',
  'autodesktop.gresource.xml',
  c_name: 'autodesktop'
)

executable('autodesktop', autodesktop_sources,
  dependencies: autodesktop_deps,
  install: true,
)
