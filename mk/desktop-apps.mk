# SPDX-License-Identifier: BSD-3-Clause
DESKTOP_APPS ?= $(if $(filter aarch64 amd64,$(ARCH)),1,0)
DESKTOP_APPS_OUT ?= $(DESKTOP_OUT)/apps
DESKTOP_APP_RUNTIME := $(addprefix $(DESKTOP_OUT)/programs/obj/common/,$(NATIVE_DESKTOP_C_SOURCES:=.o)) \
    $(DESKTOP_OUT)/programs/obj/5f8c9d766f97/arch/$(ARCH)/crt0.S.o \
    $(addprefix $(DESKTOP_OUT)/programs/obj/5764aa8ecaed/,$(NATIVE_CXX_SOURCES:=.o)) \
    $(DESKTOP_OUT)/programs/obj/common/vendor/libcxxabi/src/cxa_aux_runtime.cpp.o
DESKTOP_APP_BASE := $(ARCH_CPPFLAGS) $(filter-out --target=%,$(ARCH_CFLAGS)) --sysroot="$(SYSROOT)" -O2 -ffunction-sections -fdata-sections -fno-pic -fno-pie -fno-stack-protector -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE -D_POSIX_MONOTONIC_CLOCK=200809L -D_POSIX_TIMERS=200809L -D__lume__=1 -Iinclude/posix -Iinclude/lume -Iinclude/internal -Iarch/$(ARCH)/include -I$(KERNEL)/include/abi/native -I$(KERNEL)/include/uapi -Iports/x11/include -Ilibc/posix -ffile-prefix-map=$(CURDIR)=lume-os
DESKTOP_APP_CXX := -std=c++17 -nostdinc++ -fno-exceptions -frtti -U__BITINT_MAXWIDTH__ -DLUME_NATIVE_CXX -Iports/libcxx -Ivendor/libcxx/include -Ivendor/libcxxabi/include
DESKTOP_APP_FLTK := -Iports/fltk -Ivendor/fltk -Ivendor/libpng -Iports/x11/libpng -Ivendor/zlib
DESKTOP_APP_TLS := -Ivendor/mbedtls/include -Ivendor/mbedtls/library -Iports/mbedtls -DMBEDTLS_CONFIG_FILE='"lume_tls_config.h"'
DESKTOP_APP_DILLO := -Iports/dillo -Ivendor/dillo -Ivendor/dillo/src -Ivendor/stb $(DESKTOP_APP_FLTK) $(DESKTOP_APP_TLS) -DHAVE_CONFIG_H
DESKTOP_APP_LIBS := $(DESKTOP_APPS_OUT)/libfltk.a $(DESKTOP_APPS_OUT)/libmbedtls.a $(DESKTOP_OUT)/libraries/lib/libXpm.a $(DESKTOP_OUT)/libraries/lib/libXext.a $(DESKTOP_OUT)/libraries/lib/libXrender.a $(DESKTOP_OUT)/libraries/lib/libXcomposite.a $(DESKTOP_OUT)/libraries/lib/libXdamage.a $(DESKTOP_OUT)/libraries/lib/libXfixes.a $(DESKTOP_OUT)/libraries/lib/libXcursor.a $(DESKTOP_OUT)/libraries/lib/libX11.a $(DESKTOP_OUT)/libraries/lib/libxcb.a $(DESKTOP_OUT)/libraries/lib/libXau.a $(DESKTOP_OUT)/libraries/lib/libXdmcp.a $(DESKTOP_OUT)/libraries/lib/libpng.a $(DESKTOP_OUT)/libraries/lib/zlib.a $(DESKTOP_OUT)/cxx/libc++.a $(SYSROOT)/lib/libc.a
$(DESKTOP_OUT)/programs/obj/common/ports/picolibc/native-allocator.c.o: ports/picolibc/native-allocator.c vendor/libffi/src/dlmalloc.c mk/desktop-apps.mk | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -fno-builtin -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
-include $(DESKTOP_OUT)/programs/obj/common/ports/picolibc/native-allocator.c.o.d

$(DESKTOP_OUT)/programs/obj/common/libc/posix/directory_sort.c.o: libc/posix/directory_sort.c mk/desktop-apps.mk | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"

$(DESKTOP_OUT)/programs/obj/common/libc/posix/pause.c.o: libc/posix/pause.c mk/desktop-apps.mk | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
-include $(DESKTOP_OUT)/programs/obj/common/libc/posix/pause.c.o.d

$(DESKTOP_OUT)/programs/obj/common/vendor/libcxxabi/src/cxa_aux_runtime.cpp.o: vendor/libcxxabi/src/cxa_aux_runtime.cpp mk/desktop-apps.mk | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
-include $(DESKTOP_OUT)/programs/obj/common/vendor/libcxxabi/src/cxa_aux_runtime.cpp.o.d

$(DESKTOP_OUT)/programs/obj/common/vendor/picolibc/libm/ld/s_fpclassifyl.c.o: vendor/picolibc/libm/ld/s_fpclassifyl.c mk/desktop-apps.mk | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -Ivendor/picolibc/libm/common -Ivendor/picolibc/libm/ld -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
-include $(DESKTOP_OUT)/programs/obj/common/vendor/picolibc/libm/ld/s_fpclassifyl.c.o.d

