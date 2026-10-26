KERNEL_ROOT ?= $(if $(wildcard $(OS_ROOT)/kernel/Makefile),$(OS_ROOT)/kernel,$(OS_ROOT)/../lume-kernel)
KERNEL ?= $(KERNEL_ROOT)
