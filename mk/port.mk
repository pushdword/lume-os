ARCH ?= aarch64
include $(OS_ROOT)/mk/kernel-path.mk
include $(OS_ROOT)/arch/$(ARCH)/config.mk
CPPFLAGS += $(ARCH_CPPFLAGS) -I$(OS_ROOT)/arch/$(ARCH)/include
CFLAGS += $(ARCH_CFLAGS)
