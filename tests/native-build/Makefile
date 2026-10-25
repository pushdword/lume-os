WORKER ?= /data/bin/worker
STDIO ?= /data/bin/stdio-fd-test
.PHONY: all first second third fourth fail stdin-check holder redirected
all: first second third fourth
first second third fourth:
	$(WORKER) 1500
fail:
	$(WORKER) 0
stdin-check: holder redirected
holder:
	$(WORKER) 1500
redirected:
	$(STDIO) --eof
