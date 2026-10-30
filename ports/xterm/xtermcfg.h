/* $XTermId: xtermcfg.hin,v 1.238 2026/01/22 00:17:20 tom Exp $ */

/*
 * Copyright 1997-2025,2026 by Thomas E. Dickey
 *
 *                         All Rights Reserved
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization.
 */

#ifndef included_xtermcfg_h
#define included_xtermcfg_h 1
#define STDC_HEADERS 1
#define HAVE_INTTYPES_H 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_FCNTL_H 1
#define HAVE_DIRENT_H 1
#define HAVE_POLL 1
#define HAVE_SNPRINTF 1
#define HAVE_VSNPRINTF 1
#define HAVE_STRCASECMP 1
#define HAVE_STRNCASECMP 1
#define HAVE_STRNDUP 1
#define HAVE_STRDUP 1
#define HAVE_MEMMOVE 1
#define PACKAGE_VERSION "1.0"
#define PACKAGE_STRING "Lume X11 libraries"
#define HAVE_SYS_SELECT_H 1
#define OPT_PRINT_ON_EXIT 0
#define OPT_PRINT_ON_XERR 0
#define OPT_RENDERFONT 0
#define CANT_OPEN_DEV_TTY 1
#define USE_TERMIOS 1
#define USE_POSIX_TERMIOS 1
#define USE_SYSV_PGRP 1

/* This is a template for <xtermcfg.h> */

/*
 * There are no configure options for these features:
 * ALLOWLOGFILECHANGES
 * ALLOWLOGFILEONOFF
 * CANT_OPEN_DEV_TTY
 * DEBUG* (any debug-option)
 * DUMP_* (mostly in the ReGIS/SIXEL code)
 * HAS_LTCHARS
 * PUCC_PTYD
 * USE_LOGIN_DASH_P
 * USE_X11TERM
 */

