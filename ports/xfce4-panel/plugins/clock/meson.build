plugin_sources = [
  'clock.c',
  'clock.h',
  'clock-time.c',
  'clock-time.h',
  'clock-analog.c',
  'clock-analog.h',
  'clock-binary.c',
  'clock-binary.h',
  'clock-digital.c',
  'clock-digital.h',
  'clock-fuzzy.c',
  'clock-fuzzy.h',
  'clock-lcd.c',
  'clock-lcd.h',
  'clock-sleep-monitor.c',
  'clock-sleep-monitor.h',
]

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

if host_machine.system() == 'lume'
lume_panel_plugin = static_library(
  'clock',
  plugin_sources,
  sources: xfce4_panel_resources_c,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-Dxfce_panel_module_init=lume_panel_clock_init',
    '-Dxfce_panel_module_construct=lume_panel_clock_construct',
    '-Dxfce_panel_module_preinit=lume_panel_clock_preinit',
    '-DG_LOG_DOMAIN="@0@"'.format('libclock'),
  ],
  include_directories: [
    include_directories('..' / '..'),
  ],
  dependencies: [
    gtk,
    cairo,
    libxfce4util,
    libxfce4ui,
    libxfce4windowing,
    xfconf,
    libm,
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
  'clock',
  plugin_sources,
  sources: xfce4_panel_resources_c,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('libclock'),
  ],
  include_directories: [
    include_directories('..' / '..'),
  ],
  dependencies: [
    gtk,
    cairo,
    libxfce4util,
    libxfce4ui,
    libxfce4windowing,
    xfconf,
    libm,
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
    input: 'clock.desktop.in.in',
    output: 'clock.desktop.in',
    configuration: {
      'LIBXFCE4PANEL_VERSION_API': libxfce4panel_version_api,
    },
    install: false,
  ),
  output: 'clock.desktop',
  po_dir: '..' / '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)
