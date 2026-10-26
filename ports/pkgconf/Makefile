.DEFAULT_GOAL := all
CSTD := gnu11
PORT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SOURCE = $(OS_ROOT)/vendor/pkgconf
include $(PORT)/../standalone.mk
CPPFLAGS += -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L '-I$(OS_ROOT)/ports/pkgconf' '-I$(OS_ROOT)/vendor/pkgconf'
SOURCES := libpkgconf/argvsplit.c libpkgconf/audit.c libpkgconf/bsdstubs.c libpkgconf/buffer.c libpkgconf/cache.c libpkgconf/client.c libpkgconf/dependency.c libpkgconf/fileio.c libpkgconf/fragment.c libpkgconf/parser.c libpkgconf/path.c libpkgconf/personality.c libpkgconf/pkg.c libpkgconf/queue.c libpkgconf/tuple.c
OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(basename $(SOURCES))))
.PHONY: all
all: $(OUT)/libpkgconf.a $(OUT)/pkgconf
$(OUT)/libpkgconf.a: $(OBJECTS)
	rm -f $@
	$(AR) rcs $@ $^
$(OUT)/pkgconf: $(OUT)/obj/cli/main.o $(OUT)/obj/cli/getopt_long.o $(OUT)/obj/cli/renderer-msvc.o $(OUT)/libpkgconf.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
