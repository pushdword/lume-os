#define _POSIX_C_SOURCE 200809L
#include "pkg_format.h"
#include "sha256.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef LUME_PKG_HOST_TEST
#define PATH_BYTES 512u
#define fsync _commit
#define mkdir(path, mode) _mkdir(path)
#else
#define PATH_BYTES 128u
#endif

static const char *root = "/data";
static struct pkg_catalog catalog;
static struct pkg_manifest incoming, previous, other;
static unsigned char buffer[16384];
static char origin[256];
static bool locked;

struct repository_header
{
    char magic[8], source[256];
    unsigned char digest[32];
};

struct transaction_ready
{
    uint32_t has_previous;
    unsigned char incoming[32], previous[32];
};

static void manifest_digest(const struct pkg_manifest *manifest, unsigned char digest[32])
{
    struct lume_sha256 hash;
    lume_sha256_init(&hash);
    lume_sha256_update(&hash, manifest,
                       offsetof(struct pkg_manifest, file) +
                           manifest->identity.files * sizeof(struct pkg_file));
    lume_sha256_final(&hash, digest);
}

static int fail(const char *message)
{
    fprintf(stderr, "pkg: %s\n", message);
    return -1;
}

static int path_join(char out[PATH_BYTES], const char *base, const char *name)
{
    int n = snprintf(out, PATH_BYTES, "%s/%s", base, name);
    return n >= 0 && (unsigned)n < PATH_BYTES ? 0 : -1;
}

static int state_path(char out[PATH_BYTES], const char *name)
{
    char state[PATH_BYTES];
    return path_join(state, root, "pkg") || path_join(out, state, name) ? -1 : 0;
}

static int exists(const char *path)
{
    struct stat st;
    if (!lstat(path, &st))
        return S_ISREG(st.st_mode) ? 1 : -1;
    return errno == ENOENT ? 0 : -1;
}

static int directory(const char *path)
{
    struct stat st;
    if (!lstat(path, &st))
        return S_ISDIR(st.st_mode) ? 0 : -1;
    return errno == ENOENT ? mkdir(path, 0700) : -1;
}

static int parents(char *path)
{
    size_t start = strlen(root);
    if (strncmp(path, root, start) || path[start] != '/')
        return -1;
    for (size_t i = start + 1; path[i]; ++i)
        if (path[i] == '/')
        {
            path[i] = 0;
            int result = directory(path);
            path[i] = '/';
            if (result)
                return -1;
        }
    return 0;
}

static int read_exact(int fd, void *data, size_t bytes)
{
    unsigned char *p = data;
    while (bytes)
    {
        size_t chunk = bytes < sizeof(buffer) ? bytes : sizeof(buffer);
        ssize_t n = read(fd, p, chunk);
        if (n <= 0 || (size_t)n > bytes)
            return -1;
        p += n;
        bytes -= (size_t)n;
    }
    return 0;
}

static int write_exact(int fd, const void *data, size_t bytes)
{
    const unsigned char *p = data;
    while (bytes)
    {
        size_t chunk = bytes < sizeof(buffer) ? bytes : sizeof(buffer);
        ssize_t n = write(fd, p, chunk);
        if (n <= 0 || (size_t)n > bytes)
            return -1;
        p += n;
        bytes -= (size_t)n;
    }
    return 0;
}

static int create(const char *path, const void *data, size_t bytes)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd < 0)
        return -1;
    int result = write_exact(fd, data, bytes);
    if (fsync(fd))
        result = -1;
    if (close(fd))
        result = -1;
    return result;
}

static int hash_file(const char *path, uint32_t bytes, const unsigned char expected[32])
{
    if (exists(path) != 1)
        return -1;
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;
    struct lume_sha256 hash;
    lume_sha256_init(&hash);
    uint32_t total = 0;
    int result = -1;
    for (;;)
    {
        ssize_t n = read(fd, buffer, sizeof(buffer));
        if (n < 0 || (uint32_t)n > bytes - total)
            break;
        if (!n)
        {
            unsigned char digest[32];
            lume_sha256_final(&hash, digest);
            result = total == bytes && !memcmp(digest, expected, 32) ? 0 : -1;
            break;
        }
        total += (uint32_t)n;
        lume_sha256_update(&hash, buffer, (size_t)n);
    }
    if (close(fd))
        result = -1;
    return result;
}

static int copy_file(const char *source, const char *target)
{
    int input = open(source, O_RDONLY);
    if (input < 0)
        return -1;
    int output = open(target, O_WRONLY | O_CREAT | O_EXCL, 0600);
    int result = output < 0 ? -1 : 0;
    while (!result)
    {
        ssize_t n = read(input, buffer, sizeof(buffer));
        if (n < 0 || (n && write_exact(output, buffer, (size_t)n)))
            result = -1;
        if (n <= 0)
            break;
    }
    if (output >= 0)
    {
        if (fsync(output))
            result = -1;
        if (close(output))
            result = -1;
    }
    if (close(input))
        result = -1;
    return result;
}

