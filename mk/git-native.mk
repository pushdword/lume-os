include ports/git/sources.mk

GIT_NATIVE_FLAGS := -Ivendor/git -Ivendor/git/compat/regex -Ivendor/zlib \
    -Ivendor/curl/include -DCURL_STATICLIB -D_POSIX_C_SOURCE=200809L \
    -include ports/git/config.h
GIT_LIBRARY_OBJECTS := $(addprefix $(OUT)/obj/git/,$(GIT_LIBRARY_SOURCES:=.o))
GIT_BUILTIN_OBJECTS := $(addprefix $(OUT)/obj/git/,$(GIT_BUILTIN_SOURCES:=.o))
GIT_ENTRY := $(OUT)/obj/git/common-main.c.o
GIT_MAIN := $(OUT)/obj/git/git.c.o $(GIT_BUILTIN_OBJECTS)
GIT_HTTP := $(addprefix $(OUT)/obj/git/,$(addsuffix .c.o,remote-curl http http-walker))
GIT_OBJECTS := $(GIT_LIBRARY_OBJECTS) $(GIT_ENTRY) $(GIT_MAIN) $(GIT_HTTP)

GIT_ZLIB ?= $(OUT)/libz.a

$(GIT_OBJECTS): ports/git/config.h ports/git/sources.mk mk/git-native.mk | headers
$(OUT)/obj/git/compat/regex/regex.c.o: private CPPFLAGS += -DGAWK
$(OUT)/obj/git/compat/terminal.c.o: private CPPFLAGS += -DHAVE_DEV_TTY=1
$(OUT)/obj/git/%.c.o: vendor/git/%.c $(OUT)/configuration
	mkdir -p "$(@D)"
	$(CC) $(GIT_NATIVE_FLAGS) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"

$(OUT)/libgit.a: $(GIT_LIBRARY_OBJECTS)
	$(file >$@.rsp,$(foreach object,$(GIT_LIBRARY_OBJECTS),"$(object)"))
	rm -f "$@.tmp"
	"$(TOOLBIN)llvm-ar$(HOSTEXE)" rcsD "$@.tmp" @"$@.rsp"
	mv -f "$@.tmp" "$@"

GIT_CRT := $(NATIVE_SDK_BUILD)/crt0.o
GIT_RUNTIME := $(NATIVE_SDK_BUILD)/liblume.a
GIT_ARCHIVES := $(OUT)/libgit.a $(GIT_ZLIB)
GIT_HTTP_ARCHIVES := $(OUT)/libcurl.a $(OUT)/libmbedtls.a
$(OUT)/git.elf: $(GIT_ENTRY) $(GIT_MAIN) $(GIT_CRT) $(GIT_RUNTIME) $(GIT_ARCHIVES)
	$(file >$@.rsp,$(ARM_LD_FLAGS) $(GIT_ENTRY) $(GIT_MAIN) $(GIT_CRT) --start-group $(GIT_ARCHIVES) $(GIT_RUNTIME) "$(SYSROOT)/lib/libc.a" "$(SYSROOT)/lib/libcompiler-rt.a" --end-group)
	$(LD) @"$@.rsp" -o "$@"

$(OUT)/git-remote-http.elf: $(GIT_ENTRY) $(GIT_HTTP) $(GIT_CRT) $(GIT_RUNTIME) $(GIT_ARCHIVES) $(GIT_HTTP_ARCHIVES)
	$(file >$@.rsp,$(ARM_LD_FLAGS) $(GIT_ENTRY) $(GIT_HTTP) $(GIT_CRT) --start-group $(GIT_ARCHIVES) $(GIT_HTTP_ARCHIVES) $(GIT_RUNTIME) "$(SYSROOT)/lib/libc.a" "$(SYSROOT)/lib/libcompiler-rt.a" --end-group)
	$(LD) @"$@.rsp" -o "$@"

-include $(GIT_OBJECTS:=.d)
