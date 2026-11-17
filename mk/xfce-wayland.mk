# SPDX-License-Identifier: BSD-3-Clause
XFCE_WAYLAND ?= 0
ifeq ($(XFCE_WAYLAND),1)
ifneq ($(filter aarch64 amd64,$(ARCH)),)
XFCE_WAYLAND_CPPFLAGS := -DLUME_GTK_WAYLAND=1 -DENABLE_WAYLAND=1 \
    -DHAVE_GTK_LAYER_SHELL=1 -DHAVE_XDG_ACTIVATION=1 \
    -Iports/gtk/wayland -Iports/gtk/wayland/gdk -Iports/gtk/wayland/gdk/wayland -Ivendor/gtk/gdk/wayland \
    -Iports/xfce-wayland -Iports/gtk-layer-shell/protocol \
    -Ivendor/gtk-layer-shell/include \
    -Ivendor/libxkbcommon/include -Iports/libepoxy/egl/include -I$(MESA_SOURCE)/include \
    -I$(WAYLAND_SDK)/include -I$(WAYLAND_GENERATED)
$(XFCE_OUT)/obj/%: private override CC += $(XFCE_WAYLAND_CPPFLAGS)
$(XFCE_OUT)/obj/%: private .EXTRA_PREREQS := $(XFCE_OUT)/backend-configuration
$(XFCE_OUT)/obj/harfbuzz%: private override CC += -Iports/libcxx -Ivendor/libcxx/include
$(XFCE_OUT)/obj/epoxy/%: private override CC += -include ports/libepoxy/wayland/features.h
XFCE_WAYLAND_LIBRARIES := $(XFCE_OUT)/libgdk-wayland.a \
    $(XFCE_OUT)/libxfce-wayland.a $(XFCE_OUT)/libgtk-layer-shell.a \
    $(XFCE_OUT)/libxfce-wayland-protocols.a $(WAYLAND_LIBRARIES) \
    $(WESTON_OUT)/libxkbcommon.a $(GRAPHICS_OUT)/liblume-gpu.a \
    $(WESTON_DRM_LIBRARY) $(WAYLAND_SDK)/lib/libc++.a
XFCE_LIBRARIES += $(XFCE_WAYLAND_LIBRARIES)
GTK_DIALOG_LIBRARIES += $(XFCE_WAYLAND_LIBRARIES)
$(GTK_DIALOG_OUT)/gtk-file-dialog.o: private override CC += $(XFCE_WAYLAND_CPPFLAGS)
$(GTK_DIALOG_OUT)/lume-file-dialog.elf: $(XFCE_WAYLAND_LIBRARIES)
$(XFCE_OUT)/%.elf: private .EXTRA_PREREQS += $(XFCE_WAYLAND_LIBRARIES)
$(XFCE_OUT)/lume-xfce-session.elf $(XFCE_OUT)/xfce4-panel.elf $(XFCE_OUT)/xfdesktop.elf $(XFCE_OUT)/thunar.elf $(XFCE_OUT)/xfsettingsd.elf: $(XFCE_WAYLAND_LIBRARIES)
XFCE_WAYLAND_GDK_SOURCES := \
    vendor/gtk/gdk/wayland/gdkapplaunchcontext-wayland.c \
    vendor/gtk/gdk/wayland/gdkcursor-wayland.c \
    vendor/gtk/gdk/wayland/gdkdevice-wayland.c \
    vendor/gtk/gdk/wayland/gdkdisplay-wayland.c \
    vendor/gtk/gdk/wayland/gdkdnd-wayland.c \
    vendor/gtk/gdk/wayland/gdkeventsource.c \
    vendor/gtk/gdk/wayland/gdkglcontext-wayland.c \
    vendor/gtk/gdk/wayland/gdkkeys-wayland.c \
    vendor/gtk/gdk/wayland/gdkmonitor-wayland.c \
    vendor/gtk/gdk/wayland/gdkscreen-wayland.c \
    vendor/gtk/gdk/wayland/gdkselection-wayland.c \
    vendor/gtk/gdk/wayland/gdkwindow-wayland.c \
    vendor/gtk/gdk/wayland/wm-button-layout-translation.c
XFCE_WAYLAND_WINDOWING_SOURCES := \
    vendor/libxfce4windowing/libxfce4windowing/xfw-application-wayland.c \
    vendor/libxfce4windowing/libxfce4windowing/xfw-monitor-wayland.c \
    vendor/libxfce4windowing/libxfce4windowing/xfw-screen-wayland.c \
    vendor/libxfce4windowing/libxfce4windowing/xfw-seat-wayland.c \
    vendor/libxfce4windowing/libxfce4windowing/xfw-window-wayland.c \
    vendor/libxfce4windowing/libxfce4windowing/xfw-wl-raster-icon.c \
    vendor/libxfce4windowing/libxfce4windowing/xfw-workspace-group-wayland.c \
    vendor/libxfce4windowing/libxfce4windowing/xfw-workspace-manager-wayland.c \
    vendor/libxfce4windowing/libxfce4windowing/xfw-workspace-wayland.c
XFCE_WAYLAND_LAYER_SOURCES := \
    vendor/gtk-layer-shell/src/api.c \
    vendor/gtk-layer-shell/src/custom-shell-surface.c \
    vendor/gtk-layer-shell/src/gtk-priv-access.c \
    vendor/gtk-layer-shell/src/gtk-wayland.c \
    vendor/gtk-layer-shell/src/layer-surface.c \
    vendor/gtk-layer-shell/src/simple-conversions.c \
    vendor/gtk-layer-shell/src/xdg-popup-surface.c \
    vendor/gtk-layer-shell/src/xdg-toplevel-surface.c