static bool token(const char *text, size_t capacity)
{
    size_t n = 0;
    for (; n < capacity && text[n]; ++n)
    {
        unsigned c = (unsigned char)text[n];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '.' ||
              c == '+' || c == '_'))
            return false;
    }
    if (!n || n == capacity || text[0] == '.' || text[n - 1] == '.')
        return false;
    for (; n < capacity; ++n)
        if (text[n])
            return false;
    return true;
}

static bool identity_valid(const struct pkg_identity *id)
{
    if (!token(id->name, sizeof(id->name)) || !token(id->version, sizeof(id->version)) ||
        !id->revision || id->abi != PKG_ABI || id->files > PKG_FILES_MAX ||
        id->dependencies > PKG_DEPS_MAX)
        return false;
    for (unsigned i = 0; i < PKG_DEPS_MAX; ++i)
    {
        const struct pkg_dependency *dep = &id->dependency[i];
        if (i < id->dependencies)
        {
            if (!token(dep->name, sizeof(dep->name)) || !dep->revision ||
                !strcmp(dep->name, id->name))
                return false;
            for (unsigned j = 0; j < i; ++j)
                if (!strcmp(dep->name, id->dependency[j].name))
                    return false;
        }
        else
            for (unsigned j = 0; j < sizeof(*dep); ++j)
                if (((const unsigned char *)dep)[j])
                    return false;
    }
    return true;
}

static bool payload_path(const char text[96])
{
    size_t n = 0;
    while (n < 96 && text[n])
        ++n;
    if (!n || n == 96 || n + 6 >= 128)
        return false;
    for (size_t i = n; i < 96; ++i)
        if (text[i])
            return false;
    char component[48] = {0};
    unsigned used = 0, parts = 0;
    for (size_t i = 0; i <= n; ++i)
        if (!text[i] || text[i] == '/')
        {
            component[used] = 0;
            if (!token(component, sizeof(component)))
                return false;
            if (!parts && strcmp(component, "bin") && strcmp(component, "sdk") &&
                strcmp(component, "lib") && strcmp(component, "libexec") &&
                strcmp(component, "share") && strcmp(component, "etc"))
                return false;
            ++parts;
            used = 0;
            memset(component, 0, sizeof(component));
        }
        else
        {
            if (used + 1 >= sizeof(component))
                return false;
            unsigned c = (unsigned char)text[i];
            component[used++] = c >= 'A' && c <= 'Z' ? (char)(c + 'a' - 'A') : (char)c;
        }
    return parts >= 2;
}

static int path_compare(const char *a, const char *b, size_t limit)
{
    for (size_t i = 0; i < limit; ++i)
    {
        unsigned x = (unsigned char)a[i], y = (unsigned char)b[i];
        if (x >= 'A' && x <= 'Z')
            x += 'a' - 'A';
        if (y >= 'A' && y <= 'Z')
            y += 'a' - 'A';
        if (x != y || !x)
            return x == y ? 0 : x < y ? -1 : 1;
    }
    return 0;
}

static size_t manifest_bytes(const struct pkg_manifest *m)
{
    return offsetof(struct pkg_manifest, file) + m->identity.files * sizeof(struct pkg_file);
}

static int read_manifest(const char *path, struct pkg_manifest *m, bool archive)
{
    memset(m, 0, sizeof(*m));
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;
    int result = -1;
    if (read_exact(fd, m, offsetof(struct pkg_manifest, file)) ||
        memcmp(m->magic, PKG_FILE_MAGIC, 8) || !identity_valid(&m->identity) ||
        read_exact(fd, m->file, m->identity.files * sizeof(struct pkg_file)))
        goto done;
    uint64_t total = manifest_bytes(m);
    for (unsigned i = 0; i < m->identity.files; ++i)
    {
        if (!payload_path(m->file[i].path) || m->file[i].bytes > PKG_BYTES_MAX)
            goto done;
        total += m->file[i].bytes;
        for (unsigned j = 0; j < i; ++j)
        {
            const char *a = m->file[i].path, *b = m->file[j].path;
            size_t n = strlen(a), k = strlen(b), short_n = n < k ? n : k;
            if (!path_compare(a, b, short_n) && (n == k || a[short_n] == '/' || b[short_n] == '/'))
                goto done;
        }
    }
    struct stat st;
    if (fstat(fd, &st) || total > PKG_BYTES_MAX ||
        st.st_size != (off_t)(archive ? total : manifest_bytes(m)))
        goto done;
    result = 0;
done:
    if (close(fd))
        result = -1;
    return result;
}

static int receipt_path(char out[PATH_BYTES], const char *name)
{
    char db[PATH_BYTES];
    return state_path(db, "db") || path_join(out, db, name) ? -1 : 0;
}