.PHONY: desktop-apps desktop-apps-install desktop-apps-check
desktop-apps: $(DESKTOP_APPS_OUT)/dillo.elf $(DESKTOP_APPS_OUT)/dilloc.elf $(DESKTOP_APPS_OUT)/browser.elf $(DESKTOP_APPS_OUT)/network.elf $(DESKTOP_APPS_OUT)/clock.elf $(DESKTOP_APPS_OUT)/dpid.elf $(DESKTOP_APPS_OUT)/file.dpi.elf $(DESKTOP_APPS_OUT)/bookmarks.dpi.elf $(DESKTOP_APPS_OUT)/cookies.dpi.elf $(DESKTOP_APPS_OUT)/datauri.dpi.elf $(DESKTOP_APPS_OUT)/vsource.dpi.elf
desktop-apps-check: $(DESKTOP_APPS_OUT)/appcheck.elf $(DESKTOP_APPS_OUT)/fltkcheck.elf

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl.cxx.o: vendor/fltk/src/Fl.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Adjuster.cxx.o: vendor/fltk/src/Fl_Adjuster.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Bitmap.cxx.o: vendor/fltk/src/Fl_Bitmap.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Browser.cxx.o: vendor/fltk/src/Fl_Browser.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Browser_.cxx.o: vendor/fltk/src/Fl_Browser_.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Browser_load.cxx.o: vendor/fltk/src/Fl_Browser_load.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Box.cxx.o: vendor/fltk/src/Fl_Box.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Button.cxx.o: vendor/fltk/src/Fl_Button.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Chart.cxx.o: vendor/fltk/src/Fl_Chart.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Check_Browser.cxx.o: vendor/fltk/src/Fl_Check_Browser.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Check_Button.cxx.o: vendor/fltk/src/Fl_Check_Button.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Choice.cxx.o: vendor/fltk/src/Fl_Choice.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Clock.cxx.o: vendor/fltk/src/Fl_Clock.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Color_Chooser.cxx.o: vendor/fltk/src/Fl_Color_Chooser.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Copy_Surface.cxx.o: vendor/fltk/src/Fl_Copy_Surface.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Counter.cxx.o: vendor/fltk/src/Fl_Counter.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Device.cxx.o: vendor/fltk/src/Fl_Device.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Dial.cxx.o: vendor/fltk/src/Fl_Dial.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Help_Dialog_Dox.cxx.o: vendor/fltk/src/Fl_Help_Dialog_Dox.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Double_Window.cxx.o: vendor/fltk/src/Fl_Double_Window.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_File_Browser.cxx.o: vendor/fltk/src/Fl_File_Browser.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_File_Chooser.cxx.o: vendor/fltk/src/Fl_File_Chooser.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_File_Chooser2.cxx.o: vendor/fltk/src/Fl_File_Chooser2.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_File_Icon.cxx.o: vendor/fltk/src/Fl_File_Icon.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_File_Input.cxx.o: vendor/fltk/src/Fl_File_Input.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Group.cxx.o: vendor/fltk/src/Fl_Group.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Help_View.cxx.o: vendor/fltk/src/Fl_Help_View.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Image.cxx.o: vendor/fltk/src/Fl_Image.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Image_Surface.cxx.o: vendor/fltk/src/Fl_Image_Surface.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Input.cxx.o: vendor/fltk/src/Fl_Input.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Input_.cxx.o: vendor/fltk/src/Fl_Input_.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Light_Button.cxx.o: vendor/fltk/src/Fl_Light_Button.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Menu.cxx.o: vendor/fltk/src/Fl_Menu.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Menu_.cxx.o: vendor/fltk/src/Fl_Menu_.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Menu_Bar.cxx.o: vendor/fltk/src/Fl_Menu_Bar.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Menu_Button.cxx.o: vendor/fltk/src/Fl_Menu_Button.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Menu_Window.cxx.o: vendor/fltk/src/Fl_Menu_Window.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Menu_add.cxx.o: vendor/fltk/src/Fl_Menu_add.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Menu_global.cxx.o: vendor/fltk/src/Fl_Menu_global.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Multi_Label.cxx.o: vendor/fltk/src/Fl_Multi_Label.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Native_File_Chooser.cxx.o: vendor/fltk/src/Fl_Native_File_Chooser.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Overlay_Window.cxx.o: vendor/fltk/src/Fl_Overlay_Window.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Pack.cxx.o: vendor/fltk/src/Fl_Pack.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Paged_Device.cxx.o: vendor/fltk/src/Fl_Paged_Device.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Pixmap.cxx.o: vendor/fltk/src/Fl_Pixmap.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Positioner.cxx.o: vendor/fltk/src/Fl_Positioner.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_PostScript.cxx.o: vendor/fltk/src/Fl_PostScript.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Preferences.cxx.o: vendor/fltk/src/Fl_Preferences.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Printer.cxx.o: vendor/fltk/src/Fl_Printer.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Progress.cxx.o: vendor/fltk/src/Fl_Progress.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Repeat_Button.cxx.o: vendor/fltk/src/Fl_Repeat_Button.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Return_Button.cxx.o: vendor/fltk/src/Fl_Return_Button.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Roller.cxx.o: vendor/fltk/src/Fl_Roller.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Round_Button.cxx.o: vendor/fltk/src/Fl_Round_Button.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Scroll.cxx.o: vendor/fltk/src/Fl_Scroll.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Scrollbar.cxx.o: vendor/fltk/src/Fl_Scrollbar.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Shared_Image.cxx.o: vendor/fltk/src/Fl_Shared_Image.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Single_Window.cxx.o: vendor/fltk/src/Fl_Single_Window.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Slider.cxx.o: vendor/fltk/src/Fl_Slider.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Table.cxx.o: vendor/fltk/src/Fl_Table.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Table_Row.cxx.o: vendor/fltk/src/Fl_Table_Row.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Tabs.cxx.o: vendor/fltk/src/Fl_Tabs.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Text_Buffer.cxx.o: vendor/fltk/src/Fl_Text_Buffer.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Text_Display.cxx.o: vendor/fltk/src/Fl_Text_Display.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Text_Editor.cxx.o: vendor/fltk/src/Fl_Text_Editor.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Tile.cxx.o: vendor/fltk/src/Fl_Tile.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Tiled_Image.cxx.o: vendor/fltk/src/Fl_Tiled_Image.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Tooltip.cxx.o: vendor/fltk/src/Fl_Tooltip.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Tree.cxx.o: vendor/fltk/src/Fl_Tree.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Tree_Item_Array.cxx.o: vendor/fltk/src/Fl_Tree_Item_Array.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Tree_Item.cxx.o: vendor/fltk/src/Fl_Tree_Item.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Tree_Prefs.cxx.o: vendor/fltk/src/Fl_Tree_Prefs.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Valuator.cxx.o: vendor/fltk/src/Fl_Valuator.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Value_Input.cxx.o: vendor/fltk/src/Fl_Value_Input.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Value_Output.cxx.o: vendor/fltk/src/Fl_Value_Output.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Value_Slider.cxx.o: vendor/fltk/src/Fl_Value_Slider.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Widget.cxx.o: vendor/fltk/src/Fl_Widget.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Window.cxx.o: vendor/fltk/src/Fl_Window.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Window_fullscreen.cxx.o: vendor/fltk/src/Fl_Window_fullscreen.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Window_hotspot.cxx.o: vendor/fltk/src/Fl_Window_hotspot.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Window_iconize.cxx.o: vendor/fltk/src/Fl_Window_iconize.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Window_shape.cxx.o: vendor/fltk/src/Fl_Window_shape.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Wizard.cxx.o: vendor/fltk/src/Fl_Wizard.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_XBM_Image.cxx.o: vendor/fltk/src/Fl_XBM_Image.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_XPM_Image.cxx.o: vendor/fltk/src/Fl_XPM_Image.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_abort.cxx.o: vendor/fltk/src/Fl_abort.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_add_idle.cxx.o: vendor/fltk/src/Fl_add_idle.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_arg.cxx.o: vendor/fltk/src/Fl_arg.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_compose.cxx.o: vendor/fltk/src/Fl_compose.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_display.cxx.o: vendor/fltk/src/Fl_display.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_get_key.cxx.o: vendor/fltk/src/Fl_get_key.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_get_system_colors.cxx.o: vendor/fltk/src/Fl_get_system_colors.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_grab.cxx.o: vendor/fltk/src/Fl_grab.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_lock.cxx.o: vendor/fltk/src/Fl_lock.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_own_colormap.cxx.o: vendor/fltk/src/Fl_own_colormap.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_visual.cxx.o: vendor/fltk/src/Fl_visual.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_x.cxx.o: vendor/fltk/src/Fl_x.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/filename_absolute.cxx.o: vendor/fltk/src/filename_absolute.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/filename_expand.cxx.o: vendor/fltk/src/filename_expand.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/filename_ext.cxx.o: vendor/fltk/src/filename_ext.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/filename_isdir.cxx.o: vendor/fltk/src/filename_isdir.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/filename_list.cxx.o: vendor/fltk/src/filename_list.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/filename_match.cxx.o: vendor/fltk/src/filename_match.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/filename_setext.cxx.o: vendor/fltk/src/filename_setext.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_arc.cxx.o: vendor/fltk/src/fl_arc.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_arci.cxx.o: vendor/fltk/src/fl_arci.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_ask.cxx.o: vendor/fltk/src/fl_ask.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_boxtype.cxx.o: vendor/fltk/src/fl_boxtype.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_color.cxx.o: vendor/fltk/src/fl_color.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_cursor.cxx.o: vendor/fltk/src/fl_cursor.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_curve.cxx.o: vendor/fltk/src/fl_curve.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_diamond_box.cxx.o: vendor/fltk/src/fl_diamond_box.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_dnd.cxx.o: vendor/fltk/src/fl_dnd.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_draw.cxx.o: vendor/fltk/src/fl_draw.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_draw_image.cxx.o: vendor/fltk/src/fl_draw_image.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_draw_pixmap.cxx.o: vendor/fltk/src/fl_draw_pixmap.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_engraved_label.cxx.o: vendor/fltk/src/fl_engraved_label.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_file_dir.cxx.o: vendor/fltk/src/fl_file_dir.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_font.cxx.o: vendor/fltk/src/fl_font.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_gleam.cxx.o: vendor/fltk/src/fl_gleam.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_gtk.cxx.o: vendor/fltk/src/fl_gtk.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_labeltype.cxx.o: vendor/fltk/src/fl_labeltype.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_line_style.cxx.o: vendor/fltk/src/fl_line_style.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_open_uri.cxx.o: vendor/fltk/src/fl_open_uri.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_oval_box.cxx.o: vendor/fltk/src/fl_oval_box.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_overlay.cxx.o: vendor/fltk/src/fl_overlay.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_overlay_visual.cxx.o: vendor/fltk/src/fl_overlay_visual.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_plastic.cxx.o: vendor/fltk/src/fl_plastic.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_read_image.cxx.o: vendor/fltk/src/fl_read_image.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_rect.cxx.o: vendor/fltk/src/fl_rect.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_round_box.cxx.o: vendor/fltk/src/fl_round_box.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_rounded_box.cxx.o: vendor/fltk/src/fl_rounded_box.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_set_font.cxx.o: vendor/fltk/src/fl_set_font.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_set_fonts.cxx.o: vendor/fltk/src/fl_set_fonts.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_scroll_area.cxx.o: vendor/fltk/src/fl_scroll_area.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_shadow_box.cxx.o: vendor/fltk/src/fl_shadow_box.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_shortcut.cxx.o: vendor/fltk/src/fl_shortcut.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_show_colormap.cxx.o: vendor/fltk/src/fl_show_colormap.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_symbols.cxx.o: vendor/fltk/src/fl_symbols.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_vertex.cxx.o: vendor/fltk/src/fl_vertex.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/ps_image.cxx.o: vendor/fltk/src/ps_image.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/screen_xywh.cxx.o: vendor/fltk/src/screen_xywh.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_utf8.cxx.o: vendor/fltk/src/fl_utf8.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_encoding_latin1.cxx.o: vendor/fltk/src/fl_encoding_latin1.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_encoding_mac_roman.cxx.o: vendor/fltk/src/fl_encoding_mac_roman.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/flstring.c.o: vendor/fltk/src/flstring.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/scandir.c.o: vendor/fltk/src/scandir.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/numericsort.c.o: vendor/fltk/src/numericsort.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/vsnprintf.c.o: vendor/fltk/src/vsnprintf.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/xutf8/is_right2left.c.o: vendor/fltk/src/xutf8/is_right2left.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/xutf8/is_spacing.c.o: vendor/fltk/src/xutf8/is_spacing.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/xutf8/case.c.o: vendor/fltk/src/xutf8/case.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/xutf8/utf8Input.c.o: vendor/fltk/src/xutf8/utf8Input.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/xutf8/utf8Utils.c.o: vendor/fltk/src/xutf8/utf8Utils.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/xutf8/utf8Wrap.c.o: vendor/fltk/src/xutf8/utf8Wrap.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/xutf8/keysym2Ucs.c.o: vendor/fltk/src/xutf8/keysym2Ucs.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_utf.c.o: vendor/fltk/src/fl_utf.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_images_core.cxx.o: vendor/fltk/src/fl_images_core.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_BMP_Image.cxx.o: vendor/fltk/src/Fl_BMP_Image.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_File_Icon2.cxx.o: vendor/fltk/src/Fl_File_Icon2.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_GIF_Image.cxx.o: vendor/fltk/src/Fl_GIF_Image.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Help_Dialog.cxx.o: vendor/fltk/src/Fl_Help_Dialog.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_JPEG_Image.cxx.o: vendor/fltk/src/Fl_JPEG_Image.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_PNG_Image.cxx.o: vendor/fltk/src/Fl_PNG_Image.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_PNM_Image.cxx.o: vendor/fltk/src/Fl_PNM_Image.cxx mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -DFL_LIBRARY -Dgettimeofday=lume_fltk_monotonic_time -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/libfltk.a: $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Adjuster.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Bitmap.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Browser.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Browser_.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Browser_load.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Box.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Button.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Chart.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Check_Browser.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Check_Button.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Choice.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Clock.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Color_Chooser.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Copy_Surface.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Counter.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Device.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Dial.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Help_Dialog_Dox.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Double_Window.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_File_Browser.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_File_Chooser.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_File_Chooser2.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_File_Icon.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_File_Input.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Group.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Help_View.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Image.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Image_Surface.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Input.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Input_.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Light_Button.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Menu.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Menu_.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Menu_Bar.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Menu_Button.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Menu_Window.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Menu_add.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Menu_global.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Multi_Label.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Native_File_Chooser.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Overlay_Window.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Pack.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Paged_Device.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Pixmap.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Positioner.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_PostScript.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Preferences.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Printer.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Progress.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Repeat_Button.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Return_Button.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Roller.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Round_Button.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Scroll.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Scrollbar.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Shared_Image.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Single_Window.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Slider.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Table.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Table_Row.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Tabs.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Text_Buffer.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Text_Display.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Text_Editor.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Tile.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Tiled_Image.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Tooltip.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Tree.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Tree_Item_Array.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Tree_Item.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Tree_Prefs.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Valuator.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Value_Input.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Value_Output.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Value_Slider.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Widget.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Window.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Window_fullscreen.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Window_hotspot.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Window_iconize.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Window_shape.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Wizard.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_XBM_Image.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_XPM_Image.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_abort.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_add_idle.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_arg.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_compose.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_display.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_get_key.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_get_system_colors.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_grab.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_lock.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_own_colormap.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_visual.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_x.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/filename_absolute.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/filename_expand.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/filename_ext.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/filename_isdir.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/filename_list.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/filename_match.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/filename_setext.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_arc.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_arci.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_ask.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_boxtype.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_color.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_cursor.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_curve.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_diamond_box.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_dnd.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_draw.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_draw_image.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_draw_pixmap.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_engraved_label.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_file_dir.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_font.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_gleam.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_gtk.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_labeltype.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_line_style.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_open_uri.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_oval_box.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_overlay.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_overlay_visual.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_plastic.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_read_image.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_rect.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_round_box.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_rounded_box.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_set_font.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_set_fonts.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_scroll_area.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_shadow_box.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_shortcut.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_show_colormap.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_symbols.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_vertex.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/ps_image.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/screen_xywh.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_utf8.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_encoding_latin1.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_encoding_mac_roman.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/flstring.c.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/scandir.c.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/numericsort.c.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/vsnprintf.c.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/xutf8/is_right2left.c.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/xutf8/is_spacing.c.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/xutf8/case.c.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/xutf8/utf8Input.c.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/xutf8/utf8Utils.c.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/xutf8/utf8Wrap.c.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/xutf8/keysym2Ucs.c.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_utf.c.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/fl_images_core.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_BMP_Image.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_File_Icon2.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_GIF_Image.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_Help_Dialog.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_JPEG_Image.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_PNG_Image.cxx.o $(DESKTOP_APPS_OUT)/obj/fltk/vendor/fltk/src/Fl_PNM_Image.cxx.o
	mkdir -p "$(@D)"
	$(file >$@.rsp,rcs "$@" $(foreach object,$^,"$(object)"))
	$(DESKTOP_AR) @"$@.rsp"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/aes.c.o: vendor/mbedtls/library/aes.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/aesce.c.o: vendor/mbedtls/library/aesce.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/aesni.c.o: vendor/mbedtls/library/aesni.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/aria.c.o: vendor/mbedtls/library/aria.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/asn1parse.c.o: vendor/mbedtls/library/asn1parse.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/asn1write.c.o: vendor/mbedtls/library/asn1write.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/base64.c.o: vendor/mbedtls/library/base64.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/bignum.c.o: vendor/mbedtls/library/bignum.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/bignum_core.c.o: vendor/mbedtls/library/bignum_core.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/bignum_mod.c.o: vendor/mbedtls/library/bignum_mod.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/bignum_mod_raw.c.o: vendor/mbedtls/library/bignum_mod_raw.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/block_cipher.c.o: vendor/mbedtls/library/block_cipher.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/camellia.c.o: vendor/mbedtls/library/camellia.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ccm.c.o: vendor/mbedtls/library/ccm.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/chacha20.c.o: vendor/mbedtls/library/chacha20.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/chachapoly.c.o: vendor/mbedtls/library/chachapoly.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/cipher.c.o: vendor/mbedtls/library/cipher.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/cipher_wrap.c.o: vendor/mbedtls/library/cipher_wrap.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/cmac.c.o: vendor/mbedtls/library/cmac.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/constant_time.c.o: vendor/mbedtls/library/constant_time.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ctr_drbg.c.o: vendor/mbedtls/library/ctr_drbg.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/debug.c.o: vendor/mbedtls/library/debug.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/des.c.o: vendor/mbedtls/library/des.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/dhm.c.o: vendor/mbedtls/library/dhm.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ecdh.c.o: vendor/mbedtls/library/ecdh.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ecdsa.c.o: vendor/mbedtls/library/ecdsa.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ecjpake.c.o: vendor/mbedtls/library/ecjpake.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ecp.c.o: vendor/mbedtls/library/ecp.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ecp_curves.c.o: vendor/mbedtls/library/ecp_curves.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ecp_curves_new.c.o: vendor/mbedtls/library/ecp_curves_new.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/entropy.c.o: vendor/mbedtls/library/entropy.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/entropy_poll.c.o: vendor/mbedtls/library/entropy_poll.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/error.c.o: vendor/mbedtls/library/error.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/gcm.c.o: vendor/mbedtls/library/gcm.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/hkdf.c.o: vendor/mbedtls/library/hkdf.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/hmac_drbg.c.o: vendor/mbedtls/library/hmac_drbg.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/lmots.c.o: vendor/mbedtls/library/lmots.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/lms.c.o: vendor/mbedtls/library/lms.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/md.c.o: vendor/mbedtls/library/md.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/md5.c.o: vendor/mbedtls/library/md5.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/memory_buffer_alloc.c.o: vendor/mbedtls/library/memory_buffer_alloc.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/mps_reader.c.o: vendor/mbedtls/library/mps_reader.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/mps_trace.c.o: vendor/mbedtls/library/mps_trace.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/net_sockets.c.o: vendor/mbedtls/library/net_sockets.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/nist_kw.c.o: vendor/mbedtls/library/nist_kw.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/oid.c.o: vendor/mbedtls/library/oid.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/padlock.c.o: vendor/mbedtls/library/padlock.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/pem.c.o: vendor/mbedtls/library/pem.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/pk.c.o: vendor/mbedtls/library/pk.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/pk_ecc.c.o: vendor/mbedtls/library/pk_ecc.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/pk_wrap.c.o: vendor/mbedtls/library/pk_wrap.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/pkcs12.c.o: vendor/mbedtls/library/pkcs12.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/pkcs5.c.o: vendor/mbedtls/library/pkcs5.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/pkcs7.c.o: vendor/mbedtls/library/pkcs7.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/pkparse.c.o: vendor/mbedtls/library/pkparse.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/pkwrite.c.o: vendor/mbedtls/library/pkwrite.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/platform.c.o: vendor/mbedtls/library/platform.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/platform_util.c.o: vendor/mbedtls/library/platform_util.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/poly1305.c.o: vendor/mbedtls/library/poly1305.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto.c.o: vendor/mbedtls/library/psa_crypto.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_aead.c.o: vendor/mbedtls/library/psa_crypto_aead.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_cipher.c.o: vendor/mbedtls/library/psa_crypto_cipher.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_client.c.o: vendor/mbedtls/library/psa_crypto_client.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_driver_wrappers_no_static.c.o: vendor/mbedtls/library/psa_crypto_driver_wrappers_no_static.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_ecp.c.o: vendor/mbedtls/library/psa_crypto_ecp.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_ffdh.c.o: vendor/mbedtls/library/psa_crypto_ffdh.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_hash.c.o: vendor/mbedtls/library/psa_crypto_hash.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_mac.c.o: vendor/mbedtls/library/psa_crypto_mac.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_pake.c.o: vendor/mbedtls/library/psa_crypto_pake.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_random.c.o: vendor/mbedtls/library/psa_crypto_random.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_rsa.c.o: vendor/mbedtls/library/psa_crypto_rsa.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_se.c.o: vendor/mbedtls/library/psa_crypto_se.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_slot_management.c.o: vendor/mbedtls/library/psa_crypto_slot_management.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_storage.c.o: vendor/mbedtls/library/psa_crypto_storage.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_its_file.c.o: vendor/mbedtls/library/psa_its_file.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_util.c.o: vendor/mbedtls/library/psa_util.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ripemd160.c.o: vendor/mbedtls/library/ripemd160.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/rsa.c.o: vendor/mbedtls/library/rsa.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/rsa_alt_helpers.c.o: vendor/mbedtls/library/rsa_alt_helpers.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/sha1.c.o: vendor/mbedtls/library/sha1.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/sha256.c.o: vendor/mbedtls/library/sha256.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/sha3.c.o: vendor/mbedtls/library/sha3.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/sha512.c.o: vendor/mbedtls/library/sha512.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_cache.c.o: vendor/mbedtls/library/ssl_cache.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_ciphersuites.c.o: vendor/mbedtls/library/ssl_ciphersuites.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_client.c.o: vendor/mbedtls/library/ssl_client.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_cookie.c.o: vendor/mbedtls/library/ssl_cookie.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_debug_helpers_generated.c.o: vendor/mbedtls/library/ssl_debug_helpers_generated.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_msg.c.o: vendor/mbedtls/library/ssl_msg.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_ticket.c.o: vendor/mbedtls/library/ssl_ticket.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_tls.c.o: vendor/mbedtls/library/ssl_tls.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_tls12_client.c.o: vendor/mbedtls/library/ssl_tls12_client.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_tls12_server.c.o: vendor/mbedtls/library/ssl_tls12_server.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_tls13_client.c.o: vendor/mbedtls/library/ssl_tls13_client.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_tls13_generic.c.o: vendor/mbedtls/library/ssl_tls13_generic.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_tls13_keys.c.o: vendor/mbedtls/library/ssl_tls13_keys.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_tls13_server.c.o: vendor/mbedtls/library/ssl_tls13_server.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/threading.c.o: vendor/mbedtls/library/threading.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/timing.c.o: vendor/mbedtls/library/timing.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/version.c.o: vendor/mbedtls/library/version.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/version_features.c.o: vendor/mbedtls/library/version_features.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/x509.c.o: vendor/mbedtls/library/x509.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/x509_create.c.o: vendor/mbedtls/library/x509_create.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/x509_crl.c.o: vendor/mbedtls/library/x509_crl.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/x509_crt.c.o: vendor/mbedtls/library/x509_crt.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/x509_csr.c.o: vendor/mbedtls/library/x509_csr.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/x509write.c.o: vendor/mbedtls/library/x509write.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/x509write_crt.c.o: vendor/mbedtls/library/x509write_crt.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/x509write_csr.c.o: vendor/mbedtls/library/x509write_csr.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/libmbedtls.a: $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/aes.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/aesce.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/aesni.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/aria.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/asn1parse.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/asn1write.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/base64.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/bignum.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/bignum_core.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/bignum_mod.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/bignum_mod_raw.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/block_cipher.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/camellia.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ccm.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/chacha20.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/chachapoly.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/cipher.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/cipher_wrap.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/cmac.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/constant_time.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ctr_drbg.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/debug.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/des.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/dhm.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ecdh.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ecdsa.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ecjpake.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ecp.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ecp_curves.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ecp_curves_new.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/entropy.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/entropy_poll.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/error.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/gcm.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/hkdf.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/hmac_drbg.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/lmots.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/lms.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/md.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/md5.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/memory_buffer_alloc.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/mps_reader.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/mps_trace.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/net_sockets.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/nist_kw.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/oid.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/padlock.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/pem.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/pk.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/pk_ecc.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/pk_wrap.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/pkcs12.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/pkcs5.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/pkcs7.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/pkparse.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/pkwrite.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/platform.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/platform_util.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/poly1305.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_aead.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_cipher.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_client.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_driver_wrappers_no_static.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_ecp.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_ffdh.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_hash.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_mac.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_pake.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_random.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_rsa.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_se.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_slot_management.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_crypto_storage.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_its_file.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/psa_util.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ripemd160.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/rsa.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/rsa_alt_helpers.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/sha1.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/sha256.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/sha3.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/sha512.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_cache.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_ciphersuites.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_client.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_cookie.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_debug_helpers_generated.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_msg.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_ticket.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_tls.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_tls12_client.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_tls12_server.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_tls13_client.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_tls13_generic.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_tls13_keys.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/ssl_tls13_server.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/threading.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/timing.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/version.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/version_features.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/x509.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/x509_create.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/x509_crl.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/x509_crt.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/x509_csr.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/x509write.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/x509write_crt.c.o $(DESKTOP_APPS_OUT)/obj/tls/vendor/mbedtls/library/x509write_csr.c.o
	mkdir -p "$(@D)"
	$(file >$@.rsp,rcs "$@" $(foreach object,$^,"$(object)"))
	$(DESKTOP_AR) @"$@.rsp"

