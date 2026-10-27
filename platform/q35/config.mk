QEMU ?= qemu-system-x86_64
QEMU_ACCEL ?= tcg
QEMU_MEMORY ?= 512M
QEMU_CPUS ?= 4
ifeq ($(filter $(QEMU_CPUS),1 2 3 4),)
$(error QEMU_CPUS must be 1, 2, 3 or 4)
endif
QEMU_NET ?= user,id=net0
QEMU_ARGS ?=
QEMU_DISPLAY ?= sdl
QEMU_SERIAL ?= file:$(OUT)/serial.log
QEMU_DATA ?=
QEMU_DATA_ARGS = $(if $(strip $(QEMU_DATA)),-drive "file=$(QEMU_DATA)$(comma)format=raw$(comma)if=none$(comma)id=data" -device virtio-blk-pci$(comma)drive=data$(comma)disable-modern=on,)
comma := ,
.PHONY: run
run: image
	$(QEMU) -name "Lume AMD64 q35" -machine q35,accel=$(QEMU_ACCEL) -cpu max -smp $(QEMU_CPUS) -m $(QEMU_MEMORY) -vga std -display $(QEMU_DISPLAY) -serial $(QEMU_SERIAL) -monitor none -no-reboot -kernel "$(KERNEL)/$(KERNEL_OUT)/lume-amd64.bin" -initrd "$(OUT)/lume-root.fat" -netdev $(QEMU_NET) -device virtio-net-pci,netdev=net0,disable-modern=on -object rng-builtin,id=rng0 -device virtio-rng-pci,rng=rng0,disable-modern=on $(QEMU_DATA_ARGS) $(QEMU_ARGS)
