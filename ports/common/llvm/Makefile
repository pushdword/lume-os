SHELL := sh
.DEFAULT_GOAL := compiler
OS_ROOT := $(abspath ../../..)
KERNEL_ROOT ?= $(if $(wildcard $(OS_ROOT)/kernel/Makefile),$(OS_ROOT)/kernel,$(OS_ROOT)/../lume-kernel)
OUT ?= build/aarch64
ifeq ($(filter build/%,$(OUT)),)
$(error OUT must be below build/)
endif
ifneq ($(findstring ..,$(OUT)),)
$(error OUT cannot contain parent-directory components)
endif
LLVM_REVISION := 0d261d1ca552c95a8f007e061c787ac7132fbcbc
LLVM_SOURCE ?=
BOOTSTRAP ?=
SYSROOT ?=
CXX_ROOT ?=
BUILTINS ?=
LLVM_TABLEGEN ?= llvm-tblgen
CLANG_TABLEGEN ?= clang-tblgen
HOSTEXE := $(if $(filter Windows_NT,$(OS)),.exe,)
CMAKE ?= cmake
GENERATOR ?= Ninja
JOBS ?= 4
PREFIX ?= /data/sdk
OUT_ABS := $(abspath $(OUT))
STAGE := $(OUT_ABS)/source
PORT := $(CURDIR)
COMPONENTS := llvm clang lld cmake third-party runtimes libcxx libcxxabi libunwind compiler-rt libc
FLAGS := -nostdinc++ -isystem $(CXX_ROOT)/include/c++/v1 -I$(OUT_ABS)/compat -I$(OS_ROOT)/include/common/posix -D_GNU_SOURCE -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -D_POSIX_MONOTONIC_CLOCK=200809L -DLLVM_ON_UNIX=1 -mcpu=cortex-a76 -ffunction-sections -fdata-sections -fno-pic -fno-stack-protector -fno-exceptions -fno-rtti -D__lume__=1 -D_LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS -U__BITINT_MAXWIDTH__
OPTIONS := -DLLVM_ENABLE_PROJECTS="clang;lld" -DLLVM_TARGETS_TO_BUILD=AArch64 -DLLVM_TARGET_ARCH=AArch64 -DLLVM_HOST_TRIPLE=aarch64-unknown-none-elf -DLLVM_DEFAULT_TARGET_TRIPLE=aarch64-unknown-none-elf
DISABLED := LLVM_ENABLE_ASSERTIONS LLVM_ENABLE_THREADS LLVM_ENABLE_EH LLVM_ENABLE_RTTI LLVM_ENABLE_ZLIB LLVM_ENABLE_ZSTD LLVM_ENABLE_LIBXML2 LLVM_ENABLE_LIBEDIT LLVM_ENABLE_TERMINFO LLVM_ENABLE_LIBPFM LLVM_ENABLE_FFI LLVM_INCLUDE_TESTS LLVM_INCLUDE_BENCHMARKS LLVM_INCLUDE_EXAMPLES LLVM_BUILD_TOOLS CLANG_ENABLE_STATIC_ANALYZER CLANG_ENABLE_ARCMT CLANG_ENABLE_OBJC_REWRITER CLANG_BUILD_TOOLS CLANG_ENABLE_CIR LLVM_ENABLE_PIC LLVM_ENABLE_UNWIND_TABLES

.PHONY: prepare configure compiler
prepare:
	test -n "$(LLVM_SOURCE)"
	test ! -e "$(STAGE)"
	mkdir -p "$(STAGE)" "$(OUT_ABS)/compat"
	git -C "$(LLVM_SOURCE)" archive --format=tar -o "$(OUT_ABS)/upstream.tar" $(LLVM_REVISION) $(COMPONENTS)
	tar -xf "$(OUT_ABS)/upstream.tar" -C "$(STAGE)"
	cp -Rp platform/. "$(STAGE)/"
	cp support/Path.inc "$(STAGE)/llvm/lib/Support/Lume/Path.inc"
	cp include/external_threading.h "$(OUT_ABS)/compat/__external_threading"

configure:
	$(CMAKE) -S "$(STAGE)/llvm" -B "$(OUT_ABS)/objects" -G "$(GENERATOR)" \
	    -DCMAKE_TOOLCHAIN_FILE="$(PORT)/aarch64.cmake" -DCMAKE_BUILD_TYPE=Release \
	    -DCMAKE_C_COMPILER="$(BOOTSTRAP)/clang$(HOSTEXE)" -DCMAKE_CXX_COMPILER="$(BOOTSTRAP)/clang$(HOSTEXE)" -DCMAKE_ASM_COMPILER="$(BOOTSTRAP)/clang$(HOSTEXE)" \
	    -DCMAKE_AR="$(BOOTSTRAP)/llvm-ar$(HOSTEXE)" -DCMAKE_RANLIB="$(BOOTSTRAP)/llvm-ranlib$(HOSTEXE)" \
	    -DCMAKE_SYSROOT="$(SYSROOT)" -DCMAKE_CXX_FLAGS="$(FLAGS)" \
	    -DCMAKE_C_FLAGS="-I$(OS_ROOT)/include/common/posix -D_GNU_SOURCE -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -D__lume__=1" \
	    -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" -DHAVE_SYSEXITS_H=1 \
	    -DLUME_OS_ROOT="$(OS_ROOT)" -DLUME_KERNEL_ROOT="$(KERNEL_ROOT)" -DLUME_CXX_ROOT="$(CXX_ROOT)" \
	    -DLUME_SUPPORT_PATH_SOURCE="$(STAGE)/llvm/lib/Support/Path.cpp" -DLUME_NATIVE_PREFIX="$(PREFIX)" -DLUME_BUILTINS="$(BUILTINS)" \
	    -DLLVM_TABLEGEN="$(LLVM_TABLEGEN)" -DCLANG_TABLEGEN="$(CLANG_TABLEGEN)" \
	    -DLLVM_EXTERNAL_PROJECTS=lumec -DLLVM_EXTERNAL_LUMEC_SOURCE_DIR="$(PORT)/driver" \
	    -DLLVM_PARALLEL_COMPILE_JOBS=$(JOBS) -DLLVM_PARALLEL_LINK_JOBS=1 $(OPTIONS) $(addsuffix =OFF,$(addprefix -D,$(DISABLED))) $(CMAKE_FLAGS)

compiler: configure
	$(CMAKE) --build "$(OUT_ABS)/objects" --parallel $(JOBS) --target lumec lumeld llvm-ar llvm-objcopy llvm-nm llvm-readobj lume-tool native-runtime
