#include "lume/app_support.h"
#include "lume/hardware_abi.h"
#include "lume/module_abi.h"

static struct lume_hardware_request hardware = {.version = LUME_HARDWARE_VERSION};

static int control(unsigned operation, unsigned id)
{
    hardware.operation = operation;
    hardware.id = id;
    return (int)call(LUME_SYS_HARDWARE, (uintptr_t)&hardware, sizeof(hardware), 0);
}

static int exercise(unsigned id, unsigned module)
{
    const char driver[] = "ramblk";
    for (unsigned i = 0; i < sizeof(driver); ++i)
        hardware.driver[i] = driver[i];
    if (control(LUME_HARDWARE_BIND, id))
        return 1;
    char path[48] = "/dev/ramblk", digits[10];
    unsigned count = 0, value = id, end = 11;
    do
    {
        digits[count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    while (count)
        path[end++] = digits[--count];
    path[end] = 0;
    int64_t fd = open(path, LUME_O_RDWR);
    if (fd < 0)
        return 2;
    unsigned char original[512], observed[512];
    for (unsigned i = 0; i < sizeof(original); ++i)
        original[i] = (unsigned char)(id + i * 37);
    if (write((int)fd, original, sizeof(original)) != sizeof(original) ||
        call(LUME_SYS_SEEK, (uint64_t)fd, 0, LUME_SEEK_SET) ||
        read((int)fd, observed, sizeof(observed)) != sizeof(observed))
        return 3;
    for (unsigned i = 0; i < sizeof(original); ++i)
        if (original[i] != observed[i])
            return 4;
    if (control(LUME_HARDWARE_UNBIND, id) != -LUME_EBUSY)
        return 5;
    struct lume_module_request request = {
        .version = LUME_MODULE_ABI_VERSION, .operation = LUME_MODULE_UNLOAD, .id = module};
    if (call(LUME_SYS_MODULE, (uintptr_t)&request, sizeof(request), 0) != -LUME_EBUSY)
        return 6;
    if (close((int)fd) || control(LUME_HARDWARE_UNBIND, id))
        return 7;
    return 0;
}

int main(void)
{
    unsigned module = 0, devices[2], count = 0;
    struct lume_module_request request = {.version = LUME_MODULE_ABI_VERSION,
                                          .operation = LUME_MODULE_LIST};
    while (!call(LUME_SYS_MODULE, (uintptr_t)&request, sizeof(request), 0))
    {
        if (eq(request.info.name, "ram-block"))
            module = request.info.id;
        request.index++;
    }
    if (!module)
        return 10;
    hardware.operation = LUME_HARDWARE_LIST;
    for (hardware.index = 0; !control(LUME_HARDWARE_LIST, 0); hardware.index++)
        if (eq(hardware.info.path, "/virtual/ram-block/ram0") ||
            eq(hardware.info.path, "/virtual/ram-block/ram1"))
        {
            if (count == 2)
                return 11;
            devices[count++] = hardware.info.id;
        }
    if (count != 2)
        return 12;
    for (unsigned i = 0; i < 2; ++i)
    {
        int result = exercise(devices[i], module);
        if (result)
            return 20 + result;
    }
    puts("PASS_MODULE_BLOCK devices=2 data=512 open-unbind=busy unload=busy\n");
    return 0;
}