XFCE_WAYLAND_GDK_FLAGS := -DHAVE_LINUX_INPUT_H=1 -Iports/weston/include '-Iports/gtk/aarch64/gdk' '-Ivendor/gtk/gdk' '-Iports/gtk/aarch64' '-Ivendor/gtk' '-Ivendor/gtk/gdk/x11' '-Iports/gdk-pixbuf/aarch64' '-Ivendor/gdk-pixbuf' '-Ivendor/proxy-libintl' '-Iports/glib/aarch64' '-Iports/glib/aarch64/glib' '-Ivendor/glib' '-Ivendor/glib/glib' '-Ivendor/glib/gmodule' '-Ivendor/glib/gobject' '-Ivendor/glib/gio' '-Iports/x11/libpng' '-Ivendor/libpng' '-Iports/cairo/aarch64/src' '-Ivendor/cairo/src' '-Iports/fontconfig/aarch64' '-Ivendor/fontconfig' '-Ivendor/freetype/include' '-Iports/x11/include' '-Ivendor/expat/lib' '-Ivendor/pixman/pixman' '-Ivendor/zlib' '-Iports/pango/aarch64' '-Ivendor/pango' '-Ivendor/harfbuzz/src' '-Iports/harfbuzz/aarch64/src' '-Ivendor/fribidi/lib' '-Iports/fribidi/aarch64/lib' '-Ivendor/cairo/util/cairo-gobject' '-Iports/libepoxy/aarch64/include' '-Ivendor/libepoxy/include' '-Ivendor/libXi/include' '-Iinclude/posix' '-Iinclude/lume' '-Iarch/$(ARCH)/include' '-I$(SYSROOT)/include' '-D_FILE_OFFSET_BITS=64' '-Wall' '-Winvalid-pch' '-O3' '-DG_LOG_USE_STRUCTURED=1' '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_56' '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_58' '-DGTK_VERSION="3.24.52"' '-D_GNU_SOURCE' '-DG_DISABLE_CAST_CHECKS' '--sysroot=$(SYSROOT)' $(filter-out --target=%,$(ARCH_CFLAGS)) '-fno-stack-protector' '-fno-pic' '-fno-pie' '-D__lume__=1' $(ARCH_CPPFLAGS) -Iinclude/internal -I$(KERNEL)/include/uapi '-DLUME_NATIVE_ABI=1' '-D_DEFAULT_SOURCE' '-D_POSIX_C_SOURCE=200809L' '-D_POSIX_TIMERS=200809L' '-D_POSIX_MONOTONIC_CLOCK=200809L' '-D_POSIX_THREAD_SAFE_FUNCTIONS=200809L' '-DGDK_COMPILATION' '-DG_LOG_DOMAIN="Gdk"' '-fno-strict-aliasing' '-Wpointer-arith' '-Wimplicit-function-declaration' '-Wformat=2' '-Wformat-security' '-Wnested-externs' '-Wold-style-definition' '-Wundef' '-Wunused' '-Wcast-align' '-Wmissing-noreturn' '-Wmissing-format-attribute' '-Wmissing-include-dirs' '-Wignored-qualifiers' '-Werror=implicit' '-Werror=nonnull' '-Werror=init-self' '-Werror=main' '-Werror=missing-braces' '-Werror=sequence-point' '-Werror=return-type' '-Werror=trigraphs' '-Werror=array-bounds' '-Werror=write-strings' '-Werror=address' '-Werror=int-to-pointer-cast' '-Werror=pointer-to-int-cast' '-Werror=empty-body' '-Werror=write-strings' '-ffunction-sections' '-fdata-sections' '-fno-pic' '-fno-pie' '-ffile-prefix-map=$(CURDIR)=lume-os'
XFCE_WAYLAND_WINDOWING_FLAGS := '-Iports/libxfce4windowing/aarch64/libxfce4windowing' '-Ivendor/libxfce4windowing/libxfce4windowing' '-Iports/libxfce4windowing/aarch64' '-Ivendor/libxfce4windowing' '-Ivendor/proxy-libintl' '-Iports/glib/aarch64' '-Iports/glib/aarch64/glib' '-Ivendor/glib' '-Ivendor/glib/glib' '-Ivendor/glib/gmodule' '-Ivendor/glib/gobject' '-Ivendor/glib/gio' '-Iports/gtk/aarch64' '-Ivendor/gtk' '-Iports/pango/aarch64' '-Ivendor/pango' '-Ivendor/harfbuzz/src' '-Iports/harfbuzz/aarch64/src' '-Ivendor/fribidi/lib' '-Iports/fribidi/aarch64/lib' '-Iports/cairo/aarch64/src' '-Ivendor/cairo/src' '-Ivendor/cairo/util/cairo-gobject' '-Iports/fontconfig/aarch64' '-Ivendor/fontconfig' '-Ivendor/freetype/include' '-Iports/x11/include' '-Ivendor/expat/lib' '-Ivendor/pixman/pixman' '-Ivendor/zlib' '-Iports/gdk-pixbuf/aarch64' '-Ivendor/gdk-pixbuf' '-Iports/x11/libpng' '-Ivendor/libpng' '-Iports/libepoxy/aarch64/include' '-Ivendor/libepoxy/include' '-Ivendor/libXi/include' '-Iports/at-spi2-core/aarch64' '-Ivendor/at-spi2-core' '-Ivendor/at-spi2-core/atk-adaptor' '-Iports/dbus/aarch64' '-Ivendor/dbus' '-Ivendor/libdisplay-info/include' '-Iports/libwnck/aarch64' '-Ivendor/libwnck' '-Iinclude/posix' '-Iinclude/lume' '-Iarch/$(ARCH)/include' '-I$(SYSROOT)/include' '-fvisibility=hidden' '-D_FILE_OFFSET_BITS=64' '-Wall' '-Winvalid-pch' '-Wextra' '-std=gnu11' '-O3' '-Wmissing-declarations' '-Wmissing-noreturn' '-Wold-style-definition' '-Wredundant-decls' '-Wpointer-arith' '-Wcast-align' '-Winit-self' '-Wshadow' '-Wmissing-include-dirs' '-Wundef' '-Wformat' '-Wformat-security' '-Wformat-y2k' '-Wnested-externs' '-Wno-unused-parameter' '-Wno-declaration-after-statement' '-Werror=implicit-function-declaration' '-Wno-error=deprecated-declarations' '-DNDEBUG' '-DG_DISABLE_CAST_CHECKS' '-DG_DISABLE_ASSERT' '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_72' '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_72' '-DG_LOG_DOMAIN="libxfce4windowing"' '-DG_LOG_USE_STRUCTURED=1' '-DPACKAGE="libxfce4windowing"' '-DPACKAGE_NAME="libxfce4windowing"' '-DPACKAGE_VERSION="4.20.7"' '-DVERSION="4.20.7"' '-DVERSION_SHORT="4.20"' '-DPACKAGE_STRING="libxfce4windowing 4.20.7"' '-DPACKAGE_DATADIR="/usr/share/libxfce4windowing"' '-DCOPYRIGHT_YEAR="2026"' '-DPACKAGE_LOCALE_DIR="/usr/share/locale"' '-DPACKAGE_BUGREPORT="https://gitlab.xfce.org/xfce/libxfce4windowing/-/issues"' '-DGETTEXT_PACKAGE="libxfce4windowing"' '-DPREFIX="/usr"' '-DBINDIR="/usr/bin"' '-DDATADIR="/usr/share"' '-DINCLUDEDIR="/usr/include"' '-DLIBDIR="/usr/lib"' '-DLIBEXECDIR="/usr/libexec"' '-DLOCALEDIR="/usr/share/locale"' '-DLOCALSTATEDIR="/data/var"' '-DSBINDIR="/usr/sbin"' '-DSYSCONFDIR="/etc"' '-DHAVE_XFCE_REVISION_H=1' '-DENABLE_X11=1' '-DENABLE_SYMBOL_VISIBILITY=1' '--sysroot=$(SYSROOT)' $(filter-out --target=%,$(ARCH_CFLAGS)) '-fno-stack-protector' '-fno-pic' '-fno-pie' '-D__lume__=1' $(ARCH_CPPFLAGS) -Iinclude/internal -I$(KERNEL)/include/uapi '-DLUME_NATIVE_ABI=1' '-D_DEFAULT_SOURCE' '-D_POSIX_C_SOURCE=200809L' '-D_POSIX_TIMERS=200809L' '-D_POSIX_MONOTONIC_CLOCK=200809L' '-D_POSIX_THREAD_SAFE_FUNCTIONS=200809L' '-DDBUS_STATIC_BUILD' '-DLIBXFCE4WINDOWING_COMPILATION=1' '-DWNCK_I_KNOW_THIS_IS_UNSTABLE=1' '-ffunction-sections' '-fdata-sections' '-fno-pic' '-fno-pie' '-ffile-prefix-map=$(CURDIR)=lume-os'
XFCE_WAYLAND_PANEL_FLAGS := '-Iports/xfce4-panel/aarch64/panel' '-Ivendor/xfce4-panel/panel' '-Iports/xfce4-panel/aarch64' '-Ivendor/xfce4-panel' '-Iports/xfce4-panel/aarch64/common' '-Ivendor/proxy-libintl' '-Iports/glib/aarch64' '-Iports/glib/aarch64/glib' '-Ivendor/glib' '-Ivendor/glib/glib' '-Ivendor/glib/gmodule' '-Ivendor/glib/gobject' '-Iports/gtk/aarch64' '-Ivendor/gtk' '-Iports/pango/aarch64' '-Ivendor/pango' '-Ivendor/harfbuzz/src' '-Iports/harfbuzz/aarch64/src' '-Ivendor/fribidi/lib' '-Iports/fribidi/aarch64/lib' '-Iports/cairo/aarch64/src' '-Ivendor/cairo/src' '-Ivendor/cairo/util/cairo-gobject' '-Iports/fontconfig/aarch64' '-Ivendor/fontconfig' '-Ivendor/freetype/include' '-Iports/x11/include' '-Ivendor/expat/lib' '-Ivendor/pixman/pixman' '-Ivendor/zlib' '-Iports/gdk-pixbuf/aarch64' '-Ivendor/gdk-pixbuf' '-Iports/x11/libpng' '-Ivendor/libpng' '-Iports/libepoxy/aarch64/include' '-Ivendor/libepoxy/include' '-Ivendor/libXi/include' '-Iports/at-spi2-core/aarch64' '-Ivendor/at-spi2-core' '-Ivendor/at-spi2-core/atk-adaptor' '-Iports/dbus/aarch64' '-Ivendor/dbus' '-Ivendor/glib/gio' '-Iports/libxfce4util/aarch64' '-Ivendor/libxfce4util' '-Iports/libxfce4ui/aarch64' '-Ivendor/libxfce4ui' '-Iports/xfconf/aarch64' '-Ivendor/xfconf' '-Iports/libxfce4windowing/aarch64' '-Ivendor/libxfce4windowing' '-Iports/libwnck/aarch64' '-Ivendor/libwnck' '-Ivendor/libdisplay-info/include' '-Iinclude/posix' '-Iinclude/lume' '-Iarch/$(ARCH)/include' '-I$(SYSROOT)/include' '-D_FILE_OFFSET_BITS=64' '-Wall' '-Winvalid-pch' '-Wextra' '-std=gnu11' '-O3' '-Wmissing-declarations' '-Wmissing-noreturn' '-Wold-style-definition' '-Wredundant-decls' '-Wpointer-arith' '-Wcast-align' '-Winit-self' '-Wshadow' '-Wmissing-include-dirs' '-Wundef' '-Wformat' '-Wformat-security' '-Wformat-y2k' '-Wnested-externs' '-Wno-unused-parameter' '-Wno-declaration-after-statement' '-Wno-missing-field-initializers' '-Werror=implicit-function-declaration' '-Wno-error=deprecated-declarations' '-DNDEBUG' '-DG_DISABLE_CAST_CHECKS' '-DG_DISABLE_ASSERT' '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_72' '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_72' '-DG_LOG_USE_STRUCTURED=1' '-DPACKAGE="xfce4-panel"' '-DPACKAGE_NAME="xfce4-panel"' '-DPACKAGE_VERSION="4.20.8"' '-DVERSION="4.20.8"' '-DVERSION_SHORT="4.20"' '-DPACKAGE_STRING="xfce4-panel 4.20.8"' '-DPACKAGE_DATADIR="/usr/share/xfce4-panel"' '-DCOPYRIGHT_YEAR="2026"' '-DPACKAGE_LOCALE_DIR="/usr/share/locale"' '-DPACKAGE_BUGREPORT="https://gitlab.xfce.org/xfce/xfce4-panel/-/issues"' '-DGETTEXT_PACKAGE="xfce4-panel"' '-DPREFIX="/usr"' '-DBINDIR="/usr/bin"' '-DDATADIR="/usr/share"' '-DINCLUDEDIR="/usr/include"' '-DLIBDIR="/usr/lib"' '-DLIBEXECDIR="/usr/libexec"' '-DLOCALEDIR="/usr/share/locale"' '-DLOCALSTATEDIR="/data/var"' '-DSBINDIR="/usr/sbin"' '-DSYSCONFDIR="/etc"' '-DHAVE_XFCE_REVISION_H=1' '-DLIBXFCE4PANEL_VERSION_API="2.0"' '-DXFCE4_PANEL_CONFIG_VERSION=2' '-DENABLE_X11=1' '-DENABLE_SYMBOL_VISIBILITY=1' '-DHAVE_SYS_WAIT_H=1' '-DHAVE_MATH_H=1' '--sysroot=$(SYSROOT)' $(filter-out --target=%,$(ARCH_CFLAGS)) '-fno-stack-protector' '-fno-pic' '-fno-pie' '-D__lume__=1' $(ARCH_CPPFLAGS) -Iinclude/internal -I$(KERNEL)/include/uapi '-DLUME_NATIVE_ABI=1' '-D_DEFAULT_SOURCE' '-D_POSIX_C_SOURCE=200809L' '-D_POSIX_TIMERS=200809L' '-D_POSIX_MONOTONIC_CLOCK=200809L' '-D_POSIX_THREAD_SAFE_FUNCTIONS=200809L' '-DDBUS_STATIC_BUILD' '-DG_LOG_DOMAIN="xfce4-panel"' '-DHELPERDIR="/usr/lib/xfce4/panel"' '-ffunction-sections' '-fdata-sections' '-fno-pic' '-fno-pie' '-ffile-prefix-map=$(CURDIR)=lume-os'

