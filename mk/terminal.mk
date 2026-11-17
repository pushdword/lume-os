# SPDX-License-Identifier: BSD-3-Clause
ifneq ($(filter aarch64 amd64 i686,$(ARCH)),)
VTERM_SOURCES := $(wildcard vendor/libvterm/src/*.c)
VTERM_OBJECTS := $(addprefix $(XFCE_OUT)/obj/vterm/,$(VTERM_SOURCES:=.o))
$(XFCE_OUT)/obj/vterm/%.c.o: %.c
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -Ivendor/libvterm/include -MMD -MP -MF "$@.d" -c "$<" -o "$@"
$(XFCE_OUT)/libvterm.a: $(VTERM_OBJECTS)
	mkdir -p "$(@D)"
	$(XFCE_AR) rcs "$@" $(VTERM_OBJECTS)
$(XFCE_OUT)/obj/lume-terminal.o: apps/desktop/terminal.c
	mkdir -p "$(@D)"
	$(CC) $(GTK_DIALOG_CFLAGS) $(GTK_DIALOG_INCLUDES) -Ivendor/libvterm/include -c "$<" -o "$@"
$(XFCE_OUT)/lume-terminal.elf: $(XFCE_OUT)/obj/lume-terminal.o $(XFCE_OUT)/libvterm.a $(GTK_DIALOG_LIBRARIES) $(SYSROOT)/lib/crt0-cxx.o arch/$(ARCH)/user.ld mk/terminal.mk
	$(file >$@.rsp,--gc-sections --strip-debug --build-id=none -z max-page-size=4096 -T arch/$(ARCH)/user.ld "$(SYSROOT)/lib/crt0-cxx.o" "$<" --start-group "$(XFCE_OUT)/libvterm.a" $(foreach library,$(GTK_DIALOG_LIBRARIES),"$(library)") --end-group -o "$@")
	$(LD) "@$@.rsp"
.PHONY: terminal
terminal: $(XFCE_OUT)/lume-terminal.elf
$(XFCE_OUT)/obj/desktop-runtime.o: tests/integration/desktop_runtime.c
	mkdir -p "$(@D)"
	$(CC) $(GTK_DIALOG_CFLAGS) $(GTK_DIALOG_INCLUDES) -c "$<" -o "$@"
$(XFCE_OUT)/desktop-runtime.elf: $(XFCE_OUT)/obj/desktop-runtime.o $(GTK_DIALOG_LIBRARIES) $(SYSROOT)/lib/crt0-cxx.o arch/$(ARCH)/user.ld mk/terminal.mk
	$(file >$@.rsp,--gc-sections --strip-debug --build-id=none -z max-page-size=4096 -T arch/$(ARCH)/user.ld "$(SYSROOT)/lib/crt0-cxx.o" "$<" --start-group $(foreach library,$(GTK_DIALOG_LIBRARIES),"$(library)") --end-group -o "$@")
	$(LD) "@$@.rsp"
ifeq ($(XFCE_WAYLAND),1)
xfce-wayland: terminal
endif
-include $(VTERM_OBJECTS:=.d)
endif
