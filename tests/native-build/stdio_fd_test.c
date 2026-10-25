#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (fileno(stdin) != 0 || fileno(stdout) != 1 || fileno(stderr) != 2)
    {
        puts("STDIO_FD standard descriptors FAIL");
        return 1;
    }
    if (argc == 2 && !strcmp(argv[1], "--eof"))
    {
        char byte;
        if (read(0, &byte, 1) != 0)
            return 1;
        puts("STDIO_FD redirected EOF PASS");
        return 0;
    }
    if (argc != 2)
        return 2;
    FILE *file = fopen(argv[1], "w+");
    if (!file)
        return 1;
    int fd = fileno(file);
    int ok = fd >= 3 && fd < 16 && fwrite("fd", 1, 2, file) == 2 && !fflush(file) &&
             !fseek(file, 0, SEEK_SET);
    char bytes[2];
    ok = ok && fread(bytes, 1, sizeof bytes, file) == sizeof bytes && !memcmp(bytes, "fd", 2);
    ok = !fclose(file) && ok;
    ok = !unlink(argv[1]) && ok;
    puts(ok ? "STDIO_FD file descriptor PASS" : "STDIO_FD file descriptor FAIL");
    return ok ? 0 : 1;
}
