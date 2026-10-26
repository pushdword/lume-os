.DEFAULT_GOAL := all
CSTD := gnu11
PORT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SOURCE = $(OS_ROOT)/vendor/ports-rt
include $(PORT)/../standalone.mk
CPPFLAGS += '-I$(KERNEL)/include/abi/native' '-I$(OS_ROOT)/ports/standalone/include'
SOURCES := shared/realpath.c shared/optional.c shared/file_mode.c shared/sleep.c shared/signals.c shared/resource.c shared/process.c shared/stream_pipe.c
OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(basename $(SOURCES))))
.PHONY: all
all: $(OUT)/libports-rt.a
$(OUT)/libports-rt.a: $(OBJECTS)
	rm -f $@
	$(AR) rcs $@ $^
