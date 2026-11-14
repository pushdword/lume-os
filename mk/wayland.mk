# SPDX-License-Identifier: BSD-3-Clause
ifneq ($(filter aarch64 amd64 i686,$(ARCH)),)
WAYLAND_OUT ?= $(OUT)/wayland
WAYLAND_SDK ?= $(SYSROOT)
WAYLAND_SCANNER ?= wayland-scanner
WAYLAND_SOURCE := vendor/wayland
WAYLAND_GENERATED := $(WAYLAND_OUT)/include
WAYLAND_BASE := $(DESKTOP_APP_BASE) --sysroot="$(WAYLAND_SDK)" -std=gnu11 \
    -I$(WAYLAND_GENERATED) -Iports/wayland -I$(WAYLAND_SOURCE)/src \
    -I$(WAYLAND_SOURCE)/cursor -I$(WAYLAND_SOURCE)/egl -I$(WAYLAND_SOURCE) \
    -Iports/libffi -Ivendor/libffi/include
ifeq ($(ARCH),amd64)
WAYLAND_BASE += -mcmodel=large
WAYLAND_FFI_ARCH := x86
WAYLAND_FFI_SOURCES := vendor/libffi/src/x86/ffi64.c vendor/libffi/src/x86/ffiw64.c \
    vendor/libffi/src/x86/unix64.S vendor/libffi/src/x86/win64.S
else ifeq ($(ARCH),i686)
WAYLAND_BASE := $(filter-out -mno-sse -mno-sse2 -mno-mmx,$(WAYLAND_BASE))
WAYLAND_FFI_ARCH := x86
WAYLAND_FFI_SOURCES := vendor/libffi/src/x86/ffi.c vendor/libffi/src/x86/sysv.S
else
WAYLAND_FFI_ARCH := aarch64
WAYLAND_FFI_SOURCES := vendor/libffi/src/aarch64/ffi.c vendor/libffi/src/aarch64/sysv.S
endif
WAYLAND_BASE += -Ivendor/libffi/src/$(WAYLAND_FFI_ARCH)
WAYLAND_FFI_SOURCES += vendor/libffi/src/prep_cif.c vendor/libffi/src/types.c
WAYLAND_COMMON_SOURCES := $(WAYLAND_SOURCE)/src/wayland-util.c \
    $(WAYLAND_SOURCE)/src/connection.c ports/wayland/os.c
WAYLAND_SERVER_SOURCES := $(WAYLAND_COMMON_SOURCES) ports/wayland/overlay/src/wayland-server.c \
    ports/wayland/overlay/src/wayland-shm.c ports/wayland/event-loop.c
WAYLAND_CLIENT_SOURCES := $(WAYLAND_COMMON_SOURCES) $(WAYLAND_SOURCE)/src/wayland-client.c
WAYLAND_CURSOR_SOURCES := $(WAYLAND_SOURCE)/cursor/wayland-cursor.c \
    $(WAYLAND_SOURCE)/cursor/xcursor.c $(WAYLAND_SOURCE)/cursor/os-compatibility.c
WAYLAND_EGL_SOURCES := $(WAYLAND_SOURCE)/egl/wayland-egl.c
WAYLAND_ALL_SOURCES := $(sort $(WAYLAND_SERVER_SOURCES) $(WAYLAND_CLIENT_SOURCES) \
    $(WAYLAND_CURSOR_SOURCES) $(WAYLAND_EGL_SOURCES) $(WAYLAND_FFI_SOURCES))
wayland_objects = $(addprefix $(WAYLAND_OUT)/obj/,$(addsuffix .o,$(1)))
WAYLAND_PROTOCOL_OBJECT := $(WAYLAND_OUT)/obj/wayland-protocol.o
WAYLAND_PROTOCOL_HEADERS := $(addprefix $(WAYLAND_GENERATED)/,wayland-version.h wayland-server-protocol.h \
    wayland-server-protocol-core.h wayland-client-protocol.h wayland-client-protocol-core.h)
WAYLAND_LIBRARIES := $(addprefix $(WAYLAND_OUT)/lib/,libwayland-server.a libwayland-client.a \
    libwayland-cursor.a libwayland-egl.a libwayland-ffi.a)

