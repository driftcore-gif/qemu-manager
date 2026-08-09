project('qemu-manager', 'cpp',
  version : '1.0.0',
  license : 'GPL-2.0',
  default_options : ['warning_level=2','cpp_std=c++17','buildtype=release'])

gtk4_dep    = dependency('gtk4', version : '>= 4.8')
thread_dep  = dependency('threads')

sources = files(
  'src/main.cpp',
  'src/app.cpp',
  'src/mainwindow.cpp',
  'src/vm.cpp',
  'src/vmwizard.cpp',
  'src/vmsettings.cpp',
  'src/commandbuilder.cpp',
  'src/vmconfig_io.cpp',
  'src/storagemanager.cpp',
  'src/dashboard.cpp',
  'src/vmlistview.cpp',
  'src/snapshotmanager.cpp',
  'src/logviewer.cpp',
  'src/settings.cpp',
  'src/downloadmanager.cpp',
  'src/templates.cpp',
  'src/serialconsole.cpp',
  'src/networkconfig.cpp',
  'src/displayconfig.cpp',
  'src/qemumonitor.cpp',
  'src/logmonitor.cpp',
)

executable('qemu-manager',
  sources,
  dependencies : [gtk4_dep, thread_dep],
  include_directories : include_directories('include'),
  install : true,
  install_rpath : '/data/data/com.termux/files/usr/lib')
