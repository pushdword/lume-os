#ifndef LUME_SSH_PLATFORM_H
#define LUME_SSH_PLATFORM_H
#include <stdint.h>
int ssh_fdread(int, void *, unsigned);
int ssh_fdwrite(int, const void *, unsigned);
int ssh_fdclose(int);
int ssh_isatty(int);
int ssh_nonblock(int);
int ssh_spawn_command(const char *, const char *, int *, int *, int *, int *);
extern int ssh_child_pid;
extern unsigned ssh_columns, ssh_rows;
#endif
