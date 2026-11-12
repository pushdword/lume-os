# SPDX-License-Identifier: BSD-3-Clause
ifneq ($(filter aarch64 amd64 i686,$(ARCH)),)
GLAMOR_OUT ?= $(GRAPHICS_OUT)/glamor
GLAMOR_FLAGS = -Iports/xorg-server/accelerated $(DESKTOP_FLAGS_213a99f08f54) $(GRAPHICS_ARCH_FLAGS) -include ports/xorg-server/accelerated/dix-config.h -DLUME_XORG_GLAMOR=1 -D__lume__=1 -DEGL_NO_X11=1 -DMESA_EGL_NO_X11_HEADERS=1 -Iinclude -Iports/libdrm/include -Ivendor/xorg-server/glamor -Ivendor/xorg-server/dri3 -Ivendor/xorg-server/present -Iports/libxshmfence/include -Ivendor/libxshmfence/src $(GRAPHICS_EPOXY_INCLUDES) -I"$(MESA_PREFIX)/include" -I"$(MESA_PREFIX)/include/libdrm" -I"$(MESA_SOURCE)/src/gbm/main"
GLAMOR_SOURCES := ports/xorg-server/lume_glamor.c $(filter-out vendor/xorg-server/glamor/glamor_egl.c vendor/xorg-server/glamor/glamor_eglmodule.c vendor/xorg-server/glamor/glamor_egl_stubs.c vendor/xorg-server/glamor/glamor_glx.c vendor/xorg-server/glamor/glamor_xv.c,$(wildcard vendor/xorg-server/glamor/*.c))
GLAMOR_EXTENSION_SOURCES := $(wildcard vendor/xorg-server/dri3/*.c vendor/xorg-server/present/*.c vendor/xorg-server/randr/*.c)
GLAMOR_OBJECTS := $(addprefix $(GLAMOR_OUT)/,$(GLAMOR_SOURCES:.c=.o))
GLAMOR_EXTENSION_OBJECTS := $(addprefix $(GLAMOR_OUT)/,$(GLAMOR_EXTENSION_SOURCES:.c=.o))
GLAMOR_SERVER_SOURCES := $(patsubst $(DESKTOP_OUT)/server/obj/%.o,vendor/xorg-server/%.c,$(filter-out $(DESKTOP_OUT)/server/obj/lume.o,$(DESKTOP_SERVER_OBJECTS))) ports/xorg-server/lume.c vendor/xorg-server/miext/sync/misyncshm.c ports/xorg-server/sha1.c vendor/openssh/openbsd-compat/sha1.c
GLAMOR_SERVER_OBJECTS := $(addprefix $(GLAMOR_OUT)/server/,$(GLAMOR_SERVER_SOURCES:.c=.o))
GLAMOR_FONT_LIBRARIES := $(addprefix $(DESKTOP_OUT)/libraries/lib/,libXfont2.a libfontenc.a freetype.a pixman.a zlib.a libXau.a libXdmcp.a)
GLAMOR_MESA_LIBRARIES := $(MESA_OUT)/objects/src/egl/libEGL.a $(MESA_OUT)/objects/src/mesa/glapi/es2api/libGLESv2.a $(MESA_OUT)/objects/src/gbm/libgbm.a $(MESA_OUT)/objects/src/gbm/backends/dri/libdri_gbm.a $(MESA_OUT)/objects/src/gallium/targets/dri/libgallium-26.2.6.a $(MESA_PREFIX)/lib/libdrm.a
GLAMOR_RUNTIME_LIBRARIES := $(GRAPHICS_OUT)/libepoxy-egl.a $(GRAPHICS_OUT)/libxshmfence.a $(GRAPHICS_OUT)/libatomic.a $(GRAPHICS_OUT)/liblume-graphics.a $(SYSROOT)/lib/libexpat.a $(SYSROOT)/lib/libc++.a $(SYSROOT)/lib/liblume.a $(SYSROOT)/lib/libc.a $(SYSROOT)/lib/libcompiler-rt.a
GLAMOR_LINKER_SCRIPT := arch/$(ARCH)/$(if $(filter amd64,$(ARCH)),user,chromium).ld
GLAMOR_X11_LIBRARIES := $(if $(filter x11,$(MESA_PLATFORMS)),$(GRAPHICS_OUT)/libxcb.a $(DESKTOP_OUT)/libraries/lib/libX11.a,)

.PHONY: graphics-glamor graphics-xserver graphics-xserver-install
graphics-glamor: graphics-epoxy graphics-fences $(GLAMOR_OUT)/libglamor.a $(GLAMOR_OUT)/libgpu-extensions.a
graphics-xserver: graphics-glamor graphics-atomic graphics-runtime $(if $(filter x11,$(MESA_PLATFORMS)),graphics-x11,) $(GLAMOR_OUT)/Xlume-gpu.elf

GRAPHICS_NOTICES ?= $(MESA_PREFIX)/share/licenses/LICENSES.graphics
graphics-xserver-install: graphics-xserver
	@test -s "$(GRAPHICS_NOTICES)" || { echo 'Set GRAPHICS_NOTICES to the native graphics stack license notices'; exit 1; }
	mkdir -p "$(DESKTOP_ROOT)/usr/bin" "$(DESKTOP_ROOT)/usr/share/licenses/native-graphics"
	cp "$(GLAMOR_OUT)/Xlume-gpu.elf" "$(DESKTOP_ROOT)/usr/bin/Xlume-gpu"
	cp "$(GRAPHICS_NOTICES)" "$(DESKTOP_ROOT)/usr/share/licenses/native-graphics/LICENSES"

ifneq ($(strip $(MESA_OUT)),)
ifeq ($(MESA_PLATFORMS),x11)
desktop: graphics-xserver
desktop-install: graphics-xserver-install
endif
endif

$(GLAMOR_SERVER_OBJECTS): $(GLAMOR_OUT)/server/%.o: %.c ports/xorg-server/accelerated/dix-config.h ports/xorg-server/lume_glamor.h mk/glamor.mk $(OUT)/configuration
	mkdir -p "$(@D)"
	$(CC) $(GLAMOR_FLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"

# Keep the server geometry allocator separate from the Xlib allocator.
$(GLAMOR_SERVER_OBJECTS): GLAMOR_FLAGS += -DXkbFreeGeomOverlayKeys=lume_xkb_free_geom_overlay_keys

$(GLAMOR_OUT)/server/vendor/openssh/openbsd-compat/sha1.o: GLAMOR_FLAGS += -Iports/xorg-server/sha1

$(GLAMOR_OUT)/libxserver.a: $(GLAMOR_SERVER_OBJECTS)
	$(file >$@.rsp,rcsD "$@" $(foreach object,$^,"$(object)"))
	"$(TOOLBIN)llvm-ar$(HOSTEXE)" @$@.rsp

$(GLAMOR_OUT)/Xlume-gpu.elf: $(GLAMOR_OUT)/libxserver.a $(GLAMOR_OUT)/libglamor.a $(GLAMOR_OUT)/libgpu-extensions.a $(GLAMOR_FONT_LIBRARIES) $(GLAMOR_MESA_LIBRARIES) $(GLAMOR_RUNTIME_LIBRARIES) $(GLAMOR_X11_LIBRARIES) $(SYSROOT)/lib/crt0.o $(GLAMOR_LINKER_SCRIPT)
	$(file >$@.rsp,-T "$(GLAMOR_LINKER_SCRIPT)" --gc-sections --build-id=sha1 -z max-page-size=4096 -o "$@" "$(SYSROOT)/lib/crt0.o" --start-group $(foreach lib,$(filter %.a,$^),"$(lib)") --end-group)
	"$(TOOLBIN)ld.lld$(HOSTEXE)" @$@.rsp

$(GLAMOR_OBJECTS) $(GLAMOR_EXTENSION_OBJECTS): $(GLAMOR_OUT)/%.o: %.c ports/xorg-server/accelerated/dix-config.h mk/glamor.mk $(OUT)/configuration
	@test -n "$(MESA_OUT)" && test -f "$(MESA_PREFIX)/include/libdrm/drm.h" || { echo 'Set MESA_OUT to the compiled native Mesa stack'; exit 1; }
	mkdir -p "$(@D)"
	$(CC) $(GLAMOR_FLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"

$(GLAMOR_OUT)/libglamor.a: $(GLAMOR_OBJECTS)
	"$(TOOLBIN)llvm-ar$(HOSTEXE)" rcsD "$@" $^

$(GLAMOR_OUT)/libgpu-extensions.a: $(GLAMOR_EXTENSION_OBJECTS)
	"$(TOOLBIN)llvm-ar$(HOSTEXE)" rcsD "$@" $^

-include $(GLAMOR_OBJECTS:=.d) $(GLAMOR_EXTENSION_OBJECTS:=.d) $(GLAMOR_SERVER_OBJECTS:=.d)
endif