$(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o: ports/fltk/clock.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11  -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o: ports/mbedtls/lume.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_TLS) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dlib/dlib.c.o: vendor/dillo/dlib/dlib.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dpip/dpip.c.o: vendor/dillo/dpip/dpip.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/lout/container.cc.o: vendor/dillo/lout/container.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/lout/identity.cc.o: vendor/dillo/lout/identity.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/lout/misc.cc.o: vendor/dillo/lout/misc.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/lout/object.cc.o: vendor/dillo/lout/object.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/lout/signal.cc.o: vendor/dillo/lout/signal.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/lout/unicode.cc.o: vendor/dillo/lout/unicode.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/findtext.cc.o: vendor/dillo/dw/findtext.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/imgrenderer.cc.o: vendor/dillo/dw/imgrenderer.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/iterator.cc.o: vendor/dillo/dw/iterator.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/layout.cc.o: vendor/dillo/dw/layout.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/selection.cc.o: vendor/dillo/dw/selection.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/stackingcontextmgr.cc.o: vendor/dillo/dw/stackingcontextmgr.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/style.cc.o: vendor/dillo/dw/style.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/tools.cc.o: vendor/dillo/dw/tools.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/types.cc.o: vendor/dillo/dw/types.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/ui.cc.o: vendor/dillo/dw/ui.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/widget.cc.o: vendor/dillo/dw/widget.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkcomplexbutton.cc.o: vendor/dillo/dw/fltkcomplexbutton.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkflatview.cc.o: vendor/dillo/dw/fltkflatview.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkimgbuf.cc.o: vendor/dillo/dw/fltkimgbuf.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkmisc.cc.o: vendor/dillo/dw/fltkmisc.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkplatform.cc.o: vendor/dillo/dw/fltkplatform.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkpreview.cc.o: vendor/dillo/dw/fltkpreview.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkui.cc.o: vendor/dillo/dw/fltkui.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkviewbase.cc.o: vendor/dillo/dw/fltkviewbase.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkviewport.cc.o: vendor/dillo/dw/fltkviewport.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/alignedtablecell.cc.o: vendor/dillo/dw/alignedtablecell.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/alignedtextblock.cc.o: vendor/dillo/dw/alignedtextblock.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/bullet.cc.o: vendor/dillo/dw/bullet.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/hyphenator.cc.o: vendor/dillo/dw/hyphenator.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/image.cc.o: vendor/dillo/dw/image.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/listitem.cc.o: vendor/dillo/dw/listitem.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofawarewidget.cc.o: vendor/dillo/dw/oofawarewidget.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofawarewidget_iterator.cc.o: vendor/dillo/dw/oofawarewidget_iterator.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/ooffloatsmgr.cc.o: vendor/dillo/dw/ooffloatsmgr.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofposabslikemgr.cc.o: vendor/dillo/dw/oofposabslikemgr.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofposabsmgr.cc.o: vendor/dillo/dw/oofposabsmgr.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofposfixedmgr.cc.o: vendor/dillo/dw/oofposfixedmgr.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofpositionedmgr.cc.o: vendor/dillo/dw/oofpositionedmgr.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofposrelmgr.cc.o: vendor/dillo/dw/oofposrelmgr.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/outofflowmgr.cc.o: vendor/dillo/dw/outofflowmgr.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/regardingborder.cc.o: vendor/dillo/dw/regardingborder.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/ruler.cc.o: vendor/dillo/dw/ruler.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/simpletablecell.cc.o: vendor/dillo/dw/simpletablecell.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/table.cc.o: vendor/dillo/dw/table.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/table_iterator.cc.o: vendor/dillo/dw/table_iterator.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/tablecell.cc.o: vendor/dillo/dw/tablecell.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/textblock.cc.o: vendor/dillo/dw/textblock.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/textblock_iterator.cc.o: vendor/dillo/dw/textblock_iterator.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/textblock_linebreaking.cc.o: vendor/dillo/dw/textblock_linebreaking.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/dillo.cc.o: vendor/dillo/src/dillo.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/version.cc.o: vendor/dillo/src/version.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/paths.cc.o: vendor/dillo/src/paths.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/tipwin.cc.o: vendor/dillo/src/tipwin.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/ui.cc.o: vendor/dillo/src/ui.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/uicmd.cc.o: vendor/dillo/src/uicmd.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/bw.c.o: vendor/dillo/src/bw.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/cookies.c.o: vendor/dillo/src/cookies.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/actions.c.o: vendor/dillo/src/actions.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/hsts.c.o: vendor/dillo/src/hsts.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/auth.c.o: vendor/dillo/src/auth.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/md5.c.o: vendor/dillo/src/md5.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/digest.c.o: vendor/dillo/src/digest.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/colors.c.o: vendor/dillo/src/colors.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/misc.c.o: vendor/dillo/src/misc.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/history.c.o: vendor/dillo/src/history.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/prefs.c.o: vendor/dillo/src/prefs.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/prefsparser.cc.o: vendor/dillo/src/prefsparser.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/keys.cc.o: vendor/dillo/src/keys.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/url.c.o: vendor/dillo/src/url.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/bitvec.c.o: vendor/dillo/src/bitvec.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/klist.c.o: vendor/dillo/src/klist.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/chain.c.o: vendor/dillo/src/chain.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/utf8.cc.o: vendor/dillo/src/utf8.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/timeout.cc.o: vendor/dillo/src/timeout.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/dialog.cc.o: vendor/dillo/src/dialog.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/web.cc.o: vendor/dillo/src/web.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/nav.c.o: vendor/dillo/src/nav.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/cache.c.o: vendor/dillo/src/cache.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/decode.c.o: vendor/dillo/src/decode.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/dicache.c.o: vendor/dillo/src/dicache.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/capi.c.o: vendor/dillo/src/capi.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/domain.c.o: vendor/dillo/src/domain.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/css.cc.o: vendor/dillo/src/css.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/cssparser.cc.o: vendor/dillo/src/cssparser.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/styleengine.cc.o: vendor/dillo/src/styleengine.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/plain.cc.o: vendor/dillo/src/plain.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/html.cc.o: vendor/dillo/src/html.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/form.cc.o: vendor/dillo/src/form.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/table.cc.o: vendor/dillo/src/table.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/bookmark.c.o: vendor/dillo/src/bookmark.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/dns.c.o: vendor/dillo/src/dns.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/gif.c.o: vendor/dillo/src/gif.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/png.c.o: vendor/dillo/src/png.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/webp.c.o: vendor/dillo/src/webp.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/svg.c.o: vendor/dillo/src/svg.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/imgbuf.cc.o: vendor/dillo/src/imgbuf.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/image.cc.o: vendor/dillo/src/image.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/menu.cc.o: vendor/dillo/src/menu.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/dpiapi.c.o: vendor/dillo/src/dpiapi.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/findbar.cc.o: vendor/dillo/src/findbar.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/xembed.cc.o: vendor/dillo/src/xembed.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/mime.c.o: vendor/dillo/src/IO/mime.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/about.c.o: vendor/dillo/src/IO/about.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/control.c.o: vendor/dillo/src/IO/control.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/http.c.o: vendor/dillo/src/IO/http.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/tls.c.o: vendor/dillo/src/IO/tls.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/dpi.c.o: vendor/dillo/src/IO/dpi.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/IO.c.o: vendor/dillo/src/IO/IO.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/iowatch.cc.o: vendor/dillo/src/IO/iowatch.cc mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/tls_mbedtls.c.o: vendor/dillo/src/IO/tls_mbedtls.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dillo/ports/dillo/jpeg.c.o: ports/dillo/jpeg.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/dillo.elf: arch/$(ARCH)/user.ld $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dlib/dlib.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dpip/dpip.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/lout/container.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/lout/identity.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/lout/misc.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/lout/object.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/lout/signal.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/lout/unicode.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/findtext.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/imgrenderer.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/iterator.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/layout.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/selection.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/stackingcontextmgr.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/style.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/tools.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/types.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/ui.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/widget.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkcomplexbutton.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkflatview.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkimgbuf.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkmisc.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkplatform.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkpreview.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkui.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkviewbase.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkviewport.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/alignedtablecell.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/alignedtextblock.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/bullet.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/hyphenator.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/image.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/listitem.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofawarewidget.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofawarewidget_iterator.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/ooffloatsmgr.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofposabslikemgr.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofposabsmgr.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofposfixedmgr.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofpositionedmgr.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofposrelmgr.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/outofflowmgr.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/regardingborder.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/ruler.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/simpletablecell.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/table.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/table_iterator.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/tablecell.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/textblock.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/textblock_iterator.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/textblock_linebreaking.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/dillo.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/version.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/paths.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/tipwin.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/ui.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/uicmd.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/bw.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/cookies.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/actions.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/hsts.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/auth.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/md5.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/digest.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/colors.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/misc.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/history.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/prefs.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/prefsparser.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/keys.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/url.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/bitvec.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/klist.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/chain.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/utf8.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/timeout.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/dialog.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/web.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/nav.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/cache.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/decode.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/dicache.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/capi.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/domain.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/css.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/cssparser.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/styleengine.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/plain.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/html.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/form.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/table.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/bookmark.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/dns.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/gif.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/png.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/webp.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/svg.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/imgbuf.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/image.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/menu.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/dpiapi.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/findbar.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/xembed.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/mime.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/about.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/control.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/http.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/tls.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/dpi.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/IO.c.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/iowatch.cc.o $(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/tls_mbedtls.c.o $(DESKTOP_APPS_OUT)/obj/dillo/ports/dillo/jpeg.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o $(DESKTOP_APP_RUNTIME) $(DESKTOP_APP_LIBS) $(I686_BUILTINS)
	mkdir -p "$(@D)"
	$(file >$@.rsp,--gc-sections --build-id=sha1 -z max-page-size=4096 -T "arch/$(ARCH)/user.ld" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dlib/dlib.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dpip/dpip.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/lout/container.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/lout/identity.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/lout/misc.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/lout/object.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/lout/signal.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/lout/unicode.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/findtext.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/imgrenderer.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/iterator.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/layout.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/selection.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/stackingcontextmgr.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/style.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/tools.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/types.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/ui.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/widget.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkcomplexbutton.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkflatview.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkimgbuf.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkmisc.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkplatform.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkpreview.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkui.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkviewbase.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/fltkviewport.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/alignedtablecell.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/alignedtextblock.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/bullet.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/hyphenator.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/image.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/listitem.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofawarewidget.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofawarewidget_iterator.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/ooffloatsmgr.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofposabslikemgr.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofposabsmgr.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofposfixedmgr.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofpositionedmgr.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/oofposrelmgr.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/outofflowmgr.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/regardingborder.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/ruler.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/simpletablecell.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/table.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/table_iterator.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/tablecell.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/textblock.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/textblock_iterator.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/dw/textblock_linebreaking.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/dillo.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/version.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/paths.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/tipwin.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/ui.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/uicmd.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/bw.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/cookies.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/actions.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/hsts.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/auth.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/md5.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/digest.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/colors.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/misc.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/history.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/prefs.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/prefsparser.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/keys.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/url.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/bitvec.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/klist.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/chain.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/utf8.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/timeout.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/dialog.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/web.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/nav.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/cache.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/decode.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/dicache.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/capi.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/domain.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/css.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/cssparser.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/styleengine.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/plain.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/html.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/form.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/table.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/bookmark.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/dns.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/gif.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/png.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/webp.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/svg.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/imgbuf.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/image.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/menu.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/dpiapi.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/findbar.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/xembed.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/mime.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/about.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/control.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/http.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/tls.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/dpi.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/IO.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/iowatch.cc.o" "$(DESKTOP_APPS_OUT)/obj/dillo/vendor/dillo/src/IO/tls_mbedtls.c.o" "$(DESKTOP_APPS_OUT)/obj/dillo/ports/dillo/jpeg.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o" $(foreach object,$(DESKTOP_APP_RUNTIME) $(I686_BUILTINS),"$(object)") --start-group $(foreach archive,$(DESKTOP_APP_LIBS),"$(archive)") --end-group -o "$@")
	$(LD) @"$@.rsp"

$(DESKTOP_APPS_OUT)/obj/dilloc/vendor/dillo/src/dilloc.c.o: vendor/dillo/src/dilloc.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dilloc/vendor/dillo/dlib/dlib.c.o: vendor/dillo/dlib/dlib.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/dilloc.elf: arch/$(ARCH)/user.ld $(DESKTOP_APPS_OUT)/obj/dilloc/vendor/dillo/src/dilloc.c.o $(DESKTOP_APPS_OUT)/obj/dilloc/vendor/dillo/dlib/dlib.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o $(DESKTOP_APP_RUNTIME) $(DESKTOP_APP_LIBS) $(I686_BUILTINS)
	mkdir -p "$(@D)"
	$(file >$@.rsp,--gc-sections --build-id=sha1 -z max-page-size=4096 -T "arch/$(ARCH)/user.ld" "$(DESKTOP_APPS_OUT)/obj/dilloc/vendor/dillo/src/dilloc.c.o" "$(DESKTOP_APPS_OUT)/obj/dilloc/vendor/dillo/dlib/dlib.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o" $(foreach object,$(DESKTOP_APP_RUNTIME) $(I686_BUILTINS),"$(object)") --start-group $(foreach archive,$(DESKTOP_APP_LIBS),"$(archive)") --end-group -o "$@")
	$(LD) @"$@.rsp"

$(DESKTOP_APPS_OUT)/obj/browser/apps/browser/browser.c.o: apps/browser/browser.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_FLTK) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/browser.elf: arch/$(ARCH)/user.ld $(DESKTOP_APPS_OUT)/obj/browser/apps/browser/browser.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o $(DESKTOP_APP_RUNTIME) $(DESKTOP_APP_LIBS) $(I686_BUILTINS)
	mkdir -p "$(@D)"
	$(file >$@.rsp,--gc-sections --build-id=sha1 -z max-page-size=4096 -T "arch/$(ARCH)/user.ld" "$(DESKTOP_APPS_OUT)/obj/browser/apps/browser/browser.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o" $(foreach object,$(DESKTOP_APP_RUNTIME) $(I686_BUILTINS),"$(object)") --start-group $(foreach archive,$(DESKTOP_APP_LIBS),"$(archive)") --end-group -o "$@")
	$(LD) @"$@.rsp"

$(DESKTOP_APPS_OUT)/obj/network/apps/network/network.cpp.o: apps/network/network.cpp mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/network/services/netd/net_policy.c.o: services/netd/net_policy.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_FLTK) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/network.elf: arch/$(ARCH)/user.ld $(DESKTOP_APPS_OUT)/obj/network/apps/network/network.cpp.o $(DESKTOP_APPS_OUT)/obj/network/services/netd/net_policy.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o $(DESKTOP_APP_RUNTIME) $(DESKTOP_APP_LIBS) $(I686_BUILTINS)
	mkdir -p "$(@D)"
	$(file >$@.rsp,--gc-sections --build-id=sha1 -z max-page-size=4096 -T "arch/$(ARCH)/user.ld" "$(DESKTOP_APPS_OUT)/obj/network/apps/network/network.cpp.o" "$(DESKTOP_APPS_OUT)/obj/network/services/netd/net_policy.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o" $(foreach object,$(DESKTOP_APP_RUNTIME) $(I686_BUILTINS),"$(object)") --start-group $(foreach archive,$(DESKTOP_APP_LIBS),"$(archive)") --end-group -o "$@")
	$(LD) @"$@.rsp"

