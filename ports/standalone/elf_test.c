#include <lume/elf.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i)
    {
        FILE *file = fopen(argv[i], "rb");
        if (!file || fseek(file, 0, SEEK_END))
            return 1;
        long size = ftell(file);
        if (size <= 0 || fseek(file, 0, SEEK_SET))
            return 1;
        unsigned char *bytes = malloc((size_t)size);
        if (!bytes || fread(bytes, 1, (size_t)size, file) != (size_t)size)
            return 1;
        fclose(file);
        struct lume_elf_plan plan;
        int valid = lume_elf_plan(bytes, (size_t)size, &plan);
        free(bytes);
        if (!valid)
        {
            fprintf(stderr, "Invalid executable: %s\n", argv[i]);
            return 1;
        }
        printf("PASS %s pages=%u\n", argv[i], plan.pages);
    }
    return 0;
}
