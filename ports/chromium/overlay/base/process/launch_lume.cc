// SPDX-License-Identifier: BSD-3-Clause
#include "base/process/launch.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <spawn.h>
#include <unistd.h>

#include <set>
#include <vector>

#include "base/check.h"
#include "base/compiler_specific.h"
#include "base/files/scoped_file.h"
#include "base/logging.h"
#include "base/process/environment_internal.h"
#include "base/posix/eintr_wrapper.h"
#include "base/threading/scoped_blocking_call.h"

namespace base
{
namespace
{

class SpawnState
{
  public:
    ~SpawnState()
    {
        if (actions_ready)
        {
            posix_spawn_file_actions_destroy(&actions);
        }
        if (attributes_ready)
        {
            posix_spawnattr_destroy(&attributes);
        }
    }

    posix_spawn_file_actions_t actions;
    posix_spawnattr_t attributes;
    bool actions_ready = false, attributes_ready = false;
};

Process SpawnLume(const std::vector<std::string> &argv, const LaunchOptions &options,
                  char *const custom_environment[], bool search_path)
{
    if (argv.empty() || argv[0].empty())
    {
        errno = EINVAL;
        return Process();
    }
    if (options.pre_exec_delegate || options.new_process_group ||
        (options.maximize_rlimits && !options.maximize_rlimits->empty()))
    {
        errno = ENOTSUP;
        PLOG(ERROR) << "Unsupported process launch attributes";
        return Process();
    }
    std::set<int> targets;
    for (const auto &[source, target] : options.fds_to_remap)
    {
        if (source < 0 || target < 0 || !targets.insert(target).second ||
            fcntl(source, F_GETFD) < 0)
        {
            errno = EINVAL;
            return Process();
        }
    }
    SpawnState state;
    int error = posix_spawnattr_init(&state.attributes);
    state.attributes_ready = error == 0;
    if (!error)
    {
        error = posix_spawn_file_actions_init(&state.actions);
        state.actions_ready = error == 0;
    }
    sigset_t mask, defaults;
    if (!error)
    {
        error = pthread_sigmask(SIG_SETMASK, nullptr, &mask);
    }
    sigemptyset(&defaults);
    for (int number : {SIGHUP, SIGINT, SIGILL, SIGABRT, SIGFPE, SIGBUS, SIGSEGV, SIGSYS, SIGTERM})
    {
        sigaddset(&defaults, number);
    }
    if (!error)
    {
        error = posix_spawnattr_setsigmask(&state.attributes, &mask);
    }
    if (!error)
    {
        error = posix_spawnattr_setsigdefault(&state.attributes, &defaults);
    }
    if (!error)
    {
        error = posix_spawnattr_setflags(&state.attributes, POSIX_SPAWN_SETSIGMASK |
                                                                POSIX_SPAWN_SETSIGDEF |
                                                                POSIX_SPAWN_CLOEXEC_DEFAULT);
    }
    if (!error)
    {
        error = posix_spawn_file_actions_addopen(&state.actions, STDIN_FILENO, "/dev/null",
                                                 O_RDONLY, 0);
    }
    for (int fd : {STDOUT_FILENO, STDERR_FILENO})
    {
        if (!error && fcntl(fd, F_GETFD) >= 0)
        {
            error = posix_spawn_file_actions_adddup2(&state.actions, fd, fd);
        }
    }
    std::vector<ScopedFD> temporary;
    for (const auto &[source, target] : options.fds_to_remap)
    {
        if (error)
        {
            break;
        }
        int copy;
        do
        {
            copy = fcntl(source, F_DUPFD_CLOEXEC, 3);
            if (copy < 0)
            {
                error = errno;
                break;
            }
            temporary.emplace_back(copy);
        } while (targets.contains(copy));
        if (!error)
        {
            error = posix_spawn_file_actions_adddup2(&state.actions, copy, target);
        }
    }
    if (!error && !options.current_directory.empty())
    {
        error = posix_spawn_file_actions_addchdir_np(&state.actions,
                                                     options.current_directory.value().c_str());
    }
    if (error)
    {
        errno = error;
        PLOG(ERROR) << "Process launch setup";
        return Process();
    }
    std::vector<char *> arguments;
    for (const auto &argument : argv)
    {
        arguments.push_back(const_cast<char *>(argument.c_str()));
    }
    arguments.push_back(nullptr);
    auto parent_environment = internal::GetEnvironment();
    if (options.clear_environment)
    {
        parent_environment = {};
    }
    // AlterEnvironment returns a null-terminated array whose strings remain owned here.
    auto environment =
        UNSAFE_BUFFERS(internal::AlterEnvironment(parent_environment, options.environment));
    char *const *selected_environment =
        custom_environment ? custom_environment : environment.data();
    const char *executable =
        options.real_path.empty() ? arguments[0] : options.real_path.value().c_str();
    pid_t pid;
    error = search_path ? posix_spawnp(&pid, executable, &state.actions, &state.attributes,
                                       arguments.data(), selected_environment)
                        : posix_spawn(&pid, executable, &state.actions, &state.attributes,
                                      arguments.data(), selected_environment);
    if (error)
    {
        errno = error;
        PLOG(ERROR) << "posix_spawn";
        return Process();
    }
    Process process(pid);
    if (options.wait)
    {
        ScopedBlockingCall blocking(FROM_HERE, BlockingType::MAY_BLOCK);
        if (!process.WaitForExit(nullptr))
        {
            return Process();
        }
    }
    return process;
}

} // namespace

Process LaunchProcess(const std::vector<std::string> &argv, const LaunchOptions &options)
{
    return SpawnLume(argv, options, nullptr, true);
}

bool GetAppOutputLume(const std::vector<std::string> &argv, char *const environment[],
                      bool include_stderr, std::string *output, bool search_path, int *exit_code)
{
    CHECK(output);
    CHECK(exit_code);
    *exit_code = EXIT_FAILURE;
    int pipe_descriptors[2];
    if (pipe2(pipe_descriptors, O_CLOEXEC) < 0)
    {
        return false;
    }
    ScopedFD reader(pipe_descriptors[0]), writer(pipe_descriptors[1]);
    ScopedFD null(open("/dev/null", O_RDWR | O_CLOEXEC));
    if (!null.is_valid())
    {
        return false;
    }
    LaunchOptions options;
    options.fds_to_remap = {{writer.get(), STDOUT_FILENO},
                            {include_stderr ? writer.get() : null.get(), STDERR_FILENO}};
    Process process = SpawnLume(argv, options, environment, search_path);
    if (!process.IsValid())
    {
        return false;
    }
    writer.reset();
    null.reset();
    output->clear();
    bool read_ok = true;
    for (;;)
    {
        char buffer[4096];
        ssize_t bytes = HANDLE_EINTR(read(reader.get(), buffer, sizeof(buffer)));
        if (bytes <= 0)
        {
            read_ok = bytes == 0;
            break;
        }
        output->append(buffer, static_cast<size_t>(bytes));
    }
    reader.reset();
    internal::GetAppOutputScopedAllowBaseSyncPrimitives allow_wait;
    return process.WaitForExit(exit_code) && read_ok;
}

} // namespace base
