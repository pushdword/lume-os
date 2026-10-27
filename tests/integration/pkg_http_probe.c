#define LUME_USER_HOST_TEST 1
#include "../../apps/net/network_tool.h"
#include "../../apps/pkg/pkg_format.h"
#include "lume/syscall_native.h"
#include "lume/process.h"

static uint64_t network_bytes, file_bytes;

static void diagnostic_text(const char *text)
{
    (void)lume_native_call(LUME_SYS_WRITE, 1, (uintptr_t)text, len(text));
}

static void diagnostic_number(uint64_t value)
{
    char digits[21];
    unsigned count = 0;
    do
    {
        digits[20 - count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    (void)lume_native_call(LUME_SYS_WRITE, 1, (uintptr_t)(digits + 21 - count), count);
}

int64_t call(uint64_t operation, uint64_t a, uint64_t b, uint64_t c)
{
    if (operation == LUME_SYS_OPEN && eq((const char *)(uintptr_t)a, "-"))
        return 100;
    if ((operation == LUME_SYS_CLOSE && a == 100) ||
        (operation == LUME_SYS_FS && a == LUME_FS_SYNC && b == 100) ||
        (operation == LUME_SYS_UNLINK && eq((const char *)(uintptr_t)a, "-")))
        return 0;
    if (operation == LUME_SYS_WRITE && a == 100)
    {
        file_bytes += c;
        return (int64_t)c;
    }
    int64_t result = lume_native_call(operation, a, b, c);
    if (operation == LUME_SYS_STREAM && a == LUME_STREAM_RECEIVE && result > 0)
        network_bytes += (uint64_t)result;
    if (operation == LUME_SYS_WRITE && a > 2 && result > 0)
        file_bytes += (uint64_t)result;
    if (result < 0)
    {
        diagnostic_text("probe: syscall ");
        diagnostic_number(operation);
        diagnostic_text(" operation ");
        diagnostic_number(a);
        diagnostic_text(" error -");
        diagnostic_number((uint64_t)-result);
        diagnostic_text(" network bytes ");
        diagnostic_number(network_bytes);
        diagnostic_text(" file bytes ");
        diagnostic_number(file_bytes);
        diagnostic_text("\n");
    }
    return result;
}

static int hex(unsigned c)
{
    return c >= '0' && c <= '9' ? (int)c - '0' : c >= 'a' && c <= 'f' ? (int)c - 'a' + 10 : -1;
}

int main(int argc, char **argv)
{
    unsigned bytes;
    unsigned char expected[32];
    if (argc != 5 || !tool_unsigned(argv[3], PKG_BYTES_MAX, &bytes) || len(argv[4]) != 64)
        return 2;
    for (unsigned i = 0; i < 32; ++i)
    {
        int a = hex((unsigned char)argv[4][i * 2]);
        int b = hex((unsigned char)argv[4][i * 2 + 1]);
        if (a < 0 || b < 0)
            return 2;
        expected[i] = (unsigned char)(a * 16 + b);
    }
    int result = pkg_download(argv[1], argv[2], bytes, expected);
    diagnostic_text(result ? "probe: failed" : "probe: SHA256 verified");
    diagnostic_text(" network bytes ");
    diagnostic_number(network_bytes);
    diagnostic_text(" file bytes ");
    diagnostic_number(file_bytes);
    diagnostic_text("\n");
    return result ? 1 : 0;
}