static int load_installed(const char *name, struct pkg_manifest *m)
{
    char path[PATH_BYTES];
    if (receipt_path(path, name))
        return -1;
    int found = exists(path);
    if (found <= 0)
    {
        memset(m, 0, sizeof(*m));
        return found;
    }
    return read_manifest(path, m, false) || strcmp(m->identity.name, name) ? -1 : 1;
}

static int file_index(const struct pkg_manifest *m, const char *path)
{
    for (unsigned i = 0; i < m->identity.files; ++i)
        if (!path_compare(m->file[i].path, path, 96))
            return (int)i;
    return -1;
}

static int check_files(const struct pkg_manifest *m)
{
    char path[PATH_BYTES];
    for (unsigned i = 0; i < m->identity.files; ++i)
        if (path_join(path, root, m->file[i].path) ||
            hash_file(path, m->file[i].bytes, m->file[i].digest))
            return fail("installed file differs from its receipt");
    return 0;
}

static int installed_package_matches(const struct pkg_manifest *m,
                                     const struct pkg_catalog_entry *entry)
{
    if (memcmp(&m->identity, &entry->identity, sizeof(m->identity)))
        return -1;
    struct lume_sha256 archive_hash;
    lume_sha256_init(&archive_hash);
    lume_sha256_update(&archive_hash, m, manifest_bytes(m));
    uint64_t total = manifest_bytes(m);
    for (unsigned i = 0; i < m->identity.files; ++i)
    {
        const struct pkg_file *file = &m->file[i];
        char path[PATH_BYTES];
        if (path_join(path, root, file->path) || exists(path) != 1)
            return -1;
        int fd = open(path, O_RDONLY);
        if (fd < 0)
            return -1;
        struct lume_sha256 file_hash;
        lume_sha256_init(&file_hash);
        unsigned left = file->bytes;
        int result = 0;
        while (left)
        {
            size_t count = left > sizeof(buffer) ? sizeof(buffer) : left;
            if (read_exact(fd, buffer, count))
            {
                result = -1;
                break;
            }
            lume_sha256_update(&archive_hash, buffer, count);
            lume_sha256_update(&file_hash, buffer, count);
            left -= (unsigned)count;
        }
        unsigned char digest[32], extra;
        lume_sha256_final(&file_hash, digest);
        if (read(fd, &extra, 1) != 0 || memcmp(digest, file->digest, 32))
            result = -1;
        if (close(fd))
            result = -1;
        if (result)
            return -1;
        total += file->bytes;
    }
    unsigned char digest[32];
    lume_sha256_final(&archive_hash, digest);
    return total == entry->bytes && !memcmp(digest, entry->digest, 32) ? 0 : -1;
}

static int db_scan(bool conflicts)
{
    char path[PATH_BYTES];
    if (state_path(path, "db"))
        return -1;
    DIR *db = opendir(path);
    if (!db)
        return -1;
    int result = 0;
    struct dirent *entry;
    for (;;)
    {
        errno = 0;
        entry = readdir(db);
        if (!entry)
        {
            if (errno)
                result = -1;
            break;
        }
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;
        if (load_installed(entry->d_name, &other) != 1)
        {
            result = fail("invalid installed package receipt");
            break;
        }
        if (!conflicts)
            printf("%s %s revision=%u\n", other.identity.name, other.identity.version,
                   other.identity.revision);
        else if (strcmp(other.identity.name, incoming.identity.name))
            for (unsigned i = 0; i < incoming.identity.files; ++i)
                if (file_index(&other, incoming.file[i].path) >= 0)
                {
                    result = fail("file belongs to another package");
                    goto done;
                }
    }
done:
    if (closedir(db))
        result = -1;
    return result;
}

static int acquire(void)
{
    char path[PATH_BYTES];
    if (directory(root) || state_path(path, "db") || parents(path) || directory(path) ||
        state_path(path, "lock"))
        return fail("cannot open package database");
    uint32_t pid = pkg_current_pid();
    if (!pid || create(path, &pid, sizeof(pid)))
        return fail("package database is locked");
    locked = true;
    return 0;
}

static void release(void)
{
    char path[PATH_BYTES];
    if (locked && !state_path(path, "lock"))
        (void)unlink(path);
    locked = false;
}

static int txn_path(char out[PATH_BYTES], const char *kind, unsigned i)
{
    char name[48];
    int n = snprintf(name, sizeof(name), "txn/%s%03u", kind, i);
    return n < 0 || (unsigned)n >= sizeof(name) ? -1 : state_path(out, name);
}

static int remove_if_exists(const char *path)
{
    int found = exists(path);
    return found < 0 ? -1 : found ? unlink(path) : 0;
}

