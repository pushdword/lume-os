RECOVERY_HEADERS := boot/recovery/recovery.h $(KERNEL)/include/kernel/lume/watchdog.h
RECOVERY_CPPFLAGS := -I$(KERNEL)/platform/raspberrypi5/include -I$(KERNEL)/arch/aarch64/include/kernel -Iboot/recovery -I$(KERNEL)/include/kernel -I$(KERNEL)/include/kernel -I$(KERNEL)/include/ddk -I$(KERNEL)/include/uapi -I$(KERNEL)/include/abi/native
RECOVERY_NAMES := start recovery platform fat vfs fdt memory watchdog file_modes
RECOVERY_OBJECTS := $(addprefix $(OUT)/recovery/obj/,$(addsuffix .o,$(RECOVERY_NAMES)))
$(RECOVERY_OBJECTS): Makefile platform/raspberrypi5/recovery.mk $(OUT)/configuration
.PHONY: recovery check-recovery
recovery: $(OUT)/recovery/lume-recovery.img
$(OUT)/recovery/obj/recovery.o: boot/recovery/recovery.c $(RECOVERY_HEADERS)
	mkdir -p "$(@D)"
	$(CC) -I$(KERNEL)/include/freestanding $(RECOVERY_CPPFLAGS) $(CFLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(OUT)/recovery/obj/platform.o: platform/raspberrypi5/recovery.c $(RECOVERY_HEADERS)
	mkdir -p "$(@D)"
	$(CC) -I$(KERNEL)/include/freestanding $(RECOVERY_CPPFLAGS) $(CFLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(OUT)/recovery/obj/fat.o: $(KERNEL)/src/fs/fat/fat.c $(RECOVERY_HEADERS)
	mkdir -p "$(@D)"
	$(CC) -I$(KERNEL)/include/freestanding $(RECOVERY_CPPFLAGS) $(CFLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(OUT)/recovery/obj/vfs.o: $(KERNEL)/src/fs/vfs/vfs.c $(RECOVERY_HEADERS)
	mkdir -p "$(@D)"
	$(CC) -I$(KERNEL)/include/freestanding $(RECOVERY_CPPFLAGS) $(CFLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(OUT)/recovery/obj/fdt.o: $(KERNEL)/platform/raspberrypi5/fdt.c $(RECOVERY_HEADERS)
	mkdir -p "$(@D)"
	$(CC) -I$(KERNEL)/include/freestanding $(RECOVERY_CPPFLAGS) $(CFLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(OUT)/recovery/obj/memory.o: $(KERNEL)/src/lib/memory.c $(RECOVERY_HEADERS)
	mkdir -p "$(@D)"
	$(CC) -I$(KERNEL)/include/freestanding $(RECOVERY_CPPFLAGS) $(CFLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(OUT)/recovery/obj/watchdog.o: $(KERNEL)/src/watchdog/watchdog.c $(RECOVERY_HEADERS)
	mkdir -p "$(@D)"
	$(CC) -I$(KERNEL)/include/freestanding $(RECOVERY_CPPFLAGS) $(CFLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(OUT)/recovery/obj/start.o: arch/aarch64/boot/start.S
	mkdir -p "$(@D)"
	$(CC) $(CFLAGS) -c "$<" -o "$@"
$(OUT)/recovery/lume-recovery.elf: $(RECOVERY_OBJECTS) platform/raspberrypi5/recovery.ld
	$(LD) -z max-page-size=4096 --gc-sections --build-id=none -T platform/raspberrypi5/recovery.ld $(RECOVERY_OBJECTS) -o "$@"
$(OUT)/recovery/lume-recovery.img: $(OUT)/recovery/lume-recovery.elf
	$(OBJCOPY) -O binary "$<" "$@"
$(OUT)/tests/recovery$(HOSTEXE): tests/unit/recovery_test.c boot/recovery/recovery.c $(KERNEL)/src/fs/fat/fat.c $(KERNEL)/src/fs/vfs/vfs.c $(KERNEL)/src/storage/block.c $(KERNEL)/src/security/file_modes.c $(RECOVERY_HEADERS)
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) $(RECOVERY_CPPFLAGS) $(filter %.c,$^) -o "$@"
check-recovery: $(OUT)/tests/recovery$(HOSTEXE)
	env "$(OUT)/tests/recovery$(HOSTEXE)" "$(ROOT_IMAGE)"
-include $(RECOVERY_OBJECTS:=.d)

$(OUT)/recovery/obj/file_modes.o: $(KERNEL)/src/security/file_modes.c $(RECOVERY_HEADERS)
	mkdir -p "$(@D)"
	$(CC) -I$(KERNEL)/include/freestanding $(RECOVERY_CPPFLAGS) $(CFLAGS) -c "$<" -o "$@"
