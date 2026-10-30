include ports/vim/sources.mk
NATIVE_POSIX_SOURCES := libc/posix/fd.c libc/posix/process.c libc/posix/process_native.c \
    libc/posix/directory.c libc/posix/filesystem.c libc/posix/socket.c libc/posix/signals.c \
    libc/posix/main.c libc/posix/system_native.c libc/posix/uio.c libc/posix/path_native.c \
    libc/posix/accounts.c libc/posix/words.c vendor/posix-glob/glob.c \
    ports/standalone/file_mode.c ports/standalone/stream_pipe.c ports/standalone/limits.c
VIM_NATIVE_OBJECTS := $(addprefix $(OUT)/obj/vendor/vim/src/,$(addsuffix .o,$(VIM_SOURCES))) \
    $(OUT)/obj/ports/vim/native.c.o $(OUT)/obj/ports/vim/pathdef.c.o
NATIVE_POSIX_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(NATIVE_POSIX_SOURCES)))
$(VIM_NATIVE_OBJECTS): CPPFLAGS += -D__lume__=1 -DHAVE_CONFIG_H -DALL_BUILTIN_TCAPS -DLUME_NATIVE_TERMINAL=1 -Iports/vim -Iports/vim/auto -Ivendor/vim/src -Ivendor/vim/src/proto
$(VIM_NATIVE_OBJECTS): ports/vim/auto/config.h mk/vim-native.mk
$(OUT)/obj/vendor/vim/src/main.c.o: CPPFLAGS += -Dmain=vim_main
$(NATIVE_POSIX_OBJECTS): CPPFLAGS += -Iports/standalone -Ivendor/posix-glob
$(NATIVE_POSIX_OBJECTS) $(OUT)/obj/tests/integration/native_terminal.c.o: CPPFLAGS += -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -D_POSIX_MONOTONIC_CLOCK=200809L
$(VIM_NATIVE_OBJECTS) $(NATIVE_POSIX_OBJECTS): | headers
$(OUT)/vim.elf: $(VIM_NATIVE_OBJECTS) $(NATIVE_POSIX_OBJECTS) $(ARM_COMMON_OBJECTS)
	$(LD) $(ARM_LD_FLAGS) --wrap=setlocale $^ $(ARM_LD_LIBS) -o "$@"
$(OUT)/vi.elf: $(OUT)/vim.elf
	cp "$<" "$@"
$(OUT)/native-terminal.elf: $(OUT)/obj/tests/integration/native_terminal.c.o $(NATIVE_POSIX_OBJECTS) $(ARM_COMMON_OBJECTS)
	$(LD) $(ARM_LD_FLAGS) $^ $(ARM_LD_LIBS) -o "$@"
$(OUT)/obj/tests/integration/native_terminal.c.o: | headers
-include $(addsuffix .d,$(VIM_NATIVE_OBJECTS) $(NATIVE_POSIX_OBJECTS))
