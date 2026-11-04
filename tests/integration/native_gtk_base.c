/* SPDX-License-Identifier: BSD-3-Clause */
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <iconv.h>
#include <ffi.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint64_t combine(uint64_t first, uint64_t second, double factor)
{
    return first + second + (uint64_t)factor;
}

int main(void)
{
    int error;
    PCRE2_SIZE offset;
    pcre2_code *expression = pcre2_compile((const unsigned char *)"^caf\\p{L}+$", PCRE2_ZERO_TERMINATED,
                                          PCRE2_UTF | PCRE2_UCP, &error, &offset, NULL);
    assert(expression);
    pcre2_match_data *match = pcre2_match_data_create_from_pattern(expression, NULL);
    assert(match);
    const unsigned char sample[] = "café";
    assert(pcre2_match(expression, sample, sizeof(sample) - 1, 0, 0, match, NULL) > 0);
    const unsigned char invalid[] = {'c', 'a', 'f', 0xc0, 0xaf};
    assert(pcre2_match(expression, invalid, sizeof(invalid), 0, 0, match, NULL) < 0);
    pcre2_match_data_free(match);
    pcre2_code_free(expression);
    puts("GTK_REGEX_PASS");

    iconv_t conversion = iconv_open("UTF-8", "ISO-8859-1");
    assert(conversion != (iconv_t)-1);
    char input[] = {'a', 0xe7, 0xe3, 'o'};
    char output[16] = {0};
    char *from = input, *to = output;
    size_t input_bytes = sizeof(input), output_bytes = sizeof(output);
    assert(iconv(conversion, &from, &input_bytes, &to, &output_bytes) == 0);
    assert(!input_bytes && !strcmp(output, "ação"));
    assert(!iconv_close(conversion));
    puts("GTK_ICONV_PASS");

    ffi_cif call;
    ffi_type *types[] = {&ffi_type_uint64, &ffi_type_uint64, &ffi_type_double};
    uint64_t first = UINT64_C(0x100000000), second = 7, result = 0;
    double factor = 5.0;
    void *arguments[] = {&first, &second, &factor};
    assert(ffi_prep_cif(&call, FFI_DEFAULT_ABI, 3, &ffi_type_uint64, types) == FFI_OK);
    ffi_call(&call, FFI_FN(combine), &result, arguments);
    assert(result == first + 12);
    puts("GTK_FFI_PASS");
    puts("GTK_BASE_NATIVE_PASS");
    return 0;
}
