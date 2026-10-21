#include <vector>
#include <string>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <csetjmp>
#include <cmath>

static unsigned constructed;
static unsigned callbacks_completed;
static unsigned onexit_completed;

static void count_callback()
{
    ++callbacks_completed;
}

static void check_onexit(int status, void *)
{
    if (status || callbacks_completed != 512)
        std::_Exit(6);
    ++onexit_completed;
}

struct Lifetime
{
    Lifetime()
    {
        ++constructed;
    }

    ~Lifetime()
    {
        if (callbacks_completed != 512 || onexit_completed != 1)
            std::_Exit(7);
        std::puts("NATIVE_CXX_FINALIZE_PASS");
    }
};

static Lifetime lifetime;

extern "C" int app_main(unsigned, char **, char **)
{
    if (constructed != 1)
        return 1;
    if (on_exit(check_onexit, nullptr))
        return 8;
    for (unsigned i = 0; i < 512; ++i)
        if (std::atexit(count_callback))
            return 9;
    volatile unsigned char stack[65536];
    for (unsigned i = 0; i < sizeof(stack); ++i)
        stack[i] = (unsigned char)i;
    if (stack[65535] != 255)
        return 2;
    std::vector<std::string> words;
    for (unsigned i = 0; i < 100; ++i)
        words.emplace_back("AArch64");
    std::ostringstream output;
    output << words.size() << ':' << words[51];
    if (output.str() != "100:AArch64")
        return 3;
    void *memory = std::malloc(6 * 1024 * 1024);
    if (!memory)
        return 4;
    auto *bytes = static_cast<unsigned char *>(memory);
    for (unsigned i = 0; i < 6 * 1024 * 1024; i += 4096)
        bytes[i] = (unsigned char)(i / 4096);
    if (bytes[5 * 1024 * 1024 + 4096] != 1)
        return 5;
    std::free(memory);
    std::jmp_buf jump;
    volatile unsigned visited = 0;
    if (setjmp(jump) == 0)
    {
        visited = 1;
        std::longjmp(jump, 0);
    }
    if (visited != 1)
        return 6;
    char *end;
    volatile long double value = std::strtold("1.25", &end);
    if (*end || value != 1.25L || std::fpclassify(value) != FP_NORMAL)
        return 7;
    std::puts(
        "NATIVE_CXX_RUNTIME_PASS constructors/containers/streams/heap/stack/setjmp/binary128");
    return 0;
}
