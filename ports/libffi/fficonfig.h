/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_FFI_CONFIG_H
#define LUME_FFI_CONFIG_H
#define HAVE_ALLOCA 1
#define HAVE_ALLOCA_H 1
#define HAVE_AS_CFI_PSEUDO_OP 1
#define HAVE_HIDDEN_VISIBILITY_ATTRIBUTE 1
#define STDC_HEADERS 1
#define SIZEOF_DOUBLE 8
#define SIZEOF_LONG_DOUBLE __SIZEOF_LONG_DOUBLE__
#define SIZEOF_SIZE_T __SIZEOF_SIZE_T__
#define FFI_EXEC_TRAMPOLINE_TABLE 0
#define EH_FRAME_FLAGS "a"
#ifdef LIBFFI_ASM
#define FFI_HIDDEN(name) .hidden name
#else
#define FFI_HIDDEN __attribute__((visibility("hidden")))
#endif
#endif
