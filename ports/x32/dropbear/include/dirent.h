#ifndef LUME_SSH_DIRENT_H
#define LUME_SSH_DIRENT_H
#include <sys/types.h>
typedef struct lume_ssh_dir DIR;
struct dirent {unsigned long d_ino;char d_name[256];};
DIR *opendir(const char *);int closedir(DIR *);struct dirent *readdir(DIR *);
int dirfd(DIR *);
#endif
