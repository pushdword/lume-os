/* Initial Lume pager, not the upstream less port. Bounded memory, FD/seek,
 * UART input: space/Enter/b/g/G/q. No terminal escape parser required. */
#include "api.h"
#define PAGE_LINES 20u
#define PAGE_HISTORY 128u
static uint32_t starts[PAGE_HISTORY];
static char buffer[256];
static int key(void)
{
    for (;;)
    {
        unsigned char c;
        int n = app_read(0, &c, 1);
        if (n == 1)
            return c;
        if (n != -11)
            return 'q';
        lume_syscall(LUME_YIELD, 0, 0, 0);
    }
}
static int page(int fd, unsigned lines, uint32_t *position, int *end)
{
    unsigned count = 0, column = 0;
    *end = 0;
    while (count < lines)
    {
        int n = app_read(fd, buffer, sizeof buffer);
        if (n < 0)
            return n;
        if (!n)
        {
            *end = 1;
            break;
        }
        unsigned used = 0;
        while (used < (unsigned)n && count < lines)
        {
            unsigned c = (unsigned char)buffer[used++];
            if (c == '\n')
            {
                count++;
                column = 0;
            }
            else if (c == '\r')
                column = 0;
            else if (++column == 80)
            {
                column = 0;
                count++;
            }
        }
        /* Only display printable text; binary controls cannot operate a TTY. */
        for (unsigned i = 0; i < used; i++)
        {
            unsigned c = (unsigned char)buffer[i];
            if (c != '\n' && c != '\r' && c != '\t' && (c < 32 || c > 126))
                buffer[i] = '.';
        }
        if (app_write(1, buffer, used) != (int)used)
            return -5;
        *position += used;
        if (used < (unsigned)n && app_seek(fd, (int32_t)*position, LUME_SEEK_SET) < 0)
            return -5;
    }
    return 0;
}
int app_main(int argc, char **argv)
{
    if (argc != 2)
    {
        app_puts("usage: less file\n");
        return 2;
    }
    int fd = app_open(argv[1], LUME_RDONLY);
    if (fd < 0)
    {
        app_error("less", fd);
        return 1;
    }
    struct lume_stat stat;
    if (app_stat(argv[1], &stat) < 0 || stat.size > INT32_MAX)
    {
        (void)app_close(fd);
        app_puts("less: unsupported file size\n");
        return 1;
    }
    uint32_t position = 0;
    unsigned index = 0, lines = PAGE_LINES;
    int end = 0;
    for (;;)
    {
        int r = page(fd, lines, &position, &end);
        if (r < 0)
        {
            app_error("less", r);
            (void)app_close(fd);
            return 1;
        }
        if (position >= stat.size)
            end = 1;
        app_puts("\n-- less ");
        app_number(position);
        app_puts("/");
        app_number(stat.size);
        app_puts(end ? " (END)" : "");
        app_puts(" [space/Enter/b/g/G/q] --\n");
        int c;
        do
        {
            c = key();
        } while (!(c == 'q' || c == 3 || c == 27 || c == 'b' || c == 'g' || c == 'G' ||
                   (!end && (c == ' ' || c == '\r' || c == '\n'))));
        if (c == 'q' || c == 3 || c == 27)
            break;
        if (c == 'b' || c == 'g')
        {
            index = c == 'g' ? 0 : index ? index - 1
                                         : 0;
            position = starts[index];
            lines = PAGE_LINES;
        }
        else if (c == 'G')
        {
            /* Locate a bounded final screen by scanning a circular line index.
             * Memory stays constant even for a file larger than user RAM. */
            uint32_t last[PAGE_LINES], at = 0;
            unsigned total = 1;
            last[0] = 0;
            if (app_seek(fd, 0, LUME_SEEK_SET) < 0)
                break;
            int n;
            while ((n = app_read(fd, buffer, sizeof buffer)) > 0)
            {
                for (int i = 0; i < n; i++)
                {
                    at++;
                    if (buffer[i] == '\n')
                    {
                        last[total % PAGE_LINES] = at;
                        total++;
                    }
                }
            }
            if (n < 0)
            {
                app_error("less", n);
                break;
            }
            position = last[total > PAGE_LINES ? total % PAGE_LINES : 0];
            index = 0;
            starts[0] = position;
            lines = PAGE_LINES;
        }
        else if (!end && (c == ' ' || c == '\r' || c == '\n'))
        {
            if (index + 1 < PAGE_HISTORY)
                starts[++index] = position;
            else
            {
                for (unsigned i = 1; i < PAGE_HISTORY; i++)
                    starts[i - 1] = starts[i];
                starts[index] = position;
            }
            lines = c == ' ' ? PAGE_LINES : 1;
        }
        else
            continue;
        if (app_seek(fd, (int32_t)position, LUME_SEEK_SET) < 0)
            break;
    }
    return app_close(fd) < 0 ? 1 : 0;
}
