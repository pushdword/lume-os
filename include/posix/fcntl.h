#ifndef LUME_POSIX_FCNTL_H
#define LUME_POSIX_FCNTL_H
#include_next <fcntl.h>
#ifndef F_ADD_SEALS
#define F_ADD_SEALS 1033
#define F_GET_SEALS 1034
#define F_SEAL_SEAL 1
#define F_SEAL_SHRINK 2
#define F_SEAL_GROW 4
#define F_SEAL_WRITE 8
#define F_SEAL_FUTURE_WRITE 16
#endif

#ifndef F_OFD_GETLK
#define F_OFD_GETLK 36
#endif
#ifndef F_OFD_SETLK
#define F_OFD_SETLK 37
#endif
#endif
