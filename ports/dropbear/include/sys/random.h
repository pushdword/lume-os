#include <sys/types.h>
#define GRND_NONBLOCK 1
ssize_t getrandom(void *, size_t, unsigned);
