#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>

static unsigned events[8], count;
static pthread_mutex_t resource = PTHREAD_MUTEX_INITIALIZER;

static void prepare_first(void)
{
    assert(pthread_mutex_lock(&resource) == 0);
    events[count++] = 1;
}

static void parent_first(void)
{
    assert(pthread_mutex_unlock(&resource) == 0);
    events[count++] = 3;
}

static void child_first(void)
{
    assert(pthread_mutex_unlock(&resource) == 0);
    char name[32];
    assert(pthread_getname_np(pthread_self(), name, sizeof(name)) == 0);
    pthread_attr_t attribute;
    assert(pthread_getattr_np(pthread_self(), &attribute) == 0);
    assert(pthread_attr_destroy(&attribute) == 0);
    events[count++] = 5;
}

static void prepare_second(void)
{
    events[count++] = 2;
}

static void parent_second(void)
{
    events[count++] = 4;
}

static void child_second(void)
{
    events[count++] = 6;
}

int main(void)
{
    errno = E2BIG;
    assert(pthread_atfork(prepare_first, parent_first, child_first) == 0 && errno == E2BIG);
    assert(pthread_atfork(NULL, NULL, NULL) == 0 && errno == E2BIG);
    assert(pthread_atfork(prepare_second, parent_second, child_second) == 0 && errno == E2BIG);
    pid_t pid = fork();
    assert(pid >= 0);
    assert(count == 4 && events[0] == 2 && events[1] == 1);
    if (!pid)
        _exit(events[2] == 5 && events[3] == 6 ? 0 : 1);
    assert(events[2] == 3 && events[3] == 4);
    int status = -1;
    assert(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    puts("atfork PASS: native prepare, parent, child and wait");
    return 0;
}
