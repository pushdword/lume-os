PORT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
CSTD := gnu11
SOURCE := $(abspath $(PORT)/../../vendor/vim/src)
include $(PORT)/../standalone.mk
include $(PORT)/sources.mk
CPPFLAGS += -DHAVE_CONFIG_H -DALL_BUILTIN_TCAPS -I$(PORT) -I$(PORT)/include -I$(PORT)/auto -I$(SOURCE) -I$(SOURCE)/proto -I$(PORT)/../standalone/include
LDFLAGS += --wrap=open
VIM_OBJECTS := $(addprefix $(OUT)/obj/,$(VIM_SOURCES:.c=.o))
PORT_OBJECTS := $(OUT)/obj/port/compat.o $(OUT)/obj/port/pathdef.o
SHARED_OBJECTS := $(addprefix $(OUT)/obj/shared/,file_mode.o process.o signals.o)
HEADERS := $(wildcard $(PORT)/auto/*.h $(PORT)/include/*.h $(SOURCE)/*.h $(SOURCE)/proto/*.pro $(SOURCE)/xdiff/*.h)
.DEFAULT_GOAL := all
.PHONY: all
all: $(OUT)/vim
$(VIM_OBJECTS) $(PORT_OBJECTS): $(HEADERS)
$(OUT)/obj/main.o: CPPFLAGS += -Dmain=vim_main
$(OUT)/vim: $(VIM_OBJECTS) $(PORT_OBJECTS) $(SHARED_OBJECTS)
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
