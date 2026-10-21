/* SPDX-License-Identifier: GPL-2.0-only
 * Bounded libbb compatibility for the selected applets, not a Linux shim. */
#include "libbb.h"
const char *applet_name;
const char bb_msg_write_error[] = "write error", bb_msg_standard_input[] = "standard input";
const char *const bb_argv_dash[] = {"-", NULL};
unsigned option_mask32;
int xfunc_error_retval = 1, logmode = LOGMODE_STDIO;
char bb_common_bufsiz1[512];
const struct suffix_mult bkm_suffixes[] = {{"b", 512}, {"k", 1024}, {"m", 1024 * 1024}, {NULL, 0}};

void bb_error_msg(const char *fmt, ...)
{
    if (!logmode)
        return;
    fprintf(stderr, "%s: ", applet_name);
    va_list v;
    va_start(v, fmt);
    vfprintf(stderr, fmt, v);
    va_end(v);
    fputc('\n', stderr);
}

void bb_simple_perror_msg(const char *s)
{
    bb_error_msg("%s: %s", s, strerror(errno));
}

_Noreturn void bb_show_usage(void)
{
    bb_error_msg("invalid arguments (busybox --help lists supported options)");
    exit(2);
}

void *xmalloc(size_t n)
{
    void *p = malloc(n ? n : 1);
    if (!p)
    {
        bb_simple_perror_msg("malloc");
        exit(xfunc_error_retval);
    }
    return p;
}

ssize_t safe_write(int fd, const void *p, size_t n)
{
    ssize_t r;
    do
    {
        r = write(fd, p, n);
    } while (r < 0 && errno == EINTR);
    return r;
}

char *last_char_is(const char *s, int c)
{
    size_t n = strlen(s);
    return n && s[n - 1] == c ? (char *)s + n - 1 : NULL;
}

/* Selected short-option grammar. Unsupported sort/mkdir features are rejected
 * before the applet runs. */
unsigned getopt32(char **argv, const char *spec, ...)
{
    int argc = 0;
    while (argv[argc])
        argc++;
    bool complement = *spec == '^';
    if (complement)
        spec++;
    char option[64], letters[32], numeric[32] = {0}, list[32] = {0};
    char **argument[32] = {0};
    unsigned bits = 0, n = 0;
    va_list v;
    va_start(v, spec);
    const char *q = spec;
    if (*q == '+')
        option[n++] = *q++;
    while (*q)
    {
        if (bits == 32 || n > sizeof option - 4)
            bb_show_usage();
        letters[bits] = *q;
        option[n++] = *q++;
        if (*q == ':')
        {
            option[n++] = *q++;
            argument[bits] = va_arg(v, char **);
            if (*q == '+')
            {
                numeric[bits] = 1;
                q++;
            }
            if (*q == '*')
            {
                list[bits] = 1;
                q++;
            } /* Accept repeated vi -c options and reject sort -k. */
        }
        bits++;
    }
    va_end(v);
    option[n] = 0;
    unsigned mask = 0;
    int c;
    while ((c = getopt(argc, argv, option)) != -1)
    {
        if (c == '?' || c == ':')
            bb_show_usage();
        unsigned bit = 0;
        while (bit < bits && letters[bit] != c)
            bit++;
        if (bit == bits)
            bb_show_usage();
        if ((!strcmp(applet_name, "sort") && !strchr("rus", c)) ||
            (!strcmp(applet_name, "mkdir") && !strchr("pv", c)))
            bb_show_usage();
        mask |= 1u << bit;
        if (argument[bit])
        {
            if (list[bit])
            {
                llist_t **tail = (llist_t **)argument[bit];
                while (*tail)
                    tail = &(*tail)->link;
                *tail = xzalloc(sizeof **tail);
                size_t length = strlen(optarg) + 1;
                (*tail)->data = memcpy(xmalloc(length), optarg, length);
            }
            else if (numeric[bit])
                *(unsigned *)argument[bit] = xatou_range(optarg, 0, INT_MAX);
            else
                *argument[bit] = optarg;
        }
    }
    unsigned operands = (unsigned)(argc - optind);
    if (complement)
    {
        const char *extra = spec + strlen(spec) + 1;
        if (strstr(extra, "-1") && !operands)
            bb_show_usage();
        if (strstr(extra, "?2") && operands > 2)
            bb_show_usage();
    }
    if (!strcmp(applet_name, "cmp") && (mask & 3) == 3)
        bb_show_usage();
    if (!strcmp(applet_name, "cut"))
    {
        unsigned kinds = mask & 7u;
        if (!kinds || (kinds & (kinds - 1)))
            bb_show_usage();
    }
    if (!strcmp(applet_name, "unexpand") && (mask & 2))
        mask |= 4;
    option_mask32 = mask;
    return mask;
}

