/* SPDX-License-Identifier: BSD-3-Clause */
#include <X11/Xlib.h>
#include <stdio.h>
#include <unistd.h>

int __real_XGrabKey(Display *, int, unsigned, Window, Bool, int, int);
int __real_XUngrabKey(Display *, int, unsigned, Window);
int __real_XNextEvent(Display *, XEvent *);
pid_t __real_fork(void);

int __wrap_XGrabKey(Display *display, int code, unsigned modifiers, Window window, Bool owner,
                    int pointer_mode, int keyboard_mode)
{
    if (code == 28)
        fprintf(stderr, "WM_KEY_GRAB code=%d modifiers=%u window=%lu\n", code, modifiers, window);
    return __real_XGrabKey(display, code, modifiers, window, owner, pointer_mode, keyboard_mode);
}

int __wrap_XUngrabKey(Display *display, int code, unsigned modifiers, Window window)
{
    fprintf(stderr, "WM_KEY_UNGRAB code=%d modifiers=%u window=%lu\n", code, modifiers, window);
    return __real_XUngrabKey(display, code, modifiers, window);
}

int __wrap_XNextEvent(Display *display, XEvent *event)
{
    int result = __real_XNextEvent(display, event);
    if (event->type == KeyPress || event->type == KeyRelease)
        fprintf(stderr, "WM_KEY_EVENT type=%d code=%u state=%u\n", event->type, event->xkey.keycode,
                event->xkey.state);
    else if (event->type == MappingNotify)
        fprintf(stderr, "WM_KEY_MAPPING request=%d first=%d count=%d\n", event->xmapping.request,
                event->xmapping.first_keycode, event->xmapping.count);
    return result;
}

pid_t __wrap_fork(void)
{
    fprintf(stderr, "WM_FORK_BEGIN\n");
    pid_t result = __real_fork();
    fprintf(stderr, "WM_FORK_RESULT pid=%d\n", (int)result);
    return result;
}
