#include "../../boot/recovery/recovery.h"
#include "lume/early.h"
#include "lume/watchdog.h"
#include "lume/abi.h"

#define UART UINT64_C(0x1c00030000)
extern unsigned char __image_start[], __image_end[];
extern _Noreturn void lrec_jump(uint64_t);
static struct lrec state;
static struct lume_fdt tree;
static uint64_t device_tree;

static uint32_t pm_read(void *context, unsigned offset)
{
    (void)context;
    return *(volatile uint32_t *)(uintptr_t)(LUME_PM_BASE + offset);
}

static void pm_write(void *context, unsigned offset, uint32_t value)
{
    (void)context;
    *(volatile uint32_t *)(uintptr_t)(LUME_PM_BASE + offset) = value;
    __asm__ volatile("dsb sy" ::: "memory");
}

static const struct lume_watchdog_io watchdog = {NULL, pm_read, pm_write};

static void send(void *context, const char *data, size_t size)
{
    (void)context;
    for (size_t i = 0; i < size; ++i)
    {
        while (*(volatile uint32_t *)(uintptr_t)(UART + 0x18) & 0x20u)
            __asm__ volatile("yield");
        *(volatile uint32_t *)(uintptr_t)UART = (unsigned char)data[i];
    }
    __asm__ volatile("dsb sy" ::: "memory");
}

static bool overlap(uint64_t a, uint64_t size, uint64_t b, uint64_t count)
{
    return a < b + count && b < a + size;
}

static bool ram(uint64_t address, uint64_t size)
{
    bool found = false;
    for (unsigned i = 0; i < tree.ram_count; ++i)
        if (address >= tree.ram[i].base && size <= tree.ram[i].size &&
            address - tree.ram[i].base <= tree.ram[i].size - size)
            found = true;
    if (!found || overlap(address, size, device_tree, tree.size) ||
        overlap(address, size, (uintptr_t)__image_start,
                (uintptr_t)__image_end - (uintptr_t)__image_start))
        return false;
    for (unsigned i = 0; i < tree.reserved_count; ++i)
        if (overlap(address, size, tree.reserved[i].base, tree.reserved[i].size))
            return false;
    return true;
}

static bool prepare(void *context, const struct lrec_image *kernel, const struct lrec_image *root)
{
    (void)context;
    return kernel->valid && root->valid &&
           lume_fdt_set_initrd((void *)(uintptr_t)device_tree, tree.size, (uintptr_t)root->data,
                               (uintptr_t)root->data + root->bytes);
}

int lume_console_getc(void)
{
    return -LUME_EAGAIN;
}

void lume_console_putc(char c)
{
    send(NULL, &c, 1);
}

void lume_console_write(const char *data, size_t bytes)
{
    for (size_t i = 0; i < bytes; ++i)
        lume_console_putc(data[i]);
}

static _Noreturn void failure(const char *reason)
{
    send(NULL, "\nLUME RECOVERY ", 15);
    size_t length = 0;
    while (reason[length])
        ++length;
    send(NULL, reason, length);
    send(NULL, " ERROR\n", 7);
    for (;;)
        __asm__ volatile("wfe");
}

_Noreturn void lrec_main(uint64_t dtb)
{
    static const char ready[] = "\nLUME RECOVERY READY LREC1\n";
    device_tree = dtb;
    lume_watchdog_disarm(&watchdog);
    if ((dtb & 7) || dtb < 0x80000 || dtb > UINT64_C(0x1ffe00000) ||
        overlap(dtb, 0x200000, (uintptr_t)__image_start,
                (uintptr_t)__image_end - (uintptr_t)__image_start) ||
        !lume_fdt_read((const void *)(uintptr_t)dtb, 0x200000, &tree) || !tree.pi5)
        failure("FDT");
    if (!ram(LREC_KERNEL_ADDRESS, LREC_KERNEL_LIMIT))
        failure("KERNEL RAM");
    if (!ram(LREC_KERNEL_STAGE, 2 * LREC_KERNEL_LIMIT) ||
        !ram(LREC_ROOT_STAGE, 2 * LREC_ROOT_LIMIT))
        failure("STAGING RAM");
    uint64_t root_size = tree.initrd_end - tree.initrd_start;
    if (root_size &&
        (!ram(tree.initrd_start, root_size) ||
         overlap(tree.initrd_start, root_size, LREC_KERNEL_ADDRESS, LREC_KERNEL_LIMIT) ||
         overlap(tree.initrd_start, root_size, LREC_KERNEL_STAGE, 2 * LREC_KERNEL_LIMIT) ||
         overlap(tree.initrd_start, root_size, LREC_ROOT_STAGE, 2 * LREC_ROOT_LIMIT)))
        failure("INITRD RAM");
    const struct lrec_io io = {NULL, send, prepare};
    lrec_init(&state, &io, (void *)(uintptr_t)LREC_KERNEL_STAGE,
              (void *)(uintptr_t)(LREC_KERNEL_STAGE + LREC_KERNEL_LIMIT),
              (void *)(uintptr_t)LREC_ROOT_STAGE,
              (void *)(uintptr_t)(LREC_ROOT_STAGE + LREC_ROOT_LIMIT),
              (void *)(uintptr_t)tree.initrd_start, (uint32_t)root_size);
    send(NULL, ready, sizeof(ready) - 1);
    while (!state.run)
    {
        if (*(volatile uint32_t *)(uintptr_t)(UART + 0x18) & 0x10u)
        {
            __asm__ volatile("yield");
            continue;
        }
        uint32_t value = *(volatile uint32_t *)(uintptr_t)UART;
        if (value & 0xf00)
        {
            *(volatile uint32_t *)(uintptr_t)(UART + 4) = 0;
            lrec_feed(&state, -1);
        }
        else
            lrec_feed(&state, value & 255);
    }
    while (*(volatile uint32_t *)(uintptr_t)(UART + 0x18) & 8u)
        __asm__ volatile("yield");
    const struct lrec_image *kernel = lrec_kernel(&state);
    unsigned char *destination = (void *)(uintptr_t)LREC_KERNEL_ADDRESS;
    for (uint32_t i = 0; i < kernel->bytes; ++i)
        destination[i] = kernel->data[i];
    __asm__ volatile("dsb sy; ic iallu; dsb sy; isb" ::: "memory");
    (void)lume_watchdog_arm(&watchdog, LUME_WATCHDOG_SECONDS);
    lrec_jump(device_tree);
}
