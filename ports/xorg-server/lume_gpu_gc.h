/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_XORG_GPU_GC_H
#define LUME_XORG_GPU_GC_H
#include "gcstruct.h"

struct gpu_gc
{
    const GCFuncs *functions;
    const GCOps *operations;
    GCOps accelerated;
};

static DevPrivateKeyRec gpu_gc_key;
static CreateGCProcPtr gpu_create_gc;
static const GCFuncs gpu_gc_functions;

static struct gpu_gc *gpu_gc_state(GCPtr gc)
{
    return dixLookupPrivate(&gc->devPrivates, &gpu_gc_key);
}

static void gpu_copy_area_boxes(DrawablePtr source, DrawablePtr target, GCPtr gc, BoxPtr boxes,
                                int count, int dx, int dy, Bool reverse, Bool upside_down,
                                Pixel bitplane, void *closure)
{
    if (!gpu_copy_boxes(boxes, count, dx, dy))
        fbCopyNtoN(source, target, gc, boxes, count, dx, dy, reverse, upside_down, bitplane,
                   closure);
}

static RegionPtr gpu_copy_area(DrawablePtr source, DrawablePtr target, GCPtr gc, int source_x,
                               int source_y, int width, int height, int target_x, int target_y)
{
    ScreenPtr screen = target->pScreen;
    if (gpu_fd < 0 || source->type != DRAWABLE_WINDOW || target->type != DRAWABLE_WINDOW ||
        source->pScreen != screen || source->depth != 24 || target->depth != 24 ||
        gc->alu != GXcopy || (gc->planemask & UINT32_C(0xffffff)) != UINT32_C(0xffffff) ||
        screen->GetWindowPixmap((WindowPtr)source) != screen->GetScreenPixmap(screen) ||
        screen->GetWindowPixmap((WindowPtr)target) != screen->GetScreenPixmap(screen))
        return gpu_gc_state(gc)->operations->CopyArea(source, target, gc, source_x, source_y, width,
                                                      height, target_x, target_y);
    return miDoCopy(source, target, gc, source_x, source_y, width, height, target_x, target_y,
                    gpu_copy_area_boxes, 0, NULL);
}

static void gpu_gc_unwrap(GCPtr gc)
{
    struct gpu_gc *state = gpu_gc_state(gc);
    gc->funcs = state->functions;
    gc->ops = state->operations;
}

static void gpu_gc_wrap(GCPtr gc)
{
    struct gpu_gc *state = gpu_gc_state(gc);
    state->functions = gc->funcs;
    state->operations = gc->ops;
    if (gc->ops)
    {
        state->accelerated = *gc->ops;
        state->accelerated.CopyArea = gpu_copy_area;
        gc->ops = &state->accelerated;
    }
    gc->funcs = &gpu_gc_functions;
}

static void gpu_gc_validate(GCPtr gc, unsigned long changes, DrawablePtr drawable)
{
    gpu_gc_unwrap(gc);
    gc->funcs->ValidateGC(gc, changes, drawable);
    gpu_gc_wrap(gc);
}

static void gpu_gc_change(GCPtr gc, unsigned long changes)
{
    gpu_gc_unwrap(gc);
    gc->funcs->ChangeGC(gc, changes);
    gpu_gc_wrap(gc);
}

static void gpu_gc_copy(GCPtr source, unsigned long changes, GCPtr target)
{
    gpu_gc_unwrap(target);
    target->funcs->CopyGC(source, changes, target);
    gpu_gc_wrap(target);
}

static void gpu_gc_destroy(GCPtr gc)
{
    gpu_gc_unwrap(gc);
    gc->funcs->DestroyGC(gc);
}

static void gpu_gc_change_clip(GCPtr gc, int kind, void *value, int count)
{
    gpu_gc_unwrap(gc);
    gc->funcs->ChangeClip(gc, kind, value, count);
    gpu_gc_wrap(gc);
}

static void gpu_gc_destroy_clip(GCPtr gc)
{
    gpu_gc_unwrap(gc);
    gc->funcs->DestroyClip(gc);
    gpu_gc_wrap(gc);
}

static void gpu_gc_copy_clip(GCPtr target, GCPtr source)
{
    gpu_gc_unwrap(target);
    target->funcs->CopyClip(target, source);
    gpu_gc_wrap(target);
}

static const GCFuncs gpu_gc_functions = {gpu_gc_validate, gpu_gc_change,      gpu_gc_copy,
                                         gpu_gc_destroy,  gpu_gc_change_clip, gpu_gc_destroy_clip,
                                         gpu_gc_copy_clip};

static Bool gpu_gc_create(GCPtr gc)
{
    if (!gpu_create_gc(gc))
        return FALSE;
    gpu_gc_wrap(gc);
    return TRUE;
}

static Bool gpu_gc_install(ScreenPtr screen)
{
    if (gpu_fd < 0)
        return TRUE;
    if (!dixRegisterPrivateKey(&gpu_gc_key, PRIVATE_GC, sizeof(struct gpu_gc)))
        return FALSE;
    gpu_create_gc = screen->CreateGC;
    screen->CreateGC = gpu_gc_create;
    return TRUE;
}
#endif
