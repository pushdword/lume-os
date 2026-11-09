include ports/bash/sources.mk
BASH_OBJECTS := $(addprefix $(OUT)/obj/vendor/bash/,$(addsuffix .o,$(BASH_SOURCES))) \
    $(addprefix $(OUT)/obj/ports/bash/generated/,$(addsuffix .o,$(BASH_GENERATED))) \
    $(OUT)/obj/ports/bash/compat.c.o $(OUT)/obj/ports/bash/entry.c.o
BASH_CPPFLAGS := -Iports/bash/include -Iports/bash -Iports/bash/generated \
    -Iports/bash/generated/builtins -Ivendor/bash -Ivendor/bash/include \
    -Ivendor/bash/lib -Ivendor/bash/builtins -DHAVE_CONFIG_H -DSHELL \
    -DPROGRAM=\"bash\" -DCONF_VENDOR=\"unknown\" -D_DEFAULT_SOURCE \
    -D_POSIX_C_SOURCE=200809L -D_POSIX_MONOTONIC_CLOCK=200809L

all: $(OUT)/bash.elf
$(BASH_OBJECTS): ports/bash/config.h ports/bash/platform.h ports/bash/sources.mk mk/shell-tools.mk | headers
$(OUT)/obj/vendor/bash/%.c.o: vendor/bash/%.c
	mkdir -p "$(@D)"
	$(CC) $(BASH_CPPFLAGS) $(CPPFLAGS) $(CFLAGS) -fcommon -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(OUT)/obj/ports/bash/%.c.o: ports/bash/%.c
	mkdir -p "$(@D)"
	$(CC) $(BASH_CPPFLAGS) $(CPPFLAGS) $(CFLAGS) -fcommon -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(OUT)/bash.elf: $(BASH_OBJECTS) $(filter-out $(OUT)/obj/libc/posix/main.c.o,$(NATIVE_POSIX_OBJECTS)) $(ARM_COMMON_OBJECTS)
	$(file >$@.rsp,$(ARM_LD_FLAGS) --wrap=fileno $(foreach obj,$^,"$(obj)") $(ARM_LD_LIBS) -o "$@")
	$(LD) @"$@.rsp"
-include $(addsuffix .d,$(BASH_OBJECTS))

NET_TOOLS := ifconfig route netstat arp hostname
NET_TOOLS_LIB_NAMES := interface inet inet_sr inet_gr util proc getroute setroute \
    ether hw af unix loopback getargs nstrcmp sockets
NET_TOOLS_LIB_OBJECTS := $(addprefix $(OUT)/obj/vendor/net-tools/lib/,$(addsuffix .c.o,$(NET_TOOLS_LIB_NAMES))) \
    $(OUT)/obj/ports/net-tools/backend.c.o $(OUT)/obj/ports/net-tools/compat.c.o
NET_TOOLS_OBJECTS := $(NET_TOOLS_LIB_OBJECTS) \
    $(addprefix $(OUT)/obj/vendor/net-tools/,$(addsuffix .c.o,$(NET_TOOLS) statistics))
NET_TOOLS_CPPFLAGS := -Iports/net-tools/include -Iports/net-tools \
    -Ivendor/net-tools/include -Ivendor/net-tools/lib -Ivendor/net-tools \
    -include ports/net-tools/config.h -D__lume__=1 -D_DEFAULT_SOURCE \
    -D_POSIX_C_SOURCE=200809L -D_POSIX_MONOTONIC_CLOCK=200809L
all: $(addprefix $(OUT)/,$(addsuffix .elf,$(NET_TOOLS)))
$(NET_TOOLS_OBJECTS): mk/shell-tools.mk ports/net-tools/config.h ports/net-tools/backend.h | headers
$(OUT)/obj/vendor/net-tools/%.c.o: vendor/net-tools/%.c
	mkdir -p "$(@D)"
	$(CC) $(NET_TOOLS_CPPFLAGS) -include ports/net-tools/backend.h $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(OUT)/obj/ports/net-tools/%.c.o: ports/net-tools/%.c
	mkdir -p "$(@D)"
	$(CC) $(NET_TOOLS_CPPFLAGS) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
define net_tool
$(OUT)/$(1).elf: $(OUT)/obj/vendor/net-tools/$(1).c.o $(NET_TOOLS_LIB_OBJECTS) $(NATIVE_POSIX_OBJECTS) $(ARM_COMMON_OBJECTS)
	$$(file >$$@.rsp,$$(ARM_LD_FLAGS) $$(foreach obj,$$^,"$$(obj)") $$(ARM_LD_LIBS) -o "$$@")
	$$(LD) @"$$@.rsp"
endef
$(foreach tool,$(NET_TOOLS),$(eval $(call net_tool,$(tool))))
$(OUT)/netstat.elf: $(OUT)/obj/vendor/net-tools/statistics.c.o
-include $(addsuffix .d,$(NET_TOOLS_OBJECTS))

.PHONY: shell-tools-install
shell-tools-install: $(OUT)/bash.elf $(addprefix $(OUT)/,$(addsuffix .elf,$(NET_TOOLS)))
	mkdir -p "$(OUT)/root/bin" "$(OUT)/root/etc" "$(OUT)/root/usr/share/licenses/bash" "$(OUT)/root/usr/share/licenses/net-tools"
	cp "$(OUT)/bash.elf" "$(OUT)/root/bin/bash"
	$(foreach tool,$(NET_TOOLS),cp "$(OUT)/$(tool).elf" "$(OUT)/root/bin/$(tool)";)
	cp ports/bash/termcap "$(OUT)/root/etc/termcap"
	cp vendor/bash/COPYING "$(OUT)/root/usr/share/licenses/bash/COPYING"
	cp vendor/net-tools/COPYING "$(OUT)/root/usr/share/licenses/net-tools/COPYING"

$(OUT)/obj/tests/integration/shell_tools.c.o: | headers
$(OUT)/shell-tools-test.elf: $(OUT)/obj/tests/integration/shell_tools.c.o $(NATIVE_POSIX_OBJECTS) $(ARM_COMMON_OBJECTS)
	$(file >$@.rsp,$(ARM_LD_FLAGS) $(foreach obj,$^,"$(obj)") $(ARM_LD_LIBS) -o "$@")
	$(LD) @"$@.rsp"
