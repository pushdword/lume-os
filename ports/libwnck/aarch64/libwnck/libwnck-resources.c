#include <gio/gio.h>

#if defined (__ELF__) && ( __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 6))
# define SECTION __attribute__ ((section (".gresource.libwnck"), aligned (sizeof(void *) > 8 ? sizeof(void *) : 8)))
#else
# define SECTION
#endif

static const SECTION union { const guint8 data[696]; const double alignment; void * const ptr;}  libwnck_resource_data = {
  "\107\126\141\162\151\141\156\164\000\000\000\000\000\000\000\000"
  "\030\000\000\000\310\000\000\000\000\000\000\050\006\000\000\000"
  "\000\000\000\000\000\000\000\000\002\000\000\000\004\000\000\000"
  "\006\000\000\000\006\000\000\000\113\120\220\013\003\000\000\000"
  "\310\000\000\000\004\000\114\000\314\000\000\000\320\000\000\000"
  "\351\342\244\150\004\000\000\000\320\000\000\000\020\000\166\000"
  "\340\000\000\000\111\002\000\000\260\267\044\060\000\000\000\000"
  "\111\002\000\000\006\000\114\000\120\002\000\000\124\002\000\000"
  "\324\265\002\000\377\377\377\377\124\002\000\000\001\000\114\000"
  "\130\002\000\000\134\002\000\000\051\030\043\112\002\000\000\000"
  "\134\002\000\000\010\000\114\000\144\002\000\000\154\002\000\000"
  "\263\042\143\344\004\000\000\000\154\002\000\000\010\000\166\000"
  "\170\002\000\000\267\002\000\000\157\162\147\057\002\000\000\000"
  "\144\145\146\141\165\154\164\137\151\143\157\156\056\160\156\147"
  "\131\001\000\000\000\000\000\000\211\120\116\107\015\012\032\012"
  "\000\000\000\015\111\110\104\122\000\000\000\020\000\000\000\020"
  "\010\006\000\000\000\037\363\377\141\000\000\000\001\163\122\107"
  "\102\000\256\316\034\351\000\000\000\006\142\113\107\104\000\377"
  "\000\377\000\377\240\275\247\223\000\000\000\011\160\110\131\163"
  "\000\000\013\023\000\000\013\023\001\000\232\234\030\000\000\000"
  "\007\164\111\115\105\007\330\010\023\023\003\043\327\117\270\312"
  "\000\000\000\331\111\104\101\124\070\313\325\222\075\156\002\061"
  "\024\204\277\171\062\127\240\213\264\042\242\242\241\106\021\242"
  "\104\224\110\351\051\271\001\160\001\042\304\121\220\050\127\251"
  "\123\160\003\244\224\351\163\207\230\302\336\305\040\176\166\323"
  "\040\246\172\266\337\233\067\063\062\074\032\002\310\106\037\376"
  "\077\303\077\371\102\256\070\214\337\207\265\206\267\233\117\000"
  "\134\172\371\265\333\127\032\356\367\072\145\355\000\246\203\077"
  "\146\223\056\337\157\015\004\110\302\173\217\044\220\020\302\222"
  "\272\225\265\151\376\346\314\363\063\005\046\003\201\020\310\043"
  "\204\164\034\056\173\022\234\022\130\330\020\262\365\104\066\220"
  "\141\012\312\142\356\227\011\024\045\006\013\361\034\067\206\372"
  "\056\201\035\033\143\006\051\151\151\057\125\175\152\301\060\131"
  "\360\134\021\356\332\203\311\120\334\250\042\134\031\272\105\220"
  "\275\274\326\376\215\045\301\152\275\344\071\161\000\232\240\034"
  "\277\102\155\035\315\000\000\000\000\111\105\116\104\256\102\140"
  "\202\000\000\050\165\165\141\171\051\147\156\157\155\145\057\000"
  "\004\000\000\000\057\000\000\000\000\000\000\000\154\151\142\167"
  "\156\143\153\057\001\000\000\000\005\000\000\000\167\156\143\153"
  "\056\143\163\163\000\000\000\000\057\000\000\000\000\000\000\000"
  "\056\167\156\143\153\055\156\145\145\144\163\055\141\164\164\145"
  "\156\164\151\157\156\040\173\012\040\040\146\157\156\164\055\167"
  "\145\151\147\150\164\072\040\142\157\154\144\073\012\175\012\000"
  "\000\050\165\165\141\171\051" };

static GStaticResource static_resource = { libwnck_resource_data.data, sizeof (libwnck_resource_data.data) - 1 /* nul terminator */, NULL, NULL, NULL };

G_GNUC_INTERNAL
GResource *libwnck_get_resource (void);
GResource *libwnck_get_resource (void)
{
  return g_static_resource_get_resource (&static_resource);
}
/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __G_CONSTRUCTOR_H__
#define __G_CONSTRUCTOR_H__

