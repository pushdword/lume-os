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
SDK_RUNTIME ?= $(SDK)/lib/liblume.a
LIBS := --start-group $(SDK_RUNTIME) $(SDK)/lib/libc.a $(firstword $(wildcard $(SDK)/lib/libcompiler-rt.a $(SDK)/lib/libcrt.a)) --end-group
include $(CONFIG)/build.mk
PORT_OBJECTS := $(OUT)/obj/port/freopen.o
OBJECTS += $(PORT_OBJECTS)
LLVM_NM ?= llvm-nm
SDK_LINK_SYMBOLS := $(if $(wildcard $(SDK_RUNTIME)),$(filter fchdir linkat readlinkat symlinkat futimens mkfifo mkfifoat utime,$(shell "$(LLVM_NM)" -gj --defined-only "$(SDK_RUNTIME)")))
SDK_FALLBACKS := $(filter futimens mkfifo mkfifoat utime,$(SDK_LINK_SYMBOLS))
OBJECTS := $(filter-out $(addprefix $(OUT)/obj/lib/,$(addsuffix .o,$(SDK_FALLBACKS))),$(OBJECTS))
SDK_CHMOD_NOFOLLOW := $(findstring LUME_FILES_MODE_NOFOLLOW,$(file <$(SDK)/include/lume/files.h))
CHMOD_OBJECT := $(OUT)/obj/lib/fchmodat.o
$(CHMOD_OBJECT): CPPFLAGS += $(if $(SDK_CHMOD_NOFOLLOW),-DLUME_NATIVE_CHMOD_NOFOLLOW=1)
$(OUT)/obj/lib/linkat.o: CPPFLAGS += $(if $(filter linkat,$(SDK_LINK_SYMBOLS)),-DHAVE_LINKAT=1)
$(OUT)/obj/lib/readlinkat.o: CPPFLAGS += $(if $(filter readlinkat,$(SDK_LINK_SYMBOLS)),-DHAVE_READLINKAT=1)
$(OUT)/obj/lib/symlinkat.o: CPPFLAGS += $(if $(filter symlinkat,$(SDK_LINK_SYMBOLS)),-DHAVE_SYMLINKAT=1)
$(OUT)/obj/lib/utimens.o: CPPFLAGS += $(if $(filter futimens,$(SDK_LINK_SYMBOLS)),-DHAVE_FUTIMENS=1)
$(OUT)/obj/lib/mkfifoat.o: CPPFLAGS += $(if $(filter mkfifo,$(SDK_LINK_SYMBOLS)),-DHAVE_MKFIFO=1)
$(OUT)/obj/lib/fchdir.o: CPPFLAGS += $(if $(filter fchdir,$(SDK_LINK_SYMBOLS)),-DLUME_SDK_FCHDIR=1)
$(addprefix $(OUT)/obj/lib/,fchdir.o linkat.o readlinkat.o symlinkat.o utimens.o mkfifoat.o) $(CHMOD_OBJECT): $(OUT)/sdk-link-symbols
.PHONY: check-sdk-link-symbols
$(OUT)/sdk-link-symbols: check-sdk-link-symbols
	mkdir -p $(@D)
	printf '%s\n' '$(sort $(SDK_LINK_SYMBOLS) $(SDK_CHMOD_NOFOLLOW))' > $@.tmp
	cmp -s $@.tmp $@ || mv -f $@.tmp $@
	rm -f $@.tmp
.PHONY: all install launcher
all: $(addprefix $(OUT)/,$(PROGRAMS)) $(OUT)/coreutils
$(OUT)/libcoreutils.a: $(OBJECTS) $(OUT)/sdk-link-symbols $(wildcard $(SDK_RUNTIME))
	rm -f "$@.tmp"
	$(AR) rcs "$@.tmp" $(OBJECTS)
	mv -f "$@.tmp" "$@"
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
$(OUT)/coreutils: $(OUT)/tests/launcher.o $(wildcard $(SDK_RUNTIME))
	$(LD) $(LDFLAGS) $(RUNTIME) $< $(LIBS) -o $@
PREFIX ?= /usr
install: all
	mkdir -p $(DESTDIR)$(PREFIX)/libexec/coreutils $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(PREFIX)/share/licenses/coreutils
	$(foreach program,$(PROGRAMS),cp $(OUT)/$(program) $(DESTDIR)$(PREFIX)/libexec/coreutils/$(if $(filter ginstall,$(program)),install,$(program));)
	cp $(OUT)/coreutils $(DESTDIR)$(PREFIX)/bin/coreutils
	cp $(SOURCE)/COPYING $(DESTDIR)$(PREFIX)/share/licenses/coreutils/COPYING
$(OBJECTS): $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/fcntl.h $(PORT)/stdio_support.h
-include $(OBJECTS:.o=.d)
