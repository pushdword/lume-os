# SPDX-License-Identifier: BSD-3-Clause
ifneq ($(filter aarch64 amd64 i686,$(ARCH)),)
include ports/weston/sources.mk
WESTON_OUT ?= $(WAYLAND_OUT)/weston
WESTON_PIXMAN ?= $(DESKTOP_OUT)/libraries/lib/pixman.a
WESTON_DRM_PREFIX ?= $(WAYLAND_SDK)
WESTON_DRM_LIBRARY ?= $(WESTON_DRM_PREFIX)/lib/libdrm.a
WESTON_PROTOCOL_SOURCES := $(wildcard ports/weston/protocol/*-protocol.c)
WESTON_BASE := -Iports/weston -Iports/weston/include -Iports/weston/protocol -Ivendor/weston/shared \
    $(filter-out -Iports/wayland -Ivendor/wayland,$(WAYLAND_BASE)) \
    -Ivendor/weston/include -Ivendor/weston -Ivendor/weston/libweston \
    -Ivendor/libxkbcommon/include -Ivendor/pixman/pixman -Iports/x11/pixman \
    -Iports/libdrm/include -Iports/git/include -I$(WESTON_DRM_PREFIX)/include -I$(WESTON_DRM_PREFIX)/include/libdrm
WESTON_XKB_BASE := -Iports/libxkbcommon -Ivendor/libxkbcommon -Ivendor/libxkbcommon/include -Ivendor/libxkbcommon/src \
    $(filter-out -Iports/wayland -Ivendor/wayland,$(WAYLAND_BASE))
WESTON_SHARED_SOURCES := $(filter-out vendor/weston/shared/os-compatibility.c,$(WESTON_SHARED_SOURCES)) \
    ports/weston/overlay/shared/os-compatibility.c
WESTON_SOURCES := $(WESTON_CORE_SOURCES) $(WESTON_DESKTOP_SOURCES) \
    $(WESTON_SHARED_SOURCES) $(WESTON_PROTOCOL_SOURCES)
WESTON_OBJECTS := $(addprefix $(WESTON_OUT)/obj/,$(WESTON_SOURCES:=.o))
WESTON_XKB_SOURCES := $(filter-out vendor/libxkbcommon/src/utils.c,$(WESTON_XKB_SOURCES)) ports/libxkbcommon/overlay/src/utils.c
WESTON_XKB_OBJECTS := $(addprefix $(WESTON_OUT)/xkb/,$(WESTON_XKB_SOURCES:=.o))

$(WESTON_OUT)/obj/%.c.o: %.c mk/weston.mk ports/weston/config.h $(WAYLAND_PROTOCOL_HEADERS)
	mkdir -p "$(@D)"
	$(CC) $(WESTON_BASE) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(WESTON_OUT)/xkb/%.c.o: %.c mk/weston.mk ports/libxkbcommon/config.h
	mkdir -p "$(@D)"
	$(CC) $(WESTON_XKB_BASE) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"

$(WESTON_OUT)/libweston.a: $(WESTON_OBJECTS)
	$(file >$@.rsp,$(foreach obj,$^,"$(obj)"))
	rm -f "$@.tmp"
	"$(TOOLBIN)llvm-ar$(HOSTEXE)" rcsD "$@.tmp" @"$@.rsp"
	mv -f "$@.tmp" "$@"
$(WESTON_OUT)/libxkbcommon.a: $(WESTON_XKB_OBJECTS)
	$(file >$@.rsp,$(foreach obj,$^,"$(obj)"))
	rm -f "$@.tmp"
	"$(TOOLBIN)llvm-ar$(HOSTEXE)" rcsD "$@.tmp" @"$@.rsp"
	mv -f "$@.tmp" "$@"

.PHONY: weston-libraries
weston-libraries: $(WESTON_OUT)/libweston.a $(WESTON_OUT)/libxkbcommon.a
.PHONY: wayland-install weston-sdk-install
wayland-install: wayland
	test -n "$(DESTDIR)"
	mkdir -p "$(DESTDIR)/bin" "$(DESTDIR)/libexec" "$(DESTDIR)/share/licenses/weston" "$(DESTDIR)/share/licenses/libxkbcommon"
	cp -p "$(WESTON_OUT)/wayland.elf" "$(DESTDIR)/bin/wayland"
	cp -p "$(WESTON_OUT)/wayland-window.elf" "$(DESTDIR)/libexec/wayland-window"
	cp -p vendor/weston/COPYING "$(DESTDIR)/share/licenses/weston/"
	cp -p ports/weston/COPYING.input-events "$(DESTDIR)/share/licenses/weston/"
	cp -p vendor/libxkbcommon/LICENSE "$(DESTDIR)/share/licenses/libxkbcommon/"
weston-sdk-install: weston-libraries
	test -n "$(DESTDIR)"
	mkdir -p "$(DESTDIR)/lib/pkgconfig" "$(DESTDIR)/include/xkbcommon" "$(DESTDIR)/share/licenses/libxkbcommon"
	cp -p "$(WESTON_OUT)/libxkbcommon.a" "$(DESTDIR)/lib/"
	cp -p vendor/libxkbcommon/include/xkbcommon/*.h "$(DESTDIR)/include/xkbcommon/"
	cp -p vendor/libxkbcommon/LICENSE "$(DESTDIR)/share/licenses/libxkbcommon/"
	@printf '%s\n' 'prefix=$${pcfiledir}/../..' 'libdir=$${prefix}/lib' 'includedir=$${prefix}/include' \
        'Name: xkbcommon' 'Description: Keyboard keymap compiler and support library' 'Version: 1.11.0' \
        'Libs: -L$${libdir} -lxkbcommon' 'Cflags: -I$${includedir}' > "$(DESTDIR)/lib/pkgconfig/xkbcommon.pc"
.PHONY: wayland
wayland: $(WESTON_OUT)/wayland.elf $(WESTON_OUT)/wayland-window.elf
$(WESTON_OUT)/wayland-window.elf: $(WESTON_OUT)/obj/tests/integration/wayland_window.c.o $(WESTON_OUT)/obj/ports/weston/protocol/xdg-shell-protocol.c.o $(WAYLAND_LIBRARIES)
	$(CC) $(WESTON_BASE) -nostdlib -static \
        -Wl,--gc-sections,--build-id=none,-z,max-page-size=4096,-T,arch/$(ARCH)/native.ld \
        "$(WAYLAND_SDK)/lib/crt0.o" "$<" $(WESTON_OUT)/obj/ports/weston/protocol/xdg-shell-protocol.c.o \
        -Wl,--start-group $(WAYLAND_LIBRARIES) "$(WAYLAND_SDK)/lib/liblume.a" \
        "$(WAYLAND_SDK)/lib/libc.a" $(wildcard $(WAYLAND_SDK)/lib/libcompiler-rt.a) \
        -Wl,--end-group -o "$@"
$(WESTON_OUT)/wayland.elf: $(WESTON_OUT)/obj/ports/weston/lume.c.o $(WESTON_OUT)/obj/ports/weston/shell.c.o $(WESTON_OUT)/obj/libc/lume/render.c.o $(WESTON_OUT)/libweston.a $(WESTON_OUT)/libxkbcommon.a $(WAYLAND_LIBRARIES) $(WESTON_PIXMAN) $(WESTON_DRM_LIBRARY)
	$(CC) $(WESTON_BASE) -nostdlib -static \
        -Wl,--gc-sections,--build-id=none,-z,max-page-size=4096,-T,arch/$(ARCH)/native.ld \
        "$(WAYLAND_SDK)/lib/crt0.o" "$<" $(WESTON_OUT)/obj/ports/weston/shell.c.o $(WESTON_OUT)/obj/libc/lume/render.c.o -Wl,--start-group $(WESTON_OUT)/libweston.a \
        $(WESTON_OUT)/libxkbcommon.a $(WAYLAND_LIBRARIES) "$(WESTON_PIXMAN)" \
        "$(WESTON_DRM_LIBRARY)" "$(WAYLAND_SDK)/lib/liblume.a" "$(WAYLAND_SDK)/lib/libc.a" \
        $(wildcard $(WAYLAND_SDK)/lib/libcompiler-rt.a) -Wl,--end-group -o "$@"
-include $(WESTON_OBJECTS:=.d) $(WESTON_XKB_OBJECTS:=.d)
endif