/*
  If G_HAS_CONSTRUCTORS is true then the compiler support *both* constructors and
  destructors, in a usable way, including e.g. on library unload. If not you're on
  your own.

  Some compilers need #pragma to handle this, which does not work with macros,
  so the way you need to use this is (for constructors):

  #ifdef G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA
  #pragma G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(my_constructor)
  #endif
  G_DEFINE_CONSTRUCTOR(my_constructor)
  static void my_constructor(void) {
   ...
  }

*/

#ifndef __GTK_DOC_IGNORE__

#if  __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)

#define G_HAS_CONSTRUCTORS 1

#define G_DEFINE_CONSTRUCTOR(_func) static void __attribute__((constructor)) _func (void);
#define G_DEFINE_DESTRUCTOR(_func) static void __attribute__((destructor)) _func (void);

#elif defined (_MSC_VER)

/*
 * Only try to include gslist.h if not already included via glib.h,
 * so that items using gconstructor.h outside of GLib (such as
 * GResources) continue to build properly.
 */
#ifndef __G_LIB_H__
#include "gslist.h"
#endif

#include <stdlib.h>

#define G_HAS_CONSTRUCTORS 1

/* We do some weird things to avoid the constructors being optimized
 * away on VS2015 if WholeProgramOptimization is enabled. First we
 * make a reference to the array from the wrapper to make sure its
 * references. Then we use a pragma to make sure the wrapper function
 * symbol is always included at the link stage. Also, the symbols
 * need to be extern (but not dllexport), even though they are not
 * really used from another object file.
 */

/* We need to account for differences between the mangling of symbols
 * for x86 and x64/ARM/ARM64 programs, as symbols on x86 are prefixed
 * with an underscore but symbols on x64/ARM/ARM64 are not.
 */
#ifdef _M_IX86
#define G_MSVC_SYMBOL_PREFIX "_"
#else
#define G_MSVC_SYMBOL_PREFIX ""
#endif

#define G_DEFINE_CONSTRUCTOR(_func) G_MSVC_CTOR (_func, G_MSVC_SYMBOL_PREFIX)
#define G_DEFINE_DESTRUCTOR(_func) G_MSVC_DTOR (_func, G_MSVC_SYMBOL_PREFIX)

#define G_MSVC_CTOR(_func,_sym_prefix) \
  static void _func(void); \
  extern int (* _array ## _func)(void);              \
  int _func ## _wrapper(void);              \
  int _func ## _wrapper(void) { _func(); g_slist_find (NULL,  _array ## _func); return 0; } \
  __pragma(comment(linker,"/include:" _sym_prefix # _func "_wrapper")) \
  __pragma(section(".CRT$XCU",read)) \
  __declspec(allocate(".CRT$XCU")) int (* _array ## _func)(void) = _func ## _wrapper;

#define G_MSVC_DTOR(_func,_sym_prefix) \
  static void _func(void); \
  extern int (* _array ## _func)(void);              \
  int _func ## _constructor(void);              \
  int _func ## _constructor(void) { atexit (_func); g_slist_find (NULL,  _array ## _func); return 0; } \
   __pragma(comment(linker,"/include:" _sym_prefix # _func "_constructor")) \
  __pragma(section(".CRT$XCU",read)) \
  __declspec(allocate(".CRT$XCU")) int (* _array ## _func)(void) = _func ## _constructor;

#elif defined(__SUNPRO_C)

/* This is not tested, but i believe it should work, based on:
 * http://opensource.apple.com/source/OpenSSL098/OpenSSL098-35/src/fips/fips_premain.c
 */

#define G_HAS_CONSTRUCTORS 1

#define G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA 1
#define G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA 1

#define G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(_func) \
  init(_func)
#define G_DEFINE_CONSTRUCTOR(_func) \
  static void _func(void);

#define G_DEFINE_DESTRUCTOR_PRAGMA_ARGS(_func) \
  fini(_func)
#define G_DEFINE_DESTRUCTOR(_func) \
  static void _func(void);

#else

/* constructors not supported for this compiler */

#endif

#endif /* __GTK_DOC_IGNORE__ */
#endif /* __G_CONSTRUCTOR_H__ */

#ifdef G_HAS_CONSTRUCTORS

#ifdef G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA
#pragma G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(libwnckresource_constructor)
#endif
G_DEFINE_CONSTRUCTOR(libwnckresource_constructor)
#ifdef G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA
#pragma G_DEFINE_DESTRUCTOR_PRAGMA_ARGS(libwnckresource_destructor)
#endif
G_DEFINE_DESTRUCTOR(libwnckresource_destructor)

#else
#warning "Constructor not supported on this compiler, linking in resources will not work"
#endif

static void libwnckresource_constructor (void)
{
  g_static_resource_init (&static_resource);
}

static void libwnckresource_destructor (void)
{
  g_static_resource_fini (&static_resource);
}
