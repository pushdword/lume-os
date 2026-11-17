PORT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
include $(PORT)/Makefile
.DEFAULT_GOAL := check
HOSTCC ?= clang
TEST_ROOT := $(OS_ROOT)/tests
HOST_INODE_SOURCES := di-set.c ino-map.c hash.c next-prime.c malloc.c calloc.c realloc.c free.c

.PHONY: check integration-test
check: $(OUT)/tests/stdio $(OUT)/tests/inodes $(OUT)/tests/abi.o
	$(OUT)/tests/stdio
	$(OUT)/tests/inodes

$(OUT)/tests/stdio: $(TEST_ROOT)/unit/coreutils_stdio_test.c $(PORT)/stdio_support.h $(TEST_ROOT)/fixtures/coreutils/stdio-bufio.h
	mkdir -p $(@D)
	$(HOSTCC) -O2 -std=c11 -Wall -Wextra -Werror -I$(TEST_ROOT)/fixtures/coreutils $< -o $@

$(OUT)/tests/inodes: $(TEST_ROOT)/unit/coreutils_inode_test.c $(addprefix $(SOURCE)/lib/,$(HOST_INODE_SOURCES))
	mkdir -p $(@D)
	$(HOSTCC) -O1 -g -std=gnu23 -ffunction-sections -fdata-sections -fsanitize=address,undefined -I$(PORT)/generated/aarch64/lib -I$(SOURCE)/lib $^ -Wl,--gc-sections -o $@

$(OUT)/tests/abi.o: $(TEST_ROOT)/unit/coreutils_abi_test.c $(CONFIG)/lib/sys/stat.h $(SOURCE)/lib/dev-ino.h
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

integration-test: $(OUT)/tests/coreutils-test.elf
$(OUT)/tests/commands.o: $(TEST_ROOT)/integration/coreutils.c
	mkdir -p $(@D)
	$(CC) $(BASE_CPPFLAGS) -DLUME_NATIVE_ABI=1 -D_FILE_OFFSET_BITS=64 $(CFLAGS) -c $< -o $@
$(OUT)/tests/coreutils-test.elf: $(OUT)/tests/commands.o
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
