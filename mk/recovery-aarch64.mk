RECOVERY_HEADERS := boot/aarch64/recovery.h $(KERNEL)/include/internal/aarch64/lume/watchdog.h
RECOVERY_CPPFLAGS := -Iboot/aarch64 -I$(KERNEL)/include/internal/aarch64 -I$(KERNEL)/include/uapi/aarch64
RECOVERY_NAMES := start recovery platform fat vfs fdt memory watchdog
RECOVERY_OBJECTS := $(addprefix $(OUT)/recovery/obj/,$(addsuffix .o,$(RECOVERY_NAMES)))
$(RECOVERY_OBJECTS): Makefile mk/recovery-aarch64.mk $(OUT)/configuration
.PHONY: recovery check-recovery
recovery: $(OUT)/recovery/lume-recovery.img
$(OUT)/recovery/obj/%.o: boot/aarch64/%.c $(RECOVERY_HEADERS)
	mkdir -p "$(@D)"
	$(CC) $(RECOVERY_CPPFLAGS) $(CFLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(OUT)/recovery/obj/%.o: $(KERNEL)/src/aarch64/%.c $(RECOVERY_HEADERS)
	mkdir -p "$(@D)"
	$(CC) $(RECOVERY_CPPFLAGS) $(CFLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(OUT)/recovery/obj/start.o: boot/aarch64/start.S
	mkdir -p "$(@D)"
	$(CC) $(CFLAGS) -c "$<" -o "$@"
$(OUT)/recovery/lume-recovery.elf: $(RECOVERY_OBJECTS) boot/aarch64/recovery.ld
	$(LD) -z max-page-size=4096 --gc-sections --build-id=none -T boot/aarch64/recovery.ld $(RECOVERY_OBJECTS) -o "$@"
$(OUT)/recovery/lume-recovery.img: $(OUT)/recovery/lume-recovery.elf
	$(OBJCOPY) -O binary "$<" "$@"
$(OUT)/tests/recovery$(HOSTEXE): tests/aarch64/recovery_test.c boot/aarch64/recovery.c $(KERNEL)/src/aarch64/fat.c $(KERNEL)/src/aarch64/vfs.c $(RECOVERY_HEADERS)
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) $(RECOVERY_CPPFLAGS) $(filter %.c,$^) -o "$@"
check-recovery: $(OUT)/tests/recovery$(HOSTEXE)
	env "$(OUT)/tests/recovery$(HOSTEXE)" "$(ROOT_IMAGE)"
-include $(RECOVERY_OBJECTS:=.d)
