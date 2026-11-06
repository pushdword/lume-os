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
	$(file >$@.rsp,$(ARM_LD_FLAGS) --wrap=setlocale $^ $(ARM_LD_LIBS))
	$(LD) @"$@.rsp" -o "$@"
$(OUT)/vi.elf: $(OUT)/vim.elf
	cp "$<" "$@"
$(OUT)/native-terminal.elf: $(OUT)/obj/tests/integration/native_terminal.c.o $(NATIVE_POSIX_OBJECTS) $(ARM_COMMON_OBJECTS)
	$(LD) $(ARM_LD_FLAGS) $^ $(ARM_LD_LIBS) -o "$@"
$(OUT)/obj/tests/integration/native_terminal.c.o: | headers
$(OUT)/native-file-io.elf: $(OUT)/obj/tests/integration/native_file_io.c.o $(NATIVE_POSIX_OBJECTS) $(ARM_COMMON_OBJECTS)
	$(LD) $(ARM_LD_FLAGS) $^ $(ARM_LD_LIBS) -o "$@"
$(OUT)/obj/tests/integration/native_file_io.c.o: CPPFLAGS += -D_POSIX_C_SOURCE=200809L
$(OUT)/obj/tests/integration/native_file_io.c.o: | headers
$(OUT)/native-file-lock.elf: $(OUT)/obj/tests/integration/native_file_lock.c.o $(NATIVE_POSIX_OBJECTS) $(ARM_COMMON_OBJECTS)
	$(LD) $(ARM_LD_FLAGS) $^ $(ARM_LD_LIBS) -o "$@"
$(OUT)/obj/tests/integration/native_file_lock.c.o: CPPFLAGS += -D_POSIX_C_SOURCE=200809L
$(OUT)/obj/tests/integration/native_file_lock.c.o: | headers
.PHONY: check-file-locks
check: check-file-locks
check-file-locks: $(OUT)/tests/file-lock$(HOSTEXE)
	env "$<"
$(OUT)/tests/file-lock$(HOSTEXE): tests/unit/file_lock_test.c tests/fixtures/posix_lock/fcntl.h libc/posix/fd.c libc/posix/file_lock.c $(KERNEL)/include/abi/native/lume/file_lock.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -UNDEBUG -DLUME_USER_HOST_TEST=1 -Itests/fixtures/posix_lock -Iinclude/posix -Iinclude/lume -I"$(KERNEL)/include/uapi" -I"$(KERNEL)/include/abi/native" $(filter %file_lock_test.c %/posix/file_lock.c,$^) -o "$@"
.PHONY: check-positioned-io
check: check-positioned-io
check-positioned-io: $(OUT)/tests/positioned-io$(HOSTEXE)
	env "$<"
$(OUT)/tests/positioned-io$(HOSTEXE): tests/unit/positioned_io_test.c libc/posix/positioned_io.c $(KERNEL)/include/abi/native/lume/file_io.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -UNDEBUG -I"$(KERNEL)/include/abi/native" -iquote include/posix "$<" -o "$@"
-include $(addsuffix .d,$(VIM_NATIVE_OBJECTS) $(NATIVE_POSIX_OBJECTS))

ifeq ($(ARCH),aarch64)
$(OUT)/native-ucontext.elf: $(OUT)/obj/tests/integration/native_ucontext.c.o $(NATIVE_POSIX_OBJECTS) $(ARM_COMMON_OBJECTS)
	$(LD) $(ARM_LD_FLAGS) $^ $(ARM_LD_LIBS) -o "$@"
$(OUT)/obj/tests/integration/native_ucontext.c.o: CPPFLAGS += -D_POSIX_C_SOURCE=200809L
$(OUT)/obj/tests/integration/native_ucontext.c.o: | headers
endif

$(OUT)/native-sigwait.elf: $(OUT)/obj/tests/integration/native_sigwait.c.o $(NATIVE_POSIX_OBJECTS) $(ARM_COMMON_OBJECTS)
	$(LD) $(ARM_LD_FLAGS) $^ $(ARM_LD_LIBS) -o "$@"
$(OUT)/obj/tests/integration/native_sigwait.c.o: CPPFLAGS += -D_POSIX_C_SOURCE=200809L
$(OUT)/obj/tests/integration/native_sigwait.c.o: | headers
.PHONY: check-sigwait
check: check-sigwait
check-sigwait: $(OUT)/tests/sigwait$(HOSTEXE)
	env "$<"
$(OUT)/tests/sigwait$(HOSTEXE): tests/unit/sigwait_test.c libc/posix/sigwait.c include/posix/signal.h $(KERNEL)/include/abi/native/lume/signal.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -UNDEBUG -DLUME_USER_HOST_TEST=1 -Iinclude/posix -Ilibc/posix -I"$(KERNEL)/include/abi/native" $(filter %.c,$^) -o "$@"

$(OUT)/native-atfork.elf: $(OUT)/obj/tests/integration/native_atfork.c.o $(NATIVE_POSIX_OBJECTS) $(ARM_COMMON_OBJECTS)
	$(LD) $(ARM_LD_FLAGS) $^ $(ARM_LD_LIBS) -o "$@"
$(OUT)/obj/tests/integration/native_atfork.c.o: CPPFLAGS += -D_POSIX_C_SOURCE=200809L
$(OUT)/obj/tests/integration/native_atfork.c.o: | headers
-include $(OUT)/obj/tests/integration/native_atfork.c.o.d
