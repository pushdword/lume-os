SOURCES := lib/access.c lib/copy-acl.c lib/set-acl.c lib/acl-errno-valid.c lib/acl-internal.c lib/get-permissions.c lib/set-permissions.c lib/alignalloc.c lib/allocator.c lib/areadlink.c lib/areadlink-with-size.c lib/areadlinkat.c lib/areadlinkat-with-size.c lib/argmatch.c lib/argv-iter.c lib/arpa_inet.c lib/openat-proc.c lib/backupfile.c lib/backup-rename.c lib/backup-find.c lib/base32.c lib/base64.c lib/basename-lgpl.c lib/binary-io.c lib/btoc32.c lib/btowc.c lib/buffer-lcm.c lib/c-ctype.c lib/c-file-type.c lib/c-strcasecmp.c lib/c-strncasecmp.c lib/c-strtod.c lib/c-strtold.c lib/c32_apply_type_test.c lib/c32_get_type_test.c lib/c32isalnum.c lib/c32isalpha.c lib/c32isblank.c lib/c32iscntrl.c lib/c32isdigit.c lib/c32isgraph.c lib/c32islower.c lib/c32isprint.c lib/c32ispunct.c lib/c32isspace.c lib/c32isupper.c lib/c32isxdigit.c lib/c32tolower.c lib/c32toupper.c lib/c32width.c lib/canon-host.c lib/canonicalize.c lib/careadlinkat.c lib/chdir-long.c lib/chmod.c lib/chmodat.c lib/chown.c lib/chownat.c lib/cl-strtod.c lib/cl-strtold.c lib/cloexec.c lib/close.c lib/close-stream.c lib/closedir.c lib/closein.c lib/closeout.c lib/concat-filename.c lib/copy-file-range.c lib/cpu-supports.c lib/crc.c lib/md5-stream.c lib/md5.c lib/sha1-stream.c lib/sha1.c lib/sha256-stream.c lib/sha256.c lib/sha3-stream.c lib/sha3.c lib/sha512-stream.c lib/sha512.c lib/sm3-stream.c lib/sm3.c lib/cycle-check.c lib/di-set.c lib/dirname.c lib/basename.c lib/dirname-lgpl.c lib/stripslash.c lib/dtimespec-bound.c lib/dtoastr.c lib/dtotimespec.c lib/dup.c lib/dup2.c lib/endian.c lib/errno-iter.c lib/error.c lib/euidaccess.c lib/exclude.c lib/execvpe.c lib/exitfail.c lib/faccessat.c lib/fadvise.c lib/fchdir.c lib/fchmodat.c lib/fchownat.c lib/fclose.c lib/fcntl.c lib/creat-safer.c lib/open-safer.c lib/fd-hook.c lib/fd-reopen.c lib/fd-safer-flag.c lib/dup-safer-flag.c lib/fdatasync.c lib/fdopendir.c lib/fdutimensat.c lib/fflush.c lib/file-has-acl.c lib/file-set.c lib/file-type.c lib/filemode.c lib/filenamecat.c lib/filenamecat-lgpl.c lib/filesystem-remote.c lib/filevercmp.c lib/findprog-in.c lib/float.c lib/fopen-safer.c lib/fpending.c lib/fprintftime.c lib/fpurge.c lib/freadahead.c lib/freading.c lib/free.c lib/freopen-safer.c lib/fseek.c lib/fseeko.c lib/fseterr.c lib/fstat.c lib/fstatat.c lib/fsusage.c lib/ftoastr.c lib/fts.c lib/full-read.c lib/full-write.c lib/futimens.c lib/getcwd.c lib/getcwd-lgpl.c lib/getdelim.c lib/getgroups.c lib/gethrxtime.c lib/xtime.c lib/getline.c lib/localename-table.c lib/getlocalename_l-unsafe.c lib/getopt.c lib/getopt1.c lib/gettime.c lib/gettime-res.c lib/getugroups.c lib/malloc/dynarray_at_failure.c lib/malloc/dynarray_emplace_enlarge.c lib/malloc/dynarray_finalize.c lib/malloc/dynarray_resize.c lib/malloc/dynarray_resize_clear.c lib/malloc/scratch_buffer_grow.c lib/malloc/scratch_buffer_grow_preserve.c lib/malloc/scratch_buffer_set_array_size.c lib/group-member.c lib/hard-locale.c lib/hash.c lib/hashcode-file-inode.c lib/hashcode-mem.c lib/hashcode-named-file.c lib/hashcode-string1.c lib/hashcode-string2.c lib/heap.c lib/human.c lib/i-ring.c lib/ialloc.c lib/idcache.c lib/ino-map.c lib/imaxtostr.c lib/inttostr.c lib/offtostr.c lib/uinttostr.c lib/umaxtostr.c lib/isapipe.c lib/issymlink.c lib/issymlinkat.c lib/lchmod.c lib/lchown.c lib/ldtoastr.c lib/mini-mpq-gnulib.c lib/mini-gmp-gnulib.c lib/linebuffer.c lib/link.c lib/linkat.c lib/localcharset.c lib/localeconv.c lib/localeinfo.c lib/localename-environ.c lib/localename-unsafe.c lib/glthread/lock.c lib/long-options.c lib/lstat.c lib/malloca.c lib/math.c lib/mbbuf.c lib/mbrlen.c lib/mbrtoc32.c lib/mbrtowc.c lib/mbs_endswith.c lib/mbscasecmp.c lib/mbschr.c lib/mbslen.c lib/mbsnlen.c lib/mbsrtoc32s.c lib/mbsrtowcs.c lib/mbsstr.c lib/mbswidth.c lib/mbszero.c lib/mcel.c lib/memcasecmp.c lib/memchr.c lib/memchr2.c lib/memcmp2.c lib/memcoll.c lib/memeq.c lib/mgetgroups.c lib/mkancesdirs.c lib/mkdir.c lib/dirchownmod.c lib/mkdir-p.c lib/mkfifo.c lib/mkfifoat.c lib/mknodat.c lib/mknod.c lib/mkostemp.c lib/mkstemp.c lib/modechange.c lib/mountlist.c lib/mpsort.c lib/nanosleep.c lib/next-prime.c lib/nl_langinfo.c lib/nproc.c lib/nstrftime.c lib/obstack.c lib/glthread/once.c lib/open.c lib/openat.c lib/openat-die.c lib/openat-safer.c lib/opendir.c lib/opendirat.c lib/parse-datetime.c lib/physmem.c lib/pipe2.c lib/pipe2-safer.c lib/posix_memalign.c lib/spawn.c lib/spawni.c lib/spawn_faction_addclose.c lib/spawn_faction_adddup2.c lib/spawn_faction_destroy.c lib/spawn_faction_init.c lib/spawnattr_destroy.c lib/spawnattr_init.c lib/spawnattr_setflags.c lib/spawnattr_setdefault.c lib/spawnp.c lib/posixtm.c lib/posixver.c lib/printf-frexp.c lib/printf-frexpl.c lib/priv-set.c lib/progname.c lib/propername-lite.c lib/pselect.c lib/pthread-mutex.c lib/pthread_mutex_timedlock.c lib/putenv.c lib/qcopy-acl.c lib/qset-acl.c lib/quotearg.c lib/randint.c lib/randperm.c lib/randread.c lib/rand-isaac.c lib/read-file.c lib/readlink.c lib/readlinkat.c lib/readtokens.c lib/readtokens0.c lib/readutmp.c lib/realloc.c lib/reallocarray.c lib/regex.c lib/rename.c lib/renameat.c lib/renameatu.c lib/rmdir.c lib/root-dev-ino.c lib/safe-read.c lib/safe-write.c lib/same.c lib/same-inode.c lib/save-cwd.c lib/savedir.c lib/savewd.c lib/secure_getenv.c lib/select.c lib/selinux-at.c lib/se-context.c lib/se-label.c lib/se-selinux.c lib/setenv.c lib/setlocale_null.c lib/setlocale-lock.c lib/setlocale_null-unlocked.c lib/settime.c lib/sig-handler.c lib/signbitf.c lib/signbitd.c lib/signbitl.c lib/skipchars.c lib/sockets.c lib/stat.c lib/stat-time.c lib/stdc_bit_width.c lib/stdc_leading_zeros.c lib/stdc_memreverse8u.c lib/stdc_rotate_left.c lib/stdc_rotate_right.c lib/stdc_trailing_zeros.c lib/stdlib.c lib/mkstemp-safer.c lib/str_endswith.c lib/streq.c lib/strerror.c lib/strerror-override.c lib/strerrorname_np.c lib/strnlen1.c lib/strnul.c lib/strintcmp.c lib/strnumcmp.c lib/strtod.c lib/strtold.c lib/strtoll.c lib/strtoull.c lib/symlink.c lib/symlinkat.c lib/sys_socket.c lib/targetdir.c lib/tempname.c lib/thread-optim.c lib/glthread/threadlib.c lib/time_rz.c lib/timegm.c lib/timespec.c lib/glthread/tls.c lib/tmpdir.c lib/u64.c lib/unicase/tolower.c lib/unicase/toupper.c lib/unicodeio.c lib/unictype/ctype_alnum.c lib/unictype/ctype_alpha.c lib/unictype/ctype_blank.c lib/unictype/ctype_cntrl.c lib/unictype/ctype_digit.c lib/unictype/ctype_graph.c lib/unictype/ctype_lower.c lib/unictype/ctype_print.c lib/unictype/ctype_punct.c lib/unictype/ctype_space.c lib/unictype/ctype_upper.c lib/unictype/ctype_xdigit.c lib/unistd.c lib/dup-safer.c lib/fd-safer.c lib/pipe-safer.c lib/unistr/u32-chr.c lib/unistr/u32-cpy.c lib/unistr/u32-pcpy.c lib/unistr/u32-strcat.c lib/unistr/u32-strlen.c lib/unistr/u8-mbtoucr.c lib/unistr/u8-uctomb.c lib/unistr/u8-uctomb-aux.c lib/uniwidth/width.c lib/unlink.c lib/unlinkat.c lib/unlinkdir.c lib/unsetenv.c lib/userspec.c lib/utime.c lib/utimecmp.c lib/utimens.c lib/utimensat.c lib/vaszprintf.c lib/aszprintf.c lib/version-etc.c lib/version-etc-fsf.c lib/vfzprintf.c lib/vsnzprintf.c lib/vszprintf.c lib/vzprintf.c lib/wctype-h.c lib/wcwidth.c lib/write-any-file.c lib/xalignalloc.c lib/xmalloc.c lib/xalloc-die.c lib/xbinary-io.c lib/xdectoimax.c lib/xdectoumax.c lib/xfts.c lib/xgetcwd.c lib/xgetgroups.c lib/xgethostname.c lib/xmemcoll.c lib/xmemdup0.c lib/xnanosleep.c lib/xprintf.c lib/xreadlink.c lib/xsetenv.c lib/xsize.c lib/xstrtod.c lib/xstrtoimax.c lib/xstrtol.c lib/xstrtoul.c lib/xstrtol-error.c lib/xstrtold.c lib/xstrtoumax.c lib/xvasprintf.c lib/xasprintf.c lib/yesno.c lib/asnprintf.c lib/asprintf.c lib/at-func2.c lib/calloc.c lib/fnmatch.c lib/fopen.c lib/malloc.c lib/mbsrtoc32s-state.c lib/mbsrtowcs-state.c lib/memmem.c lib/mktime.c lib/printf.c lib/printf-args.c lib/printf-parse.c lib/snprintf.c lib/sprintf.c lib/strstr.c lib/vasnprintf.c lib/vasprintf.c lib/vfprintf.c src/version.c
OBJECTS := $(addprefix $(OUT)/obj/,$(SOURCES:.c=.o))
PROGRAMS := chroot stty [ b2sum base64 base32 basenc basename cat chgrp chmod chown cksum comm cp csplit cut date dd dir dircolors dirname du echo env expand expr factor false fmt fold ginstall groups head id join link ln logname ls md5sum mkdir mkfifo mknod mktemp mv nl nproc nohup numfmt od paste pathchk pr printenv printf ptx pwd readlink realpath rm rmdir seq sha1sum sha224sum sha256sum sha384sum sha512sum shred shuf sleep sort split stat sum sync tac tail tee test touch tr true truncate tsort tty uname unexpand uniq unlink vdir wc whoami yes
$(OUT)/chroot: $(OUT)/programs/chroot/src/chroot.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/chroot/src/chroot.o: $(SOURCE)/src/chroot.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/chroot/src/chroot.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/chroot/src/chroot.d
$(OUT)/stty: $(OUT)/programs/stty/src/stty.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/stty/src/stty.o: $(SOURCE)/src/stty.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/stty/src/stty.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/stty/src/stty.d
$(OUT)/[: $(OUT)/programs/[/src/test.o $(OUT)/programs/[/src/test-lbracket.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/[/src/test.o: $(SOURCE)/src/test.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/[/src/test.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/[/src/test.d
$(OUT)/programs/[/src/test-lbracket.o: $(SOURCE)/src/test-lbracket.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/[/src/test-lbracket.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/[/src/test-lbracket.d
$(OUT)/b2sum: $(OUT)/programs/b2sum/src/cksum.o $(OUT)/programs/b2sum/src/blake2/blake2b-ref.o $(OUT)/programs/b2sum/src/blake2/b2sum.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/b2sum/src/cksum.o: $(SOURCE)/src/cksum.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DHASH_ALGO_BLAKE2=1 $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/b2sum/src/cksum.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/b2sum/src/cksum.d
$(OUT)/programs/b2sum/src/blake2/blake2b-ref.o: $(SOURCE)/src/blake2/blake2b-ref.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DHASH_ALGO_BLAKE2=1 $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/b2sum/src/blake2/blake2b-ref.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/b2sum/src/blake2/blake2b-ref.d
$(OUT)/programs/b2sum/src/blake2/b2sum.o: $(SOURCE)/src/blake2/b2sum.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DHASH_ALGO_BLAKE2=1 $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/b2sum/src/blake2/b2sum.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/b2sum/src/blake2/b2sum.d
$(OUT)/base64: $(OUT)/programs/base64/src/basenc.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/base64/src/basenc.o: $(SOURCE)/src/basenc.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DBASE_TYPE=64 $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/base64/src/basenc.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/base64/src/basenc.d
$(OUT)/base32: $(OUT)/programs/base32/src/basenc.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/base32/src/basenc.o: $(SOURCE)/src/basenc.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DBASE_TYPE=32 $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/base32/src/basenc.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/base32/src/basenc.d
$(OUT)/basenc: $(OUT)/programs/basenc/src/basenc.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/basenc/src/basenc.o: $(SOURCE)/src/basenc.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DBASE_TYPE=42 $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/basenc/src/basenc.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/basenc/src/basenc.d
$(OUT)/basename: $(OUT)/programs/basename/src/basename.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/basename/src/basename.o: $(SOURCE)/src/basename.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/basename/src/basename.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/basename/src/basename.d
$(OUT)/cat: $(OUT)/programs/cat/src/cat.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/cat/src/cat.o: $(SOURCE)/src/cat.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/cat/src/cat.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/cat/src/cat.d
$(OUT)/chgrp: $(OUT)/programs/chgrp/src/chown.o $(OUT)/programs/chgrp/src/chown-core.o $(OUT)/programs/chgrp/src/chown-chgrp.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/chgrp/src/chown.o: $(SOURCE)/src/chown.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/chgrp/src/chown.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/chgrp/src/chown.d
$(OUT)/programs/chgrp/src/chown-core.o: $(SOURCE)/src/chown-core.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/chgrp/src/chown-core.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/chgrp/src/chown-core.d
$(OUT)/programs/chgrp/src/chown-chgrp.o: $(SOURCE)/src/chown-chgrp.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/chgrp/src/chown-chgrp.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/chgrp/src/chown-chgrp.d
$(OUT)/chmod: $(OUT)/programs/chmod/src/chmod.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/chmod/src/chmod.o: $(SOURCE)/src/chmod.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/chmod/src/chmod.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/chmod/src/chmod.d
$(OUT)/chown: $(OUT)/programs/chown/src/chown.o $(OUT)/programs/chown/src/chown-core.o $(OUT)/programs/chown/src/chown-chown.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/chown/src/chown.o: $(SOURCE)/src/chown.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/chown/src/chown.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/chown/src/chown.d
$(OUT)/programs/chown/src/chown-core.o: $(SOURCE)/src/chown-core.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/chown/src/chown-core.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/chown/src/chown-core.d
$(OUT)/programs/chown/src/chown-chown.o: $(SOURCE)/src/chown-chown.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/chown/src/chown-chown.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/chown/src/chown-chown.d
$(OUT)/cksum: $(OUT)/programs/cksum/src/cksum.o $(OUT)/programs/cksum/src/blake2/blake2b-ref.o $(OUT)/programs/cksum/src/blake2/b2sum.o $(OUT)/programs/cksum/src/sum.o $(OUT)/programs/cksum/src/cksum_crc.o $(OUT)/programs/cksum/src/crctab.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/cksum/src/cksum.o: $(SOURCE)/src/cksum.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DHASH_ALGO_CKSUM=1 $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/cksum/src/cksum.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/cksum/src/cksum.d
$(OUT)/programs/cksum/src/blake2/blake2b-ref.o: $(SOURCE)/src/blake2/blake2b-ref.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DHASH_ALGO_CKSUM=1 $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/cksum/src/blake2/blake2b-ref.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/cksum/src/blake2/blake2b-ref.d
$(OUT)/programs/cksum/src/blake2/b2sum.o: $(SOURCE)/src/blake2/b2sum.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DHASH_ALGO_CKSUM=1 $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/cksum/src/blake2/b2sum.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/cksum/src/blake2/b2sum.d
$(OUT)/programs/cksum/src/sum.o: $(SOURCE)/src/sum.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DHASH_ALGO_CKSUM=1 $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/cksum/src/sum.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/cksum/src/sum.d
$(OUT)/programs/cksum/src/cksum_crc.o: $(SOURCE)/src/cksum_crc.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DHASH_ALGO_CKSUM=1 $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/cksum/src/cksum_crc.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/cksum/src/cksum_crc.d
$(OUT)/programs/cksum/src/crctab.o: $(SOURCE)/src/crctab.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DHASH_ALGO_CKSUM=1 $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/cksum/src/crctab.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/cksum/src/crctab.d
$(OUT)/comm: $(OUT)/programs/comm/src/comm.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/comm/src/comm.o: $(SOURCE)/src/comm.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/comm/src/comm.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/comm/src/comm.d
$(OUT)/cp: $(OUT)/programs/cp/src/cp.o $(OUT)/programs/cp/src/copy.o $(OUT)/programs/cp/src/copy-file-data.o $(OUT)/programs/cp/src/cp-hash.o $(OUT)/programs/cp/src/force-link.o $(OUT)/programs/cp/src/selinux.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/cp/src/cp.o: $(SOURCE)/src/cp.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/cp/src/cp.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/cp/src/cp.d
$(OUT)/programs/cp/src/copy.o: $(SOURCE)/src/copy.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/cp/src/copy.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/cp/src/copy.d
$(OUT)/programs/cp/src/copy-file-data.o: $(SOURCE)/src/copy-file-data.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/cp/src/copy-file-data.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/cp/src/copy-file-data.d
$(OUT)/programs/cp/src/cp-hash.o: $(SOURCE)/src/cp-hash.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/cp/src/cp-hash.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/cp/src/cp-hash.d
$(OUT)/programs/cp/src/force-link.o: $(SOURCE)/src/force-link.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/cp/src/force-link.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/cp/src/force-link.d
$(OUT)/programs/cp/src/selinux.o: $(SOURCE)/src/selinux.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/cp/src/selinux.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/cp/src/selinux.d
$(OUT)/csplit: $(OUT)/programs/csplit/src/csplit.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/csplit/src/csplit.o: $(SOURCE)/src/csplit.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/csplit/src/csplit.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/csplit/src/csplit.d
$(OUT)/cut: $(OUT)/programs/cut/src/cut.o $(OUT)/programs/cut/src/set-fields.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/cut/src/cut.o: $(SOURCE)/src/cut.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/cut/src/cut.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/cut/src/cut.d
$(OUT)/programs/cut/src/set-fields.o: $(SOURCE)/src/set-fields.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/cut/src/set-fields.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/cut/src/set-fields.d
$(OUT)/date: $(OUT)/programs/date/src/date.o $(OUT)/programs/date/src/show-date.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/date/src/date.o: $(SOURCE)/src/date.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/date/src/date.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/date/src/date.d
$(OUT)/programs/date/src/show-date.o: $(SOURCE)/src/show-date.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/date/src/show-date.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/date/src/show-date.d
$(OUT)/dd: $(OUT)/programs/dd/src/dd.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/dd/src/dd.o: $(SOURCE)/src/dd.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/dd/src/dd.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/dd/src/dd.d
$(OUT)/dir: $(OUT)/programs/dir/src/ls.o $(OUT)/programs/dir/src/ls-dir.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/dir/src/ls.o: $(SOURCE)/src/ls.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/dir/src/ls.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/dir/src/ls.d
$(OUT)/programs/dir/src/ls-dir.o: $(SOURCE)/src/ls-dir.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/dir/src/ls-dir.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/dir/src/ls-dir.d
$(OUT)/dircolors: $(OUT)/programs/dircolors/src/dircolors.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/dircolors/src/dircolors.o: $(SOURCE)/src/dircolors.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/dircolors/src/dircolors.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/dircolors/src/dircolors.d
$(OUT)/dirname: $(OUT)/programs/dirname/src/dirname.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/dirname/src/dirname.o: $(SOURCE)/src/dirname.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/dirname/src/dirname.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/dirname/src/dirname.d
$(OUT)/du: $(OUT)/programs/du/src/du.o $(OUT)/programs/du/src/show-date.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/du/src/du.o: $(SOURCE)/src/du.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/du/src/du.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/du/src/du.d
$(OUT)/programs/du/src/show-date.o: $(SOURCE)/src/show-date.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/du/src/show-date.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/du/src/show-date.d
$(OUT)/echo: $(OUT)/programs/echo/src/echo.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/echo/src/echo.o: $(SOURCE)/src/echo.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/echo/src/echo.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/echo/src/echo.d
$(OUT)/env: $(OUT)/programs/env/src/env.o $(OUT)/programs/env/src/operand2sig.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/env/src/env.o: $(SOURCE)/src/env.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/env/src/env.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/env/src/env.d
$(OUT)/programs/env/src/operand2sig.o: $(SOURCE)/src/operand2sig.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/env/src/operand2sig.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/env/src/operand2sig.d
$(OUT)/expand: $(OUT)/programs/expand/src/expand.o $(OUT)/programs/expand/src/expand-common.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/expand/src/expand.o: $(SOURCE)/src/expand.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/expand/src/expand.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/expand/src/expand.d
$(OUT)/programs/expand/src/expand-common.o: $(SOURCE)/src/expand-common.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/expand/src/expand-common.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/expand/src/expand-common.d
$(OUT)/expr: $(OUT)/programs/expr/src/expr.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/expr/src/expr.o: $(SOURCE)/src/expr.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/expr/src/expr.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/expr/src/expr.d
$(OUT)/factor: $(OUT)/programs/factor/src/factor.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/factor/src/factor.o: $(SOURCE)/src/factor.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/factor/src/factor.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/factor/src/factor.d
$(OUT)/false: $(OUT)/programs/false/src/true.o $(OUT)/programs/false/src/true-false.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/false/src/true.o: $(SOURCE)/src/true.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/false/src/true.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/false/src/true.d
$(OUT)/programs/false/src/true-false.o: $(SOURCE)/src/true-false.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/false/src/true-false.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/false/src/true-false.d
$(OUT)/fmt: $(OUT)/programs/fmt/src/fmt.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/fmt/src/fmt.o: $(SOURCE)/src/fmt.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/fmt/src/fmt.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/fmt/src/fmt.d
$(OUT)/fold: $(OUT)/programs/fold/src/fold.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/fold/src/fold.o: $(SOURCE)/src/fold.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/fold/src/fold.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/fold/src/fold.d
$(OUT)/ginstall: $(OUT)/programs/ginstall/src/install.o $(OUT)/programs/ginstall/src/prog-fprintf.o $(OUT)/programs/ginstall/src/copy.o $(OUT)/programs/ginstall/src/copy-file-data.o $(OUT)/programs/ginstall/src/cp-hash.o $(OUT)/programs/ginstall/src/force-link.o $(OUT)/programs/ginstall/src/selinux.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/ginstall/src/install.o: $(SOURCE)/src/install.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/ginstall/src/install.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/ginstall/src/install.d
$(OUT)/programs/ginstall/src/prog-fprintf.o: $(SOURCE)/src/prog-fprintf.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/ginstall/src/prog-fprintf.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/ginstall/src/prog-fprintf.d
$(OUT)/programs/ginstall/src/copy.o: $(SOURCE)/src/copy.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/ginstall/src/copy.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/ginstall/src/copy.d
$(OUT)/programs/ginstall/src/copy-file-data.o: $(SOURCE)/src/copy-file-data.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/ginstall/src/copy-file-data.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/ginstall/src/copy-file-data.d
$(OUT)/programs/ginstall/src/cp-hash.o: $(SOURCE)/src/cp-hash.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/ginstall/src/cp-hash.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/ginstall/src/cp-hash.d
$(OUT)/programs/ginstall/src/force-link.o: $(SOURCE)/src/force-link.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/ginstall/src/force-link.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/ginstall/src/force-link.d
$(OUT)/programs/ginstall/src/selinux.o: $(SOURCE)/src/selinux.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/ginstall/src/selinux.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/ginstall/src/selinux.d
$(OUT)/groups: $(OUT)/programs/groups/src/groups.o $(OUT)/programs/groups/src/group-list.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/groups/src/groups.o: $(SOURCE)/src/groups.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/groups/src/groups.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/groups/src/groups.d
$(OUT)/programs/groups/src/group-list.o: $(SOURCE)/src/group-list.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/groups/src/group-list.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/groups/src/group-list.d
$(OUT)/head: $(OUT)/programs/head/src/head.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/head/src/head.o: $(SOURCE)/src/head.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/head/src/head.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/head/src/head.d
$(OUT)/id: $(OUT)/programs/id/src/id.o $(OUT)/programs/id/src/group-list.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/id/src/id.o: $(SOURCE)/src/id.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/id/src/id.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/id/src/id.d
$(OUT)/programs/id/src/group-list.o: $(SOURCE)/src/group-list.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/id/src/group-list.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/id/src/group-list.d
$(OUT)/join: $(OUT)/programs/join/src/join.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/join/src/join.o: $(SOURCE)/src/join.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/join/src/join.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/join/src/join.d
$(OUT)/link: $(OUT)/programs/link/src/link.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/link/src/link.o: $(SOURCE)/src/link.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/link/src/link.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/link/src/link.d
$(OUT)/ln: $(OUT)/programs/ln/src/ln.o $(OUT)/programs/ln/src/force-link.o $(OUT)/programs/ln/src/relpath.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/ln/src/ln.o: $(SOURCE)/src/ln.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/ln/src/ln.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/ln/src/ln.d
$(OUT)/programs/ln/src/force-link.o: $(SOURCE)/src/force-link.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/ln/src/force-link.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/ln/src/force-link.d
$(OUT)/programs/ln/src/relpath.o: $(SOURCE)/src/relpath.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/ln/src/relpath.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/ln/src/relpath.d
$(OUT)/logname: $(OUT)/programs/logname/src/logname.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/logname/src/logname.o: $(SOURCE)/src/logname.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/logname/src/logname.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/logname/src/logname.d
$(OUT)/ls: $(OUT)/programs/ls/src/ls.o $(OUT)/programs/ls/src/ls-ls.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/ls/src/ls.o: $(SOURCE)/src/ls.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/ls/src/ls.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/ls/src/ls.d
$(OUT)/programs/ls/src/ls-ls.o: $(SOURCE)/src/ls-ls.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/ls/src/ls-ls.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/ls/src/ls-ls.d
$(OUT)/md5sum: $(OUT)/programs/md5sum/src/cksum.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/md5sum/src/cksum.o: $(SOURCE)/src/cksum.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DHASH_ALGO_MD5=1 $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/md5sum/src/cksum.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/md5sum/src/cksum.d
$(OUT)/mkdir: $(OUT)/programs/mkdir/src/mkdir.o $(OUT)/programs/mkdir/src/prog-fprintf.o $(OUT)/programs/mkdir/src/selinux.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/mkdir/src/mkdir.o: $(SOURCE)/src/mkdir.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/mkdir/src/mkdir.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/mkdir/src/mkdir.d
$(OUT)/programs/mkdir/src/prog-fprintf.o: $(SOURCE)/src/prog-fprintf.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/mkdir/src/prog-fprintf.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/mkdir/src/prog-fprintf.d
$(OUT)/programs/mkdir/src/selinux.o: $(SOURCE)/src/selinux.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/mkdir/src/selinux.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/mkdir/src/selinux.d
$(OUT)/mkfifo: $(OUT)/programs/mkfifo/src/mkfifo.o $(OUT)/programs/mkfifo/src/selinux.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/mkfifo/src/mkfifo.o: $(SOURCE)/src/mkfifo.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/mkfifo/src/mkfifo.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/mkfifo/src/mkfifo.d
$(OUT)/programs/mkfifo/src/selinux.o: $(SOURCE)/src/selinux.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/mkfifo/src/selinux.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/mkfifo/src/selinux.d
$(OUT)/mknod: $(OUT)/programs/mknod/src/mknod.o $(OUT)/programs/mknod/src/selinux.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/mknod/src/mknod.o: $(SOURCE)/src/mknod.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/mknod/src/mknod.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/mknod/src/mknod.d
$(OUT)/programs/mknod/src/selinux.o: $(SOURCE)/src/selinux.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/mknod/src/selinux.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/mknod/src/selinux.d
$(OUT)/mktemp: $(OUT)/programs/mktemp/src/mktemp.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/mktemp/src/mktemp.o: $(SOURCE)/src/mktemp.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/mktemp/src/mktemp.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/mktemp/src/mktemp.d
$(OUT)/mv: $(OUT)/programs/mv/src/mv.o $(OUT)/programs/mv/src/remove.o $(OUT)/programs/mv/src/copy.o $(OUT)/programs/mv/src/copy-file-data.o $(OUT)/programs/mv/src/cp-hash.o $(OUT)/programs/mv/src/force-link.o $(OUT)/programs/mv/src/selinux.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/mv/src/mv.o: $(SOURCE)/src/mv.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/mv/src/mv.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/mv/src/mv.d
$(OUT)/programs/mv/src/remove.o: $(SOURCE)/src/remove.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/mv/src/remove.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/mv/src/remove.d
$(OUT)/programs/mv/src/copy.o: $(SOURCE)/src/copy.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/mv/src/copy.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/mv/src/copy.d
$(OUT)/programs/mv/src/copy-file-data.o: $(SOURCE)/src/copy-file-data.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/mv/src/copy-file-data.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/mv/src/copy-file-data.d
$(OUT)/programs/mv/src/cp-hash.o: $(SOURCE)/src/cp-hash.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/mv/src/cp-hash.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/mv/src/cp-hash.d
$(OUT)/programs/mv/src/force-link.o: $(SOURCE)/src/force-link.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/mv/src/force-link.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/mv/src/force-link.d
$(OUT)/programs/mv/src/selinux.o: $(SOURCE)/src/selinux.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/mv/src/selinux.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/mv/src/selinux.d
$(OUT)/nl: $(OUT)/programs/nl/src/nl.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/nl/src/nl.o: $(SOURCE)/src/nl.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/nl/src/nl.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/nl/src/nl.d
$(OUT)/nproc: $(OUT)/programs/nproc/src/nproc.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/nproc/src/nproc.o: $(SOURCE)/src/nproc.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/nproc/src/nproc.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/nproc/src/nproc.d
$(OUT)/nohup: $(OUT)/programs/nohup/src/nohup.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/nohup/src/nohup.o: $(SOURCE)/src/nohup.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/nohup/src/nohup.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/nohup/src/nohup.d
$(OUT)/numfmt: $(OUT)/programs/numfmt/src/numfmt.o $(OUT)/programs/numfmt/src/set-fields.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/numfmt/src/numfmt.o: $(SOURCE)/src/numfmt.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/numfmt/src/numfmt.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/numfmt/src/numfmt.d
$(OUT)/programs/numfmt/src/set-fields.o: $(SOURCE)/src/set-fields.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/numfmt/src/set-fields.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/numfmt/src/set-fields.d
$(OUT)/od: $(OUT)/programs/od/src/od.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/od/src/od.o: $(SOURCE)/src/od.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/od/src/od.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/od/src/od.d
$(OUT)/paste: $(OUT)/programs/paste/src/paste.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/paste/src/paste.o: $(SOURCE)/src/paste.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/paste/src/paste.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/paste/src/paste.d
$(OUT)/pathchk: $(OUT)/programs/pathchk/src/pathchk.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/pathchk/src/pathchk.o: $(SOURCE)/src/pathchk.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/pathchk/src/pathchk.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/pathchk/src/pathchk.d
$(OUT)/pr: $(OUT)/programs/pr/src/pr.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/pr/src/pr.o: $(SOURCE)/src/pr.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/pr/src/pr.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/pr/src/pr.d
$(OUT)/printenv: $(OUT)/programs/printenv/src/printenv.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/printenv/src/printenv.o: $(SOURCE)/src/printenv.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/printenv/src/printenv.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/printenv/src/printenv.d
$(OUT)/printf: $(OUT)/programs/printf/src/printf.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/printf/src/printf.o: $(SOURCE)/src/printf.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/printf/src/printf.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/printf/src/printf.d
$(OUT)/ptx: $(OUT)/programs/ptx/src/ptx.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/ptx/src/ptx.o: $(SOURCE)/src/ptx.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/ptx/src/ptx.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/ptx/src/ptx.d
$(OUT)/pwd: $(OUT)/programs/pwd/src/pwd.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/pwd/src/pwd.o: $(SOURCE)/src/pwd.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/pwd/src/pwd.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/pwd/src/pwd.d
$(OUT)/readlink: $(OUT)/programs/readlink/src/readlink.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/readlink/src/readlink.o: $(SOURCE)/src/readlink.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/readlink/src/readlink.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/readlink/src/readlink.d
$(OUT)/realpath: $(OUT)/programs/realpath/src/realpath.o $(OUT)/programs/realpath/src/relpath.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/realpath/src/realpath.o: $(SOURCE)/src/realpath.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/realpath/src/realpath.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/realpath/src/realpath.d
$(OUT)/programs/realpath/src/relpath.o: $(SOURCE)/src/relpath.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/realpath/src/relpath.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/realpath/src/relpath.d
$(OUT)/rm: $(OUT)/programs/rm/src/rm.o $(OUT)/programs/rm/src/remove.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/rm/src/rm.o: $(SOURCE)/src/rm.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/rm/src/rm.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/rm/src/rm.d
$(OUT)/programs/rm/src/remove.o: $(SOURCE)/src/remove.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/rm/src/remove.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/rm/src/remove.d
$(OUT)/rmdir: $(OUT)/programs/rmdir/src/rmdir.o $(OUT)/programs/rmdir/src/prog-fprintf.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/rmdir/src/rmdir.o: $(SOURCE)/src/rmdir.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/rmdir/src/rmdir.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/rmdir/src/rmdir.d
$(OUT)/programs/rmdir/src/prog-fprintf.o: $(SOURCE)/src/prog-fprintf.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/rmdir/src/prog-fprintf.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/rmdir/src/prog-fprintf.d
$(OUT)/seq: $(OUT)/programs/seq/src/seq.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/seq/src/seq.o: $(SOURCE)/src/seq.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/seq/src/seq.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/seq/src/seq.d
$(OUT)/sha1sum: $(OUT)/programs/sha1sum/src/cksum.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/sha1sum/src/cksum.o: $(SOURCE)/src/cksum.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DHASH_ALGO_SHA1=1 $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/sha1sum/src/cksum.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/sha1sum/src/cksum.d
$(OUT)/sha224sum: $(OUT)/programs/sha224sum/src/cksum.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/sha224sum/src/cksum.o: $(SOURCE)/src/cksum.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DHASH_ALGO_SHA224=1 $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/sha224sum/src/cksum.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/sha224sum/src/cksum.d
$(OUT)/sha256sum: $(OUT)/programs/sha256sum/src/cksum.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/sha256sum/src/cksum.o: $(SOURCE)/src/cksum.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DHASH_ALGO_SHA256=1 $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/sha256sum/src/cksum.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/sha256sum/src/cksum.d
$(OUT)/sha384sum: $(OUT)/programs/sha384sum/src/cksum.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/sha384sum/src/cksum.o: $(SOURCE)/src/cksum.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DHASH_ALGO_SHA384=1 $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/sha384sum/src/cksum.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/sha384sum/src/cksum.d
$(OUT)/sha512sum: $(OUT)/programs/sha512sum/src/cksum.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/sha512sum/src/cksum.o: $(SOURCE)/src/cksum.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DHASH_ALGO_SHA512=1 $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/sha512sum/src/cksum.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/sha512sum/src/cksum.d
$(OUT)/shred: $(OUT)/programs/shred/src/shred.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/shred/src/shred.o: $(SOURCE)/src/shred.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/shred/src/shred.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/shred/src/shred.d
$(OUT)/shuf: $(OUT)/programs/shuf/src/shuf.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/shuf/src/shuf.o: $(SOURCE)/src/shuf.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/shuf/src/shuf.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/shuf/src/shuf.d
$(OUT)/sleep: $(OUT)/programs/sleep/src/sleep.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/sleep/src/sleep.o: $(SOURCE)/src/sleep.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/sleep/src/sleep.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/sleep/src/sleep.d
$(OUT)/sort: $(OUT)/programs/sort/src/sort.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/sort/src/sort.o: $(SOURCE)/src/sort.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/sort/src/sort.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/sort/src/sort.d
$(OUT)/split: $(OUT)/programs/split/src/split.o $(OUT)/programs/split/src/temp-stream.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/split/src/split.o: $(SOURCE)/src/split.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/split/src/split.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/split/src/split.d
$(OUT)/programs/split/src/temp-stream.o: $(SOURCE)/src/temp-stream.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/split/src/temp-stream.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/split/src/temp-stream.d
$(OUT)/stat: $(OUT)/programs/stat/src/stat.o $(OUT)/programs/stat/src/find-mount-point.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/stat/src/stat.o: $(SOURCE)/src/stat.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/stat/src/stat.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/stat/src/stat.d
$(OUT)/programs/stat/src/find-mount-point.o: $(SOURCE)/src/find-mount-point.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/stat/src/find-mount-point.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/stat/src/find-mount-point.d
$(OUT)/sum: $(OUT)/programs/sum/src/sum.o $(OUT)/programs/sum/src/cksum.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/sum/src/sum.o: $(SOURCE)/src/sum.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DHASH_ALGO_SUM=1 $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/sum/src/sum.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/sum/src/sum.d
$(OUT)/programs/sum/src/cksum.o: $(SOURCE)/src/cksum.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DHASH_ALGO_SUM=1 $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/sum/src/cksum.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/sum/src/cksum.d
$(OUT)/sync: $(OUT)/programs/sync/src/sync.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/sync/src/sync.o: $(SOURCE)/src/sync.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/sync/src/sync.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/sync/src/sync.d
$(OUT)/tac: $(OUT)/programs/tac/src/tac.o $(OUT)/programs/tac/src/temp-stream.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/tac/src/tac.o: $(SOURCE)/src/tac.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/tac/src/tac.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/tac/src/tac.d
$(OUT)/programs/tac/src/temp-stream.o: $(SOURCE)/src/temp-stream.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/tac/src/temp-stream.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/tac/src/temp-stream.d
$(OUT)/tail: $(OUT)/programs/tail/src/tail.o $(OUT)/programs/tail/src/iopoll.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/tail/src/tail.o: $(SOURCE)/src/tail.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/tail/src/tail.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/tail/src/tail.d
$(OUT)/programs/tail/src/iopoll.o: $(SOURCE)/src/iopoll.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/tail/src/iopoll.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/tail/src/iopoll.d
$(OUT)/tee: $(OUT)/programs/tee/src/tee.o $(OUT)/programs/tee/src/iopoll.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/tee/src/tee.o: $(SOURCE)/src/tee.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/tee/src/tee.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/tee/src/tee.d
$(OUT)/programs/tee/src/iopoll.o: $(SOURCE)/src/iopoll.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/tee/src/iopoll.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/tee/src/iopoll.d
$(OUT)/test: $(OUT)/programs/test/src/test.o $(OUT)/programs/test/src/test-test.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/test/src/test.o: $(SOURCE)/src/test.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/test/src/test.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/test/src/test.d
$(OUT)/programs/test/src/test-test.o: $(SOURCE)/src/test-test.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/test/src/test-test.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/test/src/test-test.d
$(OUT)/touch: $(OUT)/programs/touch/src/touch.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/touch/src/touch.o: $(SOURCE)/src/touch.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/touch/src/touch.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/touch/src/touch.d
$(OUT)/tr: $(OUT)/programs/tr/src/tr.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/tr/src/tr.o: $(SOURCE)/src/tr.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/tr/src/tr.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/tr/src/tr.d
$(OUT)/true: $(OUT)/programs/true/src/true.o $(OUT)/programs/true/src/true-true.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/true/src/true.o: $(SOURCE)/src/true.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/true/src/true.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/true/src/true.d
$(OUT)/programs/true/src/true-true.o: $(SOURCE)/src/true-true.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/true/src/true-true.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/true/src/true-true.d
$(OUT)/truncate: $(OUT)/programs/truncate/src/truncate.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/truncate/src/truncate.o: $(SOURCE)/src/truncate.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/truncate/src/truncate.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/truncate/src/truncate.d
$(OUT)/tsort: $(OUT)/programs/tsort/src/tsort.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/tsort/src/tsort.o: $(SOURCE)/src/tsort.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/tsort/src/tsort.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/tsort/src/tsort.d
$(OUT)/tty: $(OUT)/programs/tty/src/tty.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/tty/src/tty.o: $(SOURCE)/src/tty.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/tty/src/tty.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/tty/src/tty.d
$(OUT)/uname: $(OUT)/programs/uname/src/uname.o $(OUT)/programs/uname/src/uname-uname.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/uname/src/uname.o: $(SOURCE)/src/uname.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/uname/src/uname.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/uname/src/uname.d
$(OUT)/programs/uname/src/uname-uname.o: $(SOURCE)/src/uname-uname.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/uname/src/uname-uname.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/uname/src/uname-uname.d
$(OUT)/unexpand: $(OUT)/programs/unexpand/src/unexpand.o $(OUT)/programs/unexpand/src/expand-common.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/unexpand/src/unexpand.o: $(SOURCE)/src/unexpand.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/unexpand/src/unexpand.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/unexpand/src/unexpand.d
$(OUT)/programs/unexpand/src/expand-common.o: $(SOURCE)/src/expand-common.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/unexpand/src/expand-common.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/unexpand/src/expand-common.d
$(OUT)/uniq: $(OUT)/programs/uniq/src/uniq.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/uniq/src/uniq.o: $(SOURCE)/src/uniq.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/uniq/src/uniq.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/uniq/src/uniq.d
$(OUT)/unlink: $(OUT)/programs/unlink/src/unlink.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/unlink/src/unlink.o: $(SOURCE)/src/unlink.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/unlink/src/unlink.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/unlink/src/unlink.d
$(OUT)/vdir: $(OUT)/programs/vdir/src/ls.o $(OUT)/programs/vdir/src/ls-vdir.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/vdir/src/ls.o: $(SOURCE)/src/ls.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/vdir/src/ls.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/vdir/src/ls.d
$(OUT)/programs/vdir/src/ls-vdir.o: $(SOURCE)/src/ls-vdir.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/vdir/src/ls-vdir.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/vdir/src/ls-vdir.d
$(OUT)/wc: $(OUT)/programs/wc/src/wc.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/wc/src/wc.o: $(SOURCE)/src/wc.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/wc/src/wc.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/wc/src/wc.d
$(OUT)/whoami: $(OUT)/programs/whoami/src/whoami.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/whoami/src/whoami.o: $(SOURCE)/src/whoami.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/whoami/src/whoami.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/whoami/src/whoami.d
$(OUT)/yes: $(OUT)/programs/yes/src/yes.o $(OUT)/libcoreutils.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
$(OUT)/programs/yes/src/yes.o: $(SOURCE)/src/yes.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS)  $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/programs/yes/src/yes.o: $(PORT)/Makefile $(CONFIG)/build.mk $(PORT)/compat.h $(PORT)/stdio_support.h
-include $(OUT)/programs/yes/src/yes.d
