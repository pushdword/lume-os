NATIVE_ZLIB_SOURCES := adler32 compress crc32 deflate gzclose gzlib gzread gzwrite \
    infback inffast inflate inftrees trees uncompr zutil
NATIVE_ZLIB_OBJECTS := $(addprefix $(OUT)/obj/vendor/zlib/,$(addsuffix .c.o,$(NATIVE_ZLIB_SOURCES)))

$(NATIVE_ZLIB_OBJECTS): CPPFLAGS += -DHAVE_UNISTD_H -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L
$(NATIVE_ZLIB_OBJECTS): mk/zlib-native.mk | headers
$(OUT)/libz.a: $(NATIVE_ZLIB_OBJECTS)
	rm -f "$@.tmp"
	"$(TOOLBIN)llvm-ar$(HOSTEXE)" rcsD "$@.tmp" $^
	mv -f "$@.tmp" "$@"

-include $(NATIVE_ZLIB_OBJECTS:=.d)
