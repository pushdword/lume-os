/* SPDX-License-Identifier: GPL-2.0-only */
#include "libbb.h"
#include "api.h"
#include "fd_client.h"
#if defined(LUME_ARCH_AMD64)
#include <sys/ioctl.h>
#endif
struct globals *ptr_to_globals;
int vi_main(int, char **);
static int console_input = 1;
static struct lume_pollfd input_poll;
static struct lume_fd_request input_request;

void *memrchr(const void *p, int c, size_t n)
{
    const unsigned char *s = p;
    while (n)
        if (s[--n] == (unsigned char)c)
            return (void *)(s + n);
    return NULL;
}

char *strchrnul(const char *s, int c)
{
    while (*s && *s != (char)c)
        s++;
    return (char *)s;
}

char *skip_whitespace(const char *s)
{
    while (isspace((unsigned char)*s))
        s++;
    return (char *)s;
}

char *skip_non_whitespace(const char *s)
{
    while (*s && !isspace((unsigned char)*s))
        s++;
    return (char *)s;
}

int index_in_strings(const char *list, const char *s)
{
    int n = 0;
    while (*list)
    {
        if (!strcmp(list, s))
            return n;
        list += strlen(list) + 1;
        n++;
    }
    return -1;
}

void *llist_pop(llist_t **list)
{
    if (!*list)
        return NULL;
    llist_t *p = *list;
    void *data = p->data;
    *list = p->link;
    free(p);
    return data;
}

int app_main(int argc, char **argv)
{
    applet_name = "vi";
    optind = 0;
    opterr = 0;
    return vi_main(argc, argv);
}

int get_terminal_width_height(int fd, unsigned *width, unsigned *height)
{
#if defined(LUME_ARCH_AMD64)
    struct winsize size;
    if (!ioctl(fd, TIOCGWINSZ, &size))
    {
        *width = size.ws_col;
        *height = size.ws_row;
        return 0;
    }
#endif
    if (fd != 0)
    {
        errno = ENOTTY;
        return -1;
    }
    int geometry = isatty(fd) ? lume_syscall(LUME_TERMINAL, 0, 0, 0) : 80 | (24 << 16);
    if (geometry < 0)
    {
        errno = -geometry;
        return -1;
    }
    *width = (unsigned)geometry & 65535;
    *height = (unsigned)geometry >> 16;
    return 0;
}

int safe_poll(struct pollfd *fds, nfds_t count, int timeout)
{
    if (count != 1 || fds[0].fd != 0 || fds[0].events != POLLIN)
    {
        errno = EINVAL;
        return -1;
    }
    if (!console_input)
    {
        input_poll = (struct lume_pollfd){.fd = 0, .events = LUME_POLLIN};
        input_request = app_fd_request();
        input_request.buffer = lume_fd_pointer(&input_poll);
        input_request.count = 1;
        input_request.timeout_ms = timeout < 0 ? UINT32_MAX : (uint32_t)timeout;
        int ready = app_fd_call(LUME_FD_POLL, &input_request);
        if (ready < 0)
        {
            errno = -ready;
            return -1;
        }
        /* A closed writer must reach read() so that the editor can leave on EOF. */
        fds[0].revents = input_poll.revents ? POLLIN : 0;
        return ready;
    }
    uint32_t start = (uint32_t)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0);
    do
    {
        int ready = lume_syscall(LUME_TERMINAL, 1, 0, 0);
        if (ready < 0)
        {
            errno = -ready;
            return -1;
        }
        fds[0].revents = ready ? POLLIN : 0;
        if (ready)
            return 1;
        if (timeout == 0)
            return 0;
        if (timeout > 0 && (uint32_t)((uint32_t)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0) -
                                      start) >= (unsigned)timeout)
            return 0;
        lume_syscall(LUME_YIELD, 0, 0, 0);
    } while (1);
}

/* The kernel supplies unbuffered bytes. These adapters change only the calling process's
 * Picolibc carriage-return and EOF translations. */
void set_termios_to_raw(int fd, struct termios *saved, int flags)
{
    (void)flags;
    if (fd != 0)
        exit(2);
    memset(saved, 0, sizeof *saved);
    saved->c_cc[VERASE] = 127;
    console_input = isatty(fd);
#if defined(LUME_ARCH_AMD64)
    struct termios mode;
    if (!tcgetattr(fd, &mode))
        console_input = 0;
#endif
    lume_stdio_raw(1);
}

void tcsetattr_stdin_TCSANOW(const struct termios *saved)
{
    (void)saved;
    lume_stdio_raw(0);
}

