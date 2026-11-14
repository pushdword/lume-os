NATIVE_SDK_BUILD ?= $(OUT)/native-sdk
NATIVE_SDK_OUT ?= $(NATIVE_SDK_BUILD)/sdk
NATIVE_SDK_FLAGS := $(filter-out -mno-sse -mno-sse2 -mno-mmx,$(DESKTOP_APP_BASE))
ifeq ($(ARCH),amd64)
NATIVE_SDK_FLAGS += -mcmodel=large -mno-red-zone
endif
ifeq ($(ARCH),i686)
NATIVE_SDK_FLAGS += -DLUME_NATIVE_LFS64=1
endif
NATIVE_SDK_FLAGS += -I$(KERNEL)/include/abi/native -I$(KERNEL)/include/uapi
NATIVE_SDK_OBJECTS := $(addprefix $(NATIVE_SDK_BUILD)/obj/,$(addsuffix .o,$(filter-out %/crt0.S,$(NATIVE_DESKTOP_SOURCES))))
NATIVE_SDK_CRT := $(NATIVE_SDK_BUILD)/crt0.o $(NATIVE_SDK_BUILD)/crt0-cxx.o

.PHONY: native-sdk-config-force
native-sdk-config-force:
$(NATIVE_SDK_BUILD)/configuration: native-sdk-config-force
	mkdir -p "$(@D)"
	@printf '%s\n' '$(ARCH) $(CC) $(NATIVE_SDK_FLAGS) $(DESKTOP_APP_CXX)' > "$@.tmp"
	@cmp -s "$@.tmp" "$@" || cp "$@.tmp" "$@"
	@rm -f "$@.tmp"
$(NATIVE_SDK_OBJECTS) $(NATIVE_SDK_CRT): $(NATIVE_SDK_BUILD)/configuration

