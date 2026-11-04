plugin_sources = [
  'sn-config.c',
  'sn-config.h',
  'sn-backend.c',
  'sn-backend.h',
  'sn-button.c',
  'sn-button.h',
  'sn-box.c',
  'sn-box.h',
  'sn-dialog.c',
  'sn-dialog.h',
  'sn-icon-box.c',
  'sn-icon-box.h',
  'sn-item.c',
  'sn-item.h',
  'sn-plugin.c',
  'sn-plugin.h',
  'sn-util.c',
  'sn-util.h',
]

if enable_x11
plugin_sources += [
  'systray.c',
  'systray.h',
  'systray-box.c',
  'systray-box.h',
  'systray-manager.c',
  'systray-manager.h',
  'systray-socket.c',
  'systray-socket.h',
]
endif

plugin_sources += gnome.genmarshal(
  'systray-marshal',
  sources: 'systray-marshal.list',
  prefix: '_systray_marshal',
  internal: true,
  install_header: false,
)

plugin_sources += gnome.gdbus_codegen(
  'sn-watcher',
  sources: 'sn-watcher.xml',
  interface_prefix: 'org.kde.StatusNotifierWatcher',
  install_header: false,
)

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

if host_machine.system() == 'lume'
lume_panel_plugin = static_library(
  'systray',
  plugin_sources,
  sources: xfce4_panel_resources_c,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-Dxfce_panel_module_init=lume_panel_systray_init',
    '-Dxfce_panel_module_construct=lume_panel_systray_construct',
    '-Dxfce_panel_module_preinit=lume_panel_systray_preinit',
    '-DG_LOG_DOMAIN="@0@"'.format('libsystray'),
  ],
  include_directories: [
    include_directories('..' / '..'),
  ],
  dependencies: [
    gio_unix,
    gtk,
    dbusmenu,
    x11_deps,
    libxfce4util,
    libxfce4ui,
    libxfce4windowing,
    xfconf,
  ],
  link_with: [
    libpanel_common,
    libxfce4panel,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('libdir') / plugin_install_subdir,
)

lume_panel_plugins += lume_panel_plugin

else
shared_module(
  'systray',
  plugin_sources,
  sources: xfce4_panel_resources_c,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('libsystray'),
  ],
  include_directories: [
    include_directories('..' / '..'),
  ],
  dependencies: [
    gio_unix,
    gtk,
    dbusmenu,
    x11_deps,
    libxfce4util,
    libxfce4ui,
    libxfce4windowing,
    xfconf,
  ],
  link_with: [
    libpanel_common,
    libxfce4panel,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('libdir') / plugin_install_subdir,
)

endif

i18n.merge_file(
  input: configure_file(
    input: 'systray.desktop.in.in',
    output: 'systray.desktop.in',
    configuration: {
      'LIBXFCE4PANEL_VERSION_API': libxfce4panel_version_api,
    },
    install: false,
  ),
  output: 'systray.desktop',
  po_dir: '..' / '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)