static int cleanup(void)
{
    char path[PATH_BYTES];
    if (state_path(path, "txn/ready") || remove_if_exists(path))
        return -1;
    for (unsigned i = 0; i < PKG_FILES_MAX; ++i)
    {
        if (txn_path(path, "old", i) || remove_if_exists(path) || txn_path(path, "new", i) ||
            remove_if_exists(path))
            return -1;
    }
    const char *names[] = {"txn/old", "txn/new", "txn/receipt", "txn/ready", "txn/committed"};
    for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
        if (state_path(path, names[i]) || remove_if_exists(path))
            return -1;
    if (state_path(path, "txn"))
        return -1;
    return rmdir(path) && errno != ENOENT ? -1 : 0;
}

static int rollback(void)
{
    char path[PATH_BYTES], backup[PATH_BYTES], staged[PATH_BYTES];
    for (unsigned i = 0; i < incoming.identity.files; ++i)
    {
        const struct pkg_file *file = &incoming.file[i];
        if (path_join(path, root, file->path) || txn_path(staged, "new", i))
            return -1;
        if (exists(staged) == 0)
        {
            int old = file_index(&previous, file->path);
            if (old >= 0 && (txn_path(backup, "old", (unsigned)old) || exists(backup) == 0))
                continue;
            int found = exists(path);
            if (found < 0 ||
                (found && (hash_file(path, file->bytes, file->digest) || unlink(path))))
                return fail("recovery found a modified destination");
        }
    }
    for (unsigned i = 0; i < previous.identity.files; ++i)
    {
        if (txn_path(backup, "old", i) || path_join(path, root, previous.file[i].path))
            return -1;
        int found = exists(backup);
        if (found < 0 || (found && (exists(path) != 0 || rename(backup, path))))
            return fail("cannot restore a package backup");
    }
    if (check_files(&previous) || receipt_path(path, incoming.identity.name))
        return -1;
    if (previous.identity.revision)
    {
        if (remove_if_exists(path) || create(path, &previous, manifest_bytes(&previous)))
            return -1;
    }
    else if (remove_if_exists(path))
        return -1;
    return cleanup();
}

static int recover_transaction(void)
{
    char path[PATH_BYTES];
    if (state_path(path, "txn/ready"))
        return -1;
    int ready = exists(path);
    if (ready < 0)
        return -1;
    if (!ready)
        return cleanup();
    struct transaction_ready expected;
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;
    unsigned char extra;
    int valid = !read_exact(fd, &expected, sizeof(expected)) && read(fd, &extra, 1) == 0 &&
                expected.has_previous <= 1;
    if (close(fd))
        valid = 0;
    if (!valid)
        return fail("invalid recovery digest record");
    if (state_path(path, "txn/new") || read_manifest(path, &incoming, false) ||
        state_path(path, "txn/old"))
        return fail("invalid recovery manifest");
    int has_previous = exists(path);
    if (has_previous < 0 || (unsigned)has_previous != expected.has_previous)
        return fail("backup manifest presence differs from the recovery record");
    if (has_previous)
    {
        if (read_manifest(path, &previous, false) ||
            strcmp(previous.identity.name, incoming.identity.name))
            return fail("invalid backup manifest");
    }
    else
        memset(&previous, 0, sizeof(previous));
    unsigned char digest[32];
    manifest_digest(&incoming, digest);
    if (memcmp(digest, expected.incoming, 32))
        return fail("incoming recovery manifest SHA256 mismatch");
    if (has_previous)
    {
        manifest_digest(&previous, digest);
        if (memcmp(digest, expected.previous, 32))
            return fail("backup recovery manifest SHA256 mismatch");
    }
    if (state_path(path, "txn/committed"))
        return -1;
    if (exists(path) == 1)
    {
        if (check_files(&incoming) || load_installed(incoming.identity.name, &other) != 1 ||
            memcmp(&incoming, &other, manifest_bytes(&incoming)))
            return fail("committed package failed verification");
        return cleanup();
    }
    return rollback();
}

static int recover(void)
{
    char path[PATH_BYTES];
    if (state_path(path, "lock"))
        return -1;
    if (exists(path) == 1)
    {
        uint32_t pid = 0;
        int fd = open(path, O_RDONLY);
        if (fd < 0)
            return -1;
        int result = read_exact(fd, &pid, sizeof(pid));
        if (close(fd))
            result = -1;
        if (result || !pid || pkg_owner_alive(pid) != 0)
            return fail("lock owner is active or cannot be identified");
        if (unlink(path))
            return -1;
    }
    if (acquire())
        return -1;
    int result = recover_transaction();
    if (!result)
    {
        release();
        puts("pkg: recovery complete");
    }
    return result;
}