$(DESKTOP_APPS_OUT)/obj/clock/apps/clock/clock.c.o: apps/clock/clock.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_FLTK) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/clock.elf: arch/$(ARCH)/user.ld $(DESKTOP_APPS_OUT)/obj/clock/apps/clock/clock.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o $(DESKTOP_APP_RUNTIME) $(DESKTOP_APP_LIBS) $(I686_BUILTINS)
	mkdir -p "$(@D)"
	$(file >$@.rsp,--gc-sections --build-id=sha1 -z max-page-size=4096 -T "arch/$(ARCH)/user.ld" "$(DESKTOP_APPS_OUT)/obj/clock/apps/clock/clock.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o" $(foreach object,$(DESKTOP_APP_RUNTIME) $(I686_BUILTINS),"$(object)") --start-group $(foreach archive,$(DESKTOP_APP_LIBS),"$(archive)") --end-group -o "$@")
	$(LD) @"$@.rsp"

$(DESKTOP_APPS_OUT)/obj/appcheck/tests/apps/appcheck.c.o: tests/apps/appcheck.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_FLTK) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/appcheck.elf: arch/$(ARCH)/user.ld $(DESKTOP_APPS_OUT)/obj/appcheck/tests/apps/appcheck.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o $(DESKTOP_APP_RUNTIME) $(DESKTOP_APP_LIBS) $(I686_BUILTINS)
	mkdir -p "$(@D)"
	$(file >$@.rsp,--gc-sections --build-id=sha1 -z max-page-size=4096 -T "arch/$(ARCH)/user.ld" "$(DESKTOP_APPS_OUT)/obj/appcheck/tests/apps/appcheck.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o" $(foreach object,$(DESKTOP_APP_RUNTIME) $(I686_BUILTINS),"$(object)") --start-group $(foreach archive,$(DESKTOP_APP_LIBS),"$(archive)") --end-group -o "$@")
	$(LD) @"$@.rsp"