XFCE_WAYLAND_GDK_OBJECTS := $(addprefix $(XFCE_OUT)/obj/gdk-wayland/,$(XFCE_WAYLAND_GDK_SOURCES:=.o))
XFCE_WAYLAND_WINDOWING_OBJECTS := $(addprefix $(XFCE_OUT)/obj/windowing-wayland/,$(XFCE_WAYLAND_WINDOWING_SOURCES:=.o))
XFCE_WAYLAND_LAYER_OBJECTS := $(addprefix $(XFCE_OUT)/obj/gtk-layer-shell/,$(XFCE_WAYLAND_LAYER_SOURCES:=.o))
XFCE_WAYLAND_PROTOCOL_SOURCES := $(wildcard ports/gtk/wayland/gdk/wayland/*-protocol.c)
XFCE_WAYLAND_PROTOCOL_OBJECTS := $(addprefix $(XFCE_OUT)/obj/protocols/,$(XFCE_WAYLAND_PROTOCOL_SOURCES:=.o))
XFCE_WAYLAND_PANEL_OBJECT := $(XFCE_OUT)/obj/panel-wayland/vendor/xfce4-panel/panel/panel-plugin-external-wrapper-wayland.c.o

$(XFCE_OUT)/obj/gdk-wayland/%.c.o: %.c mk/xfce-wayland.mk
	mkdir -p "$(@D)"
	$(CC) $(XFCE_WAYLAND_GDK_FLAGS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"
$(XFCE_OUT)/obj/windowing-wayland/%.c.o: %.c mk/xfce-wayland.mk
	mkdir -p "$(@D)"
	$(CC) $(XFCE_WAYLAND_WINDOWING_FLAGS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"
$(XFCE_OUT)/obj/gtk-layer-shell/%.c.o: %.c mk/xfce-wayland.mk
	mkdir -p "$(@D)"
	$(CC) $(XFCE_WAYLAND_GDK_FLAGS) -Iports/gtk-layer-shell/include -Ivendor/gtk-layer-shell/src -Ivendor/gtk-layer-shell/gtk-priv/h -Iports/at-spi2-core/aarch64 -Ivendor/at-spi2-core \
        -DGTK_LAYER_SHELL_MAJOR=0 -DGTK_LAYER_SHELL_MINOR=10 -DGTK_LAYER_SHELL_MICRO=1 \
        -MMD -MP -MF "$@.d" -c "$<" -o "$@"
$(XFCE_OUT)/obj/protocols/%.c.o: %.c mk/xfce-wayland.mk
	mkdir -p "$(@D)"
	$(CC) $(XFCE_WAYLAND_GDK_FLAGS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"
$(XFCE_WAYLAND_PANEL_OBJECT): vendor/xfce4-panel/panel/panel-plugin-external-wrapper-wayland.c mk/xfce-wayland.mk
	mkdir -p "$(@D)"
	$(CC) $(XFCE_WAYLAND_PANEL_FLAGS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(XFCE_OUT)/libgdk-wayland.a: $(XFCE_WAYLAND_GDK_OBJECTS)
$(XFCE_OUT)/libxfce-wayland.a: $(XFCE_WAYLAND_WINDOWING_OBJECTS) $(XFCE_WAYLAND_PANEL_OBJECT)
$(XFCE_OUT)/libgtk-layer-shell.a: $(XFCE_WAYLAND_LAYER_OBJECTS)
$(XFCE_OUT)/libxfce-wayland-protocols.a: $(XFCE_WAYLAND_PROTOCOL_OBJECTS)
$(XFCE_OUT)/libgdk-wayland.a $(XFCE_OUT)/libxfce-wayland.a $(XFCE_OUT)/libgtk-layer-shell.a $(XFCE_OUT)/libxfce-wayland-protocols.a:
	$(file >$@.rsp,rcsD "$@" $(foreach input,$^,"$(input)"))
	$(XFCE_AR) "@$@.rsp"

XFCE_WAYLAND_EPOXY_SOURCES := vendor/libepoxy/src/dispatch_egl.c ports/libepoxy/egl/src/egl_generated_dispatch.c
XFCE_WAYLAND_EPOXY_OBJECTS := $(addprefix $(XFCE_OUT)/obj/epoxy/,$(XFCE_WAYLAND_EPOXY_SOURCES:=.o))
$(XFCE_OUT)/obj/epoxy/%.c.o: %.c mk/xfce-wayland.mk
	mkdir -p "$(@D)"
	$(CC) $(XFCE_WAYLAND_GDK_FLAGS) -Iports/libepoxy/egl/src -Ivendor/libepoxy/src \
        -MMD -MP -MF "$@.d" -c "$<" -o "$@"
$(XFCE_OUT)/libepoxy.a: $(XFCE_WAYLAND_EPOXY_OBJECTS)

.PHONY: xfce-wayland
xfce-wayland: $(WESTON_OUT)/wayland.elf xfce-applications gtk-file-dialog $(XFCE_OUT)/startwayland.elf $(XFCE_OUT)/lume-xfce-session.elf $(XFCE_OUT)/xfce4-panel.elf \
    $(XFCE_OUT)/xfdesktop.elf $(XFCE_OUT)/thunar.elf $(XFCE_OUT)/xfsettingsd.elf
-include $(XFCE_WAYLAND_GDK_OBJECTS:=.d) $(XFCE_WAYLAND_WINDOWING_OBJECTS:=.d) \
    $(XFCE_WAYLAND_LAYER_OBJECTS:=.d) $(XFCE_WAYLAND_PROTOCOL_OBJECTS:=.d) \
    $(XFCE_WAYLAND_PANEL_OBJECT:=.d) $(XFCE_WAYLAND_EPOXY_OBJECTS:=.d)

.PHONY: xfce-wayland-configuration-force
xfce-wayland-configuration-force:
$(XFCE_OUT)/backend-configuration: xfce-wayland-configuration-force
	mkdir -p "$(@D)"
	@printf '%s\n' '$(CC) $(XFCE_WAYLAND_CPPFLAGS) $(ARCH_CPPFLAGS) $(ARCH_CFLAGS)' > "$@.tmp"
	@cmp -s "$@.tmp" "$@" || cp "$@.tmp" "$@"
	@rm -f "$@.tmp"

$(XFCE_OUT)/obj/startwayland.o: apps/desktop/startwayland.c mk/xfce-wayland.mk
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -MMD -MP -MF "$@.d" -c "$<" -o "$@"
$(XFCE_OUT)/startwayland.elf: $(XFCE_OUT)/obj/startwayland.o
	$(CC) $(DESKTOP_APP_BASE) -nostdlib -static \
        -Wl,--gc-sections,--build-id=none,-z,max-page-size=4096,-T,arch/$(ARCH)/user.ld \
        "$(SYSROOT)/lib/crt0.o" "$<" -Wl,--start-group "$(SYSROOT)/lib/liblume.a" \
        "$(SYSROOT)/lib/libc.a" $(wildcard $(SYSROOT)/lib/libcompiler-rt.a) -Wl,--end-group -o "$@"
-include $(XFCE_OUT)/obj/startwayland.o.d

XFCE_WAYLAND_ABI_OBJECTS := $(XFCE_OUT)/obj/abi/window.o $(XFCE_OUT)/obj/abi/seat.o
$(XFCE_OUT)/obj/abi/window.o: tests/unit/gtk_layer_window_abi.c mk/xfce-wayland.mk
	mkdir -p "$(@D)"
	$(CC) $(XFCE_WAYLAND_GDK_FLAGS) -Iports/at-spi2-core/aarch64 -Ivendor/at-spi2-core -Ivendor/gtk-layer-shell/gtk-priv/h \
        -DGTK_LAYER_SHELL_MAJOR=0 -DGTK_LAYER_SHELL_MINOR=10 -DGTK_LAYER_SHELL_MICRO=1 \
        -MMD -MP -MF "$@.d" -c "$<" -o "$@"
$(XFCE_OUT)/obj/abi/seat.o: tests/unit/gtk_layer_seat_abi.c mk/xfce-wayland.mk
	mkdir -p "$(@D)"
	$(CC) $(XFCE_WAYLAND_GDK_FLAGS) -Iports/at-spi2-core/aarch64 -Ivendor/at-spi2-core -Ivendor/gtk-layer-shell/gtk-priv/h \
        -DGTK_LAYER_SHELL_MAJOR=0 -DGTK_LAYER_SHELL_MINOR=10 -DGTK_LAYER_SHELL_MICRO=1 \
        -MMD -MP -MF "$@.d" -c "$<" -o "$@"
.PHONY: xfce-wayland-abi
xfce-wayland-abi: $(XFCE_WAYLAND_ABI_OBJECTS)
xfce-wayland: xfce-wayland-abi
-include $(XFCE_WAYLAND_ABI_OBJECTS:=.d)

XFCE_WAYLAND_DESKTOP_FLAGS := '-Iports/xfdesktop/aarch64/src' '-Ivendor/xfdesktop/src' '-Iports/xfdesktop/aarch64' '-Ivendor/xfdesktop' '-Iports/xfdesktop/aarch64/common' '-Ivendor/xfdesktop/common' '-Ivendor/proxy-libintl' '-Iports/glib/aarch64' '-Iports/glib/aarch64/glib' '-Ivendor/glib' '-Ivendor/glib/glib' '-Ivendor/glib/gmodule' '-Ivendor/glib/gobject' '-Ivendor/glib/gio' '-Iports/gtk/aarch64' '-Ivendor/gtk' '-Iports/pango/aarch64' '-Ivendor/pango' '-Ivendor/harfbuzz/src' '-Iports/harfbuzz/aarch64/src' '-Ivendor/fribidi/lib' '-Iports/fribidi/aarch64/lib' '-Iports/cairo/aarch64/src' '-Ivendor/cairo/src' '-Ivendor/cairo/util/cairo-gobject' '-Iports/fontconfig/aarch64' '-Ivendor/fontconfig' '-Ivendor/freetype/include' '-Iports/x11/include' '-Ivendor/expat/lib' '-Ivendor/pixman/pixman' '-Ivendor/zlib' '-Iports/gdk-pixbuf/aarch64' '-Ivendor/gdk-pixbuf' '-Iports/x11/libpng' '-Ivendor/libpng' '-Iports/libepoxy/aarch64/include' '-Ivendor/libepoxy/include' '-Ivendor/libXi/include' '-Iports/at-spi2-core/aarch64' '-Ivendor/at-spi2-core' '-Ivendor/at-spi2-core/atk-adaptor' '-Iports/dbus/aarch64' '-Ivendor/dbus' '-Iports/libxfce4util/aarch64' '-Ivendor/libxfce4util' '-Iports/libxfce4ui/aarch64' '-Ivendor/libxfce4ui' '-Iports/xfconf/aarch64' '-Ivendor/xfconf' '-Iports/libxfce4windowing/aarch64' '-Ivendor/libxfce4windowing' '-Iports/libwnck/aarch64' '-Ivendor/libwnck' '-Ivendor/libdisplay-info/include' '-Iports/exo/aarch64' '-Ivendor/exo' '-Iports/garcon/aarch64' '-Ivendor/garcon' '-Ivendor/libyaml/include' '-Iinclude/posix' '-Iinclude/lume' '-Iarch/$(ARCH)/include' '-I$(SYSROOT)/include' '-D_FILE_OFFSET_BITS=64' '-Wall' '-Winvalid-pch' '-Wextra' '-std=gnu11' '-O3' '-Wmissing-declarations' '-Wmissing-noreturn' '-Wold-style-definition' '-Wredundant-decls' '-Wpointer-arith' '-Wcast-align' '-Winit-self' '-Wshadow' '-Wmissing-include-dirs' '-Wundef' '-Wformat' '-Wformat-security' '-Wformat-y2k' '-Wnested-externs' '-Wno-unused-parameter' '-Wno-declaration-after-statement' '-Werror=implicit-function-declaration' '-Wno-error=deprecated-declarations' '-DNDEBUG' '-DG_DISABLE_CAST_CHECKS' '-DG_DISABLE_ASSERT' '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_72' '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_72' '-DPACKAGE="xfdesktop"' '-DPACKAGE_NAME="xfdesktop"' '-DPACKAGE_VERSION="4.20.2"' '-DVERSION="4.20.2"' '-DVERSION_SHORT="4.20"' '-DPACKAGE_STRING="xfdesktop 4.20.2"' '-DPACKAGE_DATADIR="share/xfdesktop"' '-DCOPYRIGHT_YEAR="2026"' '-DPACKAGE_LOCALE_DIR="/usr/share/locale"' '-DPACKAGE_BUGREPORT="https://gitlab.xfce.org/xfce/xfdesktop/-/issues"' '-DGETTEXT_PACKAGE="xfdesktop"' '-DPREFIX="/usr"' '-DBINDIR="/usr/bin"' '-DDATADIR="/usr/share"' '-DINCLUDEDIR="/usr/include"' '-DLIBDIR="/usr/lib"' '-DLIBEXECDIR="/usr/libexec"' '-DLOCALEDIR="/usr/share/locale"' '-DLOCALSTATEDIR="/data/var"' '-DSBINDIR="/usr/sbin"' '-DSYSCONFDIR="/etc"' '-DHAVE_XFCE_REVISION_H=1' '-DENABLE_DESKTOP_MENU=1' '-DENABLE_DESKTOP_ICONS=1' '-DENABLE_FILE_ICONS=1' '-DENABLE_X11=1' '-DFILE_MANAGER_FALLBACK="Thunar"' '-DDEFAULT_BACKDROP="/usr/share/backgrounds/xfce/xfce-x.png"' '-DHAVE_ERRNO_H=1' '-DHAVE_FCNTL_H=1' '-DHAVE_MATH_H=1' '-DHAVE_MEMORY_H=1' '-DHAVE_PWD_H=1' '-DHAVE_STDLIB_H=1' '-DHAVE_STRING_H=1' '-DHAVE_SYS_PARAM_H=1' '-DHAVE_SYS_STAT_H=1' '-DHAVE_SYS_TYPES_H=1' '-DHAVE_TIME_H=1' '-DHAVE_UNISTD_H=1' '--sysroot=$(SYSROOT)' $(filter-out --target=%,$(ARCH_CFLAGS)) '-fno-stack-protector' '-fno-pic' '-fno-pie' '-D__lume__=1' $(ARCH_CPPFLAGS) -Iinclude/internal -I$(KERNEL)/include/uapi '-DLUME_NATIVE_ABI=1' '-D_DEFAULT_SOURCE' '-D_POSIX_C_SOURCE=200809L' '-D_POSIX_TIMERS=200809L' '-D_POSIX_MONOTONIC_CLOCK=200809L' '-D_POSIX_THREAD_SAFE_FUNCTIONS=200809L' '-DDBUS_STATIC_BUILD' '-DXFCEMODDIR="/usr/lib/xfce4/modules"' '-DG_LOG_DOMAIN="xfdesktop"' '-ffunction-sections' '-fdata-sections' '-fno-pic' '-fno-pie' '-ffile-prefix-map=$(CURDIR)=lume-os'
XFCE_WAYLAND_SETTINGS_SOURCES := vendor/xfdesktop/settings/main.c vendor/xfdesktop/settings/xfdesktop-background-settings.c vendor/xfdesktop/settings/xfdesktop-file-icon-settings.c vendor/xfdesktop/settings/xfdesktop-icon-settings.c vendor/xfdesktop/settings/xfdesktop-keyboard-shortcuts-settings.c vendor/xfdesktop/settings/xfdesktop-menu-settings.c ports/xfdesktop/aarch64/settings/xfdesktop-settings-ui.c
XFCE_WAYLAND_SETTINGS_OBJECTS := $(addprefix $(XFCE_OUT)/obj/desktop-settings/,$(XFCE_WAYLAND_SETTINGS_SOURCES:=.o))
$(XFCE_OUT)/obj/desktop-settings/%.c.o: %.c mk/xfce-wayland.mk
	mkdir -p "$(@D)"
	$(CC) $(XFCE_WAYLAND_DESKTOP_FLAGS) -Ivendor/xfdesktop/settings -Iports/xfdesktop/aarch64/settings \
        -UG_LOG_DOMAIN -DG_LOG_DOMAIN='"xfdesktop-settings"' -MMD -MP -MF "$@.d" -c "$<" -o "$@"
$(XFCE_OUT)/xfdesktop-settings.elf: $(XFCE_WAYLAND_SETTINGS_OBJECTS) $(XFCE_LIBRARIES) $(GTK_BASE_LIBRARIES) $(XFCE_SDK_LIBRARIES) $(SYSROOT)/lib/crt0-cxx.o arch/$(ARCH)/user.ld
	$(file >$@.rsp,--gc-sections --strip-debug -z max-page-size=4096 -T arch/$(ARCH)/user.ld "$(SYSROOT)/lib/crt0-cxx.o" $(foreach obj,$(XFCE_WAYLAND_SETTINGS_OBJECTS),"$(obj)") --start-group $(foreach library,$(GTK_BASE_LIBRARIES) $(XFCE_LIBRARIES) $(XFCE_SDK_LIBRARIES),"$(library)") --end-group -o "$@")
	$(LD) "@$@.rsp"
xfce-wayland: $(XFCE_OUT)/xfdesktop-settings.elf
-include $(XFCE_WAYLAND_SETTINGS_OBJECTS:=.d)
$(XFCE_OUT)/obj/gtk-3/vendor/gtk/gtk/gtkapplication-wayland.c.o: vendor/gtk/gtk/gtkapplication-wayland.c mk/xfce-wayland.mk
	mkdir -p "$(@D)"
	$(CC) '-Iports/gtk/aarch64/gtk' '-Ivendor/gtk/gtk' '-Iports/gtk/aarch64' '-Ivendor/gtk' '-Iports/gtk/aarch64/gdk' '-Ivendor/gtk/gdk' '-Ivendor/gtk/gdk/x11' '-Ivendor/proxy-libintl' '-Iports/glib/aarch64' '-Iports/glib/aarch64/glib' '-Ivendor/glib' '-Ivendor/glib/glib' '-Ivendor/glib/gmodule' '-Ivendor/glib/gobject' '-Ivendor/glib/gio' '-Iports/pango/aarch64' '-Ivendor/pango' '-Ivendor/harfbuzz/src' '-Iports/harfbuzz/aarch64/src' '-Ivendor/fribidi/lib' '-Iports/fribidi/aarch64/lib' '-Iports/cairo/aarch64/src' '-Ivendor/cairo/src' '-Iports/fontconfig/aarch64' '-Ivendor/fontconfig' '-Ivendor/freetype/include' '-Iports/x11/include' '-Ivendor/expat/lib' '-Ivendor/pixman/pixman' '-Iports/x11/libpng' '-Ivendor/libpng' '-Ivendor/zlib' '-Ivendor/cairo/util/cairo-gobject' '-Iports/gdk-pixbuf/aarch64' '-Ivendor/gdk-pixbuf' '-Iports/at-spi2-core/aarch64' '-Ivendor/at-spi2-core' '-Iports/libepoxy/aarch64/include' '-Ivendor/libepoxy/include' '-Ivendor/libXi/include' '-Ivendor/at-spi2-core/atk-adaptor' '-Iports/dbus/aarch64' '-Ivendor/dbus' '-Iinclude/posix' '-Iinclude/lume' '-Iarch/$(ARCH)/include' '-I$(SYSROOT)/include' '-D_FILE_OFFSET_BITS=64' '-Wall' '-Winvalid-pch' '-O3' '-DG_LOG_USE_STRUCTURED=1' '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_56' '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_58' '-DGTK_VERSION="3.24.52"' '-D_GNU_SOURCE' '-DG_DISABLE_CAST_CHECKS' '--sysroot=$(SYSROOT)' $(filter-out --target=%,$(ARCH_CFLAGS)) '-fno-stack-protector' '-fno-pic' '-fno-pie' '-D__lume__=1' $(ARCH_CPPFLAGS) -Iinclude/internal -I$(KERNEL)/include/uapi '-DLUME_NATIVE_ABI=1' '-D_DEFAULT_SOURCE' '-D_POSIX_C_SOURCE=200809L' '-D_POSIX_TIMERS=200809L' '-D_POSIX_MONOTONIC_CLOCK=200809L' '-D_POSIX_THREAD_SAFE_FUNCTIONS=200809L' '-DDBUS_STATIC_BUILD' '-DGTK_COMPILATION' '-DG_LOG_DOMAIN="Gtk"' '-DGTK_PRINT_BACKEND_ENABLE_UNSUPPORTED' '-DGTK_BINARY_VERSION="3.0.0"' '-DGTK_HOST="lume"' '-DGTK_DATA_PREFIX="/usr"' '-DGTK_LIBDIR="/usr/lib"' '-DGTK_LOCALEDIR="/usr/share/locale"' '-DGTK_DATADIR="/usr/share"' '-DGTK_SYSCONFDIR="/etc"' '-DX11_DATA_PREFIX="/usr"' '-DINCLUDE_IM_am_et' '-DINCLUDE_IM_cedilla' '-DINCLUDE_IM_cyrillic_translit' '-DINCLUDE_IM_inuktitut' '-DINCLUDE_IM_ipa' '-DINCLUDE_IM_multipress' '-DINCLUDE_IM_thai' '-DINCLUDE_IM_ti_er' '-DINCLUDE_IM_ti_et' '-DINCLUDE_IM_viqr' '-DINCLUDE_IM_xim' '-fno-strict-aliasing' '-Wpointer-arith' '-Wimplicit-function-declaration' '-Wformat=2' '-Wformat-security' '-Wnested-externs' '-Wold-style-definition' '-Wundef' '-Wunused' '-Wcast-align' '-Wmissing-noreturn' '-Wmissing-format-attribute' '-Wmissing-include-dirs' '-Wignored-qualifiers' '-Werror=implicit' '-Werror=nonnull' '-Werror=init-self' '-Werror=main' '-Werror=missing-braces' '-Werror=sequence-point' '-Werror=return-type' '-Werror=trigraphs' '-Werror=array-bounds' '-Werror=write-strings' '-Werror=address' '-Werror=int-to-pointer-cast' '-Werror=pointer-to-int-cast' '-Werror=empty-body' '-Werror=write-strings' '-ffunction-sections' '-fdata-sections' '-fno-pic' '-fno-pie' '-ffile-prefix-map=$(CURDIR)=lume-os' -MMD -MP -MF "$@.d" -c "$<" -o "$@"
$(XFCE_OUT)/libgtk-3.a: $(XFCE_OUT)/obj/gtk-3/vendor/gtk/gtk/gtkapplication-wayland.c.o

$(XFCE_OUT)/obj/gtk-wayland-gpu.o: tests/integration/gtk_wayland_gpu.c
	mkdir -p "$(@D)"
	$(CC) $(GTK_DIALOG_CFLAGS) $(GTK_DIALOG_INCLUDES) -Iports/libepoxy/aarch64/include -Ivendor/libepoxy/include -c "$<" -o "$@"
$(XFCE_OUT)/gtk-wayland-gpu.elf: $(XFCE_OUT)/obj/gtk-wayland-gpu.o $(GTK_DIALOG_LIBRARIES)
	$(file >$@.rsp,--gc-sections --strip-debug --build-id=none -z max-page-size=4096 -T arch/$(ARCH)/user.ld "$(SYSROOT)/lib/crt0-cxx.o" "$<" --start-group $(foreach library,$(GTK_DIALOG_LIBRARIES),"$(library)") --end-group -o "$@")
	$(LD) "@$@.rsp"
xfce-wayland: $(XFCE_OUT)/gtk-wayland-gpu.elf
XFCE_WAYLAND_SETTINGS_BACKEND_SOURCES := vendor/xfce4-settings/xfsettingsd/displays-wayland.c vendor/xfce4-settings/common/xfce-wlr-output-manager.c
XFCE_WAYLAND_SETTINGS_BACKEND_OBJECTS := $(addprefix $(XFCE_OUT)/obj/settings-wayland/,$(XFCE_WAYLAND_SETTINGS_BACKEND_SOURCES:=.o))
$(XFCE_OUT)/obj/settings-wayland/%.c.o: %.c
	mkdir -p "$(@D)"
	$(CC) '-DHAVE_CONFIG_H' '-Iports/xfce4-settings/aarch64/xfsettingsd' '-Ivendor/xfce4-settings/xfsettingsd' '-Iports/xfce4-settings/aarch64' '-Ivendor/xfce4-settings' '-DDATADIR="/usr/share"' '-DBINDIR="/usr/bin"' '-DSYSCONFIGDIR="/etc"' '-DLOCALEDIR="/usr/share/locale"' '-DG_LOG_DOMAIN="xfsettingsd"' '-DNDEBUG' '-Iports/xfce4-settings/aarch64' '-Ivendor/xfce4-settings' '-Iports/gtk/aarch64' '-Ivendor/gtk' '-Iports/pango/aarch64' '-Ivendor/pango' '-Ivendor/harfbuzz/src' '-Iports/harfbuzz/aarch64/src' '-Ivendor/fribidi/lib' '-Iports/fribidi/aarch64/lib' '-Iports/cairo/aarch64/src' '-Ivendor/cairo/src' '-Ivendor/cairo/util/cairo-gobject' '-Iports/fontconfig/aarch64' '-Ivendor/fontconfig' '-Ivendor/freetype/include' '-Iports/x11/include' '-Ivendor/expat/lib' '-Ivendor/pixman/pixman' '-Ivendor/zlib' '-Iports/gdk-pixbuf/aarch64' '-Ivendor/gdk-pixbuf' '-Ivendor/proxy-libintl' '-Iports/glib/aarch64' '-Iports/glib/aarch64/glib' '-Ivendor/glib' '-Ivendor/glib/glib' '-Ivendor/glib/gmodule' '-Ivendor/glib/gobject' '-Iports/x11/libpng' '-Ivendor/libpng' '-Iports/libepoxy/aarch64/include' '-Ivendor/libepoxy/include' '-Ivendor/libXi/include' '-Iports/at-spi2-core/aarch64' '-Ivendor/at-spi2-core' '-Ivendor/at-spi2-core/atk-adaptor' '-Iports/dbus/aarch64' '-Ivendor/dbus' '-DDBUS_STATIC_BUILD' '-Ivendor/glib/gio' '-Ivendor/proxy-libintl' '-Iports/glib/aarch64' '-Iports/glib/aarch64/glib' '-Ivendor/glib' '-Ivendor/glib/glib' '-Ivendor/glib/gmodule' '-Ivendor/glib/gobject' '-Ivendor/proxy-libintl' '-Iports/glib/aarch64' '-Iports/glib/aarch64/glib' '-Ivendor/glib' '-Ivendor/glib/glib' '-Ivendor/glib/gmodule' '-Ivendor/glib/gobject' '-Ivendor/proxy-libintl' '-Iports/glib/aarch64' '-Iports/glib/aarch64/glib' '-Ivendor/glib' '-Ivendor/glib/glib' '-Ivendor/glib/gmodule' '-Ivendor/glib/gobject' '-Ivendor/glib/gio' '-Ivendor/proxy-libintl' '-Iports/glib/aarch64' '-Iports/glib/aarch64/glib' '-Ivendor/glib' '-Ivendor/glib/glib' '-Ivendor/glib/gmodule' '-Ivendor/glib/gobject' '-Iports/xfconf/aarch64' '-Ivendor/xfconf' '-Iports/libxfce4util/aarch64' '-Ivendor/libxfce4util' '-Ivendor/proxy-libintl' '-Iports/glib/aarch64' '-Iports/glib/aarch64/glib' '-Ivendor/glib' '-Ivendor/glib/glib' '-Ivendor/glib/gmodule' '-Ivendor/glib/gobject' '-Ivendor/glib/gio' '-Iports/libxfce4util/aarch64' '-Ivendor/libxfce4util' '-Ivendor/proxy-libintl' '-Iports/glib/aarch64' '-Iports/glib/aarch64/glib' '-Ivendor/glib' '-Ivendor/glib/glib' '-Ivendor/glib/gmodule' '-Ivendor/glib/gobject' '-Ivendor/glib/gio' '-Iports/libxfce4ui/aarch64' '-Ivendor/libxfce4ui' '-Iports/gtk/aarch64' '-Ivendor/gtk' '-Iports/pango/aarch64' '-Ivendor/pango' '-Ivendor/harfbuzz/src' '-Iports/harfbuzz/aarch64/src' '-Ivendor/fribidi/lib' '-Iports/fribidi/aarch64/lib' '-Iports/cairo/aarch64/src' '-Ivendor/cairo/src' '-Ivendor/cairo/util/cairo-gobject' '-Iports/fontconfig/aarch64' '-Ivendor/fontconfig' '-Ivendor/freetype/include' '-Iports/x11/include' '-Ivendor/expat/lib' '-Ivendor/pixman/pixman' '-Ivendor/zlib' '-Iports/gdk-pixbuf/aarch64' '-Ivendor/gdk-pixbuf' '-Ivendor/proxy-libintl' '-Iports/glib/aarch64' '-Iports/glib/aarch64/glib' '-Ivendor/glib' '-Ivendor/glib/glib' '-Ivendor/glib/gmodule' '-Ivendor/glib/gobject' '-Iports/x11/libpng' '-Ivendor/libpng' '-Iports/libepoxy/aarch64/include' '-Ivendor/libepoxy/include' '-Ivendor/libXi/include' '-Iports/at-spi2-core/aarch64' '-Ivendor/at-spi2-core' '-Ivendor/at-spi2-core/atk-adaptor' '-Iports/dbus/aarch64' '-Ivendor/dbus' '-DDBUS_STATIC_BUILD' '-Iports/xfconf/aarch64' '-Ivendor/xfconf' '-Iports/libxfce4util/aarch64' '-Ivendor/libxfce4util' '-Ivendor/glib/gio' '-DGTK_DISABLE_SINGLE_INCLUDES' '-DGTK_DISABLE_DEPRECATED' '-DGSEAL_ENABLE' '-DGDK_PIXBUF_DISABLE_DEPRECATED' '-DG_DISABLE_DEPRECATED' '-Iports/libxfce4ui/aarch64' '-Ivendor/libxfce4ui' '-Iports/gtk/aarch64' '-Ivendor/gtk' '-Iports/pango/aarch64' '-Ivendor/pango' '-Ivendor/harfbuzz/src' '-Iports/harfbuzz/aarch64/src' '-Ivendor/fribidi/lib' '-Iports/fribidi/aarch64/lib' '-Iports/cairo/aarch64/src' '-Ivendor/cairo/src' '-Ivendor/cairo/util/cairo-gobject' '-Iports/fontconfig/aarch64' '-Ivendor/fontconfig' '-Ivendor/freetype/include' '-Iports/x11/include' '-Ivendor/expat/lib' '-Ivendor/pixman/pixman' '-Ivendor/zlib' '-Iports/gdk-pixbuf/aarch64' '-Ivendor/gdk-pixbuf' '-Ivendor/proxy-libintl' '-Iports/glib/aarch64' '-Iports/glib/aarch64/glib' '-Ivendor/glib' '-Ivendor/glib/glib' '-Ivendor/glib/gmodule' '-Ivendor/glib/gobject' '-Iports/x11/libpng' '-Ivendor/libpng' '-Iports/libepoxy/aarch64/include' '-Ivendor/libepoxy/include' '-Ivendor/libXi/include' '-Iports/at-spi2-core/aarch64' '-Ivendor/at-spi2-core' '-Ivendor/at-spi2-core/atk-adaptor' '-Iports/dbus/aarch64' '-Ivendor/dbus' '-DDBUS_STATIC_BUILD' '-Iports/xfconf/aarch64' '-Ivendor/xfconf' '-Iports/libxfce4util/aarch64' '-Ivendor/libxfce4util' '-Ivendor/glib/gio' '-Ivendor/libXi/include' '-Iports/x11/include' '-Iports/x11/include' '-Iports/x11/include' '-Iports/fontconfig/aarch64' '-Ivendor/fontconfig' '-Ivendor/freetype/include' '-Iports/x11/include' '-Ivendor/expat/lib' '-Iports/x11/include' '-O2' '--sysroot=$(SYSROOT)' $(filter-out --target=%,$(ARCH_CFLAGS)) '-fno-stack-protector' '-fno-pic' '-fno-pie' '-D__lume__=1' $(ARCH_CPPFLAGS) -Iinclude/internal -I$(KERNEL)/include/uapi '-DLUME_NATIVE_ABI=1' '-D_DEFAULT_SOURCE' '-D_POSIX_C_SOURCE=200809L' '-D_POSIX_TIMERS=200809L' '-D_POSIX_MONOTONIC_CLOCK=200809L' '-D_POSIX_THREAD_SAFE_FUNCTIONS=200809L' '-Iinclude/posix' '-Iinclude/lume' '-Iarch/$(ARCH)/include' '-I$(SYSROOT)/include' '-Wall' '-Wextra' '-Wno-missing-field-initializers' '-Wno-unused-parameter' '-Wmissing-declarations' '-Wmissing-noreturn' '-Wpointer-arith' '-Wcast-align' '-Wformat' '-Wformat-security' '-Wformat-y2k' '-Winit-self' '-Wmissing-include-dirs' '-Wundef' '-Wredundant-decls' '-Wshadow' '-Wnested-externs' '-Wold-style-definition' '-ffunction-sections' '-fdata-sections' '-ffile-prefix-map=$(CURDIR)=lume-os' -MMD -MP -MF "$@.d" -c "$<" -o "$@"
$(XFCE_OUT)/libxfce-wayland.a: $(XFCE_WAYLAND_SETTINGS_BACKEND_OBJECTS)
-include $(XFCE_WAYLAND_SETTINGS_BACKEND_OBJECTS:=.d)
endif
endif
