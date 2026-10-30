/*
Copyright 1993, 1998 The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.
*/
#include "dix-config.h"
#include "scrnintstr.h"
#include "inputstr.h"
#include "fb.h"
#include "mi.h"
#include "micmap.h"
#include "mipointer.h"
#include "damage.h"
#include "globals.h"
#include "exevents.h"
#include "xserver-properties.h"
#include "xkbsrv.h"
#include "lume/abi.h"
#include "lume/graphics.h"
#include "lume/syscall_native.h"
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static struct lume_graphics_request framebuffer_info;
static uint32_t *pixels;
static int input_fd = -1;
static DeviceIntPtr keyboard, pointer_device;
static DamagePtr damage;
static CloseScreenProcPtr close_screen;
static CreateScreenResourcesProcPtr create_resources;
static Bool held[256];
static unsigned buttons;
static ValuatorMask *motion;

int FakeScreenFps = 60;

void DDXRingBell(int volume, int pitch, int duration)
{
    (void)volume;
    (void)pitch;
    (void)duration;
}

static int graphics(unsigned action, struct lume_graphics_request *request)
{
    request->version = LUME_GRAPHICS_VERSION;
    request->bytes = sizeof(*request);
    return (int)lume_native_call(LUME_SYS_GRAPHICS, action, (uintptr_t)request, sizeof(*request));
}

static void present(unsigned x, unsigned y, unsigned width, unsigned height)
{
    struct lume_graphics_request request = {
        .fd = input_fd,
        .format = LUME_GRAPHICS_XRGB8888,
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .pitch = framebuffer_info.pitch,
        .pixels = (uintptr_t)(pixels + (size_t)y * framebuffer_info.width + x)};
    int result = graphics(LUME_GRAPHICS_PRESENT, &request);
    if (result)
        FatalError("Framebuffer update failed (%d)\n", result);
}

static void flush_screen(void *data, void *timeout)
{
    (void)data;
    (void)timeout;
    if (!damage)
        return;
    RegionPtr region = DamageRegion(damage);
    BoxPtr boxes = RegionRects(region);
    int count = RegionNumRects(region);
    for (int i = 0; i < count; ++i)
        present(boxes[i].x1, boxes[i].y1, boxes[i].x2 - boxes[i].x1, boxes[i].y2 - boxes[i].y1);
    DamageEmpty(damage);
}

static void wake_screen(void *data, int result)
{
    (void)data;
    (void)result;
}

static Bool screen_resources(ScreenPtr screen)
{
    screen->CreateScreenResources = create_resources;
    if (!screen->CreateScreenResources(screen))
        return FALSE;
    damage = DamageCreate(NULL, NULL, DamageReportNone, TRUE, screen, NULL);
    if (!damage)
        return FALSE;
    DamageRegister(&screen->GetScreenPixmap(screen)->drawable, damage);
    present(0, 0, framebuffer_info.width, framebuffer_info.height);
    return TRUE;
}

static Bool screen_close(ScreenPtr screen)
{
    RemoveBlockAndWakeupHandlers(flush_screen, wake_screen, screen);
    if (damage)
    {
        DamageUnregister(damage);
        DamageDestroy(damage);
        damage = NULL;
    }
    screen->CloseScreen = close_screen;
    Bool result = screen->CloseScreen(screen);
    free(pixels);
    pixels = NULL;
    if (input_fd >= 0)
    {
        RemoveNotifyFd(input_fd);
        close(input_fd);
        input_fd = -1;
    }
    return result;
}

static Bool cursor_off_screen(ScreenPtr *screen, int *x, int *y)
{
    (void)screen;
    (void)x;
    (void)y;
    return FALSE;
}

static void cursor_cross_screen(ScreenPtr screen, Bool entering)
{
    (void)screen;
    (void)entering;
}

static miPointerScreenFuncRec cursor_functions = {cursor_off_screen, cursor_cross_screen,
                                                  miPointerWarpCursor};

