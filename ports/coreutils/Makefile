.DEFAULT_GOAL := all
CSTD := gnu23
PORT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SOURCE ?= $(PORT)/../../vendor/coreutils
include $(PORT)/../standalone.mk
ifeq ($(filter $(ARCH),aarch64 amd64 i686),)
$(error Unsupported coreutils architecture)
endif
CONFIG := $(PORT)/generated/$(ARCH)
CPPFLAGS := -DHAVE_CONFIG_H -D_GNU_SOURCE -DLUME_NATIVE_ABI=1 -D_FILE_OFFSET_BITS=64 -I$(PORT) -I$(CONFIG) -I$(CONFIG)/lib -I$(CONFIG)/src -I$(SOURCE)/lib -I$(SOURCE)/src -include $(PORT)/compat.h $(BASE_CPPFLAGS)
ifeq ($(ARCH),amd64)
CFLAGS += -mcmodel=large
endif
ifeq ($(ARCH),i686)
CPPFLAGS += -DLUME_NATIVE_LFS64=1
endif
LIBS := --start-group $(SDK)/lib/liblume.a $(SDK)/lib/libc.a $(firstword $(wildcard $(SDK)/lib/libcompiler-rt.a $(SDK)/lib/libcrt.a)) --end-group
include $(CONFIG)/build.mk
.PHONY: all install launcher
all: $(addprefix $(OUT)/,$(PROGRAMS)) $(OUT)/coreutils
$(OUT)/libcoreutils.a: $(OBJECTS)
	$(AR) rcs $@ $^
$(OUT)/obj/%.o: $(CONFIG)/%.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/obj/%.o: $(SOURCE)/%.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@
launcher: $(OUT)/coreutils
$(OUT)/tests/launcher.o: $(PORT)/launcher.c $(PORT)/commands.h
	mkdir -p $(@D)
	$(CC) $(BASE_CPPFLAGS) -DLUME_NATIVE_ABI=1 -D_FILE_OFFSET_BITS=64 $(CFLAGS) -I$(PORT) -c $< -o $@
$(OUT)/coreutils: $(OUT)/tests/launcher.o
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
PREFIX ?= /usr
install: all
	mkdir -p $(DESTDIR)$(PREFIX)/libexec/coreutils $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(PREFIX)/share/licenses/coreutils
	$(foreach program,$(PROGRAMS),cp $(OUT)/$(program) $(DESTDIR)$(PREFIX)/libexec/coreutils/$(if $(filter ginstall,$(program)),install,$(program));)
	cp $(OUT)/coreutils $(DESTDIR)$(PREFIX)/bin/coreutils
	cp $(SOURCE)/COPYING $(DESTDIR)$(PREFIX)/share/licenses/coreutils/COPYING
$(OBJECTS): $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OBJECTS:.o=.d)
