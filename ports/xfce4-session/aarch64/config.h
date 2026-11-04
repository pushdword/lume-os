/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* backend */
#define BACKEND "lume"

/* FreeBSD suspend/hibernate backend */
/* #undef BACKEND_TYPE_FREEBSD */

/* Linux suspend/hibernate/hybrid-sleep backend */
/* #undef BACKEND_TYPE_LINUX */

/* OpenBSD suspend/hibernate backend */
/* #undef BACKEND_TYPE_OPENBSD */

/* Solaris suspend/hibernate backend */
/* #undef BACKEND_TYPE_SOLARIS */

/* Copyright year */
#define COPYRIGHT_YEAR "2026"

/* Define for debugging support */
/* #undef DEBUG */

/* Define for tracing support */
/* #undef DEBUG_TRACE */

/* Define for X11R5 sm compat */
/* #undef ENABLE_LEGACY_SESSION_MANAGEMENT */

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
/* #undef ENABLE_NLS */

/* Define if m4_default(the Wayland windowing system, wayland) is enabled */
/* #undef ENABLE_WAYLAND */

/* Define if m4_default(the X11 windowing system, x11) is enabled */
#define ENABLE_X11 1

/* Name of default gettext domain */
#define GETTEXT_PACKAGE "xfce4-session"

/* Prevent post 2_72 APIs */
#define GLIB_VERSION_MAX_ALLOWED GLIB_VERSION_2_72

/* Ignore post 2_72 APIs */
#define GLIB_VERSION_MIN_REQUIRED GLIB_VERSION_2_72

/* Use GLib structured logging */
#define G_LOG_USE_STRUCTURED 1

/* Define to 1 if you have the Mac OS X function CFLocaleCopyCurrent in the
   CoreFoundation framework. */
/* #undef HAVE_CFLOCALECOPYCURRENT */

/* Define to 1 if you have the Mac OS X function CFPreferencesCopyAppValue in
   the CoreFoundation framework. */
/* #undef HAVE_CFPREFERENCESCOPYAPPVALUE */

/* Define if the GNU dcgettext() function is already present or preinstalled.
   */
/* #undef HAVE_DCGETTEXT */

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define if gdk-wayland-3.0 >= gtk_min_version present */
/* #undef HAVE_GDK_WAYLAND */

/* Define if gdk-x11-3.0 >= gtk_min_version present */
#define HAVE_GDK_X11 1

/* Define to 1 if you have the 'getaddrinfo' function. */
#define HAVE_GETADDRINFO 1

/* Define to 1 if you have the 'gethostbyname' function. */
#define HAVE_GETHOSTBYNAME 1

/* Define to 1 if you have the 'gethostname' function. */
#define HAVE_GETHOSTNAME 1

/* Define if the GNU gettext() function is already present or preinstalled. */
/* #undef HAVE_GETTEXT */

/* Define if gtk-layer-shell-0 >= gtk_layer_shell_min_version present */
/* #undef HAVE_GTK_LAYER_SHELL */

/* Define if you have the iconv() function and it works. */
/* #undef HAVE_ICONV */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define if ice >= libice_min_version present */
#define HAVE_LIBICE 1

/* Define if sm >= libsm_min_version present */
#define HAVE_LIBSM 1

/* Define if libwnck-3.0 >= libwnck_min_version present */
#define HAVE_LIBWNCK 1

/* Define if x11 >= libx11_min_version present */
#define HAVE_LIBX11 1

/* Define to 1 if your system has a GNU libc compatible 'malloc' function, and
   to 0 otherwise. */
#define HAVE_MALLOC 0

/* Define to 1 if you have the <math.h> header file. */
#define HAVE_MATH_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <minix/config.h> header file. */
/* #undef HAVE_MINIX_CONFIG_H */

/* Define to 1 if you have the <netdb.h> header file. */
#define HAVE_NETDB_H 1

/* Compiling under cygwin */
/* #undef HAVE_OS_CYGWIN */

/* Define if polkit-gobject-1 >= polkit_min_version present */
/* #undef HAVE_POLKIT */

/* Define to 1 if you have the 'setsid' function. */
#define HAVE_SETSID 1

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define to 1 if you have the 'sigprocmask' function. */
#define HAVE_SIGPROCMASK 1

/* Define to 1 if you have the <stdarg.h> header file. */
#define HAVE_STDARG_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/resource.h> header file. */
#define HAVE_SYS_RESOURCE_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/sysctl.h> header file. */
/* #undef HAVE_SYS_SYSCTL_H */

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/utsname.h> header file. */
#define HAVE_SYS_UTSNAME_H 1

