/* Small freestanding runtime for the selected applets. No Linux libc/syscalls. */
#include "lume_libc.h"
int errno, optind = 1, opterr = 1, optopt;
char *optarg, **environ;
static FILE streams[16];
FILE *stdin = &streams[0], *stdout = &streams[1], *stderr = &streams[2];
static unsigned char heap[32768] __attribute__((aligned(16)));
static size_t heap_used;

int64_t lume_call(uint64_t n, uint64_t a, uint64_t b, uint64_t c)
{
    register uint64_t x8 __asm__("x8") = n, x0 __asm__("x0") = a, x1 __asm__("x1") = b,
                         x2 __asm__("x2") = c;
    __asm__ volatile("svc #0x4c55" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory", "cc");
    return (int64_t)x0;
}

static int64_t result(int64_t r)
{
    if (r < 0)
    {
        errno = (int)-r;
        return -1;
    }
    return r;
}

_Noreturn void exit(int status)
{
    (void)lume_call(LUME_SYS_EXIT, (unsigned)status, 0, 0);
    for (;;)
        __asm__ volatile("yield");
}

void *memcpy(void *d, const void *s, size_t n)
{
    unsigned char *a = d;
    const unsigned char *b = s;
    while (n--)
        *a++ = *b++;
    return d;
}

void *memmove(void *d, const void *s, size_t n)
{
    unsigned char *a = d;
    const unsigned char *b = s;
    if (a > b)
    {
        while (n)
        {
            --n;
            a[n] = b[n];
        }
    }
    else
        memcpy(d, s, n);
    return d;
}

void *memset(void *d, int c, size_t n)
{
    unsigned char *a = d;
    while (n--)
        *a++ = (unsigned char)c;
    return d;
}

int memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *x = a, *y = b;
    while (n--)
    {
        if (*x != *y)
            return *x - *y;
        ++x;
        ++y;
    }
    return 0;
}

size_t strlen(const char *s)
{
    size_t n = 0;
    while (s[n])
        ++n;
    return n;
}

int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b)
    {
        ++a;
        ++b;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n)
{
    while (n--)
    {
        if (*a != *b || !*a)
            return (unsigned char)*a - (unsigned char)*b;
        ++a;
        ++b;
    }
    return 0;
}

char *stpcpy(char *d, const char *s)
{
    while ((*d = *s))
    {
        ++d;
        ++s;
    }
    return d;
}

char *strcpy(char *d, const char *s)
{
    (void)stpcpy(d, s);
    return d;
}

char *strchr(const char *s, int c)
{
    do
    {
        if ((unsigned char)*s == (unsigned char)c)
            return (char *)s;
    } while (*s++);
    return 0;
}

char *strrchr(const char *s, int c)
{
    const char *p = 0;
    do
    {
        if ((unsigned char)*s == (unsigned char)c)
            p = s;
    } while (*s++);
    return (char *)p;
}

char *strstr(const char *s, const char *q)
{
    size_t n = strlen(q);
    do
    {
        if (!strncmp(s, q, n))
            return (char *)s;
    } while (*s++);
    return 0;
}

void *malloc(size_t n)
{
    if (n > sizeof(heap) - 16)
    {
        errno = ENOMEM;
        return 0;
    }
    size_t size = (n + 31) & ~(size_t)15;
    if (size > sizeof(heap) - heap_used)
    {
        errno = ENOMEM;
        return 0;
    }
    size_t *p = (void *)(heap + heap_used);
    *p = n;
    heap_used += size;
    return (unsigned char *)p + 16;
}

void free(void *p)
{
    (void)p;
} /* Arena lifetime is the child process lifetime. */

void *realloc(void *p, size_t n)
{
    if (!p)
        return malloc(n);
    void *q = malloc(n);
    if (q)
    {
        size_t old = *(size_t *)((unsigned char *)p - 16);
        memcpy(q, p, old < n ? old : n);
    }
    return q;
}

char *strdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (p)
        memcpy(p, s, n);
    return p;
}

int isspace(int c)
{
    return c == ' ' || (c >= 9 && c <= 13);
}

int isdigit(int c)
{
    return c >= '0' && c <= '9';
}

int isprint(int c)
{
    return c >= 32 && c < 127;
}

