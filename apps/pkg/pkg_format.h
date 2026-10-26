#ifndef LUME_PKG_FORMAT_H
#define LUME_PKG_FORMAT_H
#include <stdint.h>

#define PKG_COUNT_MAX 256u
#define PKG_FILES_MAX 128u
#define PKG_DEPS_MAX 8u
#define PKG_BYTES_MAX (128u * 1024u * 1024u)
#define PKG_ABI 65536u
#define PKG_ARCH "aarch64"
#define PKG_REPO_MAGIC "LUMERP1\n"
#define PKG_FILE_MAGIC "LUMEPK1\n"

/* Integer fields use little-endian encoding. Strings include a zero terminator. */
struct pkg_dependency
{
    char name[24];
    uint32_t revision;
};

struct pkg_identity
{
    char name[24], version[24];
    uint32_t revision, abi, files, dependencies;
    struct pkg_dependency dependency[PKG_DEPS_MAX];
};

struct pkg_catalog_entry
{
    struct pkg_identity identity;
    uint32_t bytes;
    unsigned char digest[32];
};

struct pkg_catalog
{
    char magic[8], architecture[16];
    uint32_t abi, count;
    struct pkg_catalog_entry entry[PKG_COUNT_MAX];
};

struct pkg_file
{
    char path[96];
    uint32_t bytes;
    unsigned char digest[32];
};

struct pkg_manifest
{
    char magic[8];
    struct pkg_identity identity;
    struct pkg_file file[PKG_FILES_MAX];
};

_Static_assert(sizeof(struct pkg_identity) == 288, "package identity layout");
_Static_assert(sizeof(struct pkg_catalog_entry) == 324, "catalog entry layout");
_Static_assert(sizeof(struct pkg_file) == 132, "package file layout");

int pkg_owner_alive(unsigned);
unsigned pkg_current_pid(void);
unsigned pkg_current_abi(void);
int pkg_download(const char *, const char *, uint32_t, const unsigned char[32]);
#ifdef LUME_ARCH_AARCH64
int pkg_payload_mode(int, unsigned);
#endif
#endif
