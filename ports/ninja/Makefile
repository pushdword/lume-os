.DEFAULT_GOAL := all
CSTD := gnu11
PORT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SOURCE ?= $(PORT)/source
include $(PORT)/../standalone.mk
CPPFLAGS := '-I$(OS_ROOT)/ports/standalone/include' $(CPPFLAGS)
LIBS := --start-group $(SDK)/lib/libc++.a $(SDK)/lib/liblume.a $(SDK)/lib/libc.a $(SDK)/lib/libcrt.a --end-group
CPPFLAGS += -D_GNU_SOURCE -DUSE_PPOLL -include '$(OS_ROOT)/ports/ninja/config.h' '-I$(OS_ROOT)/ports/standalone/include'
SOURCES := src/build_log.cc src/build.cc src/clean.cc src/clparser.cc src/dyndep.cc src/dyndep_parser.cc src/debug_flags.cc src/deps_log.cc src/disk_interface.cc src/edit_distance.cc src/elide_middle.cc src/eval_env.cc src/graph.cc src/graphviz.cc src/jobserver.cc src/json.cc src/line_printer.cc src/manifest_parser.cc src/metrics.cc src/missing_deps.cc src/parser.cc src/real_command_runner.cc src/state.cc src/status_printer.cc src/string_piece_util.cc src/util.cc src/version.cc src/depfile_parser.cc src/lexer.cc src/jobserver-posix.cc src/subprocess-posix.cc
OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(basename $(SOURCES))))
.PHONY: all
all: $(OUT)/libninja.a $(OUT)/ninja
$(OUT)/libninja.a: $(OBJECTS)
	rm -f $@
	$(AR) rcs $@ $^
$(OUT)/ninja: $(OUT)/obj/src/ninja.o $(OUT)/libninja.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
