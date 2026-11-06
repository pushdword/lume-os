# SPDX-License-Identifier: BSD-3-Clause
X11_NATIVE_OUT ?= $(OUT)/x11-native
X11_NATIVE_SYSROOT ?= $(SYSROOT)
X11_NATIVE_CC ?= $(CC)
X11_NATIVE_AR ?= "$(TOOLBIN)llvm-ar$(HOSTEXE)"
X11_NATIVE_CPPFLAGS := $(ARCH_CPPFLAGS) -D__lume__=1 -DLUME_NATIVE_ABI=1 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -D_POSIX_TIMERS=200809L -D_POSIX_MONOTONIC_CLOCK=200809L -I"$(X11_NATIVE_OUT)/include" -Iinclude/posix -Iinclude/lume -Iinclude/internal -Iarch/$(ARCH)/include -I"$(KERNEL)/include/abi/native" -I"$(KERNEL)/include/uapi" -Iports/x11/include -Ivendor/libXrandr/include -Ivendor/libXrandr/src -Ivendor/libXrandr/include/X11/extensions -Iports/x11/include/X11/extensions
X11_NATIVE_CFLAGS = $(filter-out --target=%,$(ARCH_CFLAGS)) --sysroot="$(X11_NATIVE_SYSROOT)" -O2 -ffunction-sections -fdata-sections -fno-stack-protector -fno-pic -fno-pie -ffile-prefix-map=$(CURDIR)=lume-os
X11_NATIVE_XRANDR_SOURCES := $(addprefix vendor/libXrandr/src/,Xrandr.c XrrConfig.c XrrCrtc.c XrrMode.c XrrMonitor.c XrrOutput.c XrrProperty.c XrrProvider.c XrrProviderProperty.c XrrScreen.c)
X11_NATIVE_XTST_SOURCES := vendor/libXtst/src/XTest.c vendor/libXtst/src/XRecord.c
X11_NATIVE_XRANDR_OBJECTS := $(addprefix $(X11_NATIVE_OUT)/obj/,$(addsuffix .o,$(X11_NATIVE_XRANDR_SOURCES)))
X11_NATIVE_XTST_OBJECTS := $(addprefix $(X11_NATIVE_OUT)/obj/,$(addsuffix .o,$(X11_NATIVE_XTST_SOURCES)))
X11_NATIVE_OBJECTS := $(X11_NATIVE_XRANDR_OBJECTS) $(X11_NATIVE_XTST_OBJECTS)
X11_NATIVE_HEADERS := $(addprefix $(X11_NATIVE_OUT)/include/X11/extensions/,XTest.h record.h XInput.h)
X11_NATIVE_LIBRARIES := $(X11_NATIVE_OUT)/libXrandr.a $(X11_NATIVE_OUT)/libXtst.a

.PHONY: x11-native x11-native-install x11-native-config-force
x11-native: $(X11_NATIVE_LIBRARIES) $(X11_NATIVE_HEADERS) $(X11_NATIVE_OUT)/LICENSES.x11-native

x11_native_quote = '$(subst ','"'"',$(1))'

x11-native-config-force:

$(X11_NATIVE_OUT)/configuration: x11-native-config-force
	@set -eu; \
	x11_stamp=$(call x11_native_quote,$@); \
	x11_header=$(call x11_native_quote,$(X11_NATIVE_SYSROOT)/include/picolibc.h); \
	test -n $(call x11_native_quote,$(X11_NATIVE_SYSROOT)) && test -f "$$x11_header" || { echo 'X11_NATIVE_SYSROOT must contain Picolibc headers'; exit 1; }; \
	mkdir -p $(call x11_native_quote,$(@D)); \
	x11_temporary="$$x11_stamp.tmp.$$$$"; \
	trap 'rm -f -- "$$x11_temporary"' 0; \
	trap 'exit 1' 1 2 15; \
	x11_header_sum=$$(sha256sum < "$$x11_header"); \
	printf '%s\000' 'LUME_X11_NATIVE_CONFIG_1' \
	    $(call x11_native_quote,CC=$(X11_NATIVE_CC)) \
	    $(call x11_native_quote,AR=$(X11_NATIVE_AR)) \
	    $(call x11_native_quote,SYSROOT=$(X11_NATIVE_SYSROOT)) \
	    $(call x11_native_quote,CPPFLAGS=$(X11_NATIVE_CPPFLAGS)) \
	    $(call x11_native_quote,CFLAGS=$(X11_NATIVE_CFLAGS)) \
	    "PICOLIBC_SHA256=$${x11_header_sum%% *}" > "$$x11_temporary"; \
	if ! cmp -s "$$x11_temporary" "$$x11_stamp"; then \
	    mv -f "$$x11_temporary" "$$x11_stamp"; \
	fi

$(X11_NATIVE_OUT)/obj/%.c.o: %.c mk/x11-native.mk $(X11_NATIVE_OUT)/configuration | $(X11_NATIVE_HEADERS)
	mkdir -p "$(@D)"
	$(X11_NATIVE_CC) $(X11_NATIVE_CPPFLAGS) $(X11_NATIVE_CFLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"

$(X11_NATIVE_OUT)/libXrandr.a: $(X11_NATIVE_XRANDR_OBJECTS)
	mkdir -p "$(@D)"
	rm -f "$@.tmp"
	$(X11_NATIVE_AR) rcs "$@.tmp" $^
	mv "$@.tmp" "$@"

$(X11_NATIVE_OUT)/libXtst.a: $(X11_NATIVE_XTST_OBJECTS)
	mkdir -p "$(@D)"
	rm -f "$@.tmp"
	$(X11_NATIVE_AR) rcs "$@.tmp" $^
	mv "$@.tmp" "$@"

$(X11_NATIVE_OUT)/include/X11/extensions/XTest.h: vendor/libXtst/include/X11/extensions/XTest.h
	mkdir -p "$(@D)"
	cp "$<" "$@"

$(X11_NATIVE_OUT)/include/X11/extensions/record.h: vendor/libXtst/include/X11/extensions/record.h
	mkdir -p "$(@D)"
	cp "$<" "$@"

$(X11_NATIVE_OUT)/include/X11/extensions/XInput.h: vendor/libXi/include/X11/extensions/XInput.h
	mkdir -p "$(@D)"
	cp "$<" "$@"

$(X11_NATIVE_OUT)/LICENSES.x11-native: vendor/libXrandr/COPYING vendor/libXtst/COPYING vendor/libXi/COPYING
	mkdir -p "$(@D)"
	cat $^ > "$@"

x11-native-install: x11-native
	@test -n "$(DESTDIR)" || { echo 'DESTDIR must name an SDK staging directory'; exit 1; }
	mkdir -p "$(DESTDIR)/lib" "$(DESTDIR)/include/X11/extensions" "$(DESTDIR)/share/licenses/x11-native"
	cp $(X11_NATIVE_LIBRARIES) "$(DESTDIR)/lib/"
	cp $(X11_NATIVE_HEADERS) "$(DESTDIR)/include/X11/extensions/"
	cp "$(X11_NATIVE_OUT)/LICENSES.x11-native" "$(DESTDIR)/share/licenses/x11-native/LICENSES"

-include $(addsuffix .d,$(X11_NATIVE_OBJECTS))
