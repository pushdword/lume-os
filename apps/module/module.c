#include "../../include/internal/lume/app_support.h"
#include "lume/module_abi.h"
#include <stdbool.h>

static unsigned char file[2u * 1024u * 1024u];

static void number(uint64_t value)
{
    char buffer[21];
    unsigned count = 0;
    do
    {
        buffer[count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    while (count)
        (void)write(1, buffer + --count, 1);
}

static void identity(const unsigned char build[20])
{
    for (unsigned i = 0; i < 20; ++i)
    {
        char digits[2] = {"0123456789abcdef"[build[i] >> 4], "0123456789abcdef"[build[i] & 15]};
        (void)write(1, digits, 2);
    }
}

static void show(const struct lume_module_info *info)
{
    number(info->id);
    puts(" ");
    puts(info->name);
    puts(" bytes=");
    number(info->bytes);
    puts(" references=");
    number(info->references);
    puts(" drivers=");
    number(info->drivers);
    puts(" devices=");
    number(info->devices);
    puts("\n");
}

static int request(struct lume_module_request *request)
{
    return (int)call(LUME_SYS_MODULE, (uintptr_t)request, sizeof(*request), 0);
}

int main(int argc, char **argv)
{
    struct lume_module_request value = {.version = LUME_MODULE_ABI_VERSION};
    int result = -LUME_EINVAL;
    if (argc == 2 && eq(argv[1], "list"))
    {
        value.operation = LUME_MODULE_LIST;
        while (!(result = request(&value)))
        {
            show(&value.info);
            ++value.index;
        }
        if (result == -LUME_ENOENT)
            return 0;
    }
    else if (argc == 2 && eq(argv[1], "kernel"))
    {
        value.operation = LUME_MODULE_KERNEL;
        result = request(&value);
        if (!result)
        {
            puts("kernel=");
            identity(value.info.kernel_build);
            puts(" module_free=");
            number(value.info.bytes);
            puts("\n");
        }
    }
    else if (argc == 3 && (eq(argv[1], "load") || eq(argv[1], "inspect")))
    {
        int64_t fd = open(argv[2], LUME_O_RDONLY);
        if (fd < 0)
            result = (int)fd;
        else
        {
            struct lume_stat stat;
            result = (int)call(LUME_SYS_STAT, (uint64_t)fd, (uintptr_t)&stat, 0);
            if (!result && (stat.kind != LUME_NODE_FILE || !stat.size || stat.size > sizeof(file)))
                result = -LUME_E2BIG;
            size_t offset = 0;
            while (!result && offset < stat.size)
            {
                size_t bytes = (size_t)stat.size - offset;
                if (bytes > LUME_FILE_IO_MAX)
                    bytes = LUME_FILE_IO_MAX;
                int64_t count = read((int)fd, file + offset, bytes);
                if (count <= 0)
                    result = count < 0 ? (int)count : -LUME_EIO;
                else
                    offset += (size_t)count;
            }
            (void)close((int)fd);
            if (!result)
            {
                value.operation = eq(argv[1], "load") ? LUME_MODULE_LOAD : LUME_MODULE_INSPECT;
                value.file = (uintptr_t)file;
                value.bytes = offset;
                result = request(&value);
                if (!result)
                    show(&value.info);
            }
        }
    }
    else if (argc == 3 && eq(argv[1], "unload"))
    {
        uint64_t id = 0;
        const char *p = argv[2];
        bool valid = *p != 0;
        for (; *p; ++p)
        {
            if (*p < '0' || *p > '9' || id > (UINT32_MAX - (unsigned)(*p - '0')) / 10)
            {
                valid = false;
                break;
            }
            id = id * 10 + (unsigned)(*p - '0');
        }
        if (valid && id)
        {
            value.operation = LUME_MODULE_UNLOAD;
            value.id = (uint32_t)id;
            result = request(&value);
        }
    }
    else
        puts("usage: module list|kernel|inspect FILE|load FILE|unload ID\n");
    if (result < 0)
    {
        puts("module: error -");
        number(-(int64_t)result);
        puts("\n");
    }
    return result < 0;
}