int64_t safe_read_key(int fd, char *buffer, int timeout)
{
    if (buffer[0])
    {
        buffer[0] = 0;
        errno = 0;
        return (unsigned char)buffer[1];
    }
    struct pollfd p = {fd, POLLIN, 0};
    int ready = safe_poll(&p, 1, timeout);
    if (ready < 0)
        return -1;
    if (!ready)
    {
        errno = EAGAIN;
        return -1;
    }
    unsigned char c;
    errno = 0;
    if (read(fd, &c, 1) != 1)
        return -1;
    if (c != 27)
    {
        errno = 0;
        return c;
    }
    ready = safe_poll(&p, 1, 100);
    if (ready < 0)
        return -1;
    if (!ready)
    {
        errno = 0;
        return 27;
    }
    if (read(fd, &c, 1) != 1)
        return -1;
    if (c != '[' && c != 'O')
    {
        buffer[0] = 1;
        buffer[1] = (char)c;
        errno = 0;
        return 27;
    }
    unsigned n = 0;
    for (unsigned i = 0; i < 12; i++)
    {
        ready = safe_poll(&p, 1, 100);
        if (ready < 0)
            return -1;
        if (!ready)
        {
            errno = 0;
            return 27;
        }
        if (read(fd, &c, 1) != 1)
            return -1;
        if (c >= '0' && c <= '9')
        {
            n = n < 100 ? n * 10 + c - '0' : 100;
            continue;
        }
        int key = c == 'A'   ? KEYCODE_UP
                  : c == 'B' ? KEYCODE_DOWN
                  : c == 'C' ? KEYCODE_RIGHT
                  : c == 'D' ? KEYCODE_LEFT
                  : c == 'H' ? KEYCODE_HOME
                  : c == 'F' ? KEYCODE_END
                             : 0;
        if (c == '~')
            key = n == 1 || n == 7   ? KEYCODE_HOME
                  : n == 4 || n == 8 ? KEYCODE_END
                  : n == 2           ? KEYCODE_INSERT
                  : n == 3           ? KEYCODE_DELETE
                  : n == 5           ? KEYCODE_PAGEUP
                  : n == 6           ? KEYCODE_PAGEDOWN
                                     : 0;
        errno = 0;
        return key ? key : 27;
    }
    errno = 0;
    return 27;
}

ssize_t full_read(int fd, void *out, size_t count)
{
    size_t done = 0;
    while (done < count)
    {
        ssize_t n = read(fd, (char *)out + done, count - done);
        if (n < 0)
            return -1;
        if (!n)
            break;
        done += (size_t)n;
    }
    return (ssize_t)done;
}

ssize_t full_write(int fd, const void *in, size_t count)
{
    size_t done = 0;
    while (done < count)
    {
        ssize_t n = write(fd, (const char *)in + done, count - done);
        if (n < 0)
            return -1;
        if (!n)
            break;
        done += (size_t)n;
    }
    return (ssize_t)done;
}

char *xstrdup(const char *s)
{
    size_t n = strlen(s) + 1;
    return memcpy(xmalloc(n), s, n);
}

char *xstrndup(const char *s, size_t n)
{
    size_t length = strnlen(s, n);
    char *p = xmalloc(length + 1);
    memcpy(p, s, length);
    p[length] = 0;
    return p;
}

char *concat_path_file(const char *a, const char *b)
{
    size_t n = strlen(a), m = strlen(b);
    char *p = xmalloc(n + m + 2);
    memcpy(p, a, n);
    p[n] = '/';
    memcpy(p + n + 1, b, m + 1);
    return p;
}

char *xmalloc_open_read_close(const char *p, size_t *size)
{
    struct stat st;
    if (stat(p, &st) < 0)
        return NULL;
    size_t n = (size_t)st.st_size;
    char *out = xmalloc(n + 1);
    int fd = open(p, O_RDONLY);
    if (fd < 0)
    {
        free(out);
        return NULL;
    }
    ssize_t got = full_read(fd, out, n);
    close(fd);
    if (got < 0)
    {
        free(out);
        return NULL;
    }
    out[got] = 0;
    if (size)
        *size = (size_t)got;
    return out;
}

char *safe_strncpy(char *out, const char *in, size_t n)
{
    if (n)
    {
        strncpy(out, in, n - 1);
        out[n - 1] = 0;
    }
    return out;
}

int access(const char *p, int mode)
{
    struct stat st;
    if (stat(p, &st) < 0)
        return -1;
    if ((mode & W_OK) && !(st.st_mode & 0222))
    {
        errno = EACCES;
        return -1;
    }
    return 0;
}

uid_t getuid(void)
{
    return (uid_t)lume_syscall(LUME_UID, 0, 0, 0);
}

/* vi writes, then truncates at its current offset. The general POSIX
 * beyond-EOF offset semantics are deliberately not claimed by this adapter. */
int ftruncate(int fd, off_t size)
{
    off_t at = lseek(fd, 0, SEEK_CUR);
    if (at < 0)
        return -1;
    if (at != size)
    {
        errno = ENOTSUP;
        return -1;
    }
    int r = lume_syscall(LUME_TRUNCATE_HERE, (uint32_t)fd, 0, 0);
    if (r < 0)
    {
        errno = -r;
        return -1;
    }
    return 0;
}
