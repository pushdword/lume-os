#ifndef LUME_OPENSSH_STDIO_H
#define LUME_OPENSSH_STDIO_H
#include_next <stdio.h>
FILE *popen(const char *, const char *);
int pclose(FILE *);
int lume_ssh_flush(FILE *);
#define fflush(stream) lume_ssh_flush(stream)
#endif