/* Define to 1 if you have the <sys/wait.h> header file. */
#define HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <wchar.h> header file. */
#define HAVE_WCHAR_H 1

/* Define to 1 if you have the '_IceTransNoListen' function. */
#define HAVE__ICETRANSNOLISTEN 1

/* path to iceauth */
#define ICEAUTH_CMD "/usr/bin/iceauth"

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "xfce4-session"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "https://gitlab.xfce.org/xfce/xfce4-session"

/* Define to the full name of this package. */
#define PACKAGE_NAME "xfce4-session"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "xfce4-session 4.20.4"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "xfce4-session"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "4.20.4"

/* Define to 1 if all of the C89 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#define STDC_HEADERS 1

/* Enable extensions on AIX, Interix, z/OS.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable general extensions on macOS.  */
#ifndef _DARWIN_C_SOURCE
# define _DARWIN_C_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable X/Open compliant socket functions that do not require linking
   with -lxnet on HP-UX 11.11.  */
#ifndef _HPUX_ALT_XOPEN_SOCKET_API
# define _HPUX_ALT_XOPEN_SOCKET_API 1
#endif
/* Identify the host operating system as Minix.
   This macro does not affect the system headers' behavior.
   A future release of Autoconf may stop defining this macro.  */
#ifndef _MINIX
/* # undef _MINIX */
#endif
/* Enable general extensions on NetBSD.
   Enable NetBSD compatibility extensions on Minix.  */
#ifndef _NETBSD_SOURCE
# define _NETBSD_SOURCE 1
#endif
/* Enable OpenBSD compatibility extensions on NetBSD.
   Oddly enough, this does nothing on OpenBSD.  */
#ifndef _OPENBSD_SOURCE
# define _OPENBSD_SOURCE 1
#endif
/* Define to 1 if needed for POSIX-compatible behavior.  */
#ifndef _POSIX_SOURCE
/* # undef _POSIX_SOURCE */
#endif
/* Define to 2 if needed for POSIX-compatible behavior.  */
#ifndef _POSIX_1_SOURCE
/* # undef _POSIX_1_SOURCE */
#endif
/* Enable POSIX-compatible threading on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-5:2014.  */
#ifndef __STDC_WANT_IEC_60559_ATTRIBS_EXT__
# define __STDC_WANT_IEC_60559_ATTRIBS_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-1:2014.  */
#ifndef __STDC_WANT_IEC_60559_BFP_EXT__
# define __STDC_WANT_IEC_60559_BFP_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-2:2015.  */
#ifndef __STDC_WANT_IEC_60559_DFP_EXT__
# define __STDC_WANT_IEC_60559_DFP_EXT__ 1
#endif
/* Enable extensions specified by C23 Annex F.  */
#ifndef __STDC_WANT_IEC_60559_EXT__
# define __STDC_WANT_IEC_60559_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-4:2015.  */
#ifndef __STDC_WANT_IEC_60559_FUNCS_EXT__
# define __STDC_WANT_IEC_60559_FUNCS_EXT__ 1
#endif
/* Enable extensions specified by C23 Annex H and ISO/IEC TS 18661-3:2015.  */
#ifndef __STDC_WANT_IEC_60559_TYPES_EXT__
# define __STDC_WANT_IEC_60559_TYPES_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TR 24731-2:2010.  */
#ifndef __STDC_WANT_LIB_EXT2__
# define __STDC_WANT_LIB_EXT2__ 1
#endif
/* Enable extensions specified by ISO/IEC 24747:2009.  */
#ifndef __STDC_WANT_MATH_SPEC_FUNCS__
# define __STDC_WANT_MATH_SPEC_FUNCS__ 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable X/Open extensions.  Define to 500 only if necessary
   to make mbstate_t available.  */
#ifndef _XOPEN_SOURCE
/* # undef _XOPEN_SOURCE */
#endif


/* Version number of package */
#define VERSION "4.20.4"

/* Define to '__inline__' or '__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to rpl_malloc if the replacement function should be used. */
#define malloc rpl_malloc

/* Define to 'int' if <sys/types.h> does not define. */
/* #undef mode_t */

/* Define as a signed integer type capable of holding a process identifier. */
/* #undef pid_t */

/* Define as 'unsigned int' if <stddef.h> doesn't define. */
/* #undef size_t */
