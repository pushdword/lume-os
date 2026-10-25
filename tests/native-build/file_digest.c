#include "sha256.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    FILE *file = fopen(argv[1], "rb");
    if (!file)
        return 1;
    struct lume_sha256 state;
    lume_sha256_init(&state);
    unsigned char buffer[4096], digest[32];
    unsigned long long bytes = 0;
    size_t count;
    while ((count = fread(buffer, 1, sizeof buffer, file)))
    {
        lume_sha256_update(&state, buffer, count);
        bytes += count;
    }
    int failed = ferror(file);
    if (fclose(file) || failed)
        return 1;
    lume_sha256_final(&state, digest);
    fputs("FILE_SHA256 ", stdout);
    for (unsigned i = 0; i < sizeof digest; i++)
        printf("%02x", digest[i]);
    printf(" %llu %s\n", bytes, argv[1]);
    return fflush(stdout) ? 1 : 0;
}
