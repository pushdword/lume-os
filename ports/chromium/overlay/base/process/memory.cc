// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/process/memory.h"

#include <new>
#include <stdlib.h>
#include <string.h>

#include "base/allocator/buildflags.h"
#include "base/debug/alias.h"
#include "base/immediate_crash.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "partition_alloc/buildflags.h"

#if PA_BUILDFLAG(USE_ALLOCATOR_SHIM)
#include "partition_alloc/shim/allocator_shim.h" // nogncheck
#endif

#if PA_BUILDFLAG(USE_PARTITION_ALLOC)
#include "partition_alloc/page_allocator.h" // nogncheck
#endif

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#else
#include <unistd.h>
#endif // BUILDFLAG(IS_WIN)

namespace base::internal
{
bool ReleaseAddressSpaceReservation()
{
#if PA_BUILDFLAG(USE_PARTITION_ALLOC)
    return partition_alloc::ReleaseReservation();
#else
    return false;
#endif
}
} // namespace base::internal

#if BUILDFLAG(IS_LUME)
namespace base
{

void EnableTerminationOnHeapCorruption()
{
    // The allocator has no heap-corruption termination switch.
}

void EnableTerminationOnOutOfMemory()
{
    std::set_new_handler(
        []()
        {
            if (internal::ReleaseAddressSpaceReservation())
                return;
            TerminateBecauseOutOfMemory(0);
        });
#if PA_BUILDFLAG(USE_ALLOCATOR_SHIM)
    allocator_shim::SetCallNewHandlerOnMallocFailure(true);
#endif
}

bool UncheckedMalloc(size_t size, void **result)
{
#if PA_BUILDFLAG(USE_ALLOCATOR_SHIM)
    *result = allocator_shim::UncheckedAlloc(size);
#else
    *result = malloc(size);
#endif
    return *result != nullptr;
}

bool UncheckedCalloc(size_t num_items, size_t size, void **result)
{
#if PA_BUILDFLAG(USE_ALLOCATOR_SHIM)
    *result = allocator_shim::UncheckedCalloc(num_items, size);
#else
    *result = calloc(num_items, size);
#endif
    return *result != nullptr;
}

void UncheckedFree(void *ptr)
{
#if PA_BUILDFLAG(USE_ALLOCATOR_SHIM)
    allocator_shim::UncheckedFree(ptr);
#else
    free(ptr);
#endif
}

} // namespace base
#endif