$(NATIVE_SDK_BUILD)/obj/%.c.o: %.c mk/native-runtime.mk
	mkdir -p "$(@D)"
	$(CC) $(NATIVE_SDK_FLAGS) -std=gnu11 -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(NATIVE_SDK_BUILD)/obj/%.S.o: %.S mk/native-runtime.mk
	mkdir -p "$(@D)"
	$(CC) $(NATIVE_SDK_FLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(NATIVE_SDK_BUILD)/obj/%.cpp.o: %.cpp mk/native-runtime.mk
	mkdir -p "$(@D)"
	$(CC) $(NATIVE_SDK_FLAGS) $(DESKTOP_APP_CXX) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(addprefix $(NATIVE_SDK_BUILD)/obj/,$(NATIVE_MATH_SOURCES:=.o)): NATIVE_SDK_FLAGS += -Ivendor/picolibc/libm/common -Ivendor/picolibc/libm/ld -Iports/picolibc/platform/libm/ld
$(NATIVE_SDK_BUILD)/obj/vendor/openssh/openbsd-compat/sha1.c.o: NATIVE_SDK_FLAGS += -Iports/xorg-server/sha1
$(NATIVE_SDK_BUILD)/obj/libc/string/transfer.c.o $(NATIVE_SDK_BUILD)/obj/ports/picolibc/native-allocator.c.o: NATIVE_SDK_FLAGS += -fno-builtin
$(NATIVE_SDK_BUILD)/obj/ports/picolibc/native-allocator.c.o: vendor/libffi/src/dlmalloc.c

$(NATIVE_SDK_BUILD)/crt0.o: arch/$(ARCH)/crt0.S
	mkdir -p "$(@D)"
	$(CC) $(NATIVE_SDK_FLAGS) -c "$<" -o "$@"
$(NATIVE_SDK_BUILD)/crt0-cxx.o: arch/$(ARCH)/crt0.S
	mkdir -p "$(@D)"
	$(CC) $(NATIVE_SDK_FLAGS) -DLUME_NATIVE_CXX=1 -c "$<" -o "$@"
$(NATIVE_SDK_BUILD)/liblume.a: $(NATIVE_SDK_OBJECTS) mk/native-runtime.mk
	$(file >$@.rsp,$(foreach obj,$(NATIVE_SDK_OBJECTS),"$(obj)"))
	rm -f "$@.tmp"
	"$(TOOLBIN)llvm-ar$(HOSTEXE)" qcD "$@.tmp" @"$@.rsp"
	"$(TOOLBIN)llvm-ar$(HOSTEXE)" sD "$@.tmp"
	mv -f "$@.tmp" "$@"

.PHONY: native-sdk
native-sdk: $(NATIVE_SDK_BUILD)/liblume.a $(NATIVE_SDK_CRT)
	test "$(abspath $(SYSROOT))" != "$(abspath $(NATIVE_SDK_OUT))"
	mkdir -p "$(NATIVE_SDK_OUT)/lib" "$(NATIVE_SDK_OUT)/include/lume" "$(NATIVE_SDK_OUT)/include/posix"
	cp -pR "$(SYSROOT)/include/." "$(NATIVE_SDK_OUT)/include/"
	cp -pR "$(SYSROOT)/lib/." "$(NATIVE_SDK_OUT)/lib/"
	cp -pR include/lume/. "$(NATIVE_SDK_OUT)/include/"
	cp -pR include/lume/. "$(NATIVE_SDK_OUT)/include/lume/"
	cp -pR include/posix/. "$(NATIVE_SDK_OUT)/include/posix/"
	cp -pR "arch/$(ARCH)/include/." "$(NATIVE_SDK_OUT)/include/"
	cp -pR "$(KERNEL)/include/uapi/." "$(NATIVE_SDK_OUT)/include/"
	cp -pR "$(KERNEL)/include/abi/native/." "$(NATIVE_SDK_OUT)/include/"
	cp -p $(NATIVE_SDK_CRT) "$(NATIVE_SDK_BUILD)/liblume.a" "$(NATIVE_SDK_OUT)/lib/"
	cp -p "arch/$(ARCH)/native.ld" "$(NATIVE_SDK_OUT)/native.ld"
	cp -p "arch/$(ARCH)/chromium.ld" "$(NATIVE_SDK_OUT)/chromium.ld"
	cp -p resources/sdk/lume.mk "$(NATIVE_SDK_OUT)/lume.mk"
	@printf '%s\n' 'LUME_SDK_ARCH := $(ARCH)' 'LUME_SDK_TARGET := $(TARGET)' > "$(NATIVE_SDK_OUT)/target.mk.tmp"
	@cmp -s "$(NATIVE_SDK_OUT)/target.mk.tmp" "$(NATIVE_SDK_OUT)/target.mk" || cp -p "$(NATIVE_SDK_OUT)/target.mk.tmp" "$(NATIVE_SDK_OUT)/target.mk"
	@rm -f "$(NATIVE_SDK_OUT)/target.mk.tmp"
	mkdir -p "$(NATIVE_SDK_OUT)/share/licenses/native-runtime"
	cp -p ports/picolibc/COPYING.picolibc "$(NATIVE_SDK_OUT)/share/licenses/native-runtime/"
	cp -p vendor/libcxxabi/LICENSE "$(NATIVE_SDK_OUT)/share/licenses/native-runtime/LICENSE.libcxxabi"
	cp -p vendor/libffi/LICENSE.dlmalloc "$(NATIVE_SDK_OUT)/share/licenses/native-runtime/"
	cp -p vendor/posix-glob/glob.c "$(NATIVE_SDK_OUT)/share/licenses/native-runtime/glob.c"
	cp -p vendor/openssh/openbsd-compat/sha1.c "$(NATIVE_SDK_OUT)/share/licenses/native-runtime/sha1.c"
	if test ! -d "$(SYSROOT)/share/licenses"; then :; else cp -pR "$(SYSROOT)/share/licenses/." "$(NATIVE_SDK_OUT)/share/licenses/"; fi

-include $(NATIVE_SDK_OBJECTS:=.d)
