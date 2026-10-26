.DEFAULT_GOAL := all
CSTD := gnu11
PORT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SOURCE = $(OS_ROOT)/vendor/sqlite
include $(PORT)/../standalone.mk
CPPFLAGS += -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_OMIT_WAL '-DSQLITE_DEFAULT_UNIX_VFS="unix-dotfile"' -DSQLITE_MAX_MMAP_SIZE=0 -DSQLITE_DEFAULT_MEMSTATUS=0 -DSQLITE_TEMP_STORE=3 -DSQLITE_OS_UNIX=1 -DSQLITE_OMIT_READLINE -DSQLITE_OMIT_EDITLINE -DHAVE_USLEEP=1 -DSQLITE_OMIT_POPEN -DSQLITE_NOHAVE_SYSTEM '-I$(KERNEL)/include/abi/native' '-I$(OS_ROOT)/ports/standalone/include' '-I$(OS_ROOT)/vendor/sqlite'
SOURCES := sqlite3.c shared/realpath.c shared/optional.c shared/file_mode.c shared/sleep.c
OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(basename $(SOURCES))))
.PHONY: all
all: $(OUT)/libsqlite3.a $(OUT)/sqlite3
$(OUT)/libsqlite3.a: $(OBJECTS)
	rm -f $@
	$(AR) rcs $@ $^
$(OUT)/sqlite3: $(OUT)/obj/shell.o $(OUT)/libsqlite3.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
