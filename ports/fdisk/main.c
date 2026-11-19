/* SPDX-License-Identifier: BSD-3-Clause */
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "blkdev.h"
#include "format.h"
#include "libfdisk.h"

static const char usage[] =
    "Usage: fdisk -l [-b sector-size] [disk-or-image ...]\n"
    "       fdisk --create-gpt --write [-b sector-size] disk-or-image\n"
    "       fdisk --add-gpt FIRST:SECTORS --write [-b sector-size] disk-or-image\n"
    "       fdisk --delete PARTITION --write [-b sector-size] disk-or-image\n"
    "       fdisk --format-fat32 PARTITION --write [-b 512] disk-or-image\n"
    "Creating a GPT replaces the partition table. Formatting erases the selected partition.\n";

enum operation
{
    LIST = 'l',
    CREATE_GPT = 256,
    ADD_GPT,
    DELETE,
    FORMAT_FAT32,
    WRITE
};

static int valid_sector_size(unsigned long size)
{
    return size == 512 || size == 1024 || size == 2048 || size == 4096;
}

static int list_partitions(struct fdisk_context *context, const char *path)
{
    struct fdisk_table *table = NULL;
    int status = fdisk_get_partitions(context, &table);
    if (status)
    {
        return status;
    }

    size_t length = strlen(path);
    const char *separator = length && isdigit((unsigned char)path[length - 1]) ? "p" : "";
    puts("Device                    Start          End      Sectors Type");
    for (size_t index = 0; index < fdisk_table_get_nents(table); index++)
    {
        struct fdisk_partition *part = fdisk_table_get_partition(table, index);
        size_t number = fdisk_partition_get_partno(part);
        fdisk_sector_t start = fdisk_partition_get_start(part);
        fdisk_sector_t end = fdisk_partition_get_end(part);
        fdisk_sector_t size = fdisk_partition_get_size(part);
        struct fdisk_parttype *type = fdisk_partition_get_type(part);
        const char *name = type ? fdisk_parttype_get_name(type) : NULL;
        printf("%s%s%zu %14" PRIu64 " %12" PRIu64 " %12" PRIu64 " %s\n", path, separator,
               number + 1, (uint64_t)start, (uint64_t)end, (uint64_t)size, name ? name : "Unknown");
    }
    fdisk_unref_table(table);
    return 0;
}

static int geometry(int fd, int writing, unsigned *sector_size, uint64_t *bytes)
{
    struct stat info;
    if (fstat(fd, &info))
        return -errno;
    if (!S_ISREG(info.st_mode) && !S_ISBLK(info.st_mode))
        return -EINVAL;
    if (S_ISBLK(info.st_mode))
    {
        if (writing && geteuid() != 0)
            return -EACCES;
        int logical;
        if (!blkdev_get_sector_size(fd, &logical))
        {
            if (!valid_sector_size((unsigned)logical) ||
                (*sector_size && *sector_size != (unsigned)logical))
                return -EINVAL;
            *sector_size = (unsigned)logical;
        }
        else if (errno != ENOTTY && errno != ENOSYS)
            return -errno;
        else if (writing && !*sector_size)
            return -ENOTSUP;
    }
    if (!*sector_size)
        *sector_size = LUME_FDISK_LOGICAL_SECTOR_BYTES;
    if (info.st_size < *sector_size || (uint64_t)info.st_size % *sector_size)
        return -EINVAL;
    *bytes = (uint64_t)info.st_size;
    return 0;
}

