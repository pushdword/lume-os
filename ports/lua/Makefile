.DEFAULT_GOAL := all
CSTD := gnu11
PORT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SOURCE = $(OS_ROOT)/vendor/lua
include $(PORT)/../standalone.mk
CPPFLAGS += '-I$(OS_ROOT)/vendor/lua/src'
SOURCES := src/lapi.c src/lauxlib.c src/lbaselib.c src/lcode.c src/lcorolib.c src/lctype.c src/ldblib.c src/ldebug.c src/ldo.c src/ldump.c src/lfunc.c src/lgc.c src/linit.c src/liolib.c src/llex.c src/lmathlib.c src/lmem.c src/loadlib.c src/lobject.c src/lopcodes.c src/loslib.c src/lparser.c src/lstate.c src/lstring.c src/lstrlib.c src/ltable.c src/ltablib.c src/ltm.c src/lundump.c src/lutf8lib.c src/lvm.c src/lzio.c
OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(basename $(SOURCES))))
.PHONY: all
all: $(OUT)/liblua.a $(OUT)/lua $(OUT)/luac
$(OUT)/liblua.a: $(OBJECTS)
	rm -f $@
	$(AR) rcs $@ $^
$(OUT)/lua: $(OUT)/obj/src/lua.o $(OUT)/liblua.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/luac: $(OUT)/obj/src/luac.o $(OUT)/liblua.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