unsigned long strtoul(const char *s, char **end, int base)
{
    while (isspace((unsigned char)*s))
        ++s;
    bool negative = *s == '-';
    if (*s == '-' || *s == '+')
        ++s;
    const char *start = s;
    if (!base)
        base = *s == '0' ? 8 : 10;
    if (base == 16 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        s += 2;
    unsigned long n = 0;
    bool any = false, overflow = false;
    while (*s)
    {
        unsigned digit = *s >= '0' && *s <= '9'   ? (unsigned)(*s - '0')
                         : *s >= 'a' && *s <= 'z' ? (unsigned)(*s - 'a' + 10)
                         : *s >= 'A' && *s <= 'Z' ? (unsigned)(*s - 'A' + 10)
                                                  : 36;
        if (digit >= (unsigned)base)
            break;
        any = true;
        if (n > (ULONG_MAX - digit) / (unsigned)base)
            overflow = true;
        else
            n = n * (unsigned)base + digit;
        ++s;
    }
    if (end)
        *end = (char *)(any ? s : start);
    if (overflow)
    {
        errno = ERANGE;
        return ULONG_MAX;
    }
    return negative ? 0 - n : n;
}

unsigned long long strtoull(const char *s, char **e, int b)
{
    return strtoul(s, e, b);
}

int atoi(const char *s)
{
    return (int)strtoul(s, 0, 10);
}

char *strerror(int e)
{
    switch (e)
    {
    case 0:
        return "success";
    case ENOENT:
        return "not found";
    case ENOMEM:
        return "out of memory";
    case LUME_EROFS:
        return "read-only filesystem";
    case LUME_ENOSPC:
        return "no space";
    case LUME_ENOTEMPTY:
        return "directory not empty";
    case LUME_ENOTDIR:
        return "not a directory";
    default:
        return "I/O or argument error";
    }
}

char *getenv(const char *name)
{
    size_t n = strlen(name);
    if (environ)
        for (char **p = environ; *p; ++p)
            if (!strncmp(*p, name, n) && (*p)[n] == '=')
                return *p + n + 1;
    return 0;
}

char *getcwd(char *p, size_t n)
{
    if (!p)
    {
        n = n ? n : LUME_PATH_MAX;
        p = malloc(n);
        if (!p)
            return 0;
    }
    return result(lume_call(LUME_SYS_GETCWD, (uintptr_t)p, n, 0)) < 0 ? 0 : p;
}

int open(const char *p, int f, ...)
{
    return (int)result(lume_call(LUME_SYS_OPEN, (uintptr_t)p, (unsigned)f, 0));
}

int close(int fd)
{
    return (int)result(lume_call(LUME_SYS_CLOSE, (unsigned)fd, 0, 0));
}

ssize_t read(int fd, void *p, size_t n)
{
    if (n > 4096)
        n = 4096;
    int64_t r;
    do
    {
        r = lume_call(LUME_SYS_READ, (unsigned)fd, (uintptr_t)p, n);
    } while (r == -LUME_EAGAIN);
    if (fd == 0 && r == 1 && *(unsigned char *)p == 4)
        return 0;
    return result(r);
}

ssize_t write(int fd, const void *p, size_t n)
{
    size_t done = 0;
    while (done < n)
    {
        size_t k = n - done;
        if (k > 4096)
            k = 4096;
        int64_t r = lume_call(LUME_SYS_WRITE, (unsigned)fd, (uintptr_t)((const char *)p + done), k);
        if (r < 0)
            return done ? (ssize_t)done : result(r);
        if (!r)
            break;
        done += (size_t)r;
    }
    return (ssize_t)done;
}

off_t lseek(int fd, off_t n, int mode)
{
    return result(lume_call(LUME_SYS_SEEK, (unsigned)fd, (uint64_t)n, (unsigned)mode));
}

int stat(const char *p, struct stat *s)
{
    int fd = open(p, O_RDONLY);
    if (fd < 0)
        return -1;
    struct lume_stat st;
    int64_t r = lume_call(LUME_SYS_STAT, (unsigned)fd, (uintptr_t)&st, 0);
    close(fd);
    if (result(r) < 0)
        return -1;
    s->st_size = (off_t)st.size;
    s->st_mode = (st.kind == LUME_NODE_DIR ? S_IFDIR : S_IFREG) | 0755;
    s->st_ino = st.inode;
    s->st_dev = 1;
    return 0;
}

int mkdir(const char *p, mode_t mode)
{
    (void)mode;
    return (int)result(lume_call(LUME_SYS_MKDIR, (uintptr_t)p, 0, 0));
}

int rmdir(const char *p)
{
    return (int)result(lume_call(LUME_SYS_RMDIR, (uintptr_t)p, 0, 0));
}

int unlink(const char *p)
{
    return (int)result(lume_call(LUME_SYS_UNLINK, (uintptr_t)p, 0, 0));
}

char *dirname(char *p)
{
    if (!p || !*p)
        return ".";
    size_t n = strlen(p);
    while (n > 1 && p[n - 1] == '/')
        p[--n] = 0;
    while (n && p[n - 1] != '/')
        --n;
    if (!n)
        return ".";
    while (n > 1 && p[n - 1] == '/')
        --n;
    p[n] = 0;
    return p;
}

FILE *fopen(const char *p, const char *m)
{
    int flags = *m == 'r'   ? O_RDONLY
                : *m == 'a' ? O_WRONLY | O_CREAT | O_APPEND
                            : O_WRONLY | O_CREAT | O_TRUNC;
    int fd = open(p, flags);
    if (fd < 0)
        return 0;
    for (unsigned i = 3; i < 16; ++i)
        if (!streams[i].used)
        {
            streams[i] = (FILE){fd, 0, 0, -1, 1};
            return &streams[i];
        }
    close(fd);
    errno = LUME_EMFILE;
    return 0;
}

int fclose(FILE *f)
{
    int r = close(f->fd);
    f->used = 0;
    return r;
}

int fgetc(FILE *f)
{
    if (f->ungot >= 0)
    {
        int c = f->ungot;
        f->ungot = -1;
        return c;
    }
    unsigned char c;
    ssize_t n = read(f->fd, &c, 1);
    if (n == 1)
        return c;
    if (n < 0)
        f->error = 1;
    else
        f->eof = 1;
    return EOF;
}

int getc(FILE *f)
{
    return fgetc(f);
}

int ungetc(int c, FILE *f)
{
    if (c < 0 || f->ungot >= 0)
        return EOF;
    f->ungot = c;
    f->eof = 0;
    return c;
}

int ferror(FILE *f)
{
    return f->error;
}

int feof(FILE *f)
{
    return f->eof;
}

void clearerr(FILE *f)
{
    f->error = f->eof = 0;
}

int fputc(int c, FILE *f)
{
    unsigned char b = (unsigned char)c;
    if (write(f->fd, &b, 1) != 1)
    {
        f->error = 1;
        return EOF;
    }
    return b;
}

int putchar(int c)
{
    return fputc(c, stdout);
}

int fputs(const char *s, FILE *f)
{
    size_t n = strlen(s);
    if (write(f->fd, s, n) != (ssize_t)n)
    {
        f->error = 1;
        return EOF;
    }
    return 0;
}

int puts(const char *s)
{
    return fputs(s, stdout) < 0 || fputc('\n', stdout) < 0 ? EOF : 0;
}

int fflush(FILE *f)
{
    return f ? f->error ? -1 : 0 : streams[1].error || streams[2].error ? -1 : 0;
}

size_t fread(void *p, size_t size, size_t count, FILE *f)
{
    if (!size || count > SIZE_MAX / size)
        return 0;
    size_t done = 0, total = size * count;
    while (done < total)
    {
        int c = fgetc(f);
        if (c == EOF)
            break;
        ((unsigned char *)p)[done++] = (unsigned char)c;
    }
    return done / size;
}

size_t fwrite(const void *p, size_t size, size_t count, FILE *f)
{
    if (!size || count > SIZE_MAX / size)
        return 0;
    ssize_t n = write(f->fd, p, size * count);
    if (n < 0)
    {
        f->error = 1;
        return 0;
    }
    return (size_t)n / size;
}

ssize_t getline(char **p, size_t *cap, FILE *f)
{
    size_t n = 0;
    for (;;)
    {
        int c = fgetc(f);
        if (c == EOF)
        {
            if (!n)
                return -1;
            break;
        }
        if (n + 1 >= *cap)
        {
            size_t k = *cap ? *cap * 2 : 64;
            char *q = realloc(*p, k);
            if (!q)
                return -1;
            *p = q;
            *cap = k;
        }
        (*p)[n++] = (char)c;
        if (c == '\n')
            break;
    }
    (*p)[n] = 0;
    return (ssize_t)n;
}

struct output
{
    char *p;
    size_t capacity, count;
};

static void out(struct output *o, char c)
{
    if (o->count + 1 < o->capacity)
        o->p[o->count] = c;
    ++o->count;
}

int vsnprintf(char *p, size_t capacity, const char *fmt, va_list ap)
{
    struct output o = {p, capacity, 0};
    while (*fmt)
    {
        if (*fmt != '%')
        {
            out(&o, *fmt++);
            continue;
        }
        ++fmt;
        bool left = false, zero = false;
        if (*fmt == '-')
        {
            left = true;
            ++fmt;
        }
        if (*fmt == '0')
        {
            zero = true;
            ++fmt;
        }
        unsigned width = 0;
        int precision = -1;
        if (*fmt == '*')
        {
            int w = va_arg(ap, int);
            if (w < 0)
            {
                left = true;
                w = -w;
            }
            width = (unsigned)w;
            ++fmt;
        }
        else
            while (isdigit(*fmt))
                width = width * 10 + (unsigned)(*fmt++ - '0');
        if (*fmt == '.')
        {
            ++fmt;
            precision = 0;
            if (*fmt == '*')
            {
                precision = va_arg(ap, int);
                ++fmt;
            }
            else
                while (isdigit(*fmt))
                    precision = precision * 10 + *fmt++ - '0';
        }
        unsigned length = 0;
        if (*fmt == 'z')
        {
            length = 1;
            ++fmt;
        }
        else
            while (*fmt == 'l')
            {
                ++length;
                ++fmt;
            }
        char temp[32], single;
        const char *s = temp;
        size_t n = 0;
        char code = *fmt;
        if (!code)
            break;
        ++fmt;
        if (code == 's')
        {
            s = va_arg(ap, const char *);
            if (!s)
                s = "(null)";
            n = strlen(s);
            if (precision >= 0 && n > (unsigned)precision)
                n = (unsigned)precision;
        }
        else if (code == 'c' || code == '%')
        {
            single = code == '%' ? '%' : (char)va_arg(ap, int);
            s = &single;
            n = 1;
        }
        else
        {
            if (!strchr("diuxXo", code))
            {
                errno = EINVAL;
                return -1;
            }
            bool negative = false;
            uint64_t v;
            if (code == 'd' || code == 'i')
            {
                int64_t signed_value = length == 2 ? va_arg(ap, long long)
                                       : length    ? va_arg(ap, long)
                                                   : va_arg(ap, int);
                negative = signed_value < 0;
                v = negative ? 0 - (uint64_t)signed_value : (uint64_t)signed_value;
            }
            else
                v = length == 2 ? va_arg(ap, unsigned long long)
                    : length    ? va_arg(ap, unsigned long)
                                : va_arg(ap, unsigned);
            unsigned base = code == 'x' || code == 'X' ? 16 : code == 'o' ? 8 : 10;
            do
            {
                temp[n++] = "0123456789abcdef"[v % base];
                v /= base;
            } while (v);
            if (negative)
                temp[n++] = '-';
            for (size_t i = 0; i < n / 2; ++i)
            {
                char c = temp[i];
                temp[i] = temp[n - 1 - i];
                temp[n - 1 - i] = c;
            }
        }
        if (width > 1024)
            width = 1024;
        if (!left)
            while (width > n)
            {
                out(&o, zero ? '0' : ' ');
                --width;
            }
        for (size_t i = 0; i < n; ++i)
            out(&o, s[i]);
        if (left)
            while (width > n)
            {
                out(&o, ' ');
                --width;
            }
    }
    if (capacity)
        p[o.count < capacity ? o.count : capacity - 1] = 0;
    return (int)o.count;
}

int snprintf(char *p, size_t n, const char *f, ...)
{
    va_list a;
    va_start(a, f);
    int r = vsnprintf(p, n, f, a);
    va_end(a);
    return r;
}

int vfprintf(FILE *f, const char *fmt, va_list a)
{
    char p[1024];
    int n = vsnprintf(p, sizeof(p), fmt, a);
    if (n < 0 || n >= (int)sizeof(p))
    {
        errno = LUME_E2BIG;
        f->error = 1;
        return -1;
    }
    if (write(f->fd, p, (size_t)n) != n)
    {
        f->error = 1;
        return -1;
    }
    return n;
}

int fprintf(FILE *f, const char *fmt, ...)
{
    va_list a;
    va_start(a, fmt);
    int n = vfprintf(f, fmt, a);
    va_end(a);
    return n;
}

int printf(const char *fmt, ...)
{
    va_list a;
    va_start(a, fmt);
    int n = vfprintf(stdout, fmt, a);
    va_end(a);
    return n;
}

int getopt(int argc, char *const *argv, const char *spec)
{
    static unsigned pos;
    if (!optind)
    {
        optind = 1;
        pos = 0;
    }
    optarg = 0;
    if (optind >= argc)
        return -1;
    const char *arg = argv[optind];
    if (!pos)
    {
        if (arg[0] != '-' || !arg[1])
            return -1;
        if (!strcmp(arg, "--"))
        {
            ++optind;
            return -1;
        }
        pos = 1;
    }
    int c = (unsigned char)arg[pos++];
    const char *p = strchr(spec, c);
    if (!arg[pos])
    {
        pos = 0;
        ++optind;
    }
    if (!p || c == ':')
    {
        optopt = c;
        return '?';
    }
    if (p[1] == ':')
    {
        if (pos)
        {
            optarg = (char *)arg + pos;
            pos = 0;
            ++optind;
        }
        else if (optind < argc)
            optarg = argv[optind++];
        else
        {
            optopt = c;
            return '?';
        }
    }
    return c;
}

extern int app_main(int, char **);

int lume_start(int argc, char **argv, char **envp)
{
    environ = envp;
    for (unsigned i = 0; i < 3; ++i)
        streams[i] = (FILE){(int)i, 0, 0, -1, 1};
    return app_main(argc, argv);
}