int open_or_warn_stdin(const char *path)
{
    if (!strcmp(path, "-") || !strcmp(path, bb_msg_standard_input))
        return 0;
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        bb_simple_perror_msg(path);
    return fd;
}

FILE *fopen_or_warn_stdin(const char *path)
{
    if (!strcmp(path, "-") || !strcmp(path, bb_msg_standard_input))
        return stdin;
    FILE *f = fopen(path, "r");
    if (!f)
        bb_simple_perror_msg(path);
    return f;
}

int fclose_if_not_stdin(FILE *f)
{
    return f == stdin ? 0 : fclose(f);
}

off_t bb_copyfd_eof(int from, int to)
{
    char buf[512];
    off_t total = 0;
    for (;;)
    {
        ssize_t n = read(from, buf, sizeof buf);
        if (n < 0)
        {
            bb_simple_perror_msg("read");
            return -1;
        }
        if (!n)
            return total;
        if (full_write(to, buf, n) != n)
        {
            bb_simple_perror_msg("write");
            return -1;
        }
        total += n;
    }
}

unsigned long xatoul_sfx(const char *s, const struct suffix_mult *suffix)
{
    if (!*s || *s == '-' || *s == '+')
        bb_show_usage();
    errno = 0;
    char *end;
    unsigned long n = strtoul(s, &end, 10);
    if (errno || end == s)
        bb_show_usage();
    if (*end)
    {
        while (suffix->suffix && strcmp(suffix->suffix, end))
            suffix++;
        if (!suffix->suffix || n > ULONG_MAX / suffix->mult)
            bb_show_usage();
        n *= suffix->mult;
    }
    return n;
}

char *xrealloc_getcwd_or_warn(char *old)
{
    free(old);
    char *s = getcwd(NULL, 128);
    if (!s)
        bb_simple_perror_msg("getcwd");
    return s;
}

int fflush_all(void)
{
    int r = fflush(stdout);
    return (fflush(stderr) || r) ? 1 : 0;
}

void die_if_ferror_stdout(void)
{
    if (ferror(stdout))
    {
        bb_simple_perror_msg("stdout");
        exit(1);
    }
}

_Noreturn void fflush_stdout_and_exit(int n)
{
    if (fflush(stdout))
        n = 1;
    exit(n);
}

void *xrealloc(void *old, size_t n)
{
    void *p = realloc(old, n ? n : 1);
    if (!p)
    {
        bb_simple_perror_msg("realloc");
        exit(xfunc_error_retval);
    }
    return p;
}

void *xzalloc(size_t n)
{
    void *p = xmalloc(n);
    memset(p, 0, n);
    return p;
}

void *lume_vector(void *v, size_t size, unsigned shift, unsigned index)
{
    unsigned block = 1u << shift;
    if (!v || !(index & (block - 1)))
    {
        if (index > UINT_MAX - block - 1 || (index + block + 1) > SIZE_MAX / size)
            bb_error_msg_and_die("input too large");
        v = xrealloc(v, (index + block + 1) * size);
    }
    return v;
}

_Noreturn void bb_error_msg_and_die(const char *fmt, ...)
{
    if (logmode)
    {
        fprintf(stderr, "%s: ", applet_name);
        va_list v;
        va_start(v, fmt);
        vfprintf(stderr, fmt, v);
        va_end(v);
        fputc('\n', stderr);
    }
    exit(xfunc_error_retval);
}