static int stage_files(const char *archive, const unsigned char expected[32])
{
    int input = open(archive, O_RDONLY);
    if (input < 0)
        return -1;
    int result = -1;
    struct lume_sha256 hash;
    lume_sha256_init(&hash);
    lume_sha256_update(&hash, &incoming, manifest_bytes(&incoming));
    if (lseek(input, (off_t)manifest_bytes(&incoming), SEEK_SET) < 0)
        goto done;
    for (unsigned i = 0; i < incoming.identity.files; ++i)
    {
        char path[PATH_BYTES];
        if (txn_path(path, "new", i))
            goto done;
        int output = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
        if (output < 0)
            goto done;
        uint32_t left = incoming.file[i].bytes;
        bool failed = false;
        while (left)
        {
            size_t n = left > sizeof(buffer) ? sizeof(buffer) : left;
            if (read_exact(input, buffer, n) || write_exact(output, buffer, n))
            {
                failed = true;
                break;
            }
            lume_sha256_update(&hash, buffer, n);
            left -= (uint32_t)n;
        }
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64)
        unsigned mode = !strncmp(incoming.file[i].path, "bin/", 4) ||
                                !strncmp(incoming.file[i].path, "libexec/", 8)
                            ? 0755u
                            : 0644u;
        if (pkg_payload_mode(output, mode))
            failed = true;
#endif
        if (fsync(output))
            failed = true;
        if (close(output))
            failed = true;
        if (failed || hash_file(path, incoming.file[i].bytes, incoming.file[i].digest))
            goto done;
    }
    unsigned char digest[32], extra;
    lume_sha256_final(&hash, digest);
    if (read(input, &extra, 1) != 0 || memcmp(digest, expected, 32))
        goto done;
    result = 0;
done:
    if (close(input))
        result = -1;
    return result;
}

static int apply_package(const char *archive, const struct pkg_catalog_entry *selected)
{
    if (hash_file(archive, selected->bytes, selected->digest) ||
        read_manifest(archive, &incoming, true) ||
        memcmp(&incoming.identity, &selected->identity, sizeof(struct pkg_identity)))
        return fail("package integrity or metadata check failed");
    int installed = load_installed(incoming.identity.name, &previous);
    if (installed < 0 || db_scan(true))
        return -1;
    if (installed && previous.identity.revision > incoming.identity.revision)
        return fail("package downgrade rejected");
    if (installed && previous.identity.revision == incoming.identity.revision)
        return memcmp(&previous, &incoming, manifest_bytes(&incoming)) || check_files(&previous)
                   ? fail("installed revision differs from the catalog")
                   : 0;
    if (installed && check_files(&previous))
        return -1;
    char path[PATH_BYTES], destination[PATH_BYTES];
    for (unsigned i = 0; i < incoming.identity.files; ++i)
    {
        if (path_join(path, root, incoming.file[i].path) || parents(path))
            return fail("invalid destination parent");
        if (file_index(&previous, incoming.file[i].path) < 0 && exists(path) != 0)
            return fail("destination exists without package ownership");
    }
    if (state_path(path, "txn") || directory(path))
        return -1;
    if (stage_files(archive, selected->digest) || state_path(path, "txn/new") ||
        create(path, &incoming, manifest_bytes(&incoming)) || state_path(path, "txn/receipt") ||
        create(path, &incoming, manifest_bytes(&incoming)))
        goto prepare_failed;
    if (installed &&
        (state_path(path, "txn/old") || create(path, &previous, manifest_bytes(&previous))))
        goto prepare_failed;
    struct transaction_ready ready = {.has_previous = (unsigned)installed};
    manifest_digest(&incoming, ready.incoming);
    if (installed)
        manifest_digest(&previous, ready.previous);
    if (state_path(path, "txn/ready") || create(path, &ready, sizeof(ready)))
        goto prepare_failed;
    for (unsigned i = 0; i < previous.identity.files; ++i)
        if (path_join(path, root, previous.file[i].path) || txn_path(destination, "old", i) ||
            rename(path, destination))
            goto commit_failed;
    for (unsigned i = 0; i < incoming.identity.files; ++i)
        if (txn_path(path, "new", i) || path_join(destination, root, incoming.file[i].path) ||
            rename(path, destination))
            goto commit_failed;
    if (check_files(&incoming) || receipt_path(destination, incoming.identity.name) ||
        remove_if_exists(destination) || state_path(path, "txn/receipt") ||
        rename(path, destination) || state_path(path, "txn/committed") ||
        create(path, "committed\n", 10))
        goto commit_failed;
    if (cleanup())
        return fail("package committed with cleanup pending");
    printf("pkg: installed %s %s\n", incoming.identity.name, incoming.identity.version);
    return 0;
prepare_failed:
    if (cleanup())
        return fail("staging cleanup failed");
    return fail("package staging failed");
commit_failed:
    if (rollback())
        return fail("installation interrupted, run pkg recover");
    return fail("installation failed and previous files were restored");
}

