/* SPDX-License-Identifier: BSD-3-Clause */
#include <config.h>
#include <assert.h>
#include <stdint.h>
#include "dev-ino.h"
#include "di-set.h"
#include "ino-map.h"

int main(void)
{
    static_assert(sizeof(((struct dev_ino *)0)->st_ino) >= sizeof(uint64_t));
    struct di_set *set = di_set_alloc();
    assert(set);
    uintmax_t first = UINT64_C(1);
    uintmax_t second = UINT64_C(65537);
    uintmax_t large = UINT64_MAX - 1;
    assert(di_set_insert(set, 7, first) == 1);
    assert(di_set_lookup(set, 7, second) == 0);
    assert(di_set_insert(set, 7, second) == 1);
    assert(di_set_insert(set, 7, large) == 1);
    assert(di_set_insert(set, 7, first) == 0);
    assert(di_set_lookup(set, 7, first) == 1);
    assert(di_set_lookup(set, 7, second) == 1);
    assert(di_set_lookup(set, 7, large) == 1);
    assert(di_set_lookup(set, 8, first) == 0);
    assert(di_set_insert(set, 8, first) == 1);
    di_set_free(set);

    struct ino_map *map = ino_map_alloc(0);
    assert(map);
    size_t a = ino_map_insert(map, first);
    size_t b = ino_map_insert(map, second);
    size_t c = ino_map_insert(map, large);
    assert(a != INO_MAP_INSERT_FAILURE && b != INO_MAP_INSERT_FAILURE &&
           c != INO_MAP_INSERT_FAILURE);
    assert(a != b && a != c && b != c);
    assert(ino_map_insert(map, first) == a);
    assert(ino_map_insert(map, second) == b);
    assert(ino_map_insert(map, large) == c);
    ino_map_free(map);
    return 0;
}