static Bool screen_init(ScreenPtr screen, int argc, char **argv)
{
    (void)argc;
    (void)argv;
    framebuffer_info = (struct lume_graphics_request){0};
    int result = graphics(LUME_GRAPHICS_ACQUIRE, &framebuffer_info);
    if (result)
        FatalError("Cannot acquire graphics device (%d)\n", result);
    input_fd = framebuffer_info.fd;
    if (framebuffer_info.format != LUME_GRAPHICS_XRGB8888 ||
        framebuffer_info.pitch != framebuffer_info.width * 4)
        FatalError("Unsupported framebuffer format\n");
    pixels = calloc(framebuffer_info.height, framebuffer_info.pitch);
    if (!pixels)
        FatalError("Cannot allocate screen pixels\n");
    miSetVisualTypesAndMasks(24, TrueColorMask, 8, TrueColor, 0xff0000, 0xff00, 0xff);
    miSetPixmapDepths();
    if (!fbScreenInit(screen, pixels, framebuffer_info.width, framebuffer_info.height, 96, 96,
                      framebuffer_info.width, 32) ||
        !fbPictureInit(screen, NULL, 0) || !DamageSetup(screen))
        return FALSE;
    screen->blackPixel = 0;
    screen->whitePixel = 0xffffff;
    if (!miDCInitialize(screen, &cursor_functions) || !fbCreateDefColormap(screen))
        return FALSE;
    create_resources = screen->CreateScreenResources;
    screen->CreateScreenResources = screen_resources;
    close_screen = screen->CloseScreen;
    screen->CloseScreen = screen_close;
    return RegisterBlockAndWakeupHandlers(flush_screen, wake_screen, screen);
}

void InitOutput(ScreenInfo *info, int argc, char **argv)
{
    static const int depths[] = {1, 4, 8, 16, 24, 32};
    info->imageByteOrder = LSBFirst;
    info->bitmapScanlineUnit = 32;
    info->bitmapScanlinePad = 32;
    info->bitmapBitOrder = LSBFirst;
    info->numPixmapFormats = sizeof(depths) / sizeof(depths[0]);
    for (int i = 0; i < info->numPixmapFormats; ++i)
    {
        info->formats[i].depth = depths[i];
        info->formats[i].bitsPerPixel = depths[i] == 1    ? 1
                                        : depths[i] <= 8  ? 8
                                        : depths[i] <= 16 ? 16
                                                          : 32;
        info->formats[i].scanlinePad = 32;
    }
    if (AddScreen(screen_init, argc, argv) < 0)
        FatalError("Cannot initialize screen\n");
}

static const unsigned char keycodes[256] = {
    [4] = 38,  56,         54,        40,  26,  41,  42,         43,        31,  44,  45,  46,
    58,        57,         32,        33,  24,  27,  39,         28,        30,  55,  25,  53,
    29,        52,         [30] = 10, 11,  12,  13,  14,         15,        16,  17,  18,  19,
    [40] = 36, 9,          22,        23,  65,  20,  21,         34,        35,  51,  51,  47,
    48,        49,         59,        60,  61,  66,  [58] = 67,  68,        69,  70,  71,  72,
    73,        74,         75,        76,  95,  96,  [70] = 107, 78,        127, 118, 110, 112,
    119,       115,        117,       114, 113, 116, 111,        [83] = 77, 106, 63,  82,  86,
    104,       87,         88,        89,  83,  84,  85,         79,        80,  81,  90,  91,
    94,        [224] = 37, 50,        64,  133, 105, 62,         108,       134};

static void release_input(void)
{
    for (unsigned i = 8; i < 256; ++i)
        if (held[i])
        {
            QueueKeyboardEvents(keyboard, KeyRelease, i);
            held[i] = FALSE;
        }
    for (unsigned i = 0; i < 3; ++i)
        if (buttons & (1u << i))
            QueuePointerEvents(pointer_device, ButtonRelease, i == 0 ? 1 : i == 1 ? 3 : 2, 0, NULL);
    buttons = 0;
}

