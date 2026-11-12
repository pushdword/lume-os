# SPDX-License-Identifier: BSD-3-Clause
ifneq ($(filter aarch64 amd64 i686,$(ARCH)),)
MESA_SOURCE ?=
LIBDRM_SOURCE ?=
MESA_OUT ?=
MESA_CROSS ?=
MESA_MESON ?= meson
MESA_NINJA ?= ninja
MESA_JOBS ?= 8
MESA_PLATFORMS ?=
MESA_CPU := $(if $(filter amd64,$(ARCH)),x86_64,$(if $(filter i686,$(ARCH)),x86,aarch64))
MESA_COMMA := ,
MESA_DRIVERS := $(if $(filter aarch64,$(ARCH)),virgl$(MESA_COMMA)v3d,virgl)
MESA_PREFIX = $(MESA_OUT)/sysroot
MESA_X11_LIBDIR ?= $(SYSROOT)/lib
MESA_NATIVE_LIBRARIES := $(MESA_OUT)/objects/src/egl/libEGL.a $(MESA_OUT)/objects/src/mesa/glapi/es2api/libGLESv2.a $(MESA_OUT)/objects/src/gbm/libgbm.a $(MESA_OUT)/objects/src/gbm/backends/dri/libdri_gbm.a $(MESA_OUT)/objects/src/gallium/targets/dri/libgallium-26.2.6.a $(MESA_PREFIX)/lib/libdrm.a $(GRAPHICS_OUT)/libxcb.a $(GRAPHICS_OUT)/libxshmfence.a $(GRAPHICS_OUT)/libatomic.a $(GRAPHICS_OUT)/liblume-graphics.a $(MESA_X11_LIBDIR)/libX11.a $(MESA_X11_LIBDIR)/libXau.a $(MESA_X11_LIBDIR)/libXdmcp.a $(SYSROOT)/lib/libexpat.a $(SYSROOT)/lib/zlib.a

.PHONY: graphics-provider
graphics-provider: graphics-runtime graphics-atomic graphics-x11 $(GRAPHICS_OUT)/liblume-gpu.a

$(GRAPHICS_OUT)/liblume-gpu.a: $(MESA_NATIVE_LIBRARIES)
	@set -eu; \
	mkdir -p "$(@D)"; \
	{ printf 'CREATE "%s"\n' "$@.tmp"; \
	  for graphics_library in $(foreach library,$^,"$(library)"); do printf 'ADDLIB "%s"\n' "$$graphics_library"; done; \
	  printf 'SAVE\nEND\n'; } > "$@.mri"; \
	"$(TOOLBIN)llvm-ar$(HOSTEXE)" -M < "$@.mri"; \
	mv -f "$@.tmp" "$@"

