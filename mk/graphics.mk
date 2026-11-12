# SPDX-License-Identifier: BSD-3-Clause
ifneq ($(filter aarch64 amd64 i686,$(ARCH)),)
GRAPHICS_OUT ?= $(OUT)/graphics
GRAPHICS_ARCH_FLAGS := $(if $(filter amd64,$(ARCH)),-mcmodel=large,)
GRAPHICS_ATOMIC_OBJECT := $(GRAPHICS_OUT)/atomic.o
GRAPHICS_RUNTIME_SOURCES := libc/posix/pthread_timed.c libc/posix/directory_sort.c libc/posix/memstream.c libc/posix/filesystem.c libc/lume/render.c
GRAPHICS_RUNTIME_OBJECTS := $(addprefix $(GRAPHICS_OUT)/,$(GRAPHICS_RUNTIME_SOURCES:.c=.o))
GRAPHICS_MATH_SOURCES := ports/picolibc/platform/libm/common/math_denorm.c ports/picolibc/platform/libm/common/math_denormf.c ports/picolibc/platform/libm/common/math_denorml.c ports/picolibc/platform/libm/ld/s_finitel.c
GRAPHICS_MATH_OBJECTS := $(addprefix $(GRAPHICS_OUT)/,$(GRAPHICS_MATH_SOURCES:.c=.o))
GRAPHICS_ATOMIC_HEADERS := $(wildcard include/posix/pthread.h include/posix/sched.h)
GRAPHICS_EPOXY_SOURCES := vendor/libepoxy/src/dispatch_common.c vendor/libepoxy/src/dispatch_egl.c ports/libepoxy/aarch64/src/gl_generated_dispatch.c ports/libepoxy/egl/src/egl_generated_dispatch.c
GRAPHICS_EPOXY_OBJECTS := $(addprefix $(GRAPHICS_OUT)/epoxy/,$(GRAPHICS_EPOXY_SOURCES:.c=.o))
GRAPHICS_EPOXY_INCLUDES := -Iports/libepoxy/egl/src -Iports/libepoxy/egl/include -Iports/libepoxy/aarch64/include -Ivendor/libepoxy/include -Ivendor/libepoxy/src -I"$(MESA_SOURCE)/include"
GRAPHICS_FENCE_SOURCES := vendor/libxshmfence/src/xshmfence_futex.c ports/libxshmfence/allocate.c
GRAPHICS_FENCE_OBJECTS := $(addprefix $(GRAPHICS_OUT)/fence/,$(GRAPHICS_FENCE_SOURCES:.c=.o))
GRAPHICS_XCB_SOURCES := $(wildcard vendor/libxcb/src/xcb_*.c ports/x11/libxcb/generated/*.c)
GRAPHICS_XCB_OBJECTS := $(addprefix $(GRAPHICS_OUT)/xcb/,$(GRAPHICS_XCB_SOURCES:.c=.o))
GRAPHICS_X11_LIBDIR ?= $(SYSROOT)/lib

.PHONY: graphics-atomic graphics-runtime graphics-epoxy graphics-fences graphics-x11 graphics-x11-install
graphics-runtime: $(GRAPHICS_OUT)/liblume-graphics.a
graphics-atomic: $(GRAPHICS_OUT)/libatomic.a

graphics-epoxy: $(GRAPHICS_OUT)/libepoxy-egl.a

graphics-fences: $(GRAPHICS_OUT)/libxshmfence.a

graphics-x11: graphics-fences $(GRAPHICS_OUT)/libxcb.a

graphics-x11-install: graphics-x11
	@set -eu; \
	test -n "$(DESTDIR)" || { echo 'DESTDIR must name the native SDK directory'; exit 1; }; \
	for graphics_library in libX11.a libXau.a libXdmcp.a; do \
	    test -s "$(GRAPHICS_X11_LIBDIR)/$$graphics_library" || { echo "Missing native X11 library: $$graphics_library"; exit 1; }; \
	done; \
	mkdir -p "$(DESTDIR)/lib/pkgconfig" "$(DESTDIR)/include/X11" "$(DESTDIR)/share/licenses/native-x11"; \
	cp "$(GRAPHICS_OUT)/libxcb.a" "$(GRAPHICS_OUT)/libxshmfence.a" "$(DESTDIR)/lib/"; \
	for graphics_library in libX11.a libXau.a libXdmcp.a; do \
	    test "$(abspath $(GRAPHICS_X11_LIBDIR))" = "$(abspath $(DESTDIR)/lib)" || cp "$(GRAPHICS_X11_LIBDIR)/$$graphics_library" "$(DESTDIR)/lib/"; \
	done; \
	cp -R ports/x11/include/. "$(DESTDIR)/include/"; \
	cp vendor/libxshmfence/src/xshmfence.h "$(DESTDIR)/include/X11/xshmfence.h"; \
	for graphics_package in libX11 libXau libXdmcp libxcb xcb-proto libxshmfence; do \
	    cp "vendor/$$graphics_package/COPYING" "$(DESTDIR)/share/licenses/native-x11/$$graphics_package"; \
	done; \
	for graphics_package in xcb xcb-dri3 xcb-present xcb-shm xcb-sync xcb-xfixes xcb-randr x11 x11-xcb xshmfence; do \
	    graphics_version=1.17.0; \
	    graphics_requires='Requires: xcb'; \
	    graphics_libs=''; \
	    case "$$graphics_package" in \
	        xcb) graphics_requires=''; graphics_libs='-L$${libdir} -lxcb -lXau -lXdmcp' ;; \
	        x11|x11-xcb) graphics_version=1.8.12; graphics_libs='-L$${libdir} -lX11' ;; \
	        xshmfence) graphics_version=1.3.3; graphics_requires=''; graphics_libs='-L$${libdir} -lxshmfence' ;; \
	    esac; \
	    printf '%s\n' 'prefix=$${pcfiledir}/../..' 'libdir=$${prefix}/lib' 'includedir=$${prefix}/include' \
	        "Name: $$graphics_package" 'Description: Native X11 library' "Version: $$graphics_version" \
	        "$$graphics_requires" "Libs: $$graphics_libs" 'Cflags: -I$${includedir}' > "$(DESTDIR)/lib/pkgconfig/$$graphics_package.pc"; \
	done

$(GRAPHICS_XCB_OBJECTS): $(GRAPHICS_OUT)/xcb/%.o: %.c mk/graphics.mk $(OUT)/configuration
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_FLAGS_4f6f1f74f6ed) $(GRAPHICS_ARCH_FLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"

$(GRAPHICS_OUT)/libxcb.a: $(GRAPHICS_XCB_OBJECTS)
	$(file >$@.rsp,rcsD "$@" $(foreach object,$^,"$(object)"))
	"$(TOOLBIN)llvm-ar$(HOSTEXE)" @$@.rsp

$(GRAPHICS_FENCE_OBJECTS): $(GRAPHICS_OUT)/fence/%.o: %.c ports/libxshmfence/config.h ports/libxshmfence/lume_futex.h mk/graphics.mk $(OUT)/configuration
	mkdir -p "$(@D)"
	$(CC) $(ARCH_CPPFLAGS) $(filter-out --target=%,$(ARCH_CFLAGS)) $(GRAPHICS_ARCH_FLAGS) --sysroot="$(SYSROOT)" -O2 -std=gnu11 -ffunction-sections -fdata-sections -fno-stack-protector -fno-pic -fno-pie -D__lume__=1 -D_DEFAULT_SOURCE -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -DHAVE_CONFIG_H=1 -Iports/libxshmfence -Ivendor/libxshmfence/src -Ivendor/xorgproto/include -Iinclude -Iinclude/lume -Iinclude/posix -I"$(KERNEL)/include/uapi" -I"$(KERNEL)/include/abi/native" -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(GRAPHICS_OUT)/libxshmfence.a: $(GRAPHICS_FENCE_OBJECTS)
	"$(TOOLBIN)llvm-ar$(HOSTEXE)" rcsD "$@" $^

$(GRAPHICS_EPOXY_OBJECTS): $(GRAPHICS_OUT)/epoxy/%.o: %.c ports/libepoxy/egl/src/config.h mk/graphics.mk $(OUT)/configuration
	@test -n "$(MESA_SOURCE)" && test -f "$(MESA_SOURCE)/include/EGL/eglplatform.h" || { echo 'Set MESA_SOURCE to the native Mesa source tree'; exit 1; }
	mkdir -p "$(@D)"
	$(CC) $(ARCH_CPPFLAGS) $(filter-out --target=%,$(ARCH_CFLAGS)) $(GRAPHICS_ARCH_FLAGS) --sysroot="$(SYSROOT)" -O2 -std=gnu11 -ffunction-sections -fdata-sections -fno-stack-protector -fno-pic -fno-pie -D__lume__=1 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -Iinclude/posix $(GRAPHICS_EPOXY_INCLUDES) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(GRAPHICS_OUT)/libepoxy-egl.a: $(GRAPHICS_EPOXY_OBJECTS)
	"$(TOOLBIN)llvm-ar$(HOSTEXE)" rcsD "$@" $^

$(GRAPHICS_ATOMIC_OBJECT): vendor/compiler-rt/lib/builtins/atomic.c vendor/compiler-rt/lib/builtins/assembly.h $(GRAPHICS_ATOMIC_HEADERS) mk/graphics.mk $(OUT)/configuration
	mkdir -p "$(@D)"
	$(CC) $(ARCH_CPPFLAGS) $(filter-out --target=%,$(ARCH_CFLAGS)) $(GRAPHICS_ARCH_FLAGS) --sysroot="$(SYSROOT)" -O2 -std=gnu11 -ffunction-sections -fdata-sections -fno-stack-protector -fno-pic -fno-pie -D_LIBATOMIC_USE_PTHREAD -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -Iinclude/posix -Ivendor/compiler-rt/lib/builtins -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(GRAPHICS_OUT)/libatomic.a: $(GRAPHICS_ATOMIC_OBJECT)
	"$(TOOLBIN)llvm-ar$(HOSTEXE)" rcsD "$@" "$<"

$(GRAPHICS_RUNTIME_OBJECTS): $(GRAPHICS_OUT)/%.o: %.c mk/graphics.mk $(OUT)/configuration
	mkdir -p "$(@D)"
	$(CC) $(ARCH_CPPFLAGS) $(filter-out --target=%,$(ARCH_CFLAGS)) $(GRAPHICS_ARCH_FLAGS) --sysroot="$(SYSROOT)" -O2 -std=gnu11 -ffunction-sections -fdata-sections -fno-stack-protector -fno-pic -fno-pie -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -Iinclude -Iinclude/lume -Iinclude/posix -I"$(KERNEL)/include/uapi" -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(GRAPHICS_MATH_OBJECTS): $(GRAPHICS_OUT)/%.o: %.c mk/graphics.mk $(OUT)/configuration
	mkdir -p "$(@D)"
	$(CC) $(ARCH_CPPFLAGS) $(filter-out --target=%,$(ARCH_CFLAGS)) $(GRAPHICS_ARCH_FLAGS) --sysroot="$(SYSROOT)" -O2 -std=gnu11 -ffunction-sections -fdata-sections -fno-stack-protector -fno-pic -fno-pie -Ivendor/picolibc/libm/common -Ivendor/picolibc/libm/ld -Iports/picolibc/platform/libm/ld -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(GRAPHICS_OUT)/liblume-graphics.a: $(GRAPHICS_RUNTIME_OBJECTS) $(GRAPHICS_MATH_OBJECTS)
	"$(TOOLBIN)llvm-ar$(HOSTEXE)" rcsD "$@" $^

-include $(GRAPHICS_ATOMIC_OBJECT).d $(GRAPHICS_RUNTIME_OBJECTS:=.d) $(GRAPHICS_MATH_OBJECTS:=.d) $(GRAPHICS_EPOXY_OBJECTS:=.d) $(GRAPHICS_FENCE_OBJECTS:=.d) $(GRAPHICS_XCB_OBJECTS:=.d)

endif
