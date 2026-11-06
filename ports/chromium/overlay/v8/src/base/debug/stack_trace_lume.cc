// SPDX-License-Identifier: BSD-3-Clause
#include "src/base/debug/stack_trace.h"

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>

#include <array>
#include <ostream>

namespace v8::base::debug
{
namespace
{

volatile sig_atomic_t dump_stack = 1;

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
        text += written;
        bytes -= written;
    }
}

void StackDumpSignalHandler(int signal_number)
{
    if (dump_stack)
    {
        StackTrace().Print();
    }
    signal(signal_number, SIG_DFL);
    kill(getpid(), signal_number);
    _exit(128 + signal_number);
}

uintptr_t StripPointerAuthenticationBits(uintptr_t address)
{
#if V8_HOST_ARCH_ARM64
    register uintptr_t x30 __asm("x30") = address;
    asm("xpaclri" : "+r"(x30));
    return x30;
#else
    return address;
#endif
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
    dump_stack = 1;
    return success;
}

void DisableSignalStackDump()
{
    dump_stack = 0;
}

StackTrace::StackTrace()
{
    pthread_attr_t attributes;
    if (pthread_getattr_np(pthread_self(), &attributes))
    {
        return;
    }
    void *stack_begin = nullptr;
    size_t stack_size = 0;
    int error = pthread_attr_getstack(&attributes, &stack_begin, &stack_size);
    pthread_attr_destroy(&attributes);
    uintptr_t begin = reinterpret_cast<uintptr_t>(stack_begin);
    if (error || stack_size < 2 * sizeof(uintptr_t) || UINTPTR_MAX - begin < stack_size)
    {
        return;
    }
    uintptr_t last_frame = begin + stack_size - 2 * sizeof(uintptr_t);
    uintptr_t frame = reinterpret_cast<uintptr_t>(__builtin_frame_address(0));
    while (count_ < kMaxTraces && frame >= begin && frame <= last_frame &&
           !(frame & (alignof(uintptr_t) - 1)))
    {
        const uintptr_t *record = reinterpret_cast<const uintptr_t *>(frame);
        uintptr_t address = StripPointerAuthenticationBits(record[1]);
        if (!address)
        {
            break;
        }
        trace_[count_++] = reinterpret_cast<void *>(address);
        uintptr_t next = record[0];
        if (next <= frame)
        {
            break;
        }
        frame = next;
    }
}

void StackTrace::Print() const
{
    constexpr std::array<char, 16> digits{'0', '1', '2', '3', '4', '5', '6', '7',
                                          '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    for (size_t frame = 0; frame < count_; ++frame)
    {
        std::array<char, 2 + 2 * sizeof(uintptr_t) + 1> buffer;
        buffer[0] = '0';
        buffer[1] = 'x';
        uintptr_t value = reinterpret_cast<uintptr_t>(trace_[frame]);
        for (size_t i = buffer.size() - 1; i > 2;)
        {
            buffer[--i] = digits[value & 15];
            value >>= 4;
        }
        buffer.back() = '\n';
        WriteStderr(buffer.data(), buffer.size());
    }
}

void StackTrace::OutputToStream(std::ostream *output) const
{
    for (size_t frame = 0; frame < count_; ++frame)
    {
        *output << trace_[frame] << '\n';
    }
}

} // namespace v8::base::debug
