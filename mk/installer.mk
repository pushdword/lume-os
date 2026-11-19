.PHONY: check-installer
all: $(OUT)/install.elf

INSTALLER_SOURCES := arch/$(ARCH)/start_bare.S apps/installer/main.c apps/installer/install.c libc/crypto/sha256.c libc/string/memory.c libc/string/transfer.c

$(OUT)/install.elf: $(INSTALLER_SOURCES) include/internal/lume/install_format.h libc/crypto/sha256.h $(I686_BUILTINS) | headers
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) -Ilibc/crypto $(CFLAGS) $(ARM_LINK) $(INSTALLER_SOURCES) $(I686_BUILTINS) -o "$@"

check-installer: $(OUT)/tests/install$(HOSTEXE) $(OUT)/tests/install-app$(HOSTEXE)
	env "$(OUT)/tests/install$(HOSTEXE)"
	env "$(OUT)/tests/install-app$(HOSTEXE)"

$(OUT)/tests/install$(HOSTEXE): tests/unit/install_test.c apps/installer/install.c include/internal/lume/install_format.h libc/crypto/sha256.c libc/crypto/sha256.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -UNDEBUG -Ilibc/crypto tests/unit/install_test.c apps/installer/install.c libc/crypto/sha256.c -o "$@"

$(OUT)/tests/install-app$(HOSTEXE): tests/unit/install_app_test.c apps/installer/main.c apps/installer/install.c include/internal/lume/install_format.h libc/crypto/sha256.c libc/crypto/sha256.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -UNDEBUG -Ilibc/crypto -I"$(KERNEL)/include/abi/native" -I"$(KERNEL)/include/uapi" tests/unit/install_app_test.c apps/installer/install.c libc/crypto/sha256.c -o "$@"
