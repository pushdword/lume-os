.DEFAULT_GOAL := all
CSTD := gnu11
PORT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SOURCE ?= $(PORT)/source
include $(PORT)/../standalone.mk
CPPFLAGS += -DZ_HAVE_UNISTD_H
SOURCES := adler32.c compress.c crc32.c deflate.c gzclose.c gzlib.c gzread.c gzwrite.c infback.c inffast.c inflate.c inftrees.c trees.c uncompr.c zutil.c
OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(basename $(SOURCES))))
.PHONY: all
all: $(OUT)/libz.a
$(OUT)/libz.a: $(OBJECTS)
	rm -f $@
	$(AR) rcs $@ $^
