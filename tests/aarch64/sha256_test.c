#include "sha256.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void check(const void *p, size_t count, const char *expected)
{
    struct lume_sha256 s;
    unsigned char hash[32];
    char text[65];
    lume_sha256_init(&s);
    lume_sha256_update(&s, p, count);
    lume_sha256_final(&s, hash);
    for (unsigned i = 0; i < 32; ++i)
        sprintf(text + 2 * i, "%02x", hash[i]);
    assert(!strcmp(text, expected));
}

int main(void)
{
    check("", 0, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    check("abc", 3, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    check("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56,
          "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    struct lume_sha256 s;
    unsigned char hash[32];
    char text[65];
    lume_sha256_init(&s);
    for (unsigned i = 0; i < 1000000; ++i)
        lume_sha256_update(&s, "a", 1);
    lume_sha256_final(&s, hash);
    for (unsigned i = 0; i < 32; ++i)
        sprintf(text + 2 * i, "%02x", hash[i]);
    assert(!strcmp(text, "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"));
    puts("PASS SHA256 empty, short, padding boundary and million single-byte updates");
}
