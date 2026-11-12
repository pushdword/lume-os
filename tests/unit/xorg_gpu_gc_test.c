#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define TRUE 1
#define FALSE 0
#define DRAWABLE_WINDOW 0
#define GXcopy 3
#define PRIVATE_GC 1
typedef int Bool;
typedef unsigned long Pixel;
typedef struct gc *GCPtr;
typedef struct drawable *DrawablePtr;
typedef struct window *WindowPtr;
typedef struct screen *ScreenPtr;

typedef struct region
{
    int value;
} *RegionPtr;

typedef struct box
{
    short x1, y1, x2, y2;
} *BoxPtr;

typedef int DevPrivateKeyRec;
typedef Bool (*CreateGCProcPtr)(GCPtr);

typedef struct
{
    void (*ValidateGC)(GCPtr, unsigned long, DrawablePtr);
    void (*ChangeGC)(GCPtr, unsigned long);
    void (*CopyGC)(GCPtr, unsigned long, GCPtr);
    void (*DestroyGC)(GCPtr);
    void (*ChangeClip)(GCPtr, int, void *, int);
    void (*DestroyClip)(GCPtr);
    void (*CopyClip)(GCPtr, GCPtr);
} GCFuncs;

typedef RegionPtr (*CopyAreaProc)(DrawablePtr, DrawablePtr, GCPtr, int, int, int, int, int, int);

typedef struct
{
    CopyAreaProc CopyArea;
} GCOps;

struct gc
{
    void *devPrivates;
    const GCFuncs *funcs;
    const GCOps *ops;
    int alu;
    Pixel planemask;
};

struct drawable
{
    ScreenPtr pScreen;
    int type, depth;
};

struct window
{
    struct drawable drawable;
    void *pixmap;
};

struct screen
{
    CreateGCProcPtr CreateGC;
    void *(*GetWindowPixmap)(WindowPtr);
    void *(*GetScreenPixmap)(ScreenPtr);
};

static int gpu_fd = 3;
static unsigned copies, fallback, original, copied_gc, destroyed, validated;
static Bool accepted = TRUE;
static struct region result;
static const GCOps original_ops;
static const GCFuncs original_functions;

static void *dixLookupPrivate(void **value, DevPrivateKeyRec *key)
{
    (void)key;
    return *value;
}

static Bool dixRegisterPrivateKey(DevPrivateKeyRec *key, int kind, unsigned bytes)
{
    (void)key;
    assert(kind == PRIVATE_GC && bytes > sizeof(void *));
    return TRUE;
}

static Bool gpu_copy_boxes(BoxPtr boxes, int count, int dx, int dy)
{
    assert(boxes->x1 == 10 && count == 1 && dx == -4 && dy == 7);
    ++copies;
    return accepted;
}

static void fbCopyNtoN(DrawablePtr source, DrawablePtr target, GCPtr gc, BoxPtr boxes, int count,
                       int dx, int dy, Bool reverse, Bool upside, Pixel bitplane, void *closure)
{
    (void)source;
    (void)target;
    (void)gc;
    assert(boxes->x1 == 10 && count == 1 && dx == -4 && dy == 7);
    assert(reverse && upside && bitplane == 0 && closure == &result);
    ++fallback;
}

typedef void (*CopyBoxesProc)(DrawablePtr, DrawablePtr, GCPtr, BoxPtr, int, int, int, Bool, Bool,
                              Pixel, void *);

static RegionPtr miDoCopy(DrawablePtr source, DrawablePtr target, GCPtr gc, int sx, int sy,
                          int width, int height, int tx, int ty, CopyBoxesProc copy, Pixel plane,
                          void *closure)
{
    assert(sx == 1 && sy == 2 && width == 30 && height == 40 && tx == 5 && ty == 6);
    assert(!plane && !closure);
    struct box box = {10, 20, 30, 40};
    copy(source, target, gc, &box, 1, -4, 7, TRUE, TRUE, 0, &result);
    return &result;
}

#include "../../ports/xorg-server/lume_gpu_gc.h"

