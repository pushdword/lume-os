#include <stdbool.h>

/* Outlined atomics use the baseline exclusive instructions on native AArch64. */
__attribute__((visibility("hidden"))) const bool __aarch64_have_lse_atomics = false;
