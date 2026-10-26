ARCH ?= aarch64
.DEFAULT_GOAL := all
PORT ?=
PORTS ?= /data/share/ports
SDK ?= /data/sdk
WORK ?= /data/build/$(PORT)
JOBS ?= 2
ifneq ($(words $(JOBS)),1)
$(error JOBS must be 1, 2, 3 or 4)
endif
ifeq ($(filter $(JOBS),1 2 3 4),)
$(error JOBS must be 1, 2, 3 or 4)
endif
ifeq ($(PORT),)
$(error PORT must name an installed source package)
endif
include $(PORTS)/$(PORT).mk
COMMON_ARCHIVE := /data/share/src/lume.tar
.PHONY: all prepare
all: prepare
	$(MAKE) -j$(JOBS) -C $(WORK)/$(PORT_DIR) $(PORT_TARGET) OUT=$(WORK)/out BUILD=$(WORK)/out SDK=$(SDK)
prepare: $(WORK)/.sources-ready
$(WORK)/.sources-ready: | $(COMMON_ARCHIVE) $(PORT_ARCHIVE)
	mkdir -p $(WORK)
	tar -xf $(COMMON_ARCHIVE) -C $(WORK)
	$(if $(PORT_ARCHIVE),tar -xf $(PORT_ARCHIVE) -C $(WORK),true)
	touch $@
