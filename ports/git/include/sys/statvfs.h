#ifndef LUME_GIT_STATVFS_H
#define LUME_GIT_STATVFS_H

struct statvfs
{
    unsigned long f_bsize, f_frsize, f_blocks, f_bfree, f_bavail;
    unsigned long f_files, f_ffree, f_favail, f_fsid, f_flag, f_namemax;
};

int statvfs(const char *, struct statvfs *);
int fstatvfs(int, struct statvfs *);
#endif
