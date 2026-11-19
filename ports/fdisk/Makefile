.DEFAULT_GOAL := all
ARCH ?= amd64
CSTD := gnu11
PORT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SOURCE = $(OS_ROOT)/vendor/util-linux
include $(PORT)/../standalone.mk

ifneq ($(ARCH),amd64)
$(error fdisk requires ARCH=amd64)
endif

CPPFLAGS += -D_GNU_SOURCE -include $(PORT)/config.h -I$(PORT) -I$(OUT)/include
CPPFLAGS += -I$(SOURCE)/include -I$(SOURCE)/libfdisk/src -I$(SOURCE)/libuuid/src
FATFS_SOURCE := $(KERNEL)/vendor/fatfs
FATFS_BUILD := $(OUT)/fatfs
CPPFLAGS += -I$(FATFS_BUILD) -I$(FATFS_SOURCE)
CFLAGS += -MMD -MP

FDISK := init field item ask alignment label utils context parttype partition table iter script version wipe dos gpt
COMMON := buffer crc32 debug strutils strv c_strtod encode env randutils
UUID := clear compare copy gen_uuid isnull pack parse unpack unparse uuid_time
SOURCES := port/main.c port/block.c port/format.c $(addprefix libfdisk/src/,$(addsuffix .c,$(FDISK)))
SOURCES += $(addprefix lib/,$(addsuffix .c,$(COMMON))) $(addprefix libuuid/src/,$(addsuffix .c,$(UUID)))
OBJECTS := $(addprefix $(OUT)/obj/,$(SOURCES:.c=.o))
OBJECTS += $(OUT)/obj/fatfs/ff.o
HEADER := $(OUT)/include/libfdisk.h

.PHONY: all
all: $(OUT)/fdisk

$(HEADER): $(SOURCE)/libfdisk/src/libfdisk.h.in $(PORT)/Makefile
	mkdir -p $(@D)
	sed -e 's/@LIBFDISK_VERSION@/2.42.4/g' -e 's/@LIBFDISK_MAJOR_VERSION@/2/g' \
	    -e 's/@LIBFDISK_MINOR_VERSION@/42/g' -e 's/@LIBFDISK_PATCH_VERSION@/4/g' $< > $@

$(OBJECTS): $(HEADER)

$(FATFS_BUILD)/ff.c $(FATFS_BUILD)/ff.h: $(FATFS_BUILD)/%: $(FATFS_SOURCE)/%
	mkdir -p "$(@D)"
	cp "$<" "$@"

$(FATFS_BUILD)/ffconf.h: $(FATFS_SOURCE)/ffconf.h
	mkdir -p "$(@D)"
	sed 's/^#define FF_USE_MKFS 0/#define FF_USE_MKFS 1/' "$<" > "$@"

$(OUT)/obj/port/format.o: $(FATFS_BUILD)/ff.h $(FATFS_BUILD)/ffconf.h

$(OUT)/obj/fatfs/ff.o: $(FATFS_BUILD)/ff.c $(FATFS_BUILD)/ff.h $(FATFS_BUILD)/ffconf.h
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -c "$<" -o "$@"

$(OUT)/fdisk: $(OBJECTS)
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@

-include $(OBJECTS:.o=.d)
