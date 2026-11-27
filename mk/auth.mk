AUTH_TLS := vendor/mbedtls
CONSOLE_LOGIN ?= 0
auth-console-config = $(if $(filter 1,$(CONSOLE_LOGIN)),touch "$(OUT)/root/etc/console-login",rm -f -- "$(OUT)/root/etc/console-login")
AUTH_COMMON_SOURCES := apps/auth/auth_files.c apps/auth/password.c apps/auth/terminal.c \
    $(addprefix $(AUTH_TLS)/library/,pkcs5.c md.c sha256.c constant_time.c platform_util.c)
AUTH_COMMON := $(addprefix $(OUT)/obj/auth/,$(AUTH_COMMON_SOURCES:=.o))
AUTH_LOGIN := $(AUTH_COMMON) $(addprefix $(OUT)/obj/auth/apps/auth/,main.c.o sudo_install.c.o)
AUTH_SUDO := $(AUTH_COMMON) $(addprefix $(OUT)/obj/auth/apps/auth/,sudo.c.o sudo_policy.c.o)
AUTH_IMAGES := login passwd sudo-setup
AUTH_CPPFLAGS := -D_GNU_SOURCE -Iapps/auth -I$(AUTH_TLS)/include -I$(AUTH_TLS)/library \
    -DMBEDTLS_CONFIG_FILE='"password_config.h"'
ifeq ($(ARCH),i686)
AUTH_CPPFLAGS += -DLUME_NATIVE_LFS64=1
endif
ifneq ($(strip $(NATIVE_SDK)),)
AUTH_RUNTIME := $(NATIVE_SDK)/lib/crt0.o $(NATIVE_SDK)/lib/liblume.a $(NATIVE_SDK)/lib/libc.a
AUTH_RUNTIME += $(wildcard $(NATIVE_SDK)/lib/libcompiler-rt.a)
AUTH_SCRIPT := $(NATIVE_SDK)/native.ld
else
AUTH_RUNTIME := $(NATIVE_SDK_BUILD)/crt0.o $(NATIVE_SDK_BUILD)/liblume.a $(SYSROOT)/lib/libc.a
AUTH_RUNTIME += $(wildcard $(SYSROOT)/lib/libcompiler-rt.a)
AUTH_SCRIPT := arch/$(ARCH)/native.ld
endif

.PHONY: auth
all: auth
auth: $(addprefix $(OUT)/,$(addsuffix .elf,$(AUTH_IMAGES) sudo))

$(OUT)/obj/auth/%.c.o: %.c $(wildcard apps/auth/*.h) $(OUT)/configuration | headers
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(AUTH_CPPFLAGS) $(CFLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"

$(OUT)/login.elf: $(AUTH_LOGIN) $(AUTH_RUNTIME) $(AUTH_SCRIPT) mk/auth.mk
	$(LD) --gc-sections --build-id=none -z max-page-size=4096 -T "$(AUTH_SCRIPT)" $(AUTH_LOGIN) --start-group $(AUTH_RUNTIME) --end-group -o "$@"

$(OUT)/passwd.elf $(OUT)/sudo-setup.elf: $(OUT)/login.elf
	cp "$<" "$@"

$(OUT)/sudo.elf: $(AUTH_SUDO) $(AUTH_RUNTIME) $(AUTH_SCRIPT) mk/auth.mk
	$(LD) --gc-sections --build-id=none -z max-page-size=4096 -T "$(AUTH_SCRIPT)" $(AUTH_SUDO) --start-group $(AUTH_RUNTIME) --end-group -o "$@"

-include $(sort $(AUTH_LOGIN:=.d) $(AUTH_SUDO:=.d))
