.DEFAULT_GOAL := all
ARCH ?= amd64
CSTD := gnu11
PORT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SOURCE = $(OS_ROOT)/vendor/e2fsprogs
include $(PORT)/../standalone.mk

ifneq ($(ARCH),amd64)
$(error mkfs.ext4 requires ARCH=amd64)
endif

CPPFLAGS += -D_GNU_SOURCE -I$(PORT) -I$(SOURCE)/lib -I$(SOURCE)/lib/et -I$(SOURCE)/lib/ext2fs
CFLAGS += -MMD -MP

LIBRARY_SOURCES := $(wildcard $(SOURCE)/lib/ext2fs/*.c $(SOURCE)/lib/et/*.c)
OBJECTS := $(OUT)/obj/port/main.o $(OUT)/obj/port/device.o
OBJECTS += $(patsubst $(SOURCE)/%.c,$(OUT)/obj/%.o,$(LIBRARY_SOURCES))

.PHONY: all
all: $(OUT)/mkfs.ext4

$(OUT)/mkfs.ext4: $(OBJECTS)
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o "$@"

-include $(OBJECTS:.o=.d)
