#include "lib.h"
#include "lume/hardware_abi.h"
#include <stdbool.h>

static const char *state_name(unsigned state)
{
    static const char *const names[] = {"discovered", "disabled", "unsupported", "ready",
                                        "absent",     "deferred", "failed"};
    return state < sizeof(names) / sizeof(names[0]) ? names[state] : "unknown";
}

static void number(uint64_t value, unsigned base)
{
    char buffer[65];
    unsigned count = 0;
    do
    {
        buffer[count++] = "0123456789abcdef"[value % base];
        value /= base;
    } while (value);
    while (count)
        (void)write(1, buffer + --count, 1);
}

static bool parse(const char *text, uint64_t *out)
{
    unsigned base = 10;
    uint64_t value = 0;
    if (text[0] == '0' && text[1] == 'x')
    {
        text += 2;
        base = 16;
    }
    if (!*text)
        return false;
    while (*text)
    {
        unsigned digit = *text >= '0' && *text <= '9'   ? (unsigned)(*text - '0')
                         : *text >= 'a' && *text <= 'f' ? (unsigned)(*text - 'a' + 10)
                         : *text >= 'A' && *text <= 'F' ? (unsigned)(*text - 'A' + 10)
                                                        : 16;
        if (digit >= base || value > (UINT64_MAX - digit) / base)
            return false;
        value = value * base + digit;
        ++text;
    }
    *out = value;
    return true;
}

static int request(struct lume_hardware_request *value)
{
    return (int)call(LUME_SYS_HARDWARE, (uintptr_t)value, sizeof(*value), 0);
}

static void show(const struct lume_hardware_info *info, bool details)
{
    number(info->id, 10);
    puts(" ");
    puts(info->path);
    puts(" state=");
    puts(state_name(info->state));
    puts(" driver=");
    puts(info->driver[0] ? info->driver : "none");
    puts("\n");
    if (!details)
        return;
    puts("origin=");
    number(info->origin, 10);
    puts(" parent=");
    number(info->parent, 10);
    puts(" error=");
    if (info->error < 0)
        puts("-");
    number(info->error < 0 ? -(int64_t)info->error : info->error, 10);
    puts(" references=");
    number(info->references, 10);
    puts("\n");
    for (unsigned i = 0; i < info->identity_count; ++i)
    {
        puts("identity ");
        number(info->identities[i].kind, 10);
        puts(" ");
        puts(info->identities[i].value);
        puts("\n");
    }
    for (unsigned i = 0; i < info->resource_count; ++i)
    {
        const struct lume_hardware_resource_info *resource = info->resources + i;
        puts("resource ");
        number(i, 10);
        puts(" kind=");
        number(resource->kind, 10);
        puts(" base=0x");
        number(resource->base, 16);
        puts(" size=0x");
        number(resource->size, 16);
        puts(" domain=");
        number(resource->domain, 10);
        puts(" flags=0x");
        number(resource->flags, 16);
        puts("\n");
    }
}

static int command(int argc, char **argv)
{
    struct lume_hardware_request value = {.version = LUME_HARDWARE_VERSION};
    if (argc == 2 && (eq(argv[1], "list") || eq(argv[1], "drivers")))
    {
        value.operation = eq(argv[1], "list") ? LUME_HARDWARE_LIST : LUME_HARDWARE_DRIVER;
        for (value.index = 0;; ++value.index)
        {
            int result = request(&value);
            if (result == -LUME_ENOENT)
                return 0;
            if (result)
                return result;
            if (value.operation == LUME_HARDWARE_LIST)
                show(&value.info, false);
            else
            {
                puts(value.driver);
                puts("\n");
            }
        }
    }
    uint64_t id;
    if (argc < 3 || !parse(argv[2], &id) || !id || id > UINT32_MAX)
        return -LUME_EINVAL;
    value.id = (uint32_t)id;
    if (argc == 3 && eq(argv[1], "info"))
    {
        value.operation = LUME_HARDWARE_GET;
        int result = request(&value);
        if (!result)
            show(&value.info, true);
        return result;
    }
    if (argc == 4 && eq(argv[1], "bind"))
    {
        size_t bytes = len(argv[3]);
        if (!bytes || bytes >= sizeof(value.driver))
            return -LUME_EINVAL;
        for (size_t i = 0; i <= bytes; ++i)
            value.driver[i] = argv[3][i];
        value.operation = LUME_HARDWARE_BIND;
        return request(&value);
    }
    if (argc == 3 && eq(argv[1], "unbind"))
    {
        value.operation = LUME_HARDWARE_UNBIND;
        return request(&value);
    }
    if ((argc == 5 || argc == 7) && eq(argv[1], "read"))
    {
        uint64_t resource, offset, width = 32;
        if (!parse(argv[3], &resource) || resource > UINT32_MAX || !parse(argv[4], &offset) ||
            (argc == 7 && (!eq(argv[5], "--width") || !parse(argv[6], &width))) ||
            (width != 8 && width != 16 && width != 32 && width != 64))
            return -LUME_EINVAL;
        value.operation = LUME_HARDWARE_READ_REGISTER;
        value.resource = (uint32_t)resource;
        value.offset = offset;
        value.width = (uint32_t)(width / 8);
        int result = request(&value);
        if (!result)
        {
            puts("0x");
            number(value.value, 16);
            puts("\n");
        }
        return result;
    }
    return -LUME_EINVAL;
}

int main(int argc, char **argv)
{
    int result = command(argc, argv);
    if (result)
    {
        puts("Device operation failed: ");
        if (result < 0)
            puts("-");
        number(result < 0 ? -(int64_t)result : result, 10);
        puts("\n");
        if (result == -LUME_EINVAL)
            puts("device list|drivers|info ID|bind ID DRIVER|unbind ID\n"
                 "hwreg read ID RESOURCE OFFSET [--width 8|16|32|64]\n");
    }
    return result ? 1 : 0;
}