#undef ALLOWLOGFILEEXEC		/* CF_ARG_ENABLE(enable-logfile-exec) */
#undef ALLOWLOGGING		/* CF_ARG_ENABLE(enable-logging) */
#define CC_HAS_PROTOS 1
#undef CSRG_BASED		/* CF_TYPE_FD_MASK */
#undef DECL_ERRNO		/* CF_ERRNO */
#undef DEFDELETE_DEL		/* AC_ARG_ENABLE(delete-is-del) */
#undef DEF_ALT_SENDS_ESC	/* CF_ARG_ENABLE(alt-sends-esc) */
#undef DEF_BACKARO_BS		/* CF_ARG_DISABLE(backarrow-key) */
#undef DEF_BACKARO_ERASE	/* CF_ARG_ENABLE(backarrow-is-erase) */
#undef DEF_INITIAL_ERASE	/* CF_ARG_DISABLE(initial-erase) */
#undef DEF_META_SENDS_ESC	/* CF_ARG_ENABLE(meta-sends-esc) */
#define DFT_COLORMODE 1
#define DFT_DECID "420"
#define DFT_TERMTYPE "xterm"
#define DISABLE_SETGID 1
#define DISABLE_SETUID 1
#define HAVE_CLOCK_GETTIME 1
#undef HAVE_ENDUSERSHELL	/* AC_CHECK_FUNCS(endusershell) */
#undef HAVE_GETHOSTNAME		/* AC_CHECK_FUNCS(gethostname) */
#undef HAVE_GETLOGIN		/* AC_CHECK_FUNCS(getlogin) */
#define HAVE_GETTIMEOFDAY 1
#undef HAVE_GETUSERSHELL	/* AC_CHECK_FUNCS(getusershell) */
#define HAVE_GRANTPT 1
#define HAVE_GRANTPT_PTY_ISATTY 1
#undef HAVE_INITGROUPS		/* AC_CHECK_FUNCS(initgroups) */
#undef HAVE_LANGINFO_CODESET	/* AM_LANGINFO_CODESET */
#undef HAVE_LASTLOG_H		/* CF_LASTLOG */
#undef HAVE_LIBUTIL_H		/* AC_CHECK_HEADERS(libutil.h) */
#define HAVE_LIBXPM 1
#undef HAVE_LIB_NEXTAW		/* CF_X_ATHENA(--with-neXtaw) */
#undef HAVE_LIB_PCRE		/* CF_WITH_PCRE */
#undef HAVE_LIB_PCRE2		/* CF_WITH_PCRE2 */
#define HAVE_LIB_XAW 1
#undef HAVE_LIB_XAW3D		/* CF_X_ATHENA(--with-Xaw3d) */
#undef HAVE_LIB_XAW3DXFT	/* CF_X_ATHENA(--with-Xaw3dxft) */
#undef HAVE_LIB_XAWPLUS		/* CF_X_ATHENA(--with-XawPlus) */
#undef HAVE_LIB_XCURSOR		/* AC_CHECK_LIB(Xcursor) */
#undef HAVE_MKDTEMP		/* AC_CHECK_FUNCS(mkdtemp) */
#undef HAVE_MKSTEMP		/* CF_MKSTEMP */
#undef HAVE_NCURSES_CURSES_H	/* AC_CHECK_HEADERS(ncurses/curses.h) */
#undef HAVE_NCURSES_TERM_H	/* AC_CHECK_HEADERS(ncurses/term.h) */
#undef HAVE_PATHS_H		/* CF_LASTLOG */
#undef HAVE_PCRE2POSIX_H	/* CF_WITH_PCRE2 */
#undef HAVE_PCRE2REGCOMP	/* CF_WITH_PCRE2 */
#undef HAVE_PCREPOSIX_H		/* CF_WITH_PCRE */
#define HAVE_POSIX_OPENPT 1
#undef HAVE_POSIX_SAVED_IDS	/* CF_POSIX_SAVED_IDS */
#define HAVE_PTSNAME 1
#define HAVE_PTY_H 1
#define HAVE_PUTENV 1
#undef HAVE_SCHED_YIELD		/* AC_CHECK_FUNCS(sched_yield) */
#define HAVE_SETITIMER 1
#define HAVE_SETPGID 1
#define HAVE_SETSID 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#undef HAVE_STDNORETURN_H	/* CF_C11_NORETURN */
#undef HAVE_STRFTIME		/* AC_CHECK_FUNCS(strftime) */
#undef HAVE_STROPTS_H		/* AC_CHECK_HEADERS(stropts.h) */
#define HAVE_SYS_PARAM_H 1
#undef HAVE_SYS_PTEM_H		/* AC_CHECK_HEADERS(sys/ptem.h) */
#define HAVE_SYS_TIME_H 1
#undef HAVE_SYS_TTYDEFAULTS_H	/* AC_CHECK_HEADERS(sys/ttydefaults.h) */
#define HAVE_SYS_WAIT_H 1
#define HAVE_TCGETATTR 1
#define HAVE_TERMCAP_H 1
#define HAVE_TERMIOS_H 1
#undef HAVE_TERMIO_C_ISPEED	/* CF_TERMIO_C_ISPEED */
#undef HAVE_TERM_H		/* AC_CHECK_HEADERS(term.h) */
#undef HAVE_TIGETSTR		/* AC_CHECK_FUNCS(tigetstr) */
#define HAVE_UNISTD_H 1
#define HAVE_UNSETENV 1
#undef HAVE_USE_EXTENDED_NAMES	/* AC_CHECK_FUNCS(use_extended_names) */
#undef HAVE_UTIL_H		/* AC_CHECK_HEADERS(util.h) */
#undef HAVE_UTMP		/* CF_UTMP */
#undef HAVE_UTMP_UT_HOST	/* CF_UTMP_UT_HOST */
#undef HAVE_UTMP_UT_SESSION	/* CF_UTMP_UT_SESSION */
#undef HAVE_UTMP_UT_SYSLEN	/* CF_UTMP_UT_SYSLEN */
#undef HAVE_UTMP_UT_XSTATUS	/* CF_UTMP_UT_XSTATUS */
#undef HAVE_UTMP_UT_XTIME	/* CF_UTMP_UT_XTIME */
#define HAVE_WAITPID 1
#define HAVE_WCHAR_H 1
#define HAVE_WCSWIDTH 1
#define HAVE_WCWIDTH 1
#undef HAVE_X11_DECKEYSYM_H	/* AC_CHECK_HEADERS(X11/DECkeysym.h) */
#undef HAVE_X11_EXTENSIONS_XDBE_H /* AC_CHECK_HEADERS(X11/extensions/Xdbe.h) */
#undef HAVE_X11_EXTENSIONS_XINERAMA_H /* AC_CHECK_HEADERS(X11/extensions/Xinerama.h) */
#define HAVE_X11_EXTENSIONS_XKB_H 1
#undef HAVE_X11_SUNKEYSYM_H	/* AC_CHECK_HEADERS(X11/Sunkeysym.h) */
#undef HAVE_X11_TRANSLATEI_H	/* AC_CHECK_HEADERS(X11/TranslateI.h) */
#undef HAVE_X11_XF86KEYSYM_H	/* AC_CHECK_HEADERS(X11/XF86keysym.h) */
#define HAVE_X11_XKBLIB_H 1
#define HAVE_X11_XPOLL_H 1
#undef HAVE_XDBESWAPBUFFERS	/* AC_CHECK_FUNC(XdbeSwapBuffers) */
#undef HAVE_XFTDRAWSETCLIP	/* CF_X_FREETYPE */
#undef HAVE_XFTDRAWSETCLIPRECTANGLES /* CF_X_FREETYPE */
#define HAVE_XKBKEYCODETOKEYSYM 1
#define HAVE_XKBQUERYEXTENSION 1
#undef HAVE_XKB_BELL_EXT	/* CF_XKB_BELL_EXT */
#undef LUIT_PATH		/* CF_ARG_ENABLE(luit) */
#undef NO_ACTIVE_ICON		/* CF_ARG_DISABLE(active-icon) */
#undef NO_LEAKS			/* CF_ARG_DISABLE(leaks) */
#undef OPT_256_COLORS		/* CF_ARG_ENABLE(256-color) */
#undef OPT_88_COLORS		/* CF_ARG_ENABLE(88-color) */
#undef OPT_AIX_COLORS		/* CF_ARG_DISABLE(16-color) */
#undef OPT_BLINK_CURS		/* CF_ARG_DISABLE(blink-cursor) */
#undef OPT_BLINK_TEXT		/* CF_ARG_DISABLE(blink-text) */
#undef OPT_BLOCK_SELECT		/* CF_ARG_ENABLE(block-select) */
#undef OPT_BOX_CHARS		/* CF_ARG_DISABLE(boxchars) */
#undef OPT_BROKEN_OSC		/* CF_ARG_ENABLE(broken-osc) */
#undef OPT_BROKEN_ST		/* CF_ARG_DISABLE(broken-st) */
#define OPT_BUILTIN_XPMS 0
#undef OPT_C1_PRINT		/* CF_ARG_DISABLE(c1-print) */
#undef OPT_COLOR_CLASS		/* CF_ARG_DISABLE(color-class) */
#undef OPT_DABBREV		/* CF_ARG_ENABLE(dabbrev) */
#undef OPT_DEC_CHRSET		/* CF_ARG_DISABLE(doublechars) */
#undef OPT_DEC_LOCATOR		/* CF_ARG_ENABLE(dec-locator) */
#undef OPT_DEC_RECTOPS		/* CF_ARG_DISABLE(rectangles) */
#undef OPT_DIRECT_COLOR		/* CF_ARG_ENABLE(direct-color) */
#undef OPT_DOUBLE_BUFFER	/* CF_ARG_ENABLE(double-buffer) */
#undef OPT_EXEC_SELECTION	/* CF_ARG_ENABLE(exec-selection) */
#define OPT_EXEC_XTERM 0
#undef OPT_GRAPHICS		/* CF_ARG_ENABLE(graphics) */
#undef OPT_HIGHLIGHT_COLOR	/* CF_ARG_DISABLE(highlighting) */
#undef OPT_HP_FUNC_KEYS		/* CF_ARG_ENABLE(hp-fkeys) */
#define OPT_I18N_SUPPORT 1
#define OPT_INITIAL_ERASE 0
#define OPT_INPUT_METHOD 1
#undef OPT_ISO_COLORS		/* CF_ARG_DISABLE(ansi-color) */
#undef OPT_LOAD_VTFONTS		/* CF_ARG_ENABLE(load-vt-fonts) */
#define OPT_LUIT_PROG 0
#undef OPT_MAXIMIZE		/* CF_ARG_DISABLE(maximize) */
#undef OPT_MINI_LUIT		/* CF_ARG_ENABLE(mini-luit) */
#undef OPT_NUM_LOCK		/* CF_ARG_DISABLE(num-lock) */
#undef OPT_PASTE64		/* CF_ARG_ENABLE(past64) */
#undef OPT_PC_COLORS		/* CF_ARG_DISABLE(pc-color) */
#undef OPT_PRINT_GRAPHICS	/* CF_ARG_ENABLE(print-graphics) */
#define OPT_PTY_HANDSHAKE 1
#undef OPT_READLINE		/* CF_ARG_ENABLE(readline-mouse) */
#undef OPT_REGIS_GRAPHICS	/* CF_ARG_ENABLE(regis-graphics) */
#undef OPT_RESIZE_ADJUST	/* CF_ARG_ENABLE(resize-adjust) */
#undef OPT_SAME_NAME		/* CF_ARG_DISABLE(samename) */
#undef OPT_SCO_FUNC_KEYS	/* CF_ARG_ENABLE(sco-fkeys) */
#define OPT_SCREEN_DUMPS 0
#undef OPT_SELECTION_OPS	/* CF_ARG_DISABLE(selection-ops) */
#undef OPT_SELECT_REGEX		/* CF_ARG_DISABLE(regex) */
#define OPT_SESSION_MGT 0
#undef OPT_SIXEL_GRAPHICS	/* CF_ARG_ENABLE(sixel-graphics) */
#undef OPT_STATUS_LINE		/* CF_ARG_ENABLE(status-line) */
#undef OPT_SUN_FUNC_KEYS	/* CF_ARG_ENABLE(sun-fkeys) */
#define OPT_TCAP_FKEYS 0
#define OPT_TCAP_QUERY 0
#define OPT_TEK4014 0
#undef OPT_TOOLBAR		/* CF_ARG_ENABLE(toolbar) */
#undef OPT_VT52_MODE		/* CF_ARG_DISABLE(vt52) */
#undef OPT_WIDER_ICHAR		/* CF_ARG_ENABLE(16bit-chars) */
#undef OPT_WIDE_ATTRS		/* CF_ARG_DISABLE(wide-attrs) */
#define OPT_WIDE_CHARS 1
#undef EXP_XMC_GLITCH		/* CF_ARG_ENABLE(xmc-glitch) */
#undef OPT_ZICONBEEP		/* CF_ARG_DISABLE(ziconbeep) */
#undef OWN_TERMINFO_DIR		/* AC_ARG_WITH(own-terminfo) */
#undef OWN_TERMINFO_ENV		/* AC_ARG_ENABLE(env-terminfo) */
#undef PROCFS_ROOT		/* CF_ARG_ENABLE(exec-xterm) */
#undef SCROLLBAR_RIGHT		/* CF_ARG_ENABLE(rightbar) */
#define SIG_ATOMIC_T sig_atomic_t
#undef STDC_NORETURN		/* CF_C11_NORETURN */
#undef SVR4			/* CF_SVR4, imake */
#undef SYSV			/* CF_SYSV, imake */
#define TIME_WITH_SYS_TIME 1
#undef TTY_GROUP_NAME		/* CF_TTY_GROUP */
#undef USE_LASTLOG		/* CF_LASTLOG */
#define USE_POSIX_WAIT 1
#undef USE_STRUCT_LASTLOG	/* CF_STRUCT_LASTLOG */
#undef USE_SYSV_UTMP		/* CF_UTMP */
#define USE_SYS_SELECT_H 1
#define USE_TERMCAP 1
#undef USE_TERMINFO		/* CF_FUNC_TGETENT */
#undef USE_TTY_GROUP		/* CF_TTY_GROUP */
#undef USE_UTEMPTER		/* CF_UTEMPTER */
#undef USE_UTMP_SETGID		/* AC_ARG_WITH(utmp-setgid) */
#undef UTMPX_FOR_UTMP		/* CF_UTMP */
#undef XRENDERFONT		/* CF_X_FREETYPE */
#define XTERM_ERASE 127
#undef cc_t			/* CF_TYPE_CC_T */
#undef gid_t			/* AC_TYPE_UID_T */
#undef inline			/* AC_C_INLINE */
#undef mode_t			/* AC_TYPE_MODE_T */
#undef nfds_t			/* CF_TYPE_NFDS_T */
#undef off_t			/* AC_TYPE_OFF_T */
#undef pid_t			/* AC_TYPE_PID_T */
#undef time_t			/* AC_CHECK_TYPE(time_t, long) */
#undef uid_t			/* AC_TYPE_UID_T */
#undef ut_name			/* CF_UTMP */
#undef ut_xstatus		/* CF_UTMP_UT_XSTATUS */
#undef ut_xtime			/* CF_UTMP_UT_XTIME */

/*
 * Ifdef'd to make it simple to override.
 */
#ifndef OPT_TRACE
#undef OPT_TRACE		/* CF_ARG_ENABLE(trace) */
#undef OPT_TRACE_FLAGS		/* ...no option */
#endif

/*
 * g++ support for __attribute__() is haphazard.
 */
#ifndef __cplusplus
#undef GCC_PRINTF
#undef GCC_PRINTFLIKE
#undef GCC_NORETURN
#undef GCC_UNUSED
#endif

#ifndef HAVE_X11_XPOLL_H
#define NO_XPOLL_H	/* X11R6.1 & up use Xpoll.h for select() definitions */
#endif

/* vile:cmode
 */
#endif /* included_xtermcfg_h */
