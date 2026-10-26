#include <algorithm>
#include <vector>
#include <string>
#include <cstdio>

struct NativeLifetime
{
    ~NativeLifetime()
    {
        std::puts("NATIVE_CPP_FINALIZE_PASS");
    }
};
static NativeLifetime lifetime;

int main()
{
    std::vector<int> values{3, 1, 2};
    std::sort(values.begin(), values.end());
    std::string text = "lume";
    if (values != std::vector<int>{1, 2, 3} || text.substr(1) != "ume")
        return 1;
    std::puts("NATIVE_CPP_COMPILATION_PASS containers/strings");
    return 0;
}