_Noreturn void bb_simple_perror_msg_and_die(const char *s)
{
    bb_simple_perror_msg(s);
    exit(xfunc_error_retval);
}

void bb_perror_msg(const char *fmt, ...)
{
    if (!logmode)
        return;
    int saved = errno;
    fprintf(stderr, "%s: ", applet_name);
    va_list v;
    va_start(v, fmt);
    vfprintf(stderr, fmt, v);
    va_end(v);
    fprintf(stderr, ": %s\n", strerror(saved));
}

FILE *xfopen_stdin(const char *p)
{
    FILE *f = fopen_or_warn_stdin(p);
    if (!f)
        exit(xfunc_error_retval);
    return f;
}

static char *line(FILE *f, int keep)
{
    char *p = NULL;
    size_t capacity = 0;
    errno = 0;
    ssize_t n = getline(&p, &capacity, f);
    if (n < 0)
    {
        free(p);
        if (ferror(f) || errno == ENOMEM)
            bb_simple_perror_msg_and_die("read line");
        return NULL;
    }
    if (!keep && n && p[n - 1] == '\n')
        p[n - 1] = 0;
    return p;
}

char *xmalloc_fgets(FILE *f)
{
    return line(f, 1);
}

char *xmalloc_fgetline(FILE *f)
{
    return line(f, 0);
}

void fputs_stdout(const char *s)
{
    fputs(s, stdout);
}

void die_if_ferror(FILE *f, const char *name)
{
    if (ferror(f))
        bb_simple_perror_msg_and_die(name ? name : "read");
}

unsigned xatou_range(const char *s, unsigned low, unsigned high)
{
    if (!s || !isdigit((unsigned char)*s))
        bb_show_usage();
    errno = 0;
    char *end;
    unsigned long v = strtoul(s, &end, 10);
    if (errno || *end || v < low || v > high)
        bb_show_usage();
    return (unsigned)v;
}

int xatoi_positive(const char *s)
{
    return (int)xatou_range(s, 0, INT_MAX);
}

mode_t bb_parse_mode(const char *s, mode_t old)
{
    (void)s;
    (void)old;
    bb_error_msg_and_die("file modes are not stored on this FAT profile");
}

/* Reject mkdir -m because FAT exposes synthetic 0755 modes.
 * chmod and umask cannot change FAT permissions. */
int bb_make_directory(char *path, long mode, int flags)
{
    if (mode != -1)
    {
        errno = ENOSYS;
        bb_simple_perror_msg(path);
        return -1;
    }
    size_t n = strlen(path);
    if (!n)
    {
        errno = ENOENT;
        bb_simple_perror_msg(path);
        return -1;
    }
    for (size_t i = 1; i <= n; i++)
    {
        if (i < n && (path[i] != '/' || !(flags & FILEUTILS_RECUR)))
            continue;
        if (i < n && path[i - 1] == '/')
            continue;
        char saved = path[i];
        path[i] = 0;
        struct stat existing;
        if ((flags & FILEUTILS_RECUR) && !stat(path, &existing) && S_ISDIR(existing.st_mode))
        {
            path[i] = saved;
            continue;
        }
        int r = mkdir(path, 0755);
        int created = r == 0;
        if (r < 0 && errno == EEXIST && (flags & FILEUTILS_RECUR))
        {
            struct stat s;
            if (!stat(path, &s) && S_ISDIR(s.st_mode))
                r = 0;
        }
        if (r < 0)
        {
            bb_simple_perror_msg(path);
            path[i] = saved;
            return -1;
        }
        if (created && (flags & FILEUTILS_VERBOSE))
            printf("created directory: '%s'\n", path);
        path[i] = saved;
    }
    return 0;
}

ssize_t safe_read(int fd, void *p, size_t n)
{
    ssize_t r;
    do
    {
        r = read(fd, p, n);
    } while (r < 0 && errno == EINTR);
    return r;
}
