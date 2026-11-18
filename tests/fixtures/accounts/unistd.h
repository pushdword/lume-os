#ifndef LUME_ACCOUNTS_TEST_UNISTD_H
#define LUME_ACCOUNTS_TEST_UNISTD_H
#include "pwd.h"
#define X_OK 1
int access(const char *, int);
uid_t getuid(void);
#endif