$(DESKTOP_APPS_OUT)/obj/fltkcheck/tests/apps/fltkcheck.cpp.o: tests/apps/fltkcheck.cpp mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) $(DESKTOP_APP_CXX) $(DESKTOP_APP_FLTK) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/fltkcheck.elf: arch/$(ARCH)/user.ld $(DESKTOP_APPS_OUT)/obj/fltkcheck/tests/apps/fltkcheck.cpp.o $(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o $(DESKTOP_APP_RUNTIME) $(DESKTOP_APP_LIBS) $(I686_BUILTINS)
	mkdir -p "$(@D)"
	$(file >$@.rsp,--gc-sections --build-id=sha1 -z max-page-size=4096 -T "arch/$(ARCH)/user.ld" "$(DESKTOP_APPS_OUT)/obj/fltkcheck/tests/apps/fltkcheck.cpp.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o" $(foreach object,$(DESKTOP_APP_RUNTIME) $(I686_BUILTINS),"$(object)") --start-group $(foreach archive,$(DESKTOP_APP_LIBS),"$(archive)") --end-group -o "$@")
	$(LD) @"$@.rsp"

$(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dlib/dlib.c.o: vendor/dillo/dlib/dlib.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpip/dpip.c.o: vendor/dillo/dpip/dpip.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpid/dpi.c.o: vendor/dillo/dpid/dpi.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpid/dpi_socket_dir.c.o: vendor/dillo/dpid/dpi_socket_dir.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpid/dpid.c.o: vendor/dillo/dpid/dpid.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpid/dpid_common.c.o: vendor/dillo/dpid/dpid_common.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpid/main.c.o: vendor/dillo/dpid/main.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpid/misc_new.c.o: vendor/dillo/dpid/misc_new.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/dpid.elf: arch/$(ARCH)/user.ld $(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dlib/dlib.c.o $(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpip/dpip.c.o $(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpid/dpi.c.o $(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpid/dpi_socket_dir.c.o $(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpid/dpid.c.o $(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpid/dpid_common.c.o $(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpid/main.c.o $(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpid/misc_new.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o $(DESKTOP_APP_RUNTIME) $(DESKTOP_APP_LIBS) $(I686_BUILTINS)
	mkdir -p "$(@D)"
	$(file >$@.rsp,--gc-sections --build-id=sha1 -z max-page-size=4096 -T "arch/$(ARCH)/user.ld" "$(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dlib/dlib.c.o" "$(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpip/dpip.c.o" "$(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpid/dpi.c.o" "$(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpid/dpi_socket_dir.c.o" "$(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpid/dpid.c.o" "$(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpid/dpid_common.c.o" "$(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpid/main.c.o" "$(DESKTOP_APPS_OUT)/obj/dpid/vendor/dillo/dpid/misc_new.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o" $(foreach object,$(DESKTOP_APP_RUNTIME) $(I686_BUILTINS),"$(object)") --start-group $(foreach archive,$(DESKTOP_APP_LIBS),"$(archive)") --end-group -o "$@")
	$(LD) @"$@.rsp"

$(DESKTOP_APPS_OUT)/obj/file.dpi/vendor/dillo/dlib/dlib.c.o: vendor/dillo/dlib/dlib.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/file.dpi/vendor/dillo/dpip/dpip.c.o: vendor/dillo/dpip/dpip.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/file.dpi/vendor/dillo/dpi/file.c.o: vendor/dillo/dpi/file.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/file.dpi/vendor/dillo/dpi/dpiutil.c.o: vendor/dillo/dpi/dpiutil.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/file.dpi.elf: arch/$(ARCH)/user.ld $(DESKTOP_APPS_OUT)/obj/file.dpi/vendor/dillo/dlib/dlib.c.o $(DESKTOP_APPS_OUT)/obj/file.dpi/vendor/dillo/dpip/dpip.c.o $(DESKTOP_APPS_OUT)/obj/file.dpi/vendor/dillo/dpi/file.c.o $(DESKTOP_APPS_OUT)/obj/file.dpi/vendor/dillo/dpi/dpiutil.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o $(DESKTOP_APP_RUNTIME) $(DESKTOP_APP_LIBS) $(I686_BUILTINS)
	mkdir -p "$(@D)"
	$(file >$@.rsp,--gc-sections --build-id=sha1 -z max-page-size=4096 -T "arch/$(ARCH)/user.ld" "$(DESKTOP_APPS_OUT)/obj/file.dpi/vendor/dillo/dlib/dlib.c.o" "$(DESKTOP_APPS_OUT)/obj/file.dpi/vendor/dillo/dpip/dpip.c.o" "$(DESKTOP_APPS_OUT)/obj/file.dpi/vendor/dillo/dpi/file.c.o" "$(DESKTOP_APPS_OUT)/obj/file.dpi/vendor/dillo/dpi/dpiutil.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o" $(foreach object,$(DESKTOP_APP_RUNTIME) $(I686_BUILTINS),"$(object)") --start-group $(foreach archive,$(DESKTOP_APP_LIBS),"$(archive)") --end-group -o "$@")
	$(LD) @"$@.rsp"

$(DESKTOP_APPS_OUT)/obj/bookmarks.dpi/vendor/dillo/dlib/dlib.c.o: vendor/dillo/dlib/dlib.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/bookmarks.dpi/vendor/dillo/dpip/dpip.c.o: vendor/dillo/dpip/dpip.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/bookmarks.dpi/vendor/dillo/dpi/bookmarks.c.o: vendor/dillo/dpi/bookmarks.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/bookmarks.dpi/vendor/dillo/dpi/dpiutil.c.o: vendor/dillo/dpi/dpiutil.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/bookmarks.dpi.elf: arch/$(ARCH)/user.ld $(DESKTOP_APPS_OUT)/obj/bookmarks.dpi/vendor/dillo/dlib/dlib.c.o $(DESKTOP_APPS_OUT)/obj/bookmarks.dpi/vendor/dillo/dpip/dpip.c.o $(DESKTOP_APPS_OUT)/obj/bookmarks.dpi/vendor/dillo/dpi/bookmarks.c.o $(DESKTOP_APPS_OUT)/obj/bookmarks.dpi/vendor/dillo/dpi/dpiutil.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o $(DESKTOP_APP_RUNTIME) $(DESKTOP_APP_LIBS) $(I686_BUILTINS)
	mkdir -p "$(@D)"
	$(file >$@.rsp,--gc-sections --build-id=sha1 -z max-page-size=4096 -T "arch/$(ARCH)/user.ld" "$(DESKTOP_APPS_OUT)/obj/bookmarks.dpi/vendor/dillo/dlib/dlib.c.o" "$(DESKTOP_APPS_OUT)/obj/bookmarks.dpi/vendor/dillo/dpip/dpip.c.o" "$(DESKTOP_APPS_OUT)/obj/bookmarks.dpi/vendor/dillo/dpi/bookmarks.c.o" "$(DESKTOP_APPS_OUT)/obj/bookmarks.dpi/vendor/dillo/dpi/dpiutil.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o" $(foreach object,$(DESKTOP_APP_RUNTIME) $(I686_BUILTINS),"$(object)") --start-group $(foreach archive,$(DESKTOP_APP_LIBS),"$(archive)") --end-group -o "$@")
	$(LD) @"$@.rsp"

$(DESKTOP_APPS_OUT)/obj/cookies.dpi/vendor/dillo/dlib/dlib.c.o: vendor/dillo/dlib/dlib.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/cookies.dpi/vendor/dillo/dpip/dpip.c.o: vendor/dillo/dpip/dpip.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/cookies.dpi/vendor/dillo/dpi/cookies.c.o: vendor/dillo/dpi/cookies.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/cookies.dpi/vendor/dillo/dpi/dpiutil.c.o: vendor/dillo/dpi/dpiutil.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/cookies.dpi.elf: arch/$(ARCH)/user.ld $(DESKTOP_APPS_OUT)/obj/cookies.dpi/vendor/dillo/dlib/dlib.c.o $(DESKTOP_APPS_OUT)/obj/cookies.dpi/vendor/dillo/dpip/dpip.c.o $(DESKTOP_APPS_OUT)/obj/cookies.dpi/vendor/dillo/dpi/cookies.c.o $(DESKTOP_APPS_OUT)/obj/cookies.dpi/vendor/dillo/dpi/dpiutil.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o $(DESKTOP_APP_RUNTIME) $(DESKTOP_APP_LIBS) $(I686_BUILTINS)
	mkdir -p "$(@D)"
	$(file >$@.rsp,--gc-sections --build-id=sha1 -z max-page-size=4096 -T "arch/$(ARCH)/user.ld" "$(DESKTOP_APPS_OUT)/obj/cookies.dpi/vendor/dillo/dlib/dlib.c.o" "$(DESKTOP_APPS_OUT)/obj/cookies.dpi/vendor/dillo/dpip/dpip.c.o" "$(DESKTOP_APPS_OUT)/obj/cookies.dpi/vendor/dillo/dpi/cookies.c.o" "$(DESKTOP_APPS_OUT)/obj/cookies.dpi/vendor/dillo/dpi/dpiutil.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o" $(foreach object,$(DESKTOP_APP_RUNTIME) $(I686_BUILTINS),"$(object)") --start-group $(foreach archive,$(DESKTOP_APP_LIBS),"$(archive)") --end-group -o "$@")
	$(LD) @"$@.rsp"

$(DESKTOP_APPS_OUT)/obj/datauri.dpi/vendor/dillo/dlib/dlib.c.o: vendor/dillo/dlib/dlib.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/datauri.dpi/vendor/dillo/dpip/dpip.c.o: vendor/dillo/dpip/dpip.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/datauri.dpi/vendor/dillo/dpi/datauri.c.o: vendor/dillo/dpi/datauri.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/datauri.dpi/vendor/dillo/dpi/dpiutil.c.o: vendor/dillo/dpi/dpiutil.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/datauri.dpi.elf: arch/$(ARCH)/user.ld $(DESKTOP_APPS_OUT)/obj/datauri.dpi/vendor/dillo/dlib/dlib.c.o $(DESKTOP_APPS_OUT)/obj/datauri.dpi/vendor/dillo/dpip/dpip.c.o $(DESKTOP_APPS_OUT)/obj/datauri.dpi/vendor/dillo/dpi/datauri.c.o $(DESKTOP_APPS_OUT)/obj/datauri.dpi/vendor/dillo/dpi/dpiutil.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o $(DESKTOP_APP_RUNTIME) $(DESKTOP_APP_LIBS) $(I686_BUILTINS)
	mkdir -p "$(@D)"
	$(file >$@.rsp,--gc-sections --build-id=sha1 -z max-page-size=4096 -T "arch/$(ARCH)/user.ld" "$(DESKTOP_APPS_OUT)/obj/datauri.dpi/vendor/dillo/dlib/dlib.c.o" "$(DESKTOP_APPS_OUT)/obj/datauri.dpi/vendor/dillo/dpip/dpip.c.o" "$(DESKTOP_APPS_OUT)/obj/datauri.dpi/vendor/dillo/dpi/datauri.c.o" "$(DESKTOP_APPS_OUT)/obj/datauri.dpi/vendor/dillo/dpi/dpiutil.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o" $(foreach object,$(DESKTOP_APP_RUNTIME) $(I686_BUILTINS),"$(object)") --start-group $(foreach archive,$(DESKTOP_APP_LIBS),"$(archive)") --end-group -o "$@")
	$(LD) @"$@.rsp"

$(DESKTOP_APPS_OUT)/obj/vsource.dpi/vendor/dillo/dlib/dlib.c.o: vendor/dillo/dlib/dlib.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/vsource.dpi/vendor/dillo/dpip/dpip.c.o: vendor/dillo/dpip/dpip.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/vsource.dpi/vendor/dillo/dpi/vsource.c.o: vendor/dillo/dpi/vsource.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/obj/vsource.dpi/vendor/dillo/dpi/dpiutil.c.o: vendor/dillo/dpi/dpiutil.c mk/desktop-apps.mk ports/fltk/config.h ports/dillo/config.h ports/mbedtls/lume_tls_config.h | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -std=gnu11 $(DESKTOP_APP_DILLO) -MMD -MP -MF "$@.d" -c "$<" -o "$@"

$(DESKTOP_APPS_OUT)/vsource.dpi.elf: arch/$(ARCH)/user.ld $(DESKTOP_APPS_OUT)/obj/vsource.dpi/vendor/dillo/dlib/dlib.c.o $(DESKTOP_APPS_OUT)/obj/vsource.dpi/vendor/dillo/dpip/dpip.c.o $(DESKTOP_APPS_OUT)/obj/vsource.dpi/vendor/dillo/dpi/vsource.c.o $(DESKTOP_APPS_OUT)/obj/vsource.dpi/vendor/dillo/dpi/dpiutil.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o $(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o $(DESKTOP_APP_RUNTIME) $(DESKTOP_APP_LIBS) $(I686_BUILTINS)
	mkdir -p "$(@D)"
	$(file >$@.rsp,--gc-sections --build-id=sha1 -z max-page-size=4096 -T "arch/$(ARCH)/user.ld" "$(DESKTOP_APPS_OUT)/obj/vsource.dpi/vendor/dillo/dlib/dlib.c.o" "$(DESKTOP_APPS_OUT)/obj/vsource.dpi/vendor/dillo/dpip/dpip.c.o" "$(DESKTOP_APPS_OUT)/obj/vsource.dpi/vendor/dillo/dpi/vsource.c.o" "$(DESKTOP_APPS_OUT)/obj/vsource.dpi/vendor/dillo/dpi/dpiutil.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/fltk/clock.c.o" "$(DESKTOP_APPS_OUT)/obj/common/ports/mbedtls/lume.c.o" $(foreach object,$(DESKTOP_APP_RUNTIME) $(I686_BUILTINS),"$(object)") --start-group $(foreach archive,$(DESKTOP_APP_LIBS),"$(archive)") --end-group -o "$@")
	$(LD) @"$@.rsp"

desktop-apps-install: desktop-apps
	mkdir -p "$(DESKTOP_ROOT)/usr/share/applications"
	cp "ports/dillo/resources/dillo.desktop" "$(DESKTOP_ROOT)/usr/share/applications/dillo.desktop"
	mkdir -p "$(DESKTOP_ROOT)/usr/bin" "$(DESKTOP_ROOT)/usr/share/dillo" "$(DESKTOP_ROOT)/usr/share/lume/ssl" "$(DESKTOP_ROOT)/usr/share/licenses/dillo" "$(DESKTOP_ROOT)/usr/share/licenses/fltk" "$(DESKTOP_ROOT)/usr/share/licenses/stb" "$(DESKTOP_ROOT)/usr/share/licenses/mbedtls" "$(DESKTOP_ROOT)/usr/share/licenses/ca-certificates"
	cp "$(DESKTOP_APPS_OUT)/dillo.elf" "$(DESKTOP_ROOT)/usr/bin/dillo"
	cp "$(DESKTOP_APPS_OUT)/dilloc.elf" "$(DESKTOP_ROOT)/usr/bin/dilloc"
	cp "$(DESKTOP_APPS_OUT)/browser.elf" "$(DESKTOP_ROOT)/usr/bin/browser"
	cp "$(DESKTOP_APPS_OUT)/network.elf" "$(DESKTOP_ROOT)/usr/bin/network"
	cp "$(DESKTOP_APPS_OUT)/clock.elf" "$(DESKTOP_ROOT)/usr/bin/clock"
	cp "$(DESKTOP_APPS_OUT)/dpid.elf" "$(DESKTOP_ROOT)/usr/bin/dpid"
	mkdir -p "$(DESKTOP_ROOT)/usr/libexec/dillo/dpi/file"
	cp "$(DESKTOP_APPS_OUT)/file.dpi.elf" "$(DESKTOP_ROOT)/usr/libexec/dillo/dpi/file/file.dpi"
	mkdir -p "$(DESKTOP_ROOT)/usr/libexec/dillo/dpi/bookmarks"
	cp "$(DESKTOP_APPS_OUT)/bookmarks.dpi.elf" "$(DESKTOP_ROOT)/usr/libexec/dillo/dpi/bookmarks/bookmarks.dpi"
	mkdir -p "$(DESKTOP_ROOT)/usr/libexec/dillo/dpi/cookies"
	cp "$(DESKTOP_APPS_OUT)/cookies.dpi.elf" "$(DESKTOP_ROOT)/usr/libexec/dillo/dpi/cookies/cookies.dpi"
	mkdir -p "$(DESKTOP_ROOT)/usr/libexec/dillo/dpi/datauri"
	cp "$(DESKTOP_APPS_OUT)/datauri.dpi.elf" "$(DESKTOP_ROOT)/usr/libexec/dillo/dpi/datauri/datauri.filter.dpi"
	mkdir -p "$(DESKTOP_ROOT)/usr/libexec/dillo/dpi/vsource"
	cp "$(DESKTOP_APPS_OUT)/vsource.dpi.elf" "$(DESKTOP_ROOT)/usr/libexec/dillo/dpi/vsource/vsource.filter.dpi"
	cp "ports/dillo/resources/dillorc" "$(DESKTOP_ROOT)/usr/share/dillo/dillorc"
	cp "ports/dillo/resources/dpidrc" "$(DESKTOP_ROOT)/usr/share/dillo/dpidrc"
	cp "ports/mbedtls/resources/ca-bundle.crt" "$(DESKTOP_ROOT)/usr/share/lume/ssl/ca-bundle.crt"
	cp "ports/mbedtls/resources/COPYING" "$(DESKTOP_ROOT)/usr/share/licenses/ca-certificates/COPYING"
	cp "ports/mbedtls/resources/NOTICE" "$(DESKTOP_ROOT)/usr/share/licenses/ca-certificates/NOTICE"
	cp "vendor/mbedtls/LICENSE" "$(DESKTOP_ROOT)/usr/share/licenses/mbedtls/LICENSE"
	cp "vendor/dillo/COPYING" "$(DESKTOP_ROOT)/usr/share/licenses/dillo/COPYING"
	cp "vendor/dillo/AUTHORS" "$(DESKTOP_ROOT)/usr/share/licenses/dillo/AUTHORS"
	cp "vendor/fltk/COPYING" "$(DESKTOP_ROOT)/usr/share/licenses/fltk/COPYING"
	cp "vendor/fltk/CREDITS" "$(DESKTOP_ROOT)/usr/share/licenses/fltk/CREDITS"
	cp "vendor/stb/COPYING" "$(DESKTOP_ROOT)/usr/share/licenses/stb/COPYING"
ifeq ($(DESKTOP),1)
ifeq ($(DESKTOP_APPS),1)
all: desktop-apps
desktop-install: desktop-apps-install
endif
endif
-include $(shell find "$(DESKTOP_APPS_OUT)/obj" -name "*.o.d" 2>/dev/null)

$(DESKTOP_OUT)/programs/obj/common/libc/posix/pthread_timed.c.o: libc/posix/pthread_timed.c libc/posix/absolute_time.h include/lume/pthread_timed.h mk/desktop-apps.mk | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
-include $(DESKTOP_OUT)/programs/obj/common/libc/posix/pthread_timed.c.o.d

DESKTOP_MATH_HELPERS := $(filter-out vendor/%,$(NATIVE_MATH_SOURCES))
DESKTOP_MATH_OBJECTS := $(addprefix $(DESKTOP_OUT)/programs/obj/common/,$(DESKTOP_MATH_HELPERS:=.o))
$(DESKTOP_MATH_OBJECTS): $(DESKTOP_OUT)/programs/obj/common/%.o: % mk/desktop-apps.mk | headers
	mkdir -p "$(@D)"
	$(CC) $(DESKTOP_APP_BASE) -Ivendor/picolibc/libm/common -Ivendor/picolibc/libm/ld -Iports/picolibc/platform/libm/ld -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
-include $(DESKTOP_MATH_OBJECTS:=.d)
