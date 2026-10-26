#ifndef LUME_GIT_PROCESS_H
#define LUME_GIT_PROCESS_H
#include <sys/types.h>
pid_t lume_git_spawn(const char *, const char **, char **, const char *, int, int, int);
#endif
