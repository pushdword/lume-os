#ifndef LUME_HTOP_DIRENT_H
#define LUME_HTOP_DIRENT_H
/* Directory walking is not used by this snapshot-backed platform. */
#include <sys/types.h>
typedef struct lume_htop_directory DIR;
int dirfd(DIR *);
#endif
