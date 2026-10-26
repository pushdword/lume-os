.DEFAULT_GOAL := all
CSTD := gnu11
PORT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SOURCE = $(OS_ROOT)/vendor/jq
include $(PORT)/../standalone.mk
CPPFLAGS += -include '$(OS_ROOT)/ports/jq/config.h' '-I$(OS_ROOT)/ports/jq' '-I$(OS_ROOT)/vendor/jq' '-I$(OS_ROOT)/vendor/jq/src' '-I$(OS_ROOT)/vendor/jq/vendor/oniguruma/src' '-I$(OS_ROOT)/vendor/jq/vendor'
SOURCES := shared/realpath.c src/builtin.c src/bytecode.c src/compile.c src/execute.c src/jv.c src/jv_alloc.c src/jv_aux.c src/jv_dtoa.c src/jv_dtoa_tsd.c src/jv_file.c src/jv_parse.c src/jv_print.c src/jv_unicode.c src/lexer.c src/linker.c src/locfile.c src/parser.c src/util.c vendor/decNumber/decContext.c vendor/decNumber/decNumber.c vendor/oniguruma/src/ascii.c vendor/oniguruma/src/big5.c vendor/oniguruma/src/cp1251.c vendor/oniguruma/src/euc_jp.c vendor/oniguruma/src/euc_jp_prop.c vendor/oniguruma/src/euc_kr.c vendor/oniguruma/src/euc_tw.c vendor/oniguruma/src/gb18030.c vendor/oniguruma/src/iso8859_1.c vendor/oniguruma/src/iso8859_10.c vendor/oniguruma/src/iso8859_11.c vendor/oniguruma/src/iso8859_13.c vendor/oniguruma/src/iso8859_14.c vendor/oniguruma/src/iso8859_15.c vendor/oniguruma/src/iso8859_16.c vendor/oniguruma/src/iso8859_2.c vendor/oniguruma/src/iso8859_3.c vendor/oniguruma/src/iso8859_4.c vendor/oniguruma/src/iso8859_5.c vendor/oniguruma/src/iso8859_6.c vendor/oniguruma/src/iso8859_7.c vendor/oniguruma/src/iso8859_8.c vendor/oniguruma/src/iso8859_9.c vendor/oniguruma/src/koi8_r.c vendor/oniguruma/src/onig_init.c vendor/oniguruma/src/regcomp.c vendor/oniguruma/src/regenc.c vendor/oniguruma/src/regerror.c vendor/oniguruma/src/regexec.c vendor/oniguruma/src/regext.c vendor/oniguruma/src/reggnu.c vendor/oniguruma/src/regparse.c vendor/oniguruma/src/regsyntax.c vendor/oniguruma/src/regtrav.c vendor/oniguruma/src/regversion.c vendor/oniguruma/src/sjis.c vendor/oniguruma/src/sjis_prop.c vendor/oniguruma/src/st.c vendor/oniguruma/src/unicode.c vendor/oniguruma/src/unicode_fold1_key.c vendor/oniguruma/src/unicode_fold2_key.c vendor/oniguruma/src/unicode_fold3_key.c vendor/oniguruma/src/unicode_unfold_key.c vendor/oniguruma/src/utf16_be.c vendor/oniguruma/src/utf16_le.c vendor/oniguruma/src/utf32_be.c vendor/oniguruma/src/utf32_le.c vendor/oniguruma/src/utf8.c
OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(basename $(SOURCES))))
.PHONY: all
all: $(OUT)/libjq.a $(OUT)/jq
$(OUT)/libjq.a: $(OBJECTS)
	rm -f $@
	$(AR) rcs $@ $^
$(OUT)/jq: $(OUT)/obj/src/main.o $(OUT)/obj/src/jq_test.o $(OUT)/libjq.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
