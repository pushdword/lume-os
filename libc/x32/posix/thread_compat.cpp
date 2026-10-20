// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <__external_threading>
_LIBCPP_BEGIN_NAMESPACE_STD
static void* Values[16];
static unsigned Used;
int __libcpp_tls_create(unsigned*K,void(*)(void*)){if(Used==16)return EAGAIN;*K=Used++;return 0;}
void* __libcpp_tls_get(unsigned K){return K<Used?Values[K]:nullptr;}
int __libcpp_tls_set(unsigned K,void*V){if(K>=Used)return EINVAL;Values[K]=V;return 0;}
_LIBCPP_END_NAMESPACE_STD
