.DEFAULT_GOAL := all
CSTD := gnu11
PORT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SOURCE ?= $(PORT)/source
include $(PORT)/../standalone.mk
CPPFLAGS += -D_DEFAULT_SOURCE -DZSTD_NOBENCH -DZSTD_NOTRACE -DZSTD_DISABLE_ASM -DZSTD_LEGACY_SUPPORT=0 -DZSTD_NO_INTRINSICS -DZSTD_USE_UTIMENSAT=0 -Dclock=lume_zstd_progress_clock '-I$(KERNEL)/include/abi/native' '-I$(SOURCE)/lib' '-I$(SOURCE)/lib/common'
SOURCES := lib/common/debug.c lib/common/entropy_common.c lib/common/error_private.c lib/common/fse_decompress.c lib/common/pool.c lib/common/threading.c lib/common/xxhash.c lib/common/zstd_common.c lib/compress/fse_compress.c lib/compress/hist.c lib/compress/huf_compress.c lib/compress/zstd_compress.c lib/compress/zstd_compress_literals.c lib/compress/zstd_compress_sequences.c lib/compress/zstd_compress_superblock.c lib/compress/zstd_double_fast.c lib/compress/zstd_fast.c lib/compress/zstd_lazy.c lib/compress/zstd_ldm.c lib/compress/zstd_opt.c lib/compress/zstd_preSplit.c lib/compress/zstdmt_compress.c lib/decompress/huf_decompress.c lib/decompress/zstd_ddict.c lib/decompress/zstd_decompress.c lib/decompress/zstd_decompress_block.c lib/dictBuilder/cover.c lib/dictBuilder/divsufsort.c lib/dictBuilder/fastcover.c lib/dictBuilder/zdict.c port/platform.c
OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(basename $(SOURCES))))
.PHONY: all
all: $(OUT)/libzstd.a $(OUT)/zstd
$(OUT)/libzstd.a: $(OBJECTS)
	rm -f $@
	$(AR) rcs $@ $^
$(OUT)/zstd: $(OUT)/obj/programs/zstdcli.o $(OUT)/obj/programs/fileio.o $(OUT)/obj/programs/fileio_asyncio.o $(OUT)/obj/programs/util.o $(OUT)/obj/programs/timefn.o $(OUT)/obj/programs/dibio.o $(OUT)/obj/programs/datagen.o $(OUT)/obj/programs/lorem.o $(OUT)/libzstd.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