$(WAYLAND_GENERATED)/wayland-version.h: ports/wayland/wayland-version.h
	mkdir -p "$(@D)"
	cp -p "$<" "$@"
$(WAYLAND_GENERATED)/wayland-server-protocol.h: $(WAYLAND_SOURCE)/protocol/wayland.xml
	mkdir -p "$(@D)"
	$(WAYLAND_SCANNER) server-header "$<" "$@"
$(WAYLAND_GENERATED)/wayland-server-protocol-core.h: $(WAYLAND_SOURCE)/protocol/wayland.xml
	mkdir -p "$(@D)"
	$(WAYLAND_SCANNER) server-header -c "$<" "$@"
$(WAYLAND_GENERATED)/wayland-client-protocol.h: $(WAYLAND_SOURCE)/protocol/wayland.xml
	mkdir -p "$(@D)"
	$(WAYLAND_SCANNER) client-header "$<" "$@"
$(WAYLAND_GENERATED)/wayland-client-protocol-core.h: $(WAYLAND_SOURCE)/protocol/wayland.xml
	mkdir -p "$(@D)"
	$(WAYLAND_SCANNER) client-header -c "$<" "$@"
$(WAYLAND_OUT)/wayland-protocol.c: $(WAYLAND_SOURCE)/protocol/wayland.xml
	mkdir -p "$(@D)"
	$(WAYLAND_SCANNER) public-code "$<" "$@"
$(WAYLAND_PROTOCOL_OBJECT): $(WAYLAND_OUT)/wayland-protocol.c $(WAYLAND_PROTOCOL_HEADERS)
	mkdir -p "$(@D)"
	$(CC) $(WAYLAND_BASE) -c "$<" -o "$@"

$(WAYLAND_OUT)/obj/%.c.o: %.c mk/wayland.mk ports/wayland/config.h $(WAYLAND_PROTOCOL_HEADERS)
	mkdir -p "$(@D)"
	$(CC) $(WAYLAND_BASE) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(WAYLAND_OUT)/obj/%.S.o: %.S mk/wayland.mk ports/libffi/fficonfig.h
	mkdir -p "$(@D)"
	$(CC) $(WAYLAND_BASE) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"

define wayland_archive
$(WAYLAND_OUT)/lib/$(1): $(call wayland_objects,$(2)) $(3) | $(WAYLAND_OUT)/lib
	mkdir -p "$$(@D)"
	$$(file >$$@.rsp,$$(foreach obj,$$^,"$$(obj)"))
	rm -f "$$@.tmp"
	"$$(TOOLBIN)llvm-ar$$(HOSTEXE)" rcsD "$$@.tmp" @"$$@.rsp"
	mv -f "$$@.tmp" "$$@"
endef
$(eval $(call wayland_archive,libwayland-server.a,$(WAYLAND_SERVER_SOURCES),$(WAYLAND_PROTOCOL_OBJECT)))
$(eval $(call wayland_archive,libwayland-client.a,$(WAYLAND_CLIENT_SOURCES),$(WAYLAND_PROTOCOL_OBJECT)))
$(eval $(call wayland_archive,libwayland-cursor.a,$(WAYLAND_CURSOR_SOURCES),))
$(eval $(call wayland_archive,libwayland-egl.a,$(WAYLAND_EGL_SOURCES),))
$(eval $(call wayland_archive,libwayland-ffi.a,$(WAYLAND_FFI_SOURCES),))

$(WAYLAND_OUT)/lib:
	mkdir -p "$@"

.PHONY: wayland-libraries wayland-sdk-install
wayland-libraries: $(WAYLAND_LIBRARIES)

