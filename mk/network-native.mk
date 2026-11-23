include ports/mbedtls/sources.mk

MBEDTLS_NATIVE_FLAGS := -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L \
    -Ivendor/mbedtls/include -Ivendor/mbedtls/library -Iports/mbedtls \
    -DMBEDTLS_CONFIG_FILE='"lume_tls_config.h"'
MBEDTLS_NATIVE_OBJECTS := \
    $(addprefix $(OUT)/obj/,$(addsuffix .o,$(MBEDTLS_APP_SOURCES))) \
    $(OUT)/obj/ports/mbedtls/lume.c.o

$(MBEDTLS_NATIVE_OBJECTS): CPPFLAGS += $(MBEDTLS_NATIVE_FLAGS)
$(MBEDTLS_NATIVE_OBJECTS): ports/mbedtls/lume_tls_config.h mk/network-native.mk | headers

$(OUT)/libmbedtls.a: $(MBEDTLS_NATIVE_OBJECTS) ports/mbedtls/sources.mk
	$(file >$@.rsp,$(foreach object,$(MBEDTLS_NATIVE_OBJECTS),"$(object)"))
	rm -f "$@.tmp"
	"$(TOOLBIN)llvm-ar$(HOSTEXE)" qcD "$@.tmp" @"$@.rsp"
	"$(TOOLBIN)llvm-ar$(HOSTEXE)" sD "$@.tmp"
	mv -f "$@.tmp" "$@"

$(OUT)/native-mbedtls.elf: $(OUT)/obj/tests/integration/native_mbedtls.c.o \
    $(OUT)/libmbedtls.a $(NATIVE_POSIX_OBJECTS) $(ARM_COMMON_OBJECTS)
	$(LD) $(ARM_LD_FLAGS) $(OUT)/obj/tests/integration/native_mbedtls.c.o \
	    $(NATIVE_POSIX_OBJECTS) $(ARM_COMMON_OBJECTS) --start-group \
	    $(OUT)/libmbedtls.a $(SYSROOT)/lib/libc.a \
	    $(SYSROOT)/lib/libcompiler-rt.a --end-group -o "$@"
$(OUT)/obj/tests/integration/native_mbedtls.c.o: CPPFLAGS += $(MBEDTLS_NATIVE_FLAGS)
$(OUT)/obj/tests/integration/native_mbedtls.c.o: | headers

-include $(MBEDTLS_NATIVE_OBJECTS:=.d) \
    $(OUT)/obj/tests/integration/native_mbedtls.c.o.d

$(OUT)/native-tcp-connect.elf: $(OUT)/obj/tests/integration/native_tcp_connect.c.o \
    $(NATIVE_POSIX_OBJECTS) $(ARM_COMMON_OBJECTS)
	$(LD) $(ARM_LD_FLAGS) $(OUT)/obj/tests/integration/native_tcp_connect.c.o \
	    $(NATIVE_POSIX_OBJECTS) $(ARM_COMMON_OBJECTS) --start-group \
	    $(SYSROOT)/lib/libc.a $(SYSROOT)/lib/libcompiler-rt.a --end-group -o "$@"
$(OUT)/obj/tests/integration/native_tcp_connect.c.o: | headers

-include $(OUT)/obj/tests/integration/native_tcp_connect.c.o.d

$(OUT)/native-name-lookup.elf: $(OUT)/obj/tests/integration/native_name_lookup.c.o \
    $(NATIVE_POSIX_OBJECTS) $(ARM_COMMON_OBJECTS)
	$(LD) $(ARM_LD_FLAGS) $^ $(ARM_LD_LIBS) -o "$@"
$(OUT)/obj/tests/integration/native_name_lookup.c.o: CPPFLAGS += -D_POSIX_C_SOURCE=200809L
$(OUT)/obj/tests/integration/native_name_lookup.c.o: | headers

-include $(OUT)/obj/tests/integration/native_name_lookup.c.o.d

include vendor/curl/lib/Makefile.inc
include vendor/curl/src/Makefile.inc

CURL_NATIVE_FLAGS := -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L \
    -DHAVE_CONFIG_H -DCURL_STATICLIB -Iports/curl -Ivendor/curl/include \
    -Ivendor/curl/lib -Ivendor/curl/src -Ivendor/mbedtls/include \
    -Ivendor/mbedtls/library -Iports/mbedtls
CURL_LIB_SOURCES := $(addprefix vendor/curl/lib/,$(CSOURCES))
CURL_TOOL_SOURCES := \
    $(addprefix vendor/curl/src/,$(filter-out ../lib/%,$(CURL_CFILES))) \
    $(patsubst ../lib/%,vendor/curl/lib/%,$(CURLX_CFILES))
CURL_LIB_OBJECTS := \
    $(addprefix $(OUT)/obj/curl/,$(addsuffix .o,$(CURL_LIB_SOURCES)))
CURL_TOOL_OBJECTS := \
    $(addprefix $(OUT)/obj/curl-tool/,$(addsuffix .o,$(CURL_TOOL_SOURCES)))

$(CURL_LIB_OBJECTS): CPPFLAGS += $(CURL_NATIVE_FLAGS) -DBUILDING_LIBCURL
$(CURL_TOOL_OBJECTS): CPPFLAGS += $(CURL_NATIVE_FLAGS) -DBUILDING_CURL
$(CURL_LIB_OBJECTS) $(CURL_TOOL_OBJECTS): ports/curl/config.h \
    ports/mbedtls/lume_tls_config.h mk/network-native.mk | headers

$(OUT)/obj/curl/%.c.o: %.c $(OUT)/configuration
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(OUT)/obj/curl-tool/%.c.o: %.c $(OUT)/configuration
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"

$(OUT)/libcurl.a: $(CURL_LIB_OBJECTS) vendor/curl/lib/Makefile.inc
	$(file >$@.rsp,$(foreach object,$(CURL_LIB_OBJECTS),"$(object)"))
	rm -f "$@.tmp"
	"$(TOOLBIN)llvm-ar$(HOSTEXE)" qcD "$@.tmp" @"$@.rsp"
	"$(TOOLBIN)llvm-ar$(HOSTEXE)" sD "$@.tmp"
	mv -f "$@.tmp" "$@"

$(OUT)/curl.elf: $(CURL_TOOL_OBJECTS) $(OUT)/libcurl.a $(OUT)/libmbedtls.a \
    $(NATIVE_POSIX_OBJECTS) $(ARM_COMMON_OBJECTS)
	$(LD) $(ARM_LD_FLAGS) $(CURL_TOOL_OBJECTS) $(NATIVE_POSIX_OBJECTS) \
	    $(ARM_COMMON_OBJECTS) --start-group $(OUT)/libcurl.a \
	    $(OUT)/libmbedtls.a $(SYSROOT)/lib/libc.a \
	    $(SYSROOT)/lib/libcompiler-rt.a --end-group -o "$@"

-include $(CURL_LIB_OBJECTS:=.d) $(CURL_TOOL_OBJECTS:=.d)
