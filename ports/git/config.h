#ifndef LUME_GIT_CONFIG_H
#define LUME_GIT_CONFIG_H
#include "lume/target.h"
#define NO_OPENSSL 1
#define NO_GETTEXT 1
#define NO_ICONV 1
#define NO_SETITIMER 1
#define USE_NSEC 1
#define HAVE_GETRANDOM 1
#define NO_STRCASESTR 1
#define NO_STRLCPY 1
#define NO_STRTOUMAX 1
#define NO_MEMMEM 1
#define HAVE_STRINGS_H 1
#define NO_MBSUPPORT 1
#define SHA1_DC 1
#define SHA1DC_NO_STANDARD_INCLUDES 1
#define SHA1DC_INIT_SAFE_HASH_DEFAULT 0
#define SHA1DC_CUSTOM_INCLUDE_SHA1_C "git-compat-util.h"
#define SHA1DC_CUSTOM_INCLUDE_UBC_CHECK_C "git-compat-util.h"
#define SHA256_BLK 1
#define GIT_HOST_CPU LUME_ARCH_NAME
#define SHELL_PATH "/bin/bash"
#define DEFAULT_GIT_TEMPLATE_DIR "/data/share/git/templates"
#define GIT_EXEC_PATH "/data/libexec/git-core"
#define BINDIR "/data/bin"
#define PREFIX "/data"
#define ETC_GITCONFIG "/data/etc/gitconfig"
#define ETC_GITATTRIBUTES "/data/etc/gitattributes"
#define FALLBACK_RUNTIME_PREFIX "/data"
#define GIT_HTML_PATH "/data/share/git"
#define GIT_MAN_PATH "/data/share/git"
#define GIT_INFO_PATH "/data/share/git"
#define PAGER_ENV "LESS=FRX"
#endif
