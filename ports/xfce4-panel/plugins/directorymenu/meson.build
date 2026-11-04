plugin_sources = [
  'directorymenu.c',
  'directorymenu.h',
]

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

if host_machine.system() == 'lume'
lume_panel_plugin = static_library(
  'directorymenu',
  plugin_sources,
  sources: xfce4_panel_resources_c,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-Dxfce_panel_module_init=lume_panel_directorymenu_init',
    '-Dxfce_panel_module_construct=lume_panel_directorymenu_construct',
    '-Dxfce_panel_module_preinit=lume_panel_directorymenu_preinit',
    '-DG_LOG_DOMAIN="@0@"'.format('libdirectorymenu'),
  ],
  include_directories: [
    include_directories('..' / '..'),
  ],
  dependencies: [
    gio_unix,
    gtk,
    exo,
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
  'directorymenu',
  plugin_sources,
  sources: xfce4_panel_resources_c,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('libdirectorymenu'),
  ],
  include_directories: [
    include_directories('..' / '..'),
  ],
  dependencies: [
    gio_unix,
    gtk,
    exo,
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

configure_file(
  input: 'xfce4-popup-directorymenu.sh',
  output: 'xfce4-popup-directorymenu',
  configuration: {
    'bindir': get_option('prefix') / get_option('bindir'),
    'localedir': get_option('prefix') / get_option('localedir'),
  },
  install: true,
  install_dir: get_option('prefix') / get_option('bindir'),
  install_mode: 'rwxr-xr-x',
)

i18n.merge_file(
  input: configure_file(
    input: 'directorymenu.desktop.in.in',
    output: 'directorymenu.desktop.in',
    configuration: {
      'LIBXFCE4PANEL_VERSION_API': libxfce4panel_version_api,
    },
    install: false,
  ),
  output: 'directorymenu.desktop',
  po_dir: '..' / '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)
