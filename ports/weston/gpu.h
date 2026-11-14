/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_WESTON_GPU_H
#define LUME_WESTON_GPU_H
#include <libweston/libweston.h>
struct lume_weston_gpu;
struct lume_weston_gpu *lume_weston_gpu_create(struct weston_compositor *, int, unsigned *,
                                               unsigned *);
int lume_weston_gpu_output_create(struct lume_weston_gpu *, struct weston_output *);
struct weston_renderbuffer *lume_weston_gpu_next(struct lume_weston_gpu *);
int lume_weston_gpu_present(struct lume_weston_gpu *);
void lume_weston_gpu_output_destroy(struct lume_weston_gpu *);
void lume_weston_gpu_destroy(struct lume_weston_gpu *);
const char *lume_weston_gpu_name(struct lume_weston_gpu *);
#endif
