# SPDX-License-Identifier: BSD-3-Clause
ifneq ($(filter aarch64 amd64 i686,$(ARCH)),)
STORAGE_BENCH_SDK ?= $(SYSROOT)
STORAGE_BENCH_OUT ?= $(OUT)/storage-bench
STORAGE_BENCH_FLAGS := $(DESKTOP_APP_BASE) --sysroot="$(STORAGE_BENCH_SDK)" -std=gnu11
ifeq ($(ARCH),amd64)
STORAGE_BENCH_FLAGS += -mcmodel=large
endif
.PHONY: storage-bench
storage-bench: $(STORAGE_BENCH_OUT)/storage-bench.elf
.PHONY: storage-bench-configuration-force
storage-bench-configuration-force:
$(STORAGE_BENCH_OUT)/configuration: storage-bench-configuration-force
	mkdir -p "$(@D)"
	@printf '%s\n' '$(CC) $(STORAGE_BENCH_FLAGS)' > "$@.tmp"
	@cmp -s "$@.tmp" "$@" || cp "$@.tmp" "$@"
	@rm -f "$@.tmp"
$(STORAGE_BENCH_OUT)/storage-bench.o: tests/integration/storage_bench.c mk/storage-bench.mk $(STORAGE_BENCH_OUT)/configuration
	mkdir -p "$(@D)"
	$(CC) $(STORAGE_BENCH_FLAGS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"
$(STORAGE_BENCH_OUT)/storage-bench.elf: $(STORAGE_BENCH_OUT)/storage-bench.o $(STORAGE_BENCH_SDK)/lib/crt0.o $(STORAGE_BENCH_SDK)/lib/libc.a $(STORAGE_BENCH_SDK)/lib/liblume.a $(wildcard $(STORAGE_BENCH_SDK)/lib/libcompiler-rt.a) arch/$(ARCH)/native.ld
	$(CC) $(STORAGE_BENCH_FLAGS) -nostdlib -static \
        -Wl,--gc-sections,--build-id=none,-z,max-page-size=4096,-T,arch/$(ARCH)/native.ld \
        "$(STORAGE_BENCH_SDK)/lib/crt0.o" "$<" -Wl,--start-group \
        "$(STORAGE_BENCH_SDK)/lib/liblume.a" "$(STORAGE_BENCH_SDK)/lib/libc.a" \
        $(wildcard $(STORAGE_BENCH_SDK)/lib/libcompiler-rt.a) -Wl,--end-group -o "$@"
-include $(STORAGE_BENCH_OUT)/storage-bench.o.d
endif
