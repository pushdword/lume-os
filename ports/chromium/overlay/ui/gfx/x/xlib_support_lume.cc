// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/x/xlib_support.h"

#include "base/check.h"
#include "base/logging.h"

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>

namespace x11
{
namespace
{

int XlibErrorHandler(Display *, XErrorEvent *)
{
    DVLOG(1) << "Xlib error received";
    return 0;
}

} // namespace

void InitXlib()
{
    static const bool initialized = []
    {
        CHECK(::XInitThreads());
        SetXlibErrorHandler();
        return true;
    }();
    (void)initialized;
}

void SetXlibErrorHandler()
{
    ::XSetErrorHandler(XlibErrorHandler);
}

void XlibFree(void *data)
{
    (void)::XFree(data);
}

XlibDisplay::XlibDisplay(const std::string &address)
{
    InitXlib();
    display_ = ::XOpenDisplay(address.empty() ? nullptr : address.c_str());
}

XlibDisplay::~XlibDisplay()
{
    if (!display_)
    {
        return;
    }
    CHECK(!::XPending(display_));
    (void)::XCloseDisplay(display_.ExtractAsDangling());
}

struct xcb_connection_t *XlibDisplay::GetXcbConnection()
{
    return ::XGetXCBConnection(display_);
}

} // namespace x11
