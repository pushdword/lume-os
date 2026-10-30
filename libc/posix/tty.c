#include "native_posix.h"
#include "lume/tty.h"
#include "lume/socket.h"
#include "lume/signal.h"
#include <fd.h>
#include <pty.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include "terminal_read.h"

int posix_openpt(int flags)
{
    if ((flags & O_ACCMODE) != O_RDWR || flags & ~(O_RDWR | O_NOCTTY | O_CLOEXEC))
    {
        errno = EINVAL;
        return -1;
    }
    int master, slave;
    if (openpty(&master, &slave, NULL, NULL, NULL))
        return -1;
    close(slave);
    if (flags & O_CLOEXEC)
        fcntl(master, F_SETFD, FD_CLOEXEC);
    return master;
}

char *ptsname(int master)
{
    return ttyname(master);
}

int grantpt(int master)
{
    return ttyname(master) ? 0 : -1;
}

int unlockpt(int master)
{
    return grantpt(master);
}

static int tty_call(unsigned action, struct lume_tty_request *r)
{
    r->version = LUME_TTY_VERSION;
    r->bytes = sizeof(*r);
    int64_t result;
    do
    {
        result = native_call(LUME_SYS_TTY, action, (uintptr_t)r, sizeof(*r));
    } while (result == -LUME_EAGAIN);
    return native_result(result);
}

struct terminal_reader_context
{
    int fd;
    uint64_t signal_epoch;
};

static uint64_t terminal_clock(void *context)
{
    (void)context;
    return (uint64_t)native_call(LUME_SYS_CLOCK, 0, 0, 0);
}

static int64_t terminal_read(void *context, void *buffer, size_t bytes)
{
    struct terminal_reader_context *reader = context;
    return native_call(LUME_SYS_READ, reader->fd, (uintptr_t)buffer, bytes);
}

static int terminal_wait(void *context, uint64_t timeout)
{
    struct terminal_reader_context *reader = context;
    struct lume_pollfd ready = {.fd = reader->fd, .events = LUME_POLLIN};
    struct lume_poll_request wait = {.version = LUME_POLL_VERSION,
                                     .bytes = sizeof(wait),
                                     .buffer = (uintptr_t)&ready,
                                     .count = 1,
                                     .timeout_ms = timeout,
                                     .flags = LUME_POLL_SIGNAL_EPOCH | LUME_POLL_TTY_READ,
                                     .signal_epoch = reader->signal_epoch};
    return (int)native_call(LUME_SYS_FD, LUME_FD_WAIT, (uintptr_t)&wait, sizeof(wait));
}

ssize_t lume_posix_terminal_read(int fd, void *buffer, size_t bytes, int *handled)
{
    int saved = errno;
    struct lume_tty_request mode = {.fd = fd};
    *handled = 0;
    if (tty_call(LUME_TTY_GET, &mode) || !mode.other)
    {
        errno = saved;
        return 0;
    }
    struct lume_fd_request flags = {.version = LUME_FD_VERSION, .bytes = sizeof(flags), .fd = fd};
    int status =
        native_result(native_call(LUME_SYS_FD, LUME_FD_GETFL, (uintptr_t)&flags, sizeof(flags)));
    if (status < 0 || (status & LUME_FD_NONBLOCK))
    {
        errno = saved;
        return 0;
    }
    *handled = 1;
    struct lume_signal_request signal = {.version = LUME_SIGNAL_VERSION, .bytes = sizeof(signal)};
    if (native_result(
            native_call(LUME_SYS_SIGNAL, LUME_SIGNAL_MASK, (uintptr_t)&signal, sizeof(signal))) < 0)
        return -1;
    struct terminal_reader_context context = {.fd = fd, .signal_epoch = signal.delay_ms};
    struct lume_terminal_reader reader = {terminal_clock, terminal_read, terminal_wait, &context};
    unsigned canonical = mode.termios.local & LUME_TTY_ICANON;
    int64_t result =
        lume_terminal_read(&reader, buffer, bytes, canonical ? 1 : mode.termios.characters[6],
                           canonical ? 0 : mode.termios.characters[5]);
    return result < 0 ? native_result(result) : (ssize_t)result;
}

int lume_posix_open_controlling(int flags)
{
    if ((flags & O_ACCMODE) != O_RDWR ||
        flags & ~(O_RDWR | O_NOCTTY | O_CLOEXEC | O_NONBLOCK | O_NOFOLLOW))
    {
        errno = ENOTSUP;
        return -1;
    }
    struct lume_tty_request r = {.value = (flags & O_CLOEXEC ? LUME_FD_CLOEXEC : 0) |
                                          (flags & O_NONBLOCK ? LUME_FD_NONBLOCK : 0)};
    return tty_call(LUME_TTY_OPEN, &r) ? -1 : r.fd;
}

void lume_posix_acquire_terminal(int fd)
{
    int saved = errno;
    struct lume_tty_request r = {.fd = fd};
    (void)tty_call(LUME_TTY_CONTROLLING, &r);
    errno = saved;
}

int tcgetattr(int fd, struct termios *termios)
{
    if (!termios)
    {
        errno = EFAULT;
        return -1;
    }
    struct lume_tty_request r = {.fd = fd};
    if (tty_call(LUME_TTY_GET, &r))
        return -1;
    termios->c_iflag = r.termios.input;
    termios->c_oflag = r.termios.output;
    termios->c_cflag = r.termios.control;
    termios->c_lflag = r.termios.local;
    memcpy(termios->c_cc, r.termios.characters, NCCS);
    termios->c_ispeed = r.termios.input_speed;
    termios->c_ospeed = r.termios.output_speed;
    return 0;
}