static RegionPtr original_copy(DrawablePtr a, DrawablePtr b, GCPtr gc, int sx, int sy, int w, int h,
                               int tx, int ty)
{
    (void)a;
    (void)b;
    (void)gc;
    (void)sx;
    (void)sy;
    (void)w;
    (void)h;
    (void)tx;
    (void)ty;
    ++original;
    return &result;
}

static void validate(GCPtr gc, unsigned long changes, DrawablePtr drawable)
{
    (void)changes;
    (void)drawable;
    assert(gc->funcs == &original_functions && gc->ops == &original_ops);
    ++validated;
}

static void change(GCPtr gc, unsigned long changes)
{
    (void)changes;
    assert(gc->funcs == &original_functions);
}

static void copy_gc(GCPtr source, unsigned long changes, GCPtr target)
{
    (void)source;
    (void)changes;
    assert(target->funcs == &original_functions && target->ops == &original_ops);
    ++copied_gc;
}

static void destroy(GCPtr gc)
{
    assert(gc->funcs == &original_functions);
    ++destroyed;
}

static void change_clip(GCPtr gc, int kind, void *value, int count)
{
    (void)kind;
    (void)value;
    (void)count;
    assert(gc->funcs == &original_functions);
}

static void destroy_clip(GCPtr gc)
{
    assert(gc->funcs == &original_functions);
}

static void copy_clip(GCPtr target, GCPtr source)
{
    (void)source;
    assert(target->funcs == &original_functions);
}

static const GCFuncs original_functions = {validate,    change,       copy_gc,  destroy,
                                           change_clip, destroy_clip, copy_clip};
static const GCOps original_ops = {original_copy};

static Bool create_gc(GCPtr gc)
{
    gc->funcs = &original_functions;
    gc->ops = &original_ops;
    return TRUE;
}

static void *window_pixmap(WindowPtr window)
{
    return window->pixmap;
}

static void *screen_pixmap(ScreenPtr screen)
{
    return screen;
}

int main(void)
{
    struct screen screen = {create_gc, window_pixmap, screen_pixmap};
    assert(gpu_gc_install(&screen));
    struct gpu_gc state = {0};
    struct gc gc = {.devPrivates = &state, .alu = GXcopy, .planemask = 0xffffff};
    assert(screen.CreateGC(&gc) && gc.funcs == &gpu_gc_functions);
    struct window a = {{&screen, DRAWABLE_WINDOW, 24}, &screen};
    struct window b = a;
    gc.funcs->ValidateGC(&gc, 0, &a.drawable);
    assert(validated == 1 && gc.funcs == &gpu_gc_functions);
    assert(gc.ops->CopyArea(&a.drawable, &b.drawable, &gc, 1, 2, 30, 40, 5, 6) == &result);
    assert(copies == 1 && !fallback && !original);
    accepted = FALSE;
    gc.ops->CopyArea(&a.drawable, &b.drawable, &gc, 1, 2, 30, 40, 5, 6);
    assert(copies == 2 && fallback == 1);
    b.pixmap = &state;
    gc.ops->CopyArea(&a.drawable, &b.drawable, &gc, 1, 2, 30, 40, 5, 6);
    b.pixmap = &screen;
    gc.planemask = 1;
    gc.ops->CopyArea(&a.drawable, &b.drawable, &gc, 1, 2, 30, 40, 5, 6);
    gc.planemask = 0xffffff;
    gc.alu = 6;
    gc.ops->CopyArea(&a.drawable, &b.drawable, &gc, 1, 2, 30, 40, 5, 6);
    assert(original == 3 && copies == 2);
    gc.funcs->CopyGC(&gc, 0, &gc);
    gc.funcs->ChangeGC(&gc, 0);
    gc.funcs->ChangeClip(&gc, 0, NULL, 0);
    gc.funcs->CopyClip(&gc, &gc);
    gc.funcs->DestroyClip(&gc);
    assert(copied_gc == 1 && gc.funcs == &gpu_gc_functions);
    gc.funcs->DestroyGC(&gc);
    assert(destroyed == 1);
    puts(
        "PASS_XORG_GPU_GC copy dispatch, CPU fallback, pixmap isolation, raster operations and GC lifecycle");
    return 0;
}
