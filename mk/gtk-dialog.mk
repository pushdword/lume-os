GTK_DIALOG_OUT ?= $(OUT)/gtk-dialog
GTK_DIALOG_SHELL_QUOTE = '$(subst ','"'"',$(1))'
.PHONY: gtk-file-dialog

ifneq ($(filter aarch64 amd64,$(ARCH)),)
GTK_DIALOG_BASE_NAMES := libresolver.a libintl.a libpcre2-8.a libffi.a libcharset.a libiconv.a \
    libglib-charset.a libglib-2.0.a libgobject-2.0.a libgthread-2.0.a \
    libgmodule-2.0.a libgio-xdgmime.a libgio-2.0.a
GTK_DIALOG_XFCE_NAMES := libgtk-3.a libgdk-3.a libgdk-x11.a libcairo-gobject.a \
    libcairo.a libpango-1.0.a libpangocairo-1.0.a libpangoft2-1.0.a \
    libharfbuzz.a libharfbuzz-gobject.a libgdk_pixbuf-2.0.a libpixops.a \
    libatk-1.0.a libstaticpixbufloader-png.a libstaticpixbufloader-gif.a \
    libstaticpixbufloader-xpm.a libfribidi.a libXi.a libXinerama.a libXrandr.a \
    libepoxy.a libdbus-1.a libatk-bridge-2.0.a libatk-bridge-adaptors.a \
    libatspi.a libdroute.a libstaticimmodule-am-et.a libstaticimmodule-cedilla.a \
    libstaticimmodule-cyrillic-translit.a libstaticimmodule-inuktitut.a \
    libstaticimmodule-ipa.a libstaticimmodule-multipress.a libstaticimmodule-thai.a \
    libstaticimmodule-ti-er.a libstaticimmodule-ti-et.a libstaticimmodule-viqr.a \
    libstaticimmodule-xim.a
GTK_DIALOG_SDK_NAMES := liblume.a libc++.a libfontconfig.a libexpat.a freetype.a \
    libpng.a zlib.a pixman.a libXrender.a libXext.a libX11.a libxcb.a libXau.a \
    libXdmcp.a libXfixes.a libXdamage.a libXcomposite.a libXcursor.a \
    libcompiler-rt.a libc.a
GTK_DIALOG_LIBRARIES ?= $(addprefix $(SYSROOT)/lib/,$(GTK_DIALOG_SDK_NAMES)) \
    $(addprefix $(GTK_BASE_OUT)/,$(GTK_DIALOG_BASE_NAMES)) \
    $(addprefix $(XFCE_OUT)/,$(GTK_DIALOG_XFCE_NAMES))
GTK_DIALOG_INCLUDES := -Iinclude -Iinclude/posix -Iinclude/lume \
    -I$(KERNEL)/include/abi/native -Iports/gtk/aarch64 -Ivendor/gtk \
    -Iports/glib/aarch64 -Iports/glib/aarch64/glib -Ivendor/glib \
    -Ivendor/glib/glib -Ivendor/glib/gobject -Ivendor/glib/gio -Ivendor/glib/gmodule \
    -Iports/pango/aarch64 -Ivendor/pango -Ivendor/harfbuzz/src \
    -Iports/harfbuzz/aarch64/src -Ivendor/fribidi/lib -Iports/fribidi/aarch64/lib \
    -Iports/cairo/aarch64/src -Ivendor/cairo/src -Ivendor/cairo/util/cairo-gobject \
    -Iports/gdk-pixbuf/aarch64 -Ivendor/gdk-pixbuf -Iports/at-spi2-core/aarch64 \
    -Ivendor/at-spi2-core -Ivendor/freetype/include -Iports/x11/include \
    -Ivendor/pixman/pixman -Iports/fontconfig/aarch64 -Ivendor/fontconfig
GTK_DIALOG_CFLAGS := -O2 -std=gnu11 --sysroot="$(SYSROOT)" -mcpu=cortex-a76 \
    -mstrict-align -fno-stack-protector -fno-pic -fno-pie -D__lume__=1 \
    -DLUME_ARCH_AARCH64=1 -DLUME_NATIVE_ABI=1 -D_DEFAULT_SOURCE \
    -D_POSIX_C_SOURCE=200809L -ffunction-sections -fdata-sections \
    -ffile-prefix-map=$(CURDIR)=lume-os -isystem "$(SYSROOT)/include"

GTK_DIALOG_LICENSE_FILES := vendor/gtk/COPYING vendor/glib/LICENSES/LGPL-2.1-only.txt \
    vendor/glib/LICENSES/LGPL-2.1-or-later.txt vendor/glib/LICENSES/MIT.txt \
    vendor/cairo/COPYING vendor/cairo/COPYING-LGPL-2.1 vendor/cairo/COPYING-MPL-1.1 \
    vendor/pango/COPYING vendor/harfbuzz/COPYING vendor/fribidi/COPYING \
    vendor/gdk-pixbuf/COPYING vendor/at-spi2-core/COPYING vendor/dbus/COPYING \
    vendor/libXi/COPYING vendor/libXinerama/COPYING vendor/libXrandr/COPYING \
    vendor/libepoxy/COPYING vendor/pcre2/COPYING vendor/libffi/LICENSE \
    vendor/libiconv/COPYING.LIB vendor/proxy-libintl/COPYING vendor/musl-resolver/COPYRIGHT \
    $(SYSROOT)/licenses/THIRD_PARTY

gtk-file-dialog: $(GTK_DIALOG_OUT)/lume-file-dialog.elf $(GTK_DIALOG_OUT)/LICENSES.lume-file-dialog

$(GTK_DIALOG_OUT)/LICENSES.lume-file-dialog: $(GTK_DIALOG_LICENSE_FILES) mk/gtk-dialog.mk
	mkdir -p "$(@D)"
	@for license in $(foreach path,$(GTK_DIALOG_LICENSE_FILES),"$(path)"); do cat "$$license" || exit 1; printf '\n'; done > "$@"

$(GTK_DIALOG_OUT)/gtk-file-dialog.o: apps/gtk-file-dialog.c include/lume/file_dialog.h mk/gtk-dialog.mk
	mkdir -p "$(@D)"
	$(CC) $(GTK_DIALOG_CFLAGS) $(GTK_DIALOG_INCLUDES) -Wall -Wextra -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(GTK_DIALOG_OUT)/lume-file-dialog.elf: $(GTK_DIALOG_OUT)/gtk-file-dialog.o $(GTK_DIALOG_LIBRARIES) $(SYSROOT)/lib/crt0-cxx.o arch/aarch64/user.ld mk/gtk-dialog.mk
	mkdir -p "$(@D)"
	@printf '%s\n' $(call GTK_DIALOG_SHELL_QUOTE,--gc-sections --strip-debug --build-id=none -z max-page-size=4096 -T arch/aarch64/user.ld "$(SYSROOT)/lib/crt0-cxx.o" "$(GTK_DIALOG_OUT)/gtk-file-dialog.o" --start-group $(foreach library,$(GTK_DIALOG_LIBRARIES),"$(library)") --end-group -o "$@") > "$@.rsp"
	$(LD) "@$@.rsp"

-include $(GTK_DIALOG_OUT)/gtk-file-dialog.o.d
else
gtk-file-dialog:
	@echo 'The file dialog requires ARCH=aarch64'
	@exit 1
endif