int tcsetattr(int fd, int action, const struct termios *termios)
{
    if (!termios || action < TCSANOW || action > TCSAFLUSH)
    {
        errno = !termios ? EFAULT : EINVAL;
        return -1;
    }
    struct lume_tty_request r = {.fd = fd,
                                 .value = (unsigned)action,
                                 .termios = {.input = termios->c_iflag,
                                             .output = termios->c_oflag,
                                             .control = termios->c_cflag,
                                             .local = termios->c_lflag,
                                             .input_speed = termios->c_ispeed,
                                             .output_speed = termios->c_ospeed}};
    memcpy(r.termios.characters, termios->c_cc, NCCS);
    return tty_call(LUME_TTY_SET, &r);
}

void cfmakeraw(struct termios *termios)
{
    termios->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    termios->c_oflag &= ~OPOST;
    termios->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    termios->c_cflag = (termios->c_cflag & ~(CSIZE | PARENB)) | CS8;
    termios->c_cc[VMIN] = 1;
    termios->c_cc[VTIME] = 0;
}

speed_t cfgetispeed(const struct termios *termios)
{
    return termios->c_ispeed;
}

speed_t cfgetospeed(const struct termios *termios)
{
    return termios->c_ospeed;
}

int cfsetispeed(struct termios *termios, speed_t speed)
{
    termios->c_ispeed = speed;
    return 0;
}

int cfsetospeed(struct termios *termios, speed_t speed)
{
    termios->c_ospeed = speed;
    return 0;
}

int tcflush(int fd, int action)
{
    struct lume_tty_request r = {.fd = fd, .value = (unsigned)action};
    return tty_call(LUME_TTY_FLUSH, &r);
}

int ioctl(int fd, unsigned long operation, ...)
{
    va_list arguments;
    va_start(arguments, operation);
    void *argument = operation == TIOCSCTTY ? NULL : va_arg(arguments, void *);
    va_end(arguments);
    struct lume_tty_request r = {.fd = fd};
    if (operation == TIOCSCTTY)
        return tty_call(LUME_TTY_CONTROLLING, &r);
    if (!argument)
    {
        errno = EFAULT;
        return -1;
    }
    if (operation == FIONBIO)
    {
        struct lume_fd_request request = {.version = LUME_FD_VERSION,
                                          .bytes = sizeof(request),
                                          .fd = fd,
                                          .flags = *(int *)argument ? LUME_FD_NONBLOCK : 0};
        return native_result(
            native_call(LUME_SYS_FD, LUME_FD_SETFL, (uintptr_t)&request, sizeof(request)));
    }
    if (operation == TIOCGWINSZ)
    {
        if (tty_call(LUME_TTY_SIZE, &r))
            return -1;
        *(struct winsize *)argument = (struct winsize){r.rows, r.columns, r.xpixel, r.ypixel};
        return 0;
    }
    if (operation == TIOCSWINSZ)
    {
        const struct winsize *size = argument;
        r.rows = size->ws_row;
        r.columns = size->ws_col;
        r.xpixel = size->ws_xpixel;
        r.ypixel = size->ws_ypixel;
        return tty_call(LUME_TTY_RESIZE, &r);
    }
    if (operation == TIOCSPGRP)
    {
        r.value = (unsigned)*(const pid_t *)argument;
        return tty_call(LUME_TTY_SET_GROUP, &r);
    }
    if (operation == TIOCGPGRP || operation == FIONREAD)
    {
        if (tty_call(operation == TIOCGPGRP ? LUME_TTY_GROUP : LUME_TTY_AVAILABLE, &r))
        {
            if (operation != FIONREAD || errno != ENOTTY)
                return -1;
            struct lume_socket_request socket = {
                .version = LUME_SOCKET_VERSION, .bytes = sizeof(socket), .fd = fd};
            if (native_result(native_call(LUME_SYS_SOCKET, LUME_SOCKET_PENDING, (uintptr_t)&socket,
                                          sizeof(socket))))
                return -1;
            r.value = socket.count;
        }
        *(int *)argument = (int)r.value;
        return 0;
    }
    errno = ENOTTY;
    return -1;
}

pid_t tcgetpgrp(int fd)
{
    pid_t group;
    return ioctl(fd, TIOCGPGRP, &group) ? -1 : group;
}

int tcsetpgrp(int fd, pid_t group)
{
    return ioctl(fd, TIOCSPGRP, &group);
}

char *ttyname(int fd)
{
    static _Thread_local char name[32];
    struct lume_tty_request r = {.fd = fd};
    if (tty_call(LUME_TTY_NAME, &r))
        return NULL;
    snprintf(name, sizeof(name), "/dev/pts/%u", r.value);
    return name;
}

int openpty(int *master, int *slave, char *name, const struct termios *termios,
            const struct winsize *size)
{
    if (!master || !slave)
    {
        errno = EFAULT;
        return -1;
    }
    struct lume_tty_request r = {0};
    if (tty_call(LUME_TTY_CREATE, &r))
        return -1;
    *master = r.fd;
    *slave = r.other;
    if ((termios && tcsetattr(*slave, TCSANOW, termios)) ||
        (size && ioctl(*slave, TIOCSWINSZ, size)))
    {
        int saved = errno;
        close(*master);
        close(*slave);
        errno = saved;
        return -1;
    }
    if (name)
    {
        char *path = ttyname(*slave);
        if (!path)
        {
            close(*master);
            close(*slave);
            return -1;
        }
        strcpy(name, path);
    }
    return 0;
}
