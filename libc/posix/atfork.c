#include "pthread.h"
#include "lume/atfork.h"
#include <errno.h>
#include <stdlib.h>

struct lume_atfork_handler
{
    struct lume_atfork_handler *previous, *next;
    void (*prepare)(void);
    void (*parent)(void);
    void (*child)(void);
};

static pthread_mutex_t registry_lock = PTHREAD_MUTEX_INITIALIZER;
static struct lume_atfork_handler *first, *last;

int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void))
{
    int saved = errno;
    struct lume_atfork_handler *handler = malloc(sizeof(*handler));
    if (!handler)
    {
        errno = saved;
        return ENOMEM;
    }
    *handler = (struct lume_atfork_handler){.prepare = prepare, .parent = parent, .child = child};
    pthread_mutex_lock(&registry_lock);
    handler->previous = last;
    if (last)
        last->next = handler;
    else
        first = handler;
    last = handler;
    pthread_mutex_unlock(&registry_lock);
    errno = saved;
    return 0;
}

void lume_atfork_prepare(struct lume_atfork_snapshot *snapshot)
{
    int saved = errno;
    pthread_mutex_lock(&registry_lock);
    *snapshot = (struct lume_atfork_snapshot){.first = first, .last = last};
    pthread_mutex_unlock(&registry_lock);
    errno = saved;
    for (struct lume_atfork_handler *handler = snapshot->last; handler; handler = handler->previous)
        if (handler->prepare)
            handler->prepare();
}

static void finish(const struct lume_atfork_snapshot *snapshot, int child)
{
    /* Appends can change the final link after the snapshot was captured. */
    for (struct lume_atfork_handler *handler = snapshot->first; handler;)
    {
        void (*routine)(void) = child ? handler->child : handler->parent;
        if (routine)
            routine();
        if (handler == snapshot->last)
            break;
        handler = handler->next;
    }
}

void lume_atfork_parent(const struct lume_atfork_snapshot *snapshot)
{
    finish(snapshot, 0);
}

void lume_atfork_child(const struct lume_atfork_snapshot *snapshot)
{
    finish(snapshot, 1);
}
