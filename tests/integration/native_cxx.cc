/* SPDX-License-Identifier: BSD-3-Clause */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>

static std::string title("Lume a\xc3\xa7\xc3\xa3o");
static std::vector<unsigned> values{3, 5, 8, 13};
static bool constructed;

struct Lifetime
{
    Lifetime()
    {
        constructed = true;
    }

    ~Lifetime()
    {
        const char message[] = "NATIVE_CXX_FINALIZED\n";
        (void)write(1, message, sizeof(message) - 1);
    }
};

static Lifetime lifetime;

int main()
{
    if (!constructed || title != "Lume a\xc3\xa7\xc3\xa3o" || values.size() != 4 ||
        values[0] != 3 || values[3] != 13)
    {
        std::fputs("NATIVE_CXX_FAIL construction\n", stderr);
        return 1;
    }
    title.append(128, '!');
    values.reserve(512);
    values.push_back(21);
    if (title.size() != 139 || values.back() != 21)
    {
        std::fputs("NATIVE_CXX_FAIL allocation\n", stderr);
        return 1;
    }
    const char message[] = "NATIVE_CXX_PASS\n";
    return write(1, message, sizeof(message) - 1) != sizeof(message) - 1;
}
