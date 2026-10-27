.DEFAULT_GOAL := clients
CLIENT_PORT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
CLIENT_OS_ROOT := $(abspath $(CLIENT_PORT)/../..)
include $(CLIENT_OS_ROOT)/ports/openssh/Makefile
include $(CLIENT_PORT)/objects.mk
CPPFLAGS += -DSSH_PROGRAM='"/data/bin/ssh"' -DGETPGRP_VOID=1 -DPLEDGE_EXTRA_INET='""' -DIN_LOOPBACKNET=127 -include stdio.h -I$(CLIENT_PORT)/include
.PHONY: clients
clients: $(OUT)/ssh $(OUT)/sftp $(OUT)/scp
CLIENT_NATIVE := $(OUT)/obj/client/compat.o
$(OUT)/obj/client/%.o: $(CLIENT_PORT)/%.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/ssh: $(addprefix $(OUT)/obj/,$(SSHOBJS)) $(NATIVE) $(CLIENT_NATIVE) $(LIBRARIES) $(LINK_INPUTS)
	$(LINK) $(addprefix $(OUT)/obj/,$(SSHOBJS)) $(NATIVE) $(CLIENT_NATIVE) --start-group $(LIBRARIES) --end-group -o $@
$(OUT)/sftp: $(addprefix $(OUT)/obj/,$(SFTP_OBJS)) $(NATIVE) $(CLIENT_NATIVE) $(LIBRARIES) $(LINK_INPUTS)
	$(LINK) $(addprefix $(OUT)/obj/,$(SFTP_OBJS)) $(NATIVE) $(CLIENT_NATIVE) --start-group $(LIBRARIES) --end-group -o $@
$(OUT)/scp: $(addprefix $(OUT)/obj/,$(SCP_OBJS)) $(NATIVE) $(CLIENT_NATIVE) $(LIBRARIES) $(LINK_INPUTS)
	$(LINK) $(addprefix $(OUT)/obj/,$(SCP_OBJS)) $(NATIVE) $(CLIENT_NATIVE) --start-group $(LIBRARIES) --end-group -o $@
