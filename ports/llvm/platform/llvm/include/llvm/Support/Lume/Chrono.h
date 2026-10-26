//===- Lume/Chrono.h - Explicit clock fallback for Lume metadata ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_SUPPORT_LUME_CHRONO_H
#define LLVM_SUPPORT_LUME_CHRONO_H
#include "llvm/Support/Chrono.h"
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <time.h>

namespace llvm::sys
{
/// Read CLOCK_REALTIME when available. SOURCE_DATE_EPOCH supplies metadata
/// timestamps when no civil clock exists. Invalid or unrepresentable epochs
/// use zero. Elapsed durations use steady clocks.
inline TimePoint<> getWallTimeOrReproducibleEpoch()
{
#if defined(__lume__)
    constexpr uint64_t Limit = uint64_t(std::numeric_limits<int64_t>::max()) / 1000000000ULL;
    int SavedErrno = errno;
    struct timespec T;
    if (::clock_gettime(CLOCK_REALTIME, &T) == 0 && T.tv_sec >= 0 && uint64_t(T.tv_sec) <= Limit &&
        T.tv_nsec >= 0 && T.tv_nsec < 1000000000L &&
        uint64_t(T.tv_sec) * 1000000000ULL + uint64_t(T.tv_nsec) <=
            uint64_t(std::numeric_limits<int64_t>::max()))
    {
        errno = SavedErrno;
        return TimePoint<>(
            std::chrono::nanoseconds(uint64_t(T.tv_sec) * 1000000000ULL + uint64_t(T.tv_nsec)));
    }
    uint64_t Epoch = 0;
    if (const char *Text = std::getenv("SOURCE_DATE_EPOCH"))
    {
        for (; *Text; ++Text)
        {
            unsigned Digit = unsigned(*Text - '0');
            if (Digit > 9 || Epoch > (Limit - Digit) / 10)
            {
                Epoch = 0;
                break;
            }
            Epoch = Epoch * 10 + Digit;
        }
    }
    errno = SavedErrno;
    return TimePoint<>(std::chrono::seconds(Epoch));
#else
    return std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now());
#endif
}
} // namespace llvm::sys
#endif
