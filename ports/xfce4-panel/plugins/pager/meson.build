plugin_sources = [
  'pager.c',
  'pager.h',
  'pager-buttons.c',
  'pager-buttons.h',
]

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

if host_machine.system() == 'lume'
lume_panel_plugin = static_library(
  'pager',
  plugin_sources,
  sources: xfce4_panel_resources_c,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-Dxfce_panel_module_init=lume_panel_pager_init',
    '-Dxfce_panel_module_construct=lume_panel_pager_construct',
    '-Dxfce_panel_module_preinit=lume_panel_pager_preinit',
    '-DG_LOG_DOMAIN="@0@"'.format('libpager'),
    '-DWNCK_I_KNOW_THIS_IS_UNSTABLE=1',
  ],
  include_directories: [
    include_directories('..' / '..'),
  ],
  dependencies: [
    gtk,
    libwnck,
    libxfce4util,
    libxfce4ui,
    libxfce4windowing,
    libxfce4windowingui,
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
  'pager',
  plugin_sources,
  sources: xfce4_panel_resources_c,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('libpager'),
    '-DWNCK_I_KNOW_THIS_IS_UNSTABLE=1',
  ],
  include_directories: [
    include_directories('..' / '..'),
  ],
  dependencies: [
    gtk,
    libwnck,
    libxfce4util,
    libxfce4ui,
    libxfce4windowing,
    libxfce4windowingui,
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
    input: 'pager.desktop.in.in',
    output: 'pager.desktop.in',
    configuration: {
      'LIBXFCE4PANEL_VERSION_API': libxfce4panel_version_api,
    },
    install: false,
  ),
  output: 'pager.desktop',
  po_dir: '..' / '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)