static int load_catalog(const char *path)
{
    memset(&catalog, 0, sizeof(catalog));
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;
    int result = -1;
    if (read_exact(fd, &catalog, offsetof(struct pkg_catalog, entry)) ||
        memcmp(catalog.magic, PKG_REPO_MAGIC, 8) ||
        memcmp(catalog.architecture, PKG_ARCH, sizeof(PKG_ARCH)) || catalog.abi != PKG_ABI ||
        pkg_current_abi() != PKG_ABI || !catalog.count || catalog.count > PKG_COUNT_MAX ||
        read_exact(fd, catalog.entry, catalog.count * sizeof(struct pkg_catalog_entry)))
        goto done;
    unsigned char extra;
    if (read(fd, &extra, 1) != 0)
        goto done;
    for (unsigned i = 0; i < catalog.count; ++i)
    {
        const struct pkg_catalog_entry *entry = &catalog.entry[i];
        if (!identity_valid(&entry->identity) || entry->bytes < sizeof(struct pkg_identity) + 8 ||
            entry->bytes > PKG_BYTES_MAX)
            goto done;
        for (unsigned j = 0; j < i; ++j)
            if (!strcmp(entry->identity.name, catalog.entry[j].identity.name))
                goto done;
    }
    result = 0;
done:
    if (close(fd))
        result = -1;
    return result;
}

static int catalog_index(const char *name)
{
    for (unsigned i = 0; i < catalog.count; ++i)
        if (!strcmp(name, catalog.entry[i].identity.name))
            return (int)i;
    return -1;
}

static size_t catalog_bytes(void)
{
    return offsetof(struct pkg_catalog, entry) + catalog.count * sizeof(struct pkg_catalog_entry);
}

static int saved_catalog(void)
{
    char path[PATH_BYTES];
    if (state_path(path, "repository"))
        return -1;
    if (exists(path) == 0 && state_path(path, "repository.old"))
        return -1;
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;
    struct repository_header header;
    struct stat st;
    int result = -1;
    if (read_exact(fd, &header, sizeof(header)) || memcmp(header.magic, "LUMEST1\n", 8) ||
        !memchr(header.source, 0, sizeof(header.source)) || !*header.source || fstat(fd, &st) ||
        st.st_size < (off_t)(sizeof(header) + 32) ||
        (uint64_t)st.st_size > sizeof(header) + sizeof(catalog))
        goto done;
    memset(&catalog, 0, sizeof(catalog));
    size_t bytes = (size_t)st.st_size - sizeof(header);
    if (read_exact(fd, &catalog, bytes))
        goto done;
    struct lume_sha256 hash;
    unsigned char digest[32];
    lume_sha256_init(&hash);
    lume_sha256_update(&hash, &catalog, bytes);
    lume_sha256_final(&hash, digest);
    if (memcmp(digest, header.digest, 32) || memcmp(catalog.magic, PKG_REPO_MAGIC, 8) ||
        memcmp(catalog.architecture, PKG_ARCH, sizeof(PKG_ARCH)) || catalog.abi != PKG_ABI ||
        pkg_current_abi() != PKG_ABI || !catalog.count || catalog.count > PKG_COUNT_MAX ||
        bytes != catalog_bytes())
        goto done;
    for (unsigned i = 0; i < catalog.count; ++i)
    {
        if (!identity_valid(&catalog.entry[i].identity) || catalog.entry[i].bytes > PKG_BYTES_MAX ||
            catalog.entry[i].bytes < sizeof(struct pkg_identity) + 8)
            goto done;
        for (unsigned j = 0; j < i; ++j)
            if (!strcmp(catalog.entry[i].identity.name, catalog.entry[j].identity.name))
                goto done;
    }
    memcpy(origin, header.source, sizeof(origin));
    result = 0;
done:
    if (close(fd))
        result = -1;
    return result;
}

static unsigned plan[PKG_COUNT_MAX], plan_used, visited[PKG_COUNT_MAX];

static int resolve(unsigned i)
{
    static struct
    {
        unsigned index, dependency;
    } stack[PKG_COUNT_MAX];

    unsigned depth = 0;
    stack[depth++].index = i;
    stack[0].dependency = 0;
    while (depth)
    {
        unsigned index = stack[depth - 1].index;
        if (visited[index] == 2)
        {
            --depth;
            continue;
        }
        visited[index] = 1;
        const struct pkg_identity *id = &catalog.entry[index].identity;
        unsigned d = stack[depth - 1].dependency;
        if (d == id->dependencies)
        {
            visited[index] = 2;
            plan[plan_used++] = index;
            --depth;
            continue;
        }
        ++stack[depth - 1].dependency;
        int dependency = catalog_index(id->dependency[d].name);
        if (dependency < 0 ||
            catalog.entry[dependency].identity.revision < id->dependency[d].revision)
            return fail("dependency is absent or too old in this catalog");
        if (visited[dependency] == 1)
            return fail("dependency cycle");
        if (visited[dependency] != 2)
        {
            if (depth == PKG_COUNT_MAX)
                return fail("dependency depth exceeds catalog capacity");
            stack[depth].index = (unsigned)dependency;
            stack[depth++].dependency = 0;
        }
    }
    return 0;
}