static void input_ready(int fd, int ready, void *data)
{
    (void)ready;
    (void)data;
    struct lume_input_event events[32];
    for (;;)
    {
        ssize_t bytes = read(fd, events, sizeof(events));
        if (bytes < 0 && errno == EAGAIN)
            break;
        if (bytes <= 0 || bytes % sizeof(events[0]))
            FatalError("Graphics input disconnected\n");
        unsigned count = (unsigned)bytes / sizeof(events[0]);
        for (unsigned i = 0; i < count; ++i)
        {
            struct lume_input_event *event = events + i;
            if (event->type == LUME_INPUT_RESET)
                release_input();
            else if (event->type == LUME_INPUT_KEY && event->code < 256)
            {
                unsigned key = keycodes[event->code];
                if (key && held[key] != !!event->x)
                {
                    QueueKeyboardEvents(keyboard, event->x ? KeyPress : KeyRelease, key);
                    held[key] = !!event->x;
                }
            }
            else if (event->type == LUME_INPUT_POINTER)
            {
                valuator_mask_zero(motion);
                valuator_mask_set(motion, 0, event->x);
                valuator_mask_set(motion, 1, event->y);
                QueuePointerEvents(pointer_device, MotionNotify, 0, POINTER_RELATIVE, motion);
                unsigned changed = buttons ^ event->buttons;
                for (unsigned button = 0; button < 3; ++button)
                    if (changed & (1u << button))
                        QueuePointerEvents(pointer_device,
                                           event->buttons & (1u << button) ? ButtonPress
                                                                           : ButtonRelease,
                                           button == 0   ? 1
                                           : button == 1 ? 3
                                                         : 2,
                                           0, NULL);
                buttons = event->buttons & 7;
            }
        }
    }
}

static int keyboard_proc(DeviceIntPtr device, int operation)
{
    if (operation == DEVICE_INIT)
    {
        XkbRMLVOSet keymap = {.rules = "evdev", .model = "pc105", .layout = "pt"};
        if (!InitKeyboardDeviceStruct(device, &keymap, NULL, NULL))
            return BadImplementation;
    }
    else if (operation == DEVICE_ON || operation == DEVICE_OFF)
        device->public.on = operation == DEVICE_ON;
    return Success;
}

static int pointer_proc(DeviceIntPtr device, int operation)
{
    if (operation == DEVICE_INIT)
    {
        BYTE map[] = {0, 1, 2, 3};
        Atom labels[] = {0, 0, 0}, axes[] = {0, 0};
        InitPointerDeviceStruct(&device->public, map, 3, labels, (PtrCtrlProcPtr)NoopDDA,
                                GetMotionHistorySize(), 2, axes);
    }
    else if (operation == DEVICE_ON || operation == DEVICE_OFF)
        device->public.on = operation == DEVICE_ON;
    return Success;
}

void InitInput(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    keyboard = AddInputDevice(serverClient, keyboard_proc, TRUE);
    pointer_device = AddInputDevice(serverClient, pointer_proc, TRUE);
    if (!keyboard || !pointer_device || !mieqInit())
        FatalError("Cannot initialize input devices\n");
    AssignTypeAndName(keyboard, MakeAtom(XI_KEYBOARD, sizeof(XI_KEYBOARD) - 1, TRUE),
                      "Lume keyboard");
    AssignTypeAndName(pointer_device, MakeAtom(XI_MOUSE, sizeof(XI_MOUSE) - 1, TRUE),
                      "Lume pointer");
    motion = valuator_mask_new(2);
    if (!motion || !SetNotifyFd(input_fd, input_ready, X_NOTIFY_READ, NULL))
        FatalError("Cannot register input events\n");
}

void ProcessInputEvents(void)
{
    mieqProcessInputEvents();
}

void CloseInput(void)
{
    if (input_fd >= 0)
        RemoveNotifyFd(input_fd);
    valuator_mask_free(&motion);
    mieqFini();
}

void ddxGiveUp(enum ExitCode code)
{
    (void)code;
    if (input_fd >= 0)
        close(input_fd);
    input_fd = -1;
    free(pixels);
    pixels = NULL;
}

void OsVendorInit(void)
{
    XkbRMLVOSet keymap = {.rules = "evdev", .model = "pc105", .layout = "pt"};
    XkbSetRulesDflts(&keymap);
}

void OsVendorFatalError(const char *format, va_list arguments)
{
    (void)format;
    (void)arguments;
}

void ddxUseMsg(void)
{
    ErrorF("The screen uses the active video mode and PT keyboard layout.\n");
}

int ddxProcessArgument(int argc, char **argv, int index)
{
    (void)argc;
    (void)argv;
    (void)index;
    return 0;
}
