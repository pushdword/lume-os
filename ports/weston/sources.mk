# SPDX-License-Identifier: BSD-3-Clause
WESTON_CORE_SOURCES := \
    vendor/weston/libweston/animation.c \
    vendor/weston/libweston/auth.c \
    vendor/weston/libweston/bindings.c \
    vendor/weston/libweston/clipboard.c \
    vendor/weston/libweston/color.c \
    vendor/weston/libweston/color-properties.c \
    vendor/weston/libweston/color-management.c \
    vendor/weston/libweston/color-noop.c \
    vendor/weston/libweston/color-profile-param-builder.c \
    vendor/weston/libweston/compositor.c \
    vendor/weston/libweston/content-protection.c \
    vendor/weston/libweston/data-device.c \
    vendor/weston/libweston/drm-formats.c \
    vendor/weston/libweston/id-number-allocator.c \
    vendor/weston/libweston/input.c \
    vendor/weston/libweston/linux-dmabuf.c \
    vendor/weston/libweston/linux-explicit-synchronization.c \
    vendor/weston/libweston/linux-sync-file.c \
    vendor/weston/libweston/log.c \
    vendor/weston/libweston/noop-renderer.c \
    vendor/weston/libweston/output-capture.c \
    vendor/weston/libweston/pixel-formats.c \
    vendor/weston/libweston/pixman-renderer.c \
    vendor/weston/libweston/plugin-registry.c \
    vendor/weston/libweston/screenshooter.c \
    vendor/weston/libweston/timeline.c \
    vendor/weston/libweston/touch-calibration.c \
    vendor/weston/libweston/weston-log-wayland.c \
    vendor/weston/libweston/weston-log-file.c \
    vendor/weston/libweston/weston-log-flight-rec.c \
    vendor/weston/libweston/weston-log.c \
    vendor/weston/libweston/weston-direct-display.c
WESTON_DESKTOP_SOURCES := \
    vendor/weston/libweston/desktop/libweston-desktop.c \
    vendor/weston/libweston/desktop/client.c \
    vendor/weston/libweston/desktop/seat.c \
    vendor/weston/libweston/desktop/surface.c \
    vendor/weston/libweston/desktop/xwayland.c \
    vendor/weston/libweston/desktop/xdg-shell.c \
    vendor/weston/libweston/desktop/xdg-shell-v6.c
WESTON_SHARED_SOURCES := \
    vendor/weston/shared/config-parser.c \
    vendor/weston/shared/option-parser.c \
    vendor/weston/shared/file-util.c \
    vendor/weston/shared/os-compatibility.c \
    vendor/weston/shared/process-util.c \
    vendor/weston/shared/hash.c \
    vendor/weston/shared/matrix.c
WESTON_XKB_SOURCES := \
    vendor/libxkbcommon/src/compose/parser.c \
    vendor/libxkbcommon/src/compose/paths.c \
    vendor/libxkbcommon/src/compose/state.c \
    vendor/libxkbcommon/src/compose/table.c \
    vendor/libxkbcommon/src/xkbcomp/action.c \
    vendor/libxkbcommon/src/xkbcomp/ast-build.c \
    vendor/libxkbcommon/src/xkbcomp/compat.c \
    vendor/libxkbcommon/src/xkbcomp/expr.c \
    vendor/libxkbcommon/src/xkbcomp/include.c \
    vendor/libxkbcommon/src/xkbcomp/keycodes.c \
    vendor/libxkbcommon/src/xkbcomp/keymap.c \
    vendor/libxkbcommon/src/xkbcomp/keymap-dump.c \
    vendor/libxkbcommon/src/xkbcomp/keywords.c \
    vendor/libxkbcommon/src/xkbcomp/rules.c \
    vendor/libxkbcommon/src/xkbcomp/scanner.c \
    vendor/libxkbcommon/src/xkbcomp/symbols.c \
    vendor/libxkbcommon/src/xkbcomp/types.c \
    vendor/libxkbcommon/src/xkbcomp/vmod.c \
    vendor/libxkbcommon/src/xkbcomp/xkbcomp.c \
    vendor/libxkbcommon/src/atom.c \
    vendor/libxkbcommon/src/context.c \
    vendor/libxkbcommon/src/context-priv.c \
    vendor/libxkbcommon/src/keysym.c \
    vendor/libxkbcommon/src/keysym-case-mappings.c \
    vendor/libxkbcommon/src/keysym-utf.c \
    vendor/libxkbcommon/src/keymap.c \
    vendor/libxkbcommon/src/keymap-priv.c \
    vendor/libxkbcommon/src/rmlvo.c \
    vendor/libxkbcommon/src/scanner-utils.c \
    vendor/libxkbcommon/src/state.c \
    vendor/libxkbcommon/src/text.c \
    vendor/libxkbcommon/src/utf8.c \
    vendor/libxkbcommon/src/utf8-decoding.c \
    vendor/libxkbcommon/src/utils.c \
    vendor/libxkbcommon/src/utils-paths.c \
    ports/libxkbcommon/parser.c
