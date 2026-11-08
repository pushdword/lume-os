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
$(OUT)/tests/file-lock$(HOSTEXE): tests/unit/file_lock_test.c tests/fixtures/posix_lock/fcntl.h tests/fixtures/posix_lock/sys/file.h libc/posix/fd.c libc/posix/file_lock.c $(KERNEL)/include/abi/native/lume/file_lock.h $(KERNEL)/include/abi/native/lume/file_io.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -UNDEBUG -DLUME_USER_HOST_TEST=1 -Itests/fixtures/posix_lock -Iinclude/posix -Iinclude/lume -I"$(KERNEL)/include/uapi" -I"$(KERNEL)/include/abi/native" $(filter %file_lock_test.c %/posix/file_lock.c,$^) -o "$@"
.PHONY: check-posix-madvise
check: check-posix-madvise
check-posix-madvise: $(OUT)/tests/posix-madvise$(HOSTEXE)
	env "$<"
$(OUT)/tests/posix-madvise$(HOSTEXE): tests/unit/posix_madvise_test.c tests/fixtures/mman_host/lume/syscall_native.h tests/fixtures/mman_host/fcntl.h libc/posix/mman.c include/posix/errno_map.h $(KERNEL)/tests/unit/vm_test.c $(KERNEL)/src/mm/vm.c $(KERNEL)/include/kernel/lume/vm.h $(KERNEL)/include/abi/native/lume/memory.h $(wildcard $(KERNEL)/tests/fixtures/vm_host/lume/*.h)
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -UNDEBUG -DLUME_ARCH_AARCH64=1 -Itests/fixtures/mman_host -Iinclude/lume -I"$(KERNEL)/tests/fixtures/vm_host" -I"$(KERNEL)/tests/unit" -I"$(KERNEL)/include/kernel" -I"$(KERNEL)/include/uapi" -I"$(KERNEL)/include/abi/native" -Iinclude/posix -Ilibc/posix tests/unit/posix_madvise_test.c -o "$@"
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
$(OUT)/tests/sigwait$(HOSTEXE): tests/unit/sigwait_test.c libc/posix/sigwait.c include/posix/signal.h $(KERNEL)/include/abi/native/lume/signal.h $(KERNEL)/include/abi/native/lume/process.h $(KERNEL)/include/uapi/lume/spawn.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -UNDEBUG -DLUME_USER_HOST_TEST=1 -Iinclude/posix -Ilibc/posix -I"$(KERNEL)/include/abi/native" -I"$(KERNEL)/include/uapi" $(filter %.c,$^) -o "$@"

$(OUT)/native-atfork.elf: $(OUT)/obj/tests/integration/native_atfork.c.o $(NATIVE_POSIX_OBJECTS) $(ARM_COMMON_OBJECTS)
	$(LD) $(ARM_LD_FLAGS) $^ $(ARM_LD_LIBS) -o "$@"
$(OUT)/obj/tests/integration/native_atfork.c.o: CPPFLAGS += -D_POSIX_C_SOURCE=200809L
$(OUT)/obj/tests/integration/native_atfork.c.o: | headers
-include $(OUT)/obj/tests/integration/native_atfork.c.o.d

ifeq ($(OS),Windows_NT)
.PHONY: check-pthread-tls
check: check-pthread-tls
check-pthread-tls: $(OUT)/tests/pthread-tls$(HOSTEXE) $(OUT)/tests/pthread-cleanup-host$(HOSTEXE)
	env "$(OUT)/tests/pthread-tls$(HOSTEXE)"
	env "$(OUT)/tests/pthread-cleanup-host$(HOSTEXE)"
$(OUT)/tests/pthread-tls$(HOSTEXE) $(OUT)/tests/pthread-cleanup-host$(HOSTEXE): include/posix/pthread.h $(KERNEL)/include/uapi/lume/thread.h libc/posix/absolute_time.h
$(OUT)/tests/pthread-tls$(HOSTEXE) $(OUT)/tests/pthread-cleanup-host$(HOSTEXE): HOSTCFLAGS += -DLUME_ARCH_AMD64=1 -Iinclude/posix -I"$(KERNEL)/include/uapi" -I"$(KERNEL)/include/abi/native"
$(OUT)/tests/pthread-tls$(HOSTEXE): tests/unit/pthread_tls_test.c tests/unit/pthread_cleanup_test.c tests/integration/pthread_cleanup_probe.c libc/posix/pthread.c tests/fixtures/pthread_host_namespace.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -UNDEBUG "$<" -o "$@"
$(OUT)/tests/pthread-cleanup-host$(HOSTEXE): tests/unit/pthread_cleanup_test.c tests/integration/pthread_cleanup_probe.c libc/posix/pthread.c tests/fixtures/pthread_host_namespace.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -UNDEBUG "$<" -o "$@"

.PHONY: check-atfork
check: check-atfork
check-atfork: $(OUT)/tests/atfork$(HOSTEXE)
	env "$<"
$(OUT)/tests/atfork$(HOSTEXE): tests/unit/atfork_test.c libc/posix/atfork.c libc/posix/process_fork.h include/internal/lume/atfork.h tests/fixtures/pthread_host_namespace.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -UNDEBUG -Iinclude/posix -I"$(KERNEL)/include/uapi" -I"$(KERNEL)/include/abi/native" "$<" -o "$@"

.PHONY: check-pthread-fork
check: check-pthread-fork
check-pthread-fork: $(OUT)/tests/pthread-fork$(HOSTEXE)
	env "$<"
$(OUT)/tests/pthread-fork$(HOSTEXE): tests/unit/pthread_fork_test.c libc/posix/pthread.c include/posix/pthread.h tests/fixtures/pthread_host_namespace.h $(KERNEL)/include/uapi/lume/thread.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -UNDEBUG -Iinclude/posix -Ilibc/posix -I"$(KERNEL)/include/uapi" -I"$(KERNEL)/include/abi/native" -include tests/fixtures/pthread_host_namespace.h "$<" -o "$@"
endif
.PHONY: check-socket-message
check: check-socket-message
check-socket-message: $(OUT)/tests/socket-message$(HOSTEXE)
	env "$<"
$(OUT)/tests/socket-message$(HOSTEXE): tests/unit/socket_message_test.c libc/posix/socket_message.h include/posix/sys/socket.h include/posix/sys/uio.h include/posix/netinet/in.h $(KERNEL)/include/abi/native/lume/abi.h $(KERNEL)/include/abi/native/lume/socket.h $(KERNEL)/include/uapi/fd.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -UNDEBUG -I"$(KERNEL)/include/abi/native" -I"$(KERNEL)/include/uapi" -Iinclude/posix "$<" -o "$@"
