// SPDX-License-Identifier: BSD-3-Clause
#include "base/debug/stack_trace.h"

#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include <array>
#include <ostream>

#include "base/compiler_specific.h"

namespace base::debug
{
namespace
{

void WriteStderr(const char *text, size_t bytes)
{
    while (bytes)
    {
        ssize_t written = write(STDERR_FILENO, text, bytes);
        if (written < 0 && errno == EINTR)
        {
            continue;
        }
        if (written <= 0)
        {
            break;
        }
        UNSAFE_BUFFERS(text += written);
        bytes -= written;
    }
}

std::array<char, 2 + 2 * sizeof(uintptr_t) + 1> FormatAddress(const void *address)
{
    std::array<char, 2 + 2 * sizeof(uintptr_t) + 1> buffer;
    buffer[0] = '0';
    buffer[1] = 'x';
    uintptr_t value = reinterpret_cast<uintptr_t>(address);
    constexpr std::array<char, 16> digits{'0', '1', '2', '3', '4', '5', '6', '7',
                                          '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    for (size_t i = buffer.size() - 1; i > 2;)
    {
        buffer[--i] = digits[value & 15];
        value >>= 4;
    }
    buffer.back() = '\n';
    return buffer;
}

void StackDumpSignalHandler(int signal_number)
{
    StackTrace().Print();
    signal(signal_number, SIG_DFL);
    kill(getpid(), signal_number);
    _exit(128 + signal_number);
}

} // namespace

bool EnableInProcessStackDumping()
{
    struct sigaction action = {};
    sigemptyset(&action.sa_mask);
    action.sa_handler = SIG_IGN;
    bool success = sigaction(SIGPIPE, &action, nullptr) == 0;
    action.sa_handler = StackDumpSignalHandler;
    action.sa_flags = SA_RESETHAND;
    for (int signal_number : {SIGILL, SIGABRT, SIGFPE, SIGBUS, SIGSEGV, SIGSYS})
    {
        success &= sigaction(signal_number, &action, nullptr) == 0;
    }
    return success;
}

size_t CollectStackTrace(span<const void *> trace)
{
#if BUILDFLAG(CAN_UNWIND_WITH_FRAME_POINTERS)
    return TraceStackFramePointers(trace, 0, false);
#else
    return 0;
#endif
}

void StackTrace::PrintMessageWithPrefix(cstring_view prefix, cstring_view message)
{
    WriteStderr(prefix.c_str(), prefix.size());
    WriteStderr(message.c_str(), message.size());
}

void StackTrace::PrintWithPrefixImpl(cstring_view prefix) const
{
    for (const void *address : addresses())
    {
        WriteStderr(prefix.c_str(), prefix.size());
        auto line = FormatAddress(address);
        WriteStderr(line.data(), line.size());
    }
}

void StackTrace::OutputToStreamWithPrefixImpl(std::ostream *output, cstring_view prefix) const
{
    for (const void *address : addresses())
    {
        *output << prefix << address << '\n';
    }
}

} // namespace base::debug
