// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#if !(defined(__aarch64__) || defined(__x86_64__) || defined(__i386__))
#include "../../libc/compat/legacy32/posix/thread_compat.cpp"
#else
#include "external_threading.h"
#endif
