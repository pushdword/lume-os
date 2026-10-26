/* SPDX-License-Identifier: GPL-2.0-only */
#include "libbb.h"

void *ptr_to_globals;
void (*die_func)(void);
const char bb_msg_invalid_arg_to[] = "invalid argument '%s' to '%s'";
const char bb_msg_requires_arg[] = "%s requires an argument";

FILE *xfdopen_for_write(int fd)
{
    FILE *file = fdopen(fd, "w");
    if (!file)
        bb_simple_perror_msg_and_die("fdopen");
    return file;
}

_Noreturn void bb_die_memory_exhausted(void)
{
    bb_error_msg_and_die("out of memory");
}

char *bb_get_chunk_from_file(FILE *file, size_t *length)
{
    size_t used = 0, capacity = 128;
    char *buffer = xmalloc(capacity);
    int c;
    while ((c = fgetc(file)) != EOF)
    {
        if (used + 1 == capacity)
        {
            if (capacity > SIZE_MAX / 2)
                bb_die_memory_exhausted();
            capacity *= 2;
            buffer = xrealloc(buffer, capacity);
        }
        buffer[used++] = (char)c;
        if (c == '\n')
            break;
    }
    if (length)
        *length = used;
    if (!used)
    {
        free(buffer);
        return NULL;
    }
    buffer[used] = 0;
    return buffer;
}

void llist_add_to_end(llist_t **list, void *data)
{
    while (*list)
        list = &(*list)->link;
    llist_add_to(list, data);
}

static char *format_string(const char *format, va_list arguments)
{
    char *result;
    if (vasprintf(&result, format, arguments) < 0)
        bb_error_msg_and_die("out of memory");
    return result;
}

char *xasprintf(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    char *result = format_string(format, arguments);
    va_end(arguments);
    return result;
}

char *xasprintf_and_free(char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    char *result = format_string(format, arguments);
    va_end(arguments);
    free(format);
    return result;
}

FILE *fopen_or_warn(const char *path, const char *mode)
{
    FILE *file = fopen(path, mode);
    if (!file)
        bb_simple_perror_msg(path);
    return file;
}

int index_in_strings(const char *table, const char *text)
{
    for (int index = 0; *table; ++index)
    {
        if (!strcmp(table, text))
            return index;
        table += strlen(table) + 1;
    }
    return -1;
}

unsigned long xatoul(const char *text)
{
    return xatoul_sfx(text, (const struct suffix_mult[]){{NULL, 0}});
}

void xstat(const char *path, struct stat *metadata)
{
    if (stat(path, metadata))
        bb_simple_perror_msg_and_die(path);
}

int xmkstemp(char *path)
{
    int fd = mkstemp(path);
    if (fd < 0)
        bb_simple_perror_msg_and_die(path);
    return fd;
}

void xrename(const char *source, const char *target)
{
    if (rename(source, target))
        bb_simple_perror_msg_and_die(target);
}

char *xstrdup(const char *text)
{
    char *copy = strdup(text);
    if (!copy)
        bb_error_msg_and_die("out of memory");
    return copy;
}

char *xstrndup(const char *text, size_t count)
{
    size_t bytes = strnlen(text, count);
    char *copy = xmalloc(bytes + 1);
    memcpy(copy, text, bytes);
    copy[bytes] = 0;
    return copy;
}

char *is_prefixed_with(const char *text, const char *prefix)
{
    size_t bytes = strlen(prefix);
    return strncmp(text, prefix, bytes) ? NULL : (char *)text + bytes;
}

FILE *xfopen_for_write(const char *path)
{
    FILE *file = fopen(path, "w");
    if (!file)
        bb_simple_perror_msg_and_die(path);
    return file;
}

FILE *fopen_for_read(const char *path)
{
    return fopen(path, "r");
}

void llist_free(llist_t *list, void (*release)(void *))
{
    while (list)
    {
        llist_t *next = list->link;
        if (release)
            release(list->data);
        free(list);
        list = next;
    }
}

void llist_add_to(llist_t **list, void *data)
{
    llist_t *entry = xmalloc(sizeof(*entry));
    entry->data = data;
    entry->link = *list;
    *list = entry;
}

void *llist_pop(llist_t **list)
{
    if (!*list)
        return NULL;
    llist_t *entry = *list;
    void *data = entry->data;
    *list = entry->link;
    free(entry);
    return data;
}

int xopen(const char *path, int flags, ...)
{
    mode_t mode = 0666;
    if (flags & O_CREAT)
    {
        va_list arguments;
        va_start(arguments, flags);
        mode = va_arg(arguments, int);
        va_end(arguments);
    }
    int fd = open(path, flags, mode);
    if (fd < 0)
        bb_simple_perror_msg_and_die(path);
    return fd;
}

int xdup2(int source, int target)
{
    int result = dup2(source, target);
    if (result < 0)
        bb_simple_perror_msg_and_die("dup2");
    return result;
}

void close_on_exec_on(int fd)
{
    int flags = fcntl(fd, F_GETFD);
    if (flags < 0 || fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0)
        bb_simple_perror_msg_and_die("fcntl");
}

int spawn_and_wait(char **arguments)
{
    pid_t child;
    int error = posix_spawnp(&child, arguments[0], NULL, NULL, arguments, environ);
    if (error)
    {
        errno = error;
        return -1;
    }
    int status;
    while (waitpid(child, &status, 0) < 0)
        if (errno != EINTR)
            return -1;
    return WIFSIGNALED(status) ? 0x180 + WTERMSIG(status) : WEXITSTATUS(status);
}

unsigned bb_arg_max(void)
{
    return LUME_PORT_ARG_BYTES;
}

unsigned count_strstr(const char *text, const char *pattern)
{
    size_t bytes = strlen(pattern);
    unsigned count = 0;
    if (bytes)
        while ((text = strstr(text, pattern)))
        {
            ++count;
            text += bytes;
        }
    return count;
}

char *xmalloc_substitute_string(const char *text, unsigned count, const char *pattern,
                                const char *replacement)
{
    size_t before = strlen(pattern), after = strlen(replacement), length = strlen(text);
    if (after > before && count > (SIZE_MAX - length - 1) / (after - before))
        bb_error_msg_and_die("argument is too long");
    size_t total =
        after >= before ? length + count * (after - before) : length - count * (before - after);
    char *result = xmalloc(total + 1), *out = result;
    for (unsigned i = 0; i < count; ++i)
    {
        const char *match = strstr(text, pattern);
        if (!match || !before)
            break;
        size_t bytes = (size_t)(match - text);
        memcpy(out, text, bytes);
        out += bytes;
        memcpy(out, replacement, after);
        out += after;
        text = match + before;
    }
    strcpy(out, text);
    return result;
}