define mesa_stage
while IFS="$$(printf '\t')" read -r upstream expected relative; do \
    case "$$relative" in ''|'#'*) continue ;; esac; \
    case "/$$relative/" in */../*|*/.git/*|*:*|*\\*) echo 'Invalid graphics overlay path'; exit 1 ;; esac; \
    graphics_file="$(CURDIR)/ports/$(1)/overlay/$$relative"; \
    graphics_hash=$$(tr -d '\r' < "$$graphics_file" | sha256sum); \
    test "$${graphics_hash%% *}" = "$$expected" || { echo "Graphics overlay differs: $$relative"; exit 1; }; \
    graphics_target="$(2)/$$relative"; \
    graphics_current=-; \
    if test -f "$$graphics_target"; then \
        graphics_hash=$$(tr -d '\r' < "$$graphics_target" | sha256sum); \
        graphics_current=$${graphics_hash%% *}; \
    fi; \
    case "$$graphics_current" in "$$expected") continue ;; "$$upstream") ;; *) echo "Graphics source differs: $$relative"; exit 1 ;; esac; \
    mkdir -p "$$(dirname "$$graphics_target")"; \
    graphics_temporary="$$graphics_target.lume-stage-$$$$"; \
    tr -d '\r' < "$$graphics_file" > "$$graphics_temporary"; \
    mv -f -- "$$graphics_temporary" "$$graphics_target"; \
done < "$(CURDIR)/ports/$(1)/overlay.inputs";
endef

.PHONY: mesa-configure mesa-libraries mesa-install
mesa-configure: graphics-runtime graphics-atomic
	@set -eu; \
	test -n "$(MESA_SOURCE)" && test -n "$(LIBDRM_SOURCE)" && test -n "$(MESA_OUT)" && test -f "$(MESA_CROSS)" || { echo 'Set MESA_SOURCE, LIBDRM_SOURCE, MESA_OUT and MESA_CROSS'; exit 1; }; \
	case "$(MESA_JOBS)" in ''|*[!0-9]*) echo 'MESA_JOBS must be a positive integer'; exit 1 ;; esac; \
	test "$(MESA_JOBS)" -gt 0 || { echo 'MESA_JOBS must be a positive integer'; exit 1; }; \
	case "$(MESA_PLATFORMS)" in ''|x11) ;; *) echo 'Unsupported native EGL platform'; exit 1 ;; esac; \
	graphics_product=$$(realpath "$(CURDIR)"); \
	for graphics_path in "$(MESA_SOURCE)" "$(LIBDRM_SOURCE)" "$(MESA_OUT)"; do \
	    graphics_absolute=$$(realpath -m "$$graphics_path"); \
	    case "$$graphics_absolute/" in "$$graphics_product/"*) echo 'Graphics source and build trees must be outside the OS source tree'; exit 1 ;; esac; \
	done; \
	$(call mesa_stage,libdrm,$(LIBDRM_SOURCE)) \
	$(call mesa_stage,mesa,$(MESA_SOURCE)) \
	mkdir -p "$(MESA_PREFIX)/lib" "$(MESA_PREFIX)/include/lume"; \
	cp include/lume/render_native.h "$(KERNEL)/include/uapi/lume/render.h" "$(MESA_PREFIX)/include/lume/"; \
	cp "$(GRAPHICS_OUT)/libatomic.a" "$(GRAPHICS_OUT)/liblume-graphics.a" "$(MESA_PREFIX)/lib/"; \
	graphics_reconfigure=; \
	test ! -f "$(MESA_OUT)/libdrm/build.ninja" || graphics_reconfigure=--reconfigure; \
	$(MESA_MESON) setup $$graphics_reconfigure "$(MESA_OUT)/libdrm" "$(LIBDRM_SOURCE)" --cross-file "$(MESA_CROSS)" --prefix "$(MESA_PREFIX)" --libdir lib -Ddefault_library=static -Db_staticpic=false -Dauto_features=disabled -Dtests=false -Dinstall-test-programs=false -Dudev=false; \
	$(MESA_MESON) introspect --machines "$(MESA_OUT)/libdrm" | $(PYTHON) -c "import json,sys; h=json.load(sys.stdin)['host']; sys.exit(0 if h['system']=='lume' and h['cpu_family']=='$(MESA_CPU)' else 'Native graphics target differs')"; \
	$(MESA_NINJA) -C "$(MESA_OUT)/libdrm" -j "$(MESA_JOBS)"; \
	$(MESA_MESON) install -C "$(MESA_OUT)/libdrm" --no-rebuild; \
	graphics_reconfigure=; \
	test ! -f "$(MESA_OUT)/objects/build.ninja" || graphics_reconfigure=--reconfigure; \
	$(MESA_MESON) setup $$graphics_reconfigure "$(MESA_OUT)/objects" "$(MESA_SOURCE)" --cross-file "$(MESA_CROSS)" --prefix "$(MESA_PREFIX)" --libdir lib -Ddefault_library=static -Db_staticpic=false -Dgallium-drivers=$(MESA_DRIVERS) -Dvulkan-drivers= -Dplatforms=$(MESA_PLATFORMS) -Dxlib-lease=disabled -Dglx=disabled -Degl=enabled -Dgbm=enabled -Dgles1=disabled -Dgles2=enabled -Dopengl=true -Dllvm=disabled -Dbuild-tests=false -Dvideo-codecs= -Dzstd=disabled -Dlmsensors=disabled -Dglvnd=disabled -Dvalgrind=disabled -Dlibunwind=disabled -Dgallium-va=disabled; \
	$(MESA_MESON) introspect --machines "$(MESA_OUT)/objects" | $(PYTHON) -c "import json,sys; h=json.load(sys.stdin)['host']; sys.exit(0 if h['system']=='lume' and h['cpu_family']=='$(MESA_CPU)' else 'Native graphics target differs')"

mesa-libraries: mesa-configure
	$(MESA_NINJA) -C "$(MESA_OUT)/objects" -j "$(MESA_JOBS)"

mesa-install: mesa-libraries
	$(MESA_MESON) install -C "$(MESA_OUT)/objects" --no-rebuild
endif
