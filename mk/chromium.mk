CHROMIUM_SOURCE ?=
CHROMIUM_TARGET_CPU := $(if $(filter amd64,$(ARCH)),x64,$(if $(filter i686,$(ARCH)),x86,arm64))
CHROMIUM_OUT ?= $(CHROMIUM_SOURCE)/out/lume-$(CHROMIUM_TARGET_CPU)
CHROMIUM_OVERLAY ?= $(CURDIR)/ports/chromium/overlay
CHROMIUM_INPUTS ?= $(CURDIR)/ports/chromium/overlay.inputs
CHROMIUM_ARGS ?= $(CURDIR)/ports/chromium/native.gn
CHROMIUM_SYSROOT ?= $(SYSROOT)
CHROMIUM_NATIVE_CXX_EXTENSION ?=
CHROMIUM_OS_SOURCE ?= $(CURDIR)
CHROMIUM_KERNEL_SOURCE ?= $(KERNEL)
CHROMIUM_LLVM ?= $(CHROMIUM_SOURCE)/third_party/llvm-build/Release+Asserts
CHROMIUM_LLVM_TOOLS ?= $(CHROMIUM_LLVM)
CHROMIUM_NM ?= $(CHROMIUM_LLVM_TOOLS)/bin/llvm-nm.exe
CHROMIUM_READELF ?= $(CHROMIUM_LLVM_TOOLS)/bin/llvm-readelf.exe
CHROMIUM_BUILTINS ?= $(CHROMIUM_SYSROOT)/lib/libcompiler-rt.a
CHROMIUM_LINKER_SCRIPT ?= $(CURDIR)/arch/$(ARCH)/chromium.ld
CHROMIUM_GN ?= $(CHROMIUM_SOURCE)/buildtools/win/gn.exe
CHROMIUM_NINJA ?= $(CHROMIUM_SOURCE)/third_party/ninja/ninja.exe
CHROMIUM_PYTHON ?= $(CHROMIUM_SOURCE)/third_party/cpython3/host/bin/python3.exe
CHROMIUM_GPERF ?=
CHROMIUM_X11_LIBRARY_DIR ?= $(X11_NATIVE_OUT)
CHROMIUM_X11_INCLUDE_DIR ?= $(X11_NATIVE_OUT)/include
CHROMIUM_X11_LICENSES ?= $(X11_NATIVE_OUT)/LICENSES.x11-native
CHROMIUM_VISUAL_STUDIO ?= $(GYP_MSVS_OVERRIDE_PATH)
CHROMIUM_VISUAL_STUDIO_VERSION ?= 2022
CHROMIUM_JOBS ?= 8
CHROMIUM_FILE_DIALOG ?= $(if $(filter aarch64 amd64,$(ARCH)),1,0)
CHROMIUM_LOCALES ?= en-US pt-PT
CHROMIUM_QUALIFIED_LICENSES ?=
CHROMIUM_QUALIFIED_LICENSES_SHA256 ?=
CHROMIUM_GRAPHICS_LICENSES ?=
CHROMIUM_GRAPHICS_LICENSES_SHA256 ?=
CHROMIUM_NATIVE_EGL ?= $(if $(and $(strip $(MESA_OUT)),$(filter x11,$(MESA_PLATFORMS))),true,)
CHROMIUM_GRAPHICS_LIBRARY_DIR ?= $(GRAPHICS_OUT)
CHROMIUM_FONT_DIRECTORY := $(CURDIR)/resources/fonts/dejavu
CHROMIUM_FONT_NAMES := DejaVuSans.ttf DejaVuSans-Bold.ttf DejaVuSansMono.ttf
CHROMIUM_FONT_INPUTS := $(addprefix $(CHROMIUM_FONT_DIRECTORY)/,$(CHROMIUM_FONT_NAMES)) $(CHROMIUM_FONT_DIRECTORY)/LICENSE
CHROMIUM_FONT_CONFIG := $(CURDIR)/resources/chromium/fonts.conf
CHROMIUM_XCURSOR_LICENSE := $(CURDIR)/resources/chromium/LICENSES.Xcursor
chromium_make_short_flags = $(if $(filter --%,$(firstword $(MAKEFLAGS))),,$(if $(findstring =,$(firstword $(MAKEFLAGS))),,$(firstword $(MAKEFLAGS))))
override CHROMIUM_REVISION := a733b5afbb3ba618844ff8876b89ad1a774b2f18
override CHROMIUM_V8_REVISION := 440c1c463e2e528c309af5067952e7b11631507f