static int hex_digest(const char *text, unsigned char out[32])
{
    if (strlen(text) != 64)
        return -1;
    for (unsigned i = 0; i < 32; ++i)
    {
        unsigned n = 0;
        for (unsigned j = 0; j < 2; ++j)
        {
            unsigned c = (unsigned char)text[2 * i + j];
            unsigned value = c >= '0' && c <= '9'   ? c - '0'
                             : c >= 'a' && c <= 'f' ? c - 'a' + 10
                                                    : 16;
            if (value > 15)
                return -1;
            n = n * 16 + value;
        }
        out[i] = (unsigned char)n;
    }
    return 0;
}

static int update(const char *source, const char *digest_text)
{
    unsigned char digest[32];
    if (hex_digest(digest_text, digest) || strlen(source) >= sizeof(origin) || !*source)
        return fail("invalid repository source or SHA256");
    char temporary[PATH_BYTES], active[PATH_BYTES], saved[PATH_BYTES];
    if (state_path(temporary, "index.new") || state_path(active, "index") ||
        state_path(saved, "index.old") || remove_if_exists(temporary))
        return -1;
    if (!strncmp(source, "http://", 7))
    {
        if (pkg_download(source, temporary, 0, digest))
            goto failed;
    }
    else
    {
        struct stat st;
        if (lstat(source, &st) || !S_ISREG(st.st_mode) || st.st_size < 32 ||
            (uint64_t)st.st_size > sizeof(catalog) || copy_file(source, temporary) ||
            hash_file(temporary, (uint32_t)st.st_size, digest))
            goto failed;
    }
    if (load_catalog(temporary))
        goto failed;
    strcpy(origin, source);
    char *slash = strrchr(origin, '/');
    if (!slash || slash == origin)
        goto failed;
    *slash = 0;
    if (remove_if_exists(temporary))
        return -1;

    struct repository_header header = {0};
    memcpy(header.magic, "LUMEST1\n", 8);
    memcpy(header.source, origin, strlen(origin) + 1);
    memcpy(header.digest, digest, 32);
    if (state_path(temporary, "repository.new") || state_path(active, "repository") ||
        state_path(saved, "repository.old") || remove_if_exists(temporary))
        goto failed;
    int fd = open(temporary, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd < 0)
        goto failed;
    int result =
        write_exact(fd, &header, sizeof(header)) || write_exact(fd, &catalog, catalog_bytes());
    if (fsync(fd))
        result = -1;
    if (close(fd))
        result = -1;
    if (result || remove_if_exists(saved) || (exists(active) == 1 && rename(active, saved)) ||
        rename(temporary, active))
        goto failed;
    puts("pkg: catalog SHA256 verified");
    return 0;
failed:
    (void)remove_if_exists(temporary);
    return fail("catalog update rejected");
}

static int adopt(const char *source, const char *digest_text)
{
    unsigned char expected[32], actual[32];
    if (hex_digest(digest_text, expected) || read_manifest(source, &incoming, false))
        return fail("invalid adoption manifest");
    manifest_digest(&incoming, actual);
    if (memcmp(expected, actual, sizeof(actual)))
        return fail("adoption manifest SHA256 mismatch");
    int installed = load_installed(incoming.identity.name, &previous);
    if (installed < 0 || db_scan(true))
        return -1;
    if (installed)
        return memcmp(&previous, &incoming, manifest_bytes(&incoming)) || check_files(&incoming)
                   ? fail("installed receipt differs from the adoption manifest")
                   : 0;
    for (unsigned i = 0; i < incoming.identity.dependencies; ++i)
    {
        const struct pkg_dependency *dependency = &incoming.identity.dependency[i];
        if (load_installed(dependency->name, &other) != 1 ||
            other.identity.revision < dependency->revision || check_files(&other))
            return fail("adoption requires verified installed dependencies");
    }
    if (check_files(&incoming))
        return -1;
    char temporary[PATH_BYTES], destination[PATH_BYTES];
    if (state_path(temporary, "adopt.new") || receipt_path(destination, incoming.identity.name) ||
        remove_if_exists(temporary) || create(temporary, &incoming, manifest_bytes(&incoming)) ||
        read_manifest(temporary, &other, false) ||
        memcmp(&incoming, &other, manifest_bytes(&incoming)) || rename(temporary, destination))
    {
        if (!state_path(temporary, "adopt.new"))
            (void)remove_if_exists(temporary);
        return fail("cannot store the verified adoption receipt");
    }
    printf("pkg: adopted %s %s\n", incoming.identity.name, incoming.identity.version);
    return 0;
}