.PHONY: wayland-probes
wayland-probes: $(WAYLAND_OUT)/wayland-core.elf $(WAYLAND_OUT)/wayland-loop.elf
$(WAYLAND_OUT)/wayland-loop.elf: tests/integration/wayland_event_loop.c $(WAYLAND_LIBRARIES)
	$(CC) $(WAYLAND_BASE) -nostdlib -static \
        -Wl,--gc-sections,--build-id=none,-z,max-page-size=4096,-T,arch/$(ARCH)/native.ld \
        "$(WAYLAND_SDK)/lib/crt0.o" "$<" -Wl,--start-group $(WAYLAND_LIBRARIES) \
        "$(WAYLAND_SDK)/lib/liblume.a" "$(WAYLAND_SDK)/lib/libc.a" \
        $(wildcard $(WAYLAND_SDK)/lib/libcompiler-rt.a) -Wl,--end-group -o "$@"
$(WAYLAND_OUT)/wayland-core.elf: tests/integration/wayland_core.c $(WAYLAND_LIBRARIES)
	$(CC) $(WAYLAND_BASE) -I$(WAYLAND_SOURCE)/src -nostdlib -static \
        -Wl,--gc-sections,--build-id=none,-z,max-page-size=4096,-T,arch/$(ARCH)/native.ld \
        "$(WAYLAND_SDK)/lib/crt0.o" "$<" -Wl,--start-group $(WAYLAND_LIBRARIES) \
        "$(WAYLAND_SDK)/lib/liblume.a" "$(WAYLAND_SDK)/lib/libc.a" \
        $(wildcard $(WAYLAND_SDK)/lib/libcompiler-rt.a) -Wl,--end-group -o "$@"

wayland-sdk-install: wayland-libraries
	test -n "$(DESTDIR)"
	mkdir -p "$(DESTDIR)/lib/pkgconfig" "$(DESTDIR)/include" "$(DESTDIR)/share/licenses/wayland"
	cp -p $(WAYLAND_LIBRARIES) "$(DESTDIR)/lib/"
	cp -p $(filter-out %/wayland-server-private.h,$(wildcard $(WAYLAND_SOURCE)/src/wayland-client*.h $(WAYLAND_SOURCE)/src/wayland-server*.h)) $(WAYLAND_SOURCE)/src/wayland-util.h "$(DESTDIR)/include/"
	cp -p $(WAYLAND_SOURCE)/cursor/wayland-cursor.h $(WAYLAND_SOURCE)/egl/wayland-egl*.h "$(DESTDIR)/include/"
	cp -p $(WAYLAND_PROTOCOL_HEADERS) "$(DESTDIR)/include/"
	mkdir -p "$(DESTDIR)/share/wayland" "$(DESTDIR)/share/wayland-protocols"
	cp -p $(WAYLAND_SOURCE)/protocol/wayland.xml "$(DESTDIR)/share/wayland/"
	cp -pR vendor/wayland-protocols/. "$(DESTDIR)/share/wayland-protocols/"
	cp -p $(WAYLAND_SOURCE)/COPYING "$(DESTDIR)/share/licenses/wayland/"
	cp -p vendor/libffi/LICENSE "$(DESTDIR)/share/licenses/wayland/LICENSE.libffi"
	@for wayland_library in wayland-client wayland-server wayland-cursor wayland-egl; do \
        printf '%s\n' 'prefix=$${pcfiledir}/../..' 'libdir=$${prefix}/lib' 'includedir=$${prefix}/include' \
            "Name: $$wayland_library" 'Description: Wayland native library' 'Version: 1.26.0' \
            'Libs: -L$${libdir}'" -l$$wayland_library" 'Libs.private: -lwayland-ffi -llume' \
            'Cflags: -I$${includedir}' > "$(DESTDIR)/lib/pkgconfig/$$wayland_library.pc"; \
        if test "$$wayland_library" != wayland-cursor; then :; else printf '%s\n' 'Requires: wayland-client' >> "$(DESTDIR)/lib/pkgconfig/$$wayland_library.pc"; fi; \
    done
	@printf '%s\n' 'prefix=$${pcfiledir}/../..' 'pkgdatadir=$${prefix}/share/wayland-protocols' \
        'Name: Wayland Protocols' 'Description: Wayland protocol definitions' 'Version: 1.49' \
        > "$(DESTDIR)/lib/pkgconfig/wayland-protocols.pc"

-include $(addsuffix .d,$(call wayland_objects,$(WAYLAND_ALL_SOURCES)))
endif
