/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_RENDER_NATIVE_H
#define LUME_RENDER_NATIVE_H
#include <stddef.h>
#include <stdint.h>
#include "lume/render.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* Calls return -1 and set errno on failure. Exported memory descriptors and
 * mappings retain their storage independently of the render resource. */
    int lume_render_control(unsigned, struct lume_render_request *);
    int lume_render_open(unsigned);
    int lume_render_query(int, struct lume_render_info *);
    int lume_render_pci(int, struct lume_render_pci *);
    int lume_render_display(int, unsigned, struct lume_render_display *);
    int lume_render_present(int, const struct lume_render_present *);
    int lume_render_context(int, unsigned, unsigned);
    int lume_render_capability_info(int, unsigned, unsigned *, unsigned *, size_t *);
    int lume_render_capability(int, unsigned, unsigned, unsigned, void *, size_t);
    int lume_render_resource_create(int, struct lume_render_resource *);
    int lume_render_resource_export(int, unsigned);
    int lume_render_resource_close(int, unsigned);
    int lume_render_resource_info(int, unsigned, struct lume_render_resource *);
    int lume_render_resource_wait(int, unsigned, unsigned);
    int lume_render_prime_export(int, unsigned, unsigned);
    int lume_render_prime_import(int, int, unsigned *);
    int lume_render_transfer(int, unsigned, const struct lume_render_transfer *);
    int lume_render_submit(int, const void *, size_t, uint64_t *);
    int lume_render_submit_resources(int, const void *, size_t, const uint32_t *, unsigned,
                                     uint64_t *);
    int lume_render_wait(int, uint64_t, unsigned);

#ifdef __cplusplus
}
#endif
#endif
