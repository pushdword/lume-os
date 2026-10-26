// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "llvm/Support/Errc.h"

namespace llvm
{
static Error noSockets()
{
    return errorCodeToError(make_error_code(errc::function_not_supported));
}

ListeningSocket::ListeningSocket(int F, StringRef P, int Pipes[2])
    : FD(F), SocketPath(P), PipeFD{Pipes[0], Pipes[1]}
{
}

ListeningSocket::ListeningSocket(ListeningSocket &&O)
    : FD(O.FD.exchange(-1)), SocketPath(std::move(O.SocketPath)), PipeFD{-1, -1}
{
}

ListeningSocket::~ListeningSocket() = default;

void ListeningSocket::shutdown()
{
    FD = -1;
}

Expected<ListeningSocket> ListeningSocket::createUnix(StringRef, int)
{
    return noSockets();
}

Expected<std::unique_ptr<raw_socket_stream>>
ListeningSocket::accept(const std::chrono::milliseconds &)
{
    return noSockets();
}

raw_socket_stream::raw_socket_stream(int F) : raw_fd_stream(F, true)
{
}

raw_socket_stream::~raw_socket_stream() = default;

Expected<std::unique_ptr<raw_socket_stream>> raw_socket_stream::createConnectedUnix(StringRef)
{
    return noSockets();
}

ssize_t raw_socket_stream::read(char *, size_t, const std::chrono::milliseconds &)
{
    error_detected(make_error_code(errc::function_not_supported));
    return -1;
}
}
