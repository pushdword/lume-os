#ifndef LUME_ELF_FORMAT_H
#define LUME_ELF_FORMAT_H
#include <stdint.h>
#include <sys/elf32.h>
#include <sys/elf64.h>

typedef struct
{
    Elf32_Word a_type;

    union
    {
        Elf32_Word a_val;
    } a_un;
} Elf32_auxv_t;

typedef struct
{
    Elf64_Xword a_type;

    union
    {
        Elf64_Xword a_val;
    } a_un;
} Elf64_auxv_t;

#ifdef __cplusplus
static_assert(sizeof(Elf32_auxv_t) == 8 && sizeof(Elf64_auxv_t) == 16,
              "ELF auxiliary entry layout");
#else
_Static_assert(sizeof(Elf32_auxv_t) == 8 && sizeof(Elf64_auxv_t) == 16,
               "ELF auxiliary entry layout");
#endif
#endif