static int install(const char *name)
{
    char path[PATH_BYTES];
    if (saved_catalog())
        return fail("no valid repository catalog, run pkg update");
    int target = catalog_index(name);
    if (target < 0)
        return fail("package is absent from the catalog");
    memset(visited, 0, sizeof(visited));
    plan_used = 0;
    if (resolve((unsigned)target))
        return -1;
    char cache[PATH_BYTES];
    if (state_path(cache, "cache") || directory(cache))
        return -1;
    bool reuse[PKG_COUNT_MAX] = {false};
    /* Download and verify the entire dependency plan before changing installed files. */
    for (unsigned i = 0; i < plan_used; ++i)
    {
        const struct pkg_catalog_entry *entry = &catalog.entry[plan[i]];
        int installed = load_installed(entry->identity.name, &incoming);
        if (installed < 0)
            return fail("invalid installed package receipt");
        if (installed && incoming.identity.revision > entry->identity.revision)
            return fail("package downgrade rejected");
        if (installed && incoming.identity.revision == entry->identity.revision)
        {
            if (installed_package_matches(&incoming, entry))
                return fail("installed revision differs from the catalog");
            reuse[i] = true;
            continue;
        }
        printf("pkg: preparing %s %s\n", entry->identity.name, entry->identity.version);
        char filename[48], source[512];
        snprintf(filename, sizeof(filename), "%s-%u.lpk", entry->identity.name,
                 entry->identity.revision);
        int n = snprintf(source, sizeof(source), "%s/%s", origin, filename);
        if (n < 0 || (unsigned)n >= sizeof(source) || path_join(path, cache, filename))
            return -1;
        if (hash_file(path, entry->bytes, entry->digest))
        {
            if (remove_if_exists(path))
                return -1;
            if (!strncmp(source, "http://", 7))
            {
                if (pkg_download(source, path, entry->bytes, entry->digest))
                    return fail("package download failed");
            }
            else if (copy_file(source, path))
                return fail("package copy failed");
        }
        if (hash_file(path, entry->bytes, entry->digest) || read_manifest(path, &incoming, true) ||
            memcmp(&incoming.identity, &entry->identity, sizeof(struct pkg_identity)))
            return fail("package validation failed before installation");
    }
    for (unsigned i = 0; i < plan_used; ++i)
    {
        const struct pkg_catalog_entry *entry = &catalog.entry[plan[i]];
        if (reuse[i])
        {
            printf("pkg: verified installed %s %s\n", entry->identity.name,
                   entry->identity.version);
            continue;
        }
        char filename[48];
        snprintf(filename, sizeof(filename), "%s-%u.lpk", entry->identity.name,
                 entry->identity.revision);
        if (path_join(path, cache, filename) || apply_package(path, entry))
            return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
#ifdef LUME_PKG_HOST_TEST
    const char *test_root = getenv("LUME_PKG_ROOT");
    if (!test_root || !*test_root)
        return 2;
    root = test_root;
#endif
    if (argc == 2 && !strcmp(argv[1], "--version"))
    {
        puts("pkg 0.2.1 (" PKG_ARCH " ABI 1.0)");
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "recover"))
        return recover() ? 1 : 0;
    if ((argc == 2 && (!strcmp(argv[1], "list") || !strcmp(argv[1], "search"))) ||
        (argc == 3 && (!strcmp(argv[1], "install") || !strcmp(argv[1], "verify"))) ||
        (argc == 4 && (!strcmp(argv[1], "update") || !strcmp(argv[1], "adopt"))))
    {
        if (acquire())
            return 1;
        char path[PATH_BYTES];
        struct stat st;
        if (state_path(path, "txn") || !lstat(path, &st))
        {
            release();
            fail("unfinished transaction, run pkg recover");
            return 1;
        }
        int result;
        if (!strcmp(argv[1], "update"))
            result = update(argv[2], argv[3]);
        else if (!strcmp(argv[1], "adopt"))
            result = adopt(argv[2], argv[3]);
        else if (!strcmp(argv[1], "install"))
            result = install(argv[2]);
        else if (!strcmp(argv[1], "verify"))
        {
            result = load_installed(argv[2], &incoming) == 1 ? check_files(&incoming)
                                                             : fail("package is not installed");
            if (!result)
                puts("pkg: installed files verified");
        }
        else if (!strcmp(argv[1], "list"))
            result = db_scan(false);
        else
        {
            result = saved_catalog();
            if (!result)
                for (unsigned i = 0; i < catalog.count; ++i)
                    printf("%s %s\n", catalog.entry[i].identity.name,
                           catalog.entry[i].identity.version);
        }
        if (state_path(path, "txn") || !lstat(path, &st))
            locked = false;
        release();
        return result ? 1 : 0;
    }
    fputs("usage: pkg update catalog-path-or-http-url SHA256\n"
          "       pkg adopt manifest-path SHA256\n"
          "       pkg search|list|install name|verify name|recover\n",
          stderr);
    return 2;
}
