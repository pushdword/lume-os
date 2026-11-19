/* SPDX-License-Identifier: BSD-3-Clause */
#include "device.h"

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <unistd.h>

#define BLOCK_BYTES UINT64_C(4096)
#define MIN_BLOCKS UINT64_C(2048)

static const char usage[] =
    "Usage: mkfs.ext4 --write [-L label] [--offset bytes --length bytes] device-or-image\n"
    "Creates journaled ext4 with 4096-byte blocks. The selected range is overwritten.\n";

static int number(const char *text, uint64_t *value)
{
    if (*text < '0' || *text > '9')
        return EINVAL;
    char *end;
    errno = 0;
    uint64_t parsed = strtoull(text, &end, 10);
    if (errno || *end || parsed > INT64_MAX || parsed % BLOCK_BYTES)
        return EINVAL;
    *value = parsed;
    return 0;
}

static errcode_t initialize_inodes(ext2_filsys fs)
{
    for (dgrp_t group = 0; group < fs->group_desc_count; ++group)
    {
        errcode_t error = ext2fs_zero_blocks2(fs, ext2fs_inode_table_loc(fs, group),
                                              fs->inode_blocks_per_group, NULL, NULL);
        if (error)
            return error;
        ext2fs_bg_flags_set(fs, group, EXT2_BG_INODE_ZEROED);
        ext2fs_group_desc_csum_set(fs, group);
    }
    unsigned size = EXT2_INODE_SIZE(fs->super);
    struct ext2_inode *inode = calloc(1, size);
    if (!inode)
        return ENOMEM;
    errcode_t error = 0;
    for (ext2_ino_t index = EXT2_BAD_INO; index < EXT2_FIRST_INO(fs->super); ++index)
    {
        if (index == EXT2_ROOT_INO)
            continue;
        ext2fs_inode_alloc_stats2(fs, index, 1, 0);
        error = ext2fs_write_inode_full(fs, index, inode, size);
        if (error)
            break;
    }
    free(inode);
    return error;
}

static errcode_t format(const char *path, const char *label)
{
    uint64_t blocks = mkfs_device.byte_count / BLOCK_BYTES;
    int journal_blocks = ext2fs_default_journal_size(blocks);
    if (journal_blocks < 0 || (uint64_t)journal_blocks >= blocks)
        return EINVAL;
    unsigned char uuid[16];
    if (getentropy(uuid, sizeof(uuid)))
        return errno;
    uuid[6] = (uuid[6] & 15) | 0x40;
    uuid[8] = (uuid[8] & 63) | 0x80;

    struct ext2_super_block parameters = {
        .s_rev_level = EXT2_DYNAMIC_REV,
        .s_log_block_size = 2,
        .s_inode_size = 256,
        .s_feature_incompat = EXT2_FEATURE_INCOMPAT_FILETYPE | EXT3_FEATURE_INCOMPAT_EXTENTS |
                              EXT4_FEATURE_INCOMPAT_64BIT,
        .s_feature_ro_compat =
            EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER | EXT2_FEATURE_RO_COMPAT_LARGE_FILE |
            EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE | EXT4_FEATURE_RO_COMPAT_METADATA_CSUM,
    };
    uint64_t inodes = blocks / 16;
    parameters.s_inodes_count = inodes > UINT32_MAX ? UINT32_MAX : (uint32_t)inodes;
    ext2fs_blocks_count_set(&parameters, blocks);
    ext2_filsys fs;
    errcode_t error = ext2fs_initialize(path, EXT2_FLAG_64BITS, &parameters, lume_ext4_io, &fs);
    if (error)
        return error;

    memcpy(fs->super->s_uuid, uuid, sizeof(uuid));
    fs->super->s_checksum_type = EXT2_CRC32C_CHKSUM;
    if (label)
        memcpy(fs->super->s_volume_name, label, strlen(label));
    ext2fs_init_csum_seed(fs);
    error = ext2fs_allocate_tables(fs);
    if (!error)
        error = initialize_inodes(fs);
    if (!error)
        error = ext2fs_mkdir(fs, EXT2_ROOT_INO, EXT2_ROOT_INO, NULL);
    if (!error)
    {
        fs->umask = 077;
        error = ext2fs_mkdir(fs, EXT2_ROOT_INO, 0, "lost+found");
    }
    if (!error)
        error = ext2fs_add_journal_inode(fs, (blk_t)journal_blocks, EXT2_MKJOURNAL_NO_MNT_CHECK);
    if (!error)
        return ext2fs_close_free(&fs);
    ext2fs_free(fs);
    return error;
}

static errcode_t mkfs(const char *path, const char *label, uint64_t offset, uint64_t length)
{
    int fd = open(path, O_RDWR | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0)
        return errno;
    struct stat info;
    errcode_t error = 0;
    if (fstat(fd, &info))
        error = errno;
    else if (!S_ISBLK(info.st_mode) && !S_ISREG(info.st_mode))
        error = EINVAL;
    else if (S_ISBLK(info.st_mode) && geteuid() != 0)
        error = EACCES;
    else if (info.st_size < 0 || offset > (uint64_t)info.st_size ||
             length > (uint64_t)info.st_size - offset)
        error = EINVAL;
    else
    {
        if (!length)
            length = (uint64_t)info.st_size & ~(BLOCK_BYTES - 1);
        if (length / BLOCK_BYTES < MIN_BLOCKS)
            error = ENOSPC;
        else if (fsync(fd))
            error = errno;
        else
        {
            mkfs_device = (struct mkfs_device){fd, offset, length};
            error = format(path, label);
        }
    }
    if (close(fd) && !error)
        error = errno;
    return error;
}

int main(int argc, char **argv)
{
    static const struct option options[] = {{"write", no_argument, NULL, 'w'},
                                            {"label", required_argument, NULL, 'L'},
                                            {"offset", required_argument, NULL, 'o'},
                                            {"length", required_argument, NULL, 'l'},
                                            {"help", no_argument, NULL, 'h'},
                                            {"version", no_argument, NULL, 'V'},
                                            {NULL, 0, NULL, 0}};
    uint64_t offset = 0, length = 0;
    const char *label = NULL;
    int write = 0;
    int option;
    while ((option = getopt_long(argc, argv, "L:hV", options, NULL)) != -1)
    {
        switch (option)
        {
        case 'w':
            write = 1;
            break;
        case 'L':
            if (strlen(optarg) > 16)
                goto invalid;
            label = optarg;
            break;
        case 'o':
            if (number(optarg, &offset))
                goto invalid;
            break;
        case 'l':
            if (number(optarg, &length) || !length)
                goto invalid;
            break;
        case 'h':
            fputs(usage, stdout);
            return 0;
        case 'V':
            puts("mkfs.ext4 (libext2fs 1.47.4)");
            return 0;
        default:
            goto invalid;
        }
    }
    if (!write || argc - optind != 1 || (offset && !length))
        goto invalid;
    const char *path = argv[optind];
    errcode_t error = mkfs(path, label, offset, length);
    if (error)
    {
        if (error > 0 && error < 256)
            fprintf(stderr, "mkfs.ext4: %s: %s\n", path, strerror((int)error));
        else
            fprintf(stderr, "mkfs.ext4: %s: libext2fs error %ld\n", path, (long)error);
        return 1;
    }
    printf("Created journaled ext4 on %s (%" PRIu64 " bytes).\n", path, mkfs_device.byte_count);
    return 0;

invalid:
    fputs(usage, stderr);
    return 2;
}
