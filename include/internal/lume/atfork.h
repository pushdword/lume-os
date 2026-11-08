#ifndef LUME_ATFORK_H
#define LUME_ATFORK_H

struct lume_atfork_handler;

struct lume_atfork_snapshot
{
    struct lume_atfork_handler *first;
    struct lume_atfork_handler *last;
};

void lume_atfork_prepare(struct lume_atfork_snapshot *);
void lume_atfork_parent(const struct lume_atfork_snapshot *);
void lume_atfork_child(const struct lume_atfork_snapshot *);
void lume_pthread_fork_prepare(void);
void lume_pthread_fork_parent(void);
void lume_pthread_fork_child(void);

#endif