static int list_disk(const char *path, unsigned sector_size)
{
    int descriptor = open(path, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    if (descriptor < 0)
        return -errno;
    uint64_t bytes;
    int status = geometry(descriptor, 0, &sector_size, &bytes);
    if (status)
    {
        close(descriptor);
        return status;
    }

    struct fdisk_context *context = fdisk_new_context();
    if (!context)
    {
        close(descriptor);
        return -ENOMEM;
    }
    fdisk_enable_listonly(context, 1);
    fdisk_disable_dialogs(context, 1);
    fdisk_save_user_sector_size(context, sector_size, sector_size);
    status = fdisk_assign_device_by_fd(context, descriptor, path, 1);
    if (!status)
    {
        printf("Disk %s: %" PRIu64 " bytes, %" PRIu64 " sectors\n", path, bytes,
               bytes / sector_size);
        printf("Logical sector size: %u bytes\n", sector_size);
        if (fdisk_is_label(context, GPT) || fdisk_is_label(context, DOS))
        {
            printf("Disklabel type: %s\n", fdisk_is_label(context, GPT) ? "gpt" : "dos");
            char *identifier = NULL;
            if (!fdisk_get_disklabel_id(context, &identifier) && identifier)
            {
                printf("Disk identifier: %s\n", identifier);
            }
            free(identifier);
            status = list_partitions(context, path);
        }
        else
        {
            puts("No GPT or MBR partition table found.");
        }
    }
    fdisk_unref_context(context);
    close(descriptor);
    return status;
}

static int parse_number(const char *text, char **end, uint64_t *number)
{
    if (*text < '0' || *text > '9')
        return -EINVAL;
    errno = 0;
    *number = strtoull(text, end, 10);
    return errno == ERANGE || !*number ? -EINVAL : 0;
}

static int add_partition(struct fdisk_context *context, uint64_t first, uint64_t count)
{
    struct fdisk_partition *part = fdisk_new_partition();
    if (!part)
        return -ENOMEM;
    int status = fdisk_partition_partno_follow_default(part, 1);
    if (!status)
        status = fdisk_partition_set_start(part, first);
    if (!status)
        status = fdisk_partition_set_size(part, count);
    if (!status)
        status = fdisk_partition_size_explicit(part, 1);
    if (!status)
        status = fdisk_add_partition(context, part, NULL);
    fdisk_unref_partition(part);
    return status;
}

static int edit_disk(const char *path, unsigned sector_size, enum operation action, const char *arg)
{
    uint64_t first = 0, count = 0;
    char *end;
    if (action == ADD_GPT)
    {
        if (parse_number(arg, &end, &first) || *end != ':' || parse_number(end + 1, &end, &count) ||
            *end)
            return -EINVAL;
    }
    else if (action != CREATE_GPT)
    {
        if (parse_number(arg, &end, &first) || *end || first > SIZE_MAX)
            return -EINVAL;
    }

    int fd = open(path, O_RDWR | O_CLOEXEC | O_NONBLOCK | O_NOFOLLOW);
    if (fd < 0)
        return -errno;
    uint64_t bytes;
    int status = geometry(fd, 1, &sector_size, &bytes);
    if (!status && fsync(fd))
        status = -errno;
    if (status)
    {
        if (status == -ENOTSUP)
            fputs("fdisk: logical sector size unavailable. Specify -b for this device.\n", stderr);
        close(fd);
        return status;
    }
    uint64_t sectors = bytes / sector_size;
    struct fdisk_context *context = fdisk_new_context();
    if (!context)
    {
        close(fd);
        return -ENOMEM;
    }
    fdisk_disable_dialogs(context, 1);
    fdisk_save_user_sector_size(context, sector_size, sector_size);
    status = fdisk_assign_device_by_fd(context, fd, path, 0);
    if (status)
        goto done;

    if (action == CREATE_GPT)
        status = fdisk_create_disklabel(context, "gpt");
    else if (!fdisk_is_label(context, GPT) || fdisk_verify_disklabel(context))
        status = -EINVAL;
    else if (action == ADD_GPT)
        status = first < sectors && count <= sectors - first ? add_partition(context, first, count)
                                                             : -EINVAL;
    else if (action == DELETE)
        status = fdisk_delete_partition(context, (size_t)first - 1);
    else
    {
        struct fdisk_partition *part = NULL;
        status = fdisk_get_partition(context, (size_t)first - 1, &part);
        if (!status)
        {
            uint64_t start = fdisk_partition_get_start(part);
            uint64_t size = fdisk_partition_get_size(part);
            status = sector_size == 512 && start && size && start < sectors &&
                             size <= sectors - start && start >= fdisk_get_first_lba(context) &&
                             start <= fdisk_get_last_lba(context) &&
                             size - 1 <= fdisk_get_last_lba(context) - start
                         ? format_fat32(fd, start, size)
                         : -EINVAL;
        }
        fdisk_unref_partition(part);
    }
    if (!status && action != FORMAT_FAT32)
        status = fdisk_write_disklabel(context);
    if (!status && fsync(fd))
        status = -errno;
done:
    fdisk_unref_context(context);
    if (close(fd) && !status)
        status = -errno;
    return status;
}

static int report_disk(const char *path, unsigned sector_size)
{
    int status = list_disk(path, sector_size);
    if (status)
    {
        fprintf(stderr, "fdisk: %s: %s\n", path, strerror(-status));
    }
    return status != 0;
}

static int list_devices(unsigned sector_size)
{
    DIR *devices = opendir("/dev");
    if (!devices)
    {
        perror("fdisk: /dev");
        return 1;
    }

    int found = 0, result = 0;
    for (;;)
    {
        errno = 0;
        struct dirent *item = readdir(devices);
        if (!item)
        {
            if (errno)
            {
                perror("fdisk: /dev");
                result = 1;
            }
            break;
        }
        char path[PATH_MAX];
        int length = snprintf(path, sizeof(path), "/dev/%s", item->d_name);
        struct stat info;
        if (length > 0 && (size_t)length < sizeof(path) && !lstat(path, &info) &&
            S_ISBLK(info.st_mode))
        {
            found = 1;
            result |= report_disk(path, sector_size);
        }
    }
    closedir(devices);
    if (!found && !result)
    {
        fputs("fdisk: no block devices found\n", stderr);
        result = 1;
    }
    return result;
}

int main(int argc, char **argv)
{
    static const struct option options[] = {{"list", no_argument, NULL, 'l'},
                                            {"sector-size", required_argument, NULL, 'b'},
                                            {"create-gpt", no_argument, NULL, CREATE_GPT},
                                            {"add-gpt", required_argument, NULL, ADD_GPT},
                                            {"delete", required_argument, NULL, DELETE},
                                            {"format-fat32", required_argument, NULL, FORMAT_FAT32},
                                            {"write", no_argument, NULL, WRITE},
                                            {"help", no_argument, NULL, 'h'},
                                            {"version", no_argument, NULL, 'V'},
                                            {NULL, 0, NULL, 0}};
    unsigned sector_size = 0;
    enum operation action = 0;
    const char *argument = NULL;
    int writing = 0;
    int option;
    while ((option = getopt_long(argc, argv, "lb:hV", options, NULL)) != -1)
    {
        switch (option)
        {
        case LIST:
        case CREATE_GPT:
        case ADD_GPT:
        case DELETE:
        case FORMAT_FAT32:
            if (action)
            {
                fputs(usage, stderr);
                return 2;
            }
            action = (enum operation)option;
            argument = optarg;
            break;
        case WRITE:
            writing = 1;
            break;
        case 'b':
        {
            char *end;
            unsigned long size = strtoul(optarg, &end, 10);
            if (*end || !valid_sector_size(size))
            {
                fputs("fdisk: invalid logical sector size\n", stderr);
                return 2;
            }
            sector_size = (unsigned)size;
            break;
        }
        case 'h':
            fputs(usage, stdout);
            return 0;
        case 'V':
            printf("fdisk (libfdisk %s)\n", LIBFDISK_VERSION);
            return 0;
        default:
            return 2;
        }
    }
    if (!action || writing != (action != LIST) || (writing && argc - optind != 1))
    {
        fputs(usage, stderr);
        return 2;
    }
    if (writing)
    {
        const char *path = argv[optind];
        int status = edit_disk(path, sector_size, action, argument);
        if (status)
            fprintf(stderr, "fdisk: %s: %s\n", path, strerror(status < 0 ? -status : EIO));
        return status != 0;
    }
    if (optind == argc)
    {
        return list_devices(sector_size);
    }

    int result = 0;
    for (int index = optind; index < argc; index++)
    {
        result |= report_disk(argv[index], sector_size);
    }
    return result;
}
