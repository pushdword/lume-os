#include "pthread.h"

static void increment(void *argument)
{
    ++*static_cast<unsigned *>(argument);
}

int main()
{
    unsigned count = 0;
    pthread_cleanup_push(increment, &count);
    pthread_cleanup_push(increment, &count);
    pthread_cleanup_pop(0);
    pthread_cleanup_pop(1);
    return count != 1;
}
