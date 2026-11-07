/* SPDX-License-Identifier: BSD-3-Clause */
#include <fenv.h>
#include "fp_mode.h"

CRT_FE_ROUND_MODE __fe_getround(void)
{
    switch (fegetround())
    {
    case FE_DOWNWARD:
        return CRT_FE_DOWNWARD;
    case FE_UPWARD:
        return CRT_FE_UPWARD;
    case FE_TOWARDZERO:
        return CRT_FE_TOWARDZERO;
    case FE_TONEAREST:
    default:
        return CRT_FE_TONEAREST;
    }
}

int __fe_raise_inexact(void)
{
    return feraiseexcept(FE_INEXACT);
}
