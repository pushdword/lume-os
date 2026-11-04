/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_CAIRO_RUNTIME_H
#define LUME_CAIRO_RUNTIME_H

#include "config.h"

#if defined(__lume__)
#undef HAVE_X11_EXTENSIONS_XSHM_H
#define HAVE_X11_EXTENSIONS_XSHM_H 0
#endif

#endif