define chromium_lock
set -eu; \
test "$(OS)" = Windows_NT || { echo 'Chromium requires Windows'; exit 1; }; \
case "$(ARCH)" in aarch64|amd64|i686) ;; *) echo 'Chromium requires ARCH=aarch64, ARCH=amd64 or ARCH=i686'; exit 1 ;; esac; \
case "$(CHROMIUM_JOBS)" in ''|*[!0-9]*) echo 'CHROMIUM_JOBS must be a positive integer'; exit 1 ;; esac; \
test "$(CHROMIUM_JOBS)" -gt 0 || { echo 'CHROMIUM_JOBS must be a positive integer'; exit 1; }; \
case "$(CHROMIUM_FILE_DIALOG)" in 0|1) ;; *) echo 'CHROMIUM_FILE_DIALOG must be zero or one'; exit 1 ;; esac; \
case "$(CHROMIUM_NATIVE_EGL)" in ''|true|false) ;; *) echo 'CHROMIUM_NATIVE_EGL must be true or false'; exit 1 ;; esac; \
test -n "$(CHROMIUM_SOURCE)" || { echo 'CHROMIUM_SOURCE must name an external pinned checkout'; exit 1; }; \
chrome_source=$$(cygpath -am "$(CHROMIUM_SOURCE)"); \
chrome_source_real=$$(realpath "$$chrome_source"); \
chrome_product=$$(realpath "$(CURDIR)"); \
case "$$chrome_source_real" in "$$chrome_product"|"$$chrome_product"/*) echo 'Chromium source must be outside the OS source tree'; exit 1 ;; esac; \
test "$$(git -C "$$chrome_source" rev-parse HEAD)" = "$(CHROMIUM_REVISION)" || { echo 'Chromium source revision differs'; exit 1; }; \
test "$$(git -C "$$chrome_source/v8" rev-parse HEAD)" = "$(CHROMIUM_V8_REVISION)" || { echo 'V8 source revision differs'; exit 1; }; \
chrome_lock="$$chrome_source/.lume-chromium.lock"; \
mkdir "$$chrome_lock" || { echo 'Chromium source tree is locked'; exit 1; }; \
trap 'rm -f -- "$$chrome_lock/normalized" "$$chrome_lock/overlay" "$$chrome_lock/args.gn" "$$chrome_lock/inputs" "$$chrome_lock/listed" "$$chrome_lock/sorted" "$$chrome_lock/unique" "$$chrome_lock/actual-raw" "$$chrome_lock/actual" "$$chrome_lock/LICENSES.chromium" "$$chrome_lock/LICENSES.graphics"; rmdir -- "$$chrome_lock"' 0; \
trap 'exit 1' 1 2 15;
endef

define chromium_prepare
chrome_manifest=$$(cygpath -am "$(CHROMIUM_INPUTS)"); \
chrome_overlay=$$(cygpath -am "$(CHROMIUM_OVERLAY)"); \
chrome_overlay_real=$$(realpath "$$chrome_overlay"); \
chrome_tab=$$(printf '\t'); \
chrome_count=0; \
: > "$$chrome_lock/listed"; \
sed 's/\r$$//' "$$chrome_manifest" > "$$chrome_lock/inputs"; \
test "$$(sed -n '1p' "$$chrome_lock/inputs")" = '# LUME_CHROMIUM_OVERLAY 1' || { echo 'Unsupported Chromium input manifest'; exit 1; }; \
test "$$(sed -n '2p' "$$chrome_lock/inputs")" = '# Chromium $(CHROMIUM_REVISION)' || { echo 'Chromium manifest revision differs'; exit 1; }; \
test "$$(sed -n '3p' "$$chrome_lock/inputs")" = '# V8 $(CHROMIUM_V8_REVISION)' || { echo 'V8 manifest revision differs'; exit 1; }; \
chrome_hash() { \
    sed 's/\r$$//' "$$1" > "$$chrome_lock/normalized" || return; \
    chrome_sum=$$(sha256sum "$$chrome_lock/normalized") || return; \
    printf '%s' "$${chrome_sum%% *}"; \
}; \
while IFS="$$chrome_tab" read -r chrome_before chrome_after chrome_path chrome_extra; do \
    case "$$chrome_before" in \#*) continue ;; esac; \
    test -n "$$chrome_path" && test -z "$$chrome_extra" || { echo 'Invalid Chromium input record'; exit 1; }; \
    case "$$chrome_path" in *[!A-Za-z0-9_./+-]*|.git|.git/*|*/.git|*/.git/*) echo 'Invalid Chromium input path'; exit 1 ;; esac; \
    case "/$$chrome_path/" in *'/../'*|*'/./'*|*'//'*) echo 'Invalid Chromium input path'; exit 1 ;; esac; \
    case "$$chrome_after" in ''|*[!0-9a-f]*) echo 'Invalid Chromium overlay digest'; exit 1 ;; esac; \
    test "$${#chrome_after}" = 64 || { echo 'Invalid Chromium overlay digest'; exit 1; }; \
    if test "$$chrome_before" != -; then \
        case "$$chrome_before" in ''|*[!0-9a-f]*) echo 'Invalid Chromium preimage digest'; exit 1 ;; esac; \
        test "$${#chrome_before}" = 64 || { echo 'Invalid Chromium preimage digest'; exit 1; }; \
    fi; \
    chrome_input="$$chrome_overlay/$$chrome_path"; \
    chrome_target="$$chrome_source/$$chrome_path"; \
    case "$$(realpath -m "$$chrome_input")" in "$$chrome_overlay_real"/*) ;; *) echo 'Chromium overlay escapes its source tree'; exit 1 ;; esac; \
    case "$$(realpath -m "$$chrome_target")" in "$$chrome_source_real"/*) ;; *) echo 'Chromium preimage escapes its source tree'; exit 1 ;; esac; \
    test -f "$$chrome_input" && test ! -L "$$chrome_input" || { echo "Missing Chromium overlay: $$chrome_path"; exit 1; }; \
    test "$$(chrome_hash "$$chrome_input")" = "$$chrome_after" || { echo "Chromium overlay changed: $$chrome_path"; exit 1; }; \
    if test -e "$$chrome_target" || test -L "$$chrome_target"; then \
        test -f "$$chrome_target" && test ! -L "$$chrome_target" || { echo "Invalid Chromium preimage: $$chrome_path"; exit 1; }; \
        chrome_current=$$(chrome_hash "$$chrome_target"); \
        test "$$chrome_current" = "$$chrome_before" || test "$$chrome_current" = "$$chrome_after" || { echo "Unknown Chromium preimage: $$chrome_path"; exit 1; }; \
    else \
        test "$$chrome_before" = - || { echo "Missing Chromium preimage: $$chrome_path"; exit 1; }; \
    fi; \
    chrome_count=$$((chrome_count + 1)); \
    printf './%s\n' "$$chrome_path" >> "$$chrome_lock/listed"; \
done < "$$chrome_lock/inputs"; \
test "$$chrome_count" -gt 0 || { echo 'Chromium input manifest is empty'; exit 1; }; \
LC_ALL=C sort "$$chrome_lock/listed" > "$$chrome_lock/sorted"; \
LC_ALL=C sort -u "$$chrome_lock/listed" > "$$chrome_lock/unique"; \
cmp -s "$$chrome_lock/sorted" "$$chrome_lock/unique" || { echo 'Duplicate Chromium input path'; exit 1; }; \
(cd "$$chrome_overlay" && find . -type f -print) > "$$chrome_lock/actual-raw"; \
LC_ALL=C sort "$$chrome_lock/actual-raw" > "$$chrome_lock/actual"; \
cmp -s "$$chrome_lock/unique" "$$chrome_lock/actual" || { echo 'Chromium input manifest does not cover the overlay tree'; exit 1; }; \
while IFS="$$chrome_tab" read -r chrome_before chrome_after chrome_path chrome_extra; do \
    case "$$chrome_before" in \#*) continue ;; esac; \
    chrome_input="$$chrome_overlay/$$chrome_path"; \
    chrome_target="$$chrome_source/$$chrome_path"; \
    case "$$(realpath -m "$$chrome_input")" in "$$chrome_overlay_real"/*) ;; *) echo 'Chromium overlay escapes its source tree'; exit 1 ;; esac; \
    case "$$(realpath -m "$$chrome_target")" in "$$chrome_source_real"/*) ;; *) echo 'Chromium preimage escapes its source tree'; exit 1 ;; esac; \
    test "$$(chrome_hash "$$chrome_input")" = "$$chrome_after" || { echo "Chromium overlay changed: $$chrome_path"; exit 1; }; \
    if test -e "$$chrome_target" || test -L "$$chrome_target"; then \
        test -f "$$chrome_target" && test ! -L "$$chrome_target" || exit 1; \
        chrome_current=$$(chrome_hash "$$chrome_target"); \
        test "$$chrome_current" != "$$chrome_after" || continue; \
        test "$$chrome_current" = "$$chrome_before" || { echo "Unknown Chromium preimage: $$chrome_path"; exit 1; }; \
    else \
        test "$$chrome_before" = - || { echo "Missing Chromium preimage: $$chrome_path"; exit 1; }; \
    fi; \
    mkdir -p "$$(dirname "$$chrome_target")"; \
    cp "$$chrome_input" "$$chrome_lock/overlay"; \
    test "$$(chrome_hash "$$chrome_lock/overlay")" = "$$chrome_after" || exit 1; \
    mv -f "$$chrome_lock/overlay" "$$chrome_target"; \
done < "$$chrome_lock/inputs"; \
rm -f -- "$$chrome_lock/overlay";
endef

define chromium_generate
test -n "$(CHROMIUM_VISUAL_STUDIO)" || { echo 'CHROMIUM_VISUAL_STUDIO must name a complete Visual Studio installation'; exit 1; }; \
test -n "$(CHROMIUM_GPERF)" || { echo 'CHROMIUM_GPERF must name a native Windows gperf executable'; exit 1; }; \
chrome_out=$$(cygpath -am "$(CHROMIUM_OUT)"); \
chrome_out_real=$$(realpath -m "$$chrome_out"); \
case "$$chrome_out_real" in "$$chrome_source_real"/out/?*) ;; *) echo 'CHROMIUM_OUT must be below the source out directory'; exit 1 ;; esac; \
chrome_python=$$(cygpath -am "$(CHROMIUM_PYTHON)"); \
chrome_sdk=$$(cygpath -am "$(CHROMIUM_SYSROOT)"); \
chrome_llvm=$$(cygpath -am "$(CHROMIUM_LLVM)"); \
chrome_tools=$$(cygpath -am "$(CHROMIUM_LLVM_TOOLS)"); \
for chrome_file in "$(CHROMIUM_GN)" "$(CHROMIUM_NINJA)" "$$chrome_python" \
    "$(CHROMIUM_GPERF)" \
    "$(CHROMIUM_X11_LIBRARY_DIR)/libXrandr.a" "$(CHROMIUM_X11_LIBRARY_DIR)/libXtst.a" \
    "$(CHROMIUM_X11_INCLUDE_DIR)/X11/extensions/XTest.h" \
    "$(CHROMIUM_X11_INCLUDE_DIR)/X11/extensions/record.h" \
    "$(CHROMIUM_X11_INCLUDE_DIR)/X11/extensions/XInput.h" \
    "$$chrome_llvm/bin/clang-cl.exe" "$$chrome_tools/bin/llvm-ar.exe" \
    "$$chrome_tools/bin/ld.lld.exe" "$(CHROMIUM_NM)" "$(CHROMIUM_READELF)" \
    "$(CHROMIUM_BUILTINS)" "$(CHROMIUM_LINKER_SCRIPT)" \
    "$$chrome_sdk/lib/liblume.a" "$$chrome_sdk/lib/libc++.a" \
    "$$chrome_sdk/lib/libc.a" "$$chrome_sdk/lib/crt0-cxx.o"; do \
    test -f "$$chrome_file" || { echo "Missing Chromium build input: $$chrome_file"; exit 1; }; \
done; \
chrome_cxx_extension=; \
if test -n "$(CHROMIUM_NATIVE_CXX_EXTENSION)"; then \
    chrome_cxx_extension=$$(cygpath -am "$(CHROMIUM_NATIVE_CXX_EXTENSION)"); \
    for chrome_file in "$$chrome_cxx_extension/include/__config_site" \
        "$$chrome_cxx_extension/lib/libc++.a" "$$chrome_cxx_extension/lib/liblume.a"; do \
        test -f "$$chrome_file" || { echo "Missing native C++ extension input: $$chrome_file"; exit 1; }; \
    done; \
fi; \
chrome_graphics=; \
if test "$(CHROMIUM_NATIVE_EGL)" = true; then \
    test -n "$(CHROMIUM_GRAPHICS_LIBRARY_DIR)" || { echo 'Native EGL requires CHROMIUM_GRAPHICS_LIBRARY_DIR'; exit 1; }; \
    chrome_graphics=$$(cygpath -am "$(CHROMIUM_GRAPHICS_LIBRARY_DIR)"); \
    test -f "$$chrome_graphics/liblume-gpu.a" || { echo 'Missing native graphics provider archive'; exit 1; }; \
fi; \
mkdir -p "$$chrome_out"; \
{ \
    cat "$(CHROMIUM_ARGS)"; \
    printf '\ntarget_os = "lume"\ntarget_cpu = "%s"\n' "$(CHROMIUM_TARGET_CPU)"; \
    printf 'concurrent_links = 1\nlume_linker_threads = 2\n'; \
    printf 'lume_host_symbol_level = 1\nv8_use_external_startup_data = false\n'; \
    printf '\nlume_toolchain_root = "%s"\n' "$$chrome_llvm"; \
    printf 'lume_llvm_tools_root = "%s"\n' "$$chrome_tools"; \
    printf 'lume_native_nm = "%s"\n' "$$(cygpath -am "$(CHROMIUM_NM)")"; \
    printf 'lume_native_readelf = "%s"\n' "$$(cygpath -am "$(CHROMIUM_READELF)")"; \
    printf 'lume_compiler_builtins = "%s"\n' "$$(cygpath -am "$(CHROMIUM_BUILTINS)")"; \
    printf 'lume_native_sysroot = "%s"\n' "$$chrome_sdk"; \
    if test -n "$(CHROMIUM_NATIVE_EGL)"; then \
        printf 'lume_native_egl = %s\n' "$(CHROMIUM_NATIVE_EGL)"; \
        if test "$(CHROMIUM_NATIVE_EGL)" = true; then printf 'lume_graphics_library_dir = "%s"\n' "$$chrome_graphics"; fi; \
    fi; \
    printf 'lume_native_cxx_extension = "%s"\n' "$$chrome_cxx_extension"; \
    printf 'lume_x11_library_dir = "%s"\n' "$$(cygpath -am "$(CHROMIUM_X11_LIBRARY_DIR)")"; \
    printf 'lume_x11_include_dir = "%s"\n' "$$(cygpath -am "$(CHROMIUM_X11_INCLUDE_DIR)")"; \
    printf 'lume_os_source = "%s"\n' "$$(cygpath -am "$(CHROMIUM_OS_SOURCE)")"; \
    printf 'lume_kernel_source = "%s"\n' "$$(cygpath -am "$(CHROMIUM_KERNEL_SOURCE)")"; \
    printf 'lume_linker_script = "%s"\n' "$$(cygpath -am "$(CHROMIUM_LINKER_SCRIPT)")"; \
    printf 'lume_host_visual_studio_version = "%s"\n' "$(CHROMIUM_VISUAL_STUDIO_VERSION)"; \
    printf 'lume_host_gperf = "%s"\n' "$$(cygpath -am "$(CHROMIUM_GPERF)")"; \
} > "$$chrome_lock/args.gn"; \
if ! cmp -s "$$chrome_lock/args.gn" "$$chrome_out/args.gn"; then \
    cp "$$chrome_lock/args.gn" "$$chrome_out/args.gn"; \
fi; \
rm -f -- "$$chrome_lock/args.gn"; \
export MSYS2_ARG_CONV_EXCL='*' MSYS2_ENV_CONV_EXCL=LUME_GN_ROOT_TARGET; \
export DEPOT_TOOLS_WIN_TOOLCHAIN=0 GYP_MSVS_VERSION="$(CHROMIUM_VISUAL_STUDIO_VERSION)"; \
export GYP_MSVS_OVERRIDE_PATH="$$(cygpath -am "$(CHROMIUM_VISUAL_STUDIO)")"; \
export LUME_GN_ROOT_TARGET=//chrome:chrome; \
PATH="$$(cygpath -au "$$(dirname "$$chrome_python")"):$$PATH"; export PATH; \
"$(CHROMIUM_GN)" gen "$$chrome_out" --root="$$chrome_source" \
    --script-executable="$$chrome_python" --root-target=//chrome:chrome \
    --root-pattern=//chrome:chrome;
endef

define chromium_build
"$(CHROMIUM_NINJA)" -C "$$chrome_out" -j$(CHROMIUM_JOBS) chrome;
endef

.PHONY: chromium-prepare chromium-gen chromium chromium-install
ifeq ($(CHROMIUM_NATIVE_EGL),true)
ifneq ($(strip $(MESA_OUT)),)
chromium-gen chromium chromium-install: graphics-provider
endif
endif
ifneq ($(filter $(X11_NATIVE_OUT),$(CHROMIUM_X11_LIBRARY_DIR)),)
chromium-gen chromium chromium-install: X11_NATIVE_SYSROOT = $(CHROMIUM_SYSROOT)
chromium-gen chromium chromium-install: x11-native
endif
chromium-prepare: $(CHROMIUM_INPUTS)
	@$(chromium_lock) \
	$(chromium_prepare)

chromium-gen: $(CHROMIUM_INPUTS) $(CHROMIUM_ARGS)
	@$(chromium_lock) \
	$(chromium_prepare) \
	$(chromium_generate)

chromium: $(CHROMIUM_INPUTS) $(CHROMIUM_ARGS)
	@$(chromium_lock) \
	$(chromium_prepare) \
	$(chromium_generate) \
	$(chromium_build)

chromium-install: $(CHROMIUM_INPUTS) $(CHROMIUM_ARGS) $(CHROMIUM_FONT_INPUTS) $(CHROMIUM_FONT_CONFIG) $(CHROMIUM_XCURSOR_LICENSE) $(CHROMIUM_QUALIFIED_LICENSES) $(CHROMIUM_GRAPHICS_LICENSES)
	@test -n "$(DESTDIR)" && test "$(DESTDIR)" != / || { echo 'DESTDIR staging directory is required'; exit 1; }
	@chrome_destination=$$(cygpath -am "$(DESTDIR)"); case "$$chrome_destination" in ''|/|?:|?:/) echo 'DESTDIR must be a staging directory'; exit 1 ;; esac
	@$(chromium_lock) :
ifeq ($(CHROMIUM_FILE_DIALOG),1)
ifeq ($(findstring n,$(chromium_make_short_flags)),)
	@MSYS2_ARG_CONV_EXCL= $(MAKE) --no-print-directory -j$(CHROMIUM_JOBS) gtk-file-dialog ARCH="$(ARCH)" OUT="$(OUT)" SYSROOT="$(CHROMIUM_SYSROOT)"
endif
endif
	@$(chromium_lock) \
	$(chromium_prepare) \
	$(chromium_generate) \
	chrome_native_egl=$$("$(CHROMIUM_GN)" args "$$chrome_out" --root="$$chrome_source" --list=lume_native_egl --short | tr -d '\r' | sed -n 's/^lume_native_egl = \(true\|false\)$$/\1/p'); \
	case "$$chrome_native_egl" in true|false) ;; *) echo 'Cannot query native graphics configuration'; exit 1 ;; esac; \
	if test "$$chrome_native_egl" = true; then \
	    test -s "$(CHROMIUM_GRAPHICS_LICENSES)" || { echo 'Missing native graphics license notices'; exit 1; }; \
	    chrome_graphics_sha="$(CHROMIUM_GRAPHICS_LICENSES_SHA256)"; \
	    case "$$chrome_graphics_sha" in *[!0-9a-f]*) echo 'Invalid graphics license SHA256'; exit 1 ;; esac; \
	    test "$${#chrome_graphics_sha}" -eq 64 || { echo 'Graphics license SHA256 is required'; exit 1; }; \
	    cp "$(CHROMIUM_GRAPHICS_LICENSES)" "$$chrome_lock/LICENSES.graphics"; \
	    test "$$(chrome_hash "$$chrome_lock/LICENSES.graphics")" = "$$chrome_graphics_sha" || { echo 'Graphics license notices differ'; exit 1; }; \
	    mv -f "$$chrome_lock/LICENSES.graphics" "$$chrome_out/LICENSES.graphics"; \
	fi; \
	$(chromium_build) \
	if test "$(CHROMIUM_FILE_DIALOG)" = 1; then \
	    test -f "$(GTK_DIALOG_OUT)/lume-file-dialog.elf" || { echo 'Missing Chromium file dialog helper'; exit 1; }; \
	    test -f "$(GTK_DIALOG_OUT)/LICENSES.lume-file-dialog" || { echo 'Missing file dialog license notices'; exit 1; }; \
	fi; \
	test -f "$(CHROMIUM_X11_LICENSES)" || { echo 'Missing X11 library license notices'; exit 1; }; \
	if test -n "$(CHROMIUM_QUALIFIED_LICENSES)$(CHROMIUM_QUALIFIED_LICENSES_SHA256)"; then \
	    test -n "$(CHROMIUM_QUALIFIED_LICENSES)" && test -n "$(CHROMIUM_QUALIFIED_LICENSES_SHA256)" || { echo 'Both qualified license input and SHA256 are required'; exit 1; }; \
	    chrome_notice_sha="$(CHROMIUM_QUALIFIED_LICENSES_SHA256)"; \
	    case "$$chrome_notice_sha" in *[!0-9a-f]*) echo 'Invalid qualified license SHA256'; exit 1 ;; esac; \
	    test "$${#chrome_notice_sha}" -eq 64 || { echo 'Invalid qualified license SHA256'; exit 1; }; \
	    test -s "$(CHROMIUM_QUALIFIED_LICENSES)" || { echo 'Missing qualified license notice'; exit 1; }; \
	    cp "$(CHROMIUM_QUALIFIED_LICENSES)" "$$chrome_lock/LICENSES.chromium"; \
	    test "$$(chrome_hash "$$chrome_lock/LICENSES.chromium")" = "$$chrome_notice_sha" || { echo 'Qualified license notice differs'; exit 1; }; \
	    mv -f "$$chrome_lock/LICENSES.chromium" "$$chrome_out/LICENSES.chromium"; \
	else \
	    "$$chrome_python" "$$chrome_source/tools/licenses/licenses.py" scan \
	        --target-os=lume --gn-out-dir="$$chrome_out" --gn-target=//chrome:chrome; \
	    "$$chrome_python" "$$chrome_source/tools/licenses/licenses.py" license_file \
	        --target-os=lume --gn-out-dir="$$chrome_out" --gn-target=//chrome:chrome \
	        "$$chrome_out/LICENSES.chromium"; \
	fi; \
	for chrome_file in chrome resources.pak chrome_100_percent.pak icudtl.dat LICENSES.chromium; do \
	    test -f "$$chrome_out/$$chrome_file" || { echo "Missing Chromium install input: $$chrome_file"; exit 1; }; \
	done; \
	for chrome_locale in $(CHROMIUM_LOCALES); do \
	    case "$$chrome_locale" in ''|*[!A-Za-z0-9_-]*) echo 'Invalid Chromium locale'; exit 1 ;; esac; \
	    test -f "$$chrome_out/locales/$$chrome_locale.pak" || { echo "Missing Chromium locale: $$chrome_locale"; exit 1; }; \
	done; \
	chrome_install="$(DESTDIR)/data/libexec/chromium"; \
	mkdir -p "$$chrome_install/locales" "$$chrome_install/fonts"; \
	if test "$$chrome_native_egl" = true; then printf 'egl=1\n'; else printf 'egl=0\n'; fi > "$$chrome_install/graphics.conf"; \
	cp "$$chrome_out/chrome" "$$chrome_install/chromium"; \
	chmod 0755 "$$chrome_install/chromium"; \
	if test "$(CHROMIUM_FILE_DIALOG)" = 1; then \
	    cp "$(GTK_DIALOG_OUT)/lume-file-dialog.elf" "$$chrome_install/lume-file-dialog"; \
	    cp "$(GTK_DIALOG_OUT)/LICENSES.lume-file-dialog" "$$chrome_install/"; \
	    chmod 0755 "$$chrome_install/lume-file-dialog"; \
	else \
	    rm -f "$$chrome_install/lume-file-dialog" "$$chrome_install/LICENSES.lume-file-dialog"; \
	fi; \
	for chrome_file in "$$chrome_out"/*.pak; do \
	    test ! -f "$$chrome_file" || cp "$$chrome_file" "$$chrome_install/"; \
	done; \
	for chrome_locale in $(CHROMIUM_LOCALES); do \
	    cp "$$chrome_out/locales/$$chrome_locale.pak" "$$chrome_install/locales/"; \
	done; \
	cp "$$chrome_out/icudtl.dat" "$$chrome_out/LICENSES.chromium" "$$chrome_install/"; \
	if test "$$chrome_native_egl" = true; then cp "$$chrome_out/LICENSES.graphics" "$$chrome_install/"; fi; \
	cp "$(CHROMIUM_X11_LICENSES)" "$$chrome_install/LICENSES.x11-native"; \
	cp "$(CHROMIUM_XCURSOR_LICENSE)" "$$chrome_install/LICENSES.Xcursor"; \
	cp "$(CHROMIUM_FONT_CONFIG)" "$$chrome_install/fonts.conf"; \
	for chrome_font in $(CHROMIUM_FONT_NAMES); do \
	    cp "$(CHROMIUM_FONT_DIRECTORY)/$$chrome_font" "$$chrome_install/fonts/$$chrome_font"; \
	done; \
	cp "$(CHROMIUM_FONT_DIRECTORY)/LICENSE" "$$chrome_install/LICENSE.dejavu"; \
	cp "$$chrome_source/LICENSE" "$$chrome_install/LICENSE.chromium"; \
	cp "$$chrome_source/v8/LICENSE" "$$chrome_install/LICENSE.v8"
