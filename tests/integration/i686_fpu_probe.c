/* SPDX-License-Identifier: BSD-3-Clause */
#include <stdint.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined(LUME_BENCH_APP)
#define main app_main
#endif

struct __attribute__((packed)) x87_image
{
    uint32_t control, status, tags, instruction, code, operand, data;
    unsigned char registers[80];
};

_Static_assert(sizeof(struct x87_image) == 108, "x87 save image size");

static int fresh_context(void)
{
    struct x87_image image;
    __asm__ volatile("fnsave %0" : "=m"(image) : : "memory");
    if ((image.control & 0xffffu) != 0x37fu || ((image.status >> 11) & 7u) ||
        (image.tags & 0xffffu) != 0xffffu)
        return 9;
    for (unsigned i = 0; i < sizeof(image.registers); ++i)
        if (image.registers[i])
            return 10;
    puts("PASS_NATIVE_X87_FRESH default control, empty stack and zero physical registers");
    return 0;
}

static void first_value(const uint64_t *value)
{
    __asm__ volatile("fninit\nfldl %0" : : "m"(*value) : "st");
}

static uint64_t read_value(void)
{
    uint64_t value;
    __asm__ volatile("fstl %0" : "=m"(value));
    return value;
}

static void replace_value(const uint64_t *value)
{
    __asm__ volatile("fstp %%st(0)\nfldl %0" : : "m"(*value) : "st");
}

int main(int argc, char **argv)
{
    if (argc > 1 && argv[1][0] == '-' && argv[1][1] == '-' && argv[1][2] == 'f')
        return fresh_context();
    const uint64_t parent_value = UINT64_C(0x3ff8000000000000);
    const uint64_t child_value = UINT64_C(0x4004000000000000);
    int ready[2], resume[2];
    if (pipe(ready) || pipe(resume))
        return 1;
    first_value(&parent_value);
    if (read_value() != parent_value)
        return 2;
    pid_t child = fork();
    if (child < 0)
        return 3;
    if (!child)
    {
        close(ready[0]);
        close(resume[1]);
        if (read_value() != parent_value)
            _exit(4);
        replace_value(&child_value);
        unsigned char value = 1;
        if (write(ready[1], &value, 1) != 1 || read(resume[0], &value, 1) != 1)
            _exit(5);
        _exit(read_value() == child_value ? 0 : 6);
    }
    close(ready[1]);
    close(resume[0]);
    unsigned char value = 0;
    if (read(ready[0], &value, 1) != 1 || read_value() != parent_value ||
        write(resume[1], &value, 1) != 1)
        return 7;
    int status;
    if (waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status) ||
        read_value() != parent_value)
        return 8;
    close(ready[0]);
    close(resume[1]);
    const uint16_t custom_control = 0x0b7f;
    __asm__ volatile("fldcw %0" : : "m"(custom_control));
    child = fork();
    if (child < 0)
        return 11;
    if (!child)
    {
        char *arguments[] = {argv[0], "--fresh", NULL};
        execv(argv[0], arguments);
        _exit(12);
    }
    if (waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status))
        return 13;
    uint16_t observed_control;
    __asm__ volatile("fnstcw %0" : "=m"(observed_control));
    if (observed_control != custom_control || read_value() != parent_value)
        return 14;
    puts("PASS_NATIVE_X87_COMPLETE first use, fork inheritance, context switches and fresh exec isolation");
    return 0;
}
