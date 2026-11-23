/* SPDX-License-Identifier: BSD-3-Clause */
#include "password.h"
#include "auth_files.h"
#include "terminal.h"
#include "lume/abi.h"
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <poll.h>
#include <pty.h>
#include <pwd.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <mbedtls/constant_time.h>
#include <mbedtls/platform_util.h>

static struct passwd *account_for(const char *name, struct passwd *entry, char buffer[4096])
{
    struct passwd *found = NULL;
    if (!auth_name_valid(name) || getpwnam_r(name, entry, buffer, 4096, &found))
        return NULL;
    return found;
}

static int change_password(const char *name)
{
    struct passwd entry;
    char account_buffer[4096];
    if (!account_for(name, &entry, account_buffer))
    {
        fputs("passwd: account unavailable\n", stderr);
        return 1;
    }
    char password[AUTH_PASSWORD_MAX + 1] = {0}, confirmation[AUTH_PASSWORD_MAX + 1] = {0};
    int first = auth_read_line("New password: ", password, sizeof(password), false);
    int second = first < 0 ? -1
                           : auth_read_line("Retype new password: ", confirmation,
                                            sizeof(confirmation), false);
    int result = 1;
    if (first >= 0 && second >= 0)
    {
        if (first < AUTH_PASSWORD_MIN)
            fputs("passwd: use at least 15 bytes\n", stderr);
        else if (first != second || mbedtls_ct_memcmp(password, confirmation, sizeof(password)))
            fputs("passwd: passwords differ\n", stderr);
        else if (auth_password_store(entry.pw_name, entry.pw_uid, password, (size_t)first))
            perror("passwd: update password");
        else
        {
            puts("Password updated.");
            result = 0;
        }
    }
    mbedtls_platform_zeroize(password, sizeof(password));
    mbedtls_platform_zeroize(confirmation, sizeof(confirmation));
    return result;
}

static int start_session(const struct passwd *account)
{
    char home[LUME_PATH_MAX + 5], shell[LUME_PATH_MAX + 6];
    char user[AUTH_NAME_MAX + 6], logname[AUTH_NAME_MAX + 9];
    const char *shell_path = account->pw_uid ? account->pw_shell : "/bin/sh";
    if (account->pw_dir[0] != '/' || shell_path[0] != '/' ||
        strlen(account->pw_dir) >= LUME_PATH_MAX || strlen(shell_path) >= LUME_PATH_MAX)
        return 1;
    snprintf(home, sizeof(home), "HOME=%s", account->pw_dir);
    snprintf(shell, sizeof(shell), "SHELL=%s", shell_path);
    snprintf(user, sizeof(user), "USER=%s", account->pw_name);
    snprintf(logname, sizeof(logname), "LOGNAME=%s", account->pw_name);
    if (!auth_close_descriptors())
    {
        fputs("login: cannot close inherited descriptors\n", stderr);
        return 1;
    }
    if (initgroups(account->pw_name, account->pw_gid) || setgid(account->pw_gid) ||
        setuid(account->pw_uid) || chdir(account->pw_dir))
    {
        perror("login: start session");
        return 1;
    }
    umask(022);
    char *const environment[] = {home,         shell, user, logname, "PATH=/bin:/usr/bin:/data/bin",
                                 "TERM=vt100", NULL};
    char *const argv[] = {(char *)shell_path, NULL};
    execve(shell_path, argv, environment);
    perror("login: execute shell");
    return 1;
}

static int relay_output(int master)
{
    char output[512];
    ssize_t bytes = read(master, output, sizeof(output));
    if (bytes < 0)
        return errno == EAGAIN || errno == EINTR ? 0 : -1;
    if (!bytes)
        return -1;
    for (ssize_t offset = 0; offset < bytes;)
    {
        ssize_t got = write(STDOUT_FILENO, output + offset, (size_t)(bytes - offset));
        if (got > 0)
            offset += got;
        else if (!got || errno != EINTR)
            return -1;
    }
    return (int)bytes;
}

/* The privileged process retains the physical console and relays only this session's PTY. */
static int relay_console(int master, pid_t child)
{
    unsigned char byte;
    bool pending = false;
    int status;
    while (!auth_terminal_interrupted())
    {
        pid_t ended = waitpid(child, &status, WNOHANG);
        if (ended == child)
        {
            for (unsigned drain = 0; drain < 16 && relay_output(master) > 0; ++drain)
            {
            }
            return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        }
        if (ended < 0 && errno != EINTR)
            return 1;
        struct pollfd fds[] = {{.fd = STDIN_FILENO, .events = pending ? 0 : POLLIN},
                               {.fd = master, .events = POLLIN | (pending ? POLLOUT : 0)}};
        if (poll(fds, 2, 100) < 0)
        {
            if (errno == EINTR)
                continue;
            break;
        }
        if (fds[1].revents & (POLLIN | POLLHUP))
        {
            if (relay_output(master) < 0)
                break;
        }
        if (fds[0].revents & (POLLHUP | POLLERR | POLLNVAL) ||
            fds[1].revents & (POLLERR | POLLNVAL))
            break;
        if (fds[0].revents & POLLIN)
        {
            ssize_t got = read(STDIN_FILENO, &byte, 1);
            if (got <= 0 && (!got || (errno != EAGAIN && errno != EINTR)))
                break;
            pending = got == 1;
        }
        if (pending)
        {
            ssize_t sent = write(master, &byte, 1);
            if (sent <= 0 && (!sent || (errno != EAGAIN && errno != EINTR)))
                break;
            pending = sent != 1;
        }
    }
    (void)kill(child, SIGKILL);
    while (waitpid(child, &status, 0) < 0 && errno == EINTR)
    {
    }
    return 1;
}

static int console_session(const struct passwd *account)
{
    int master, slave;
    if (openpty(&master, &slave, NULL, NULL, NULL))
    {
        perror("login: open session terminal");
        return 1;
    }
    pid_t child = -1;
    if (!fcntl(master, F_SETFL, O_NONBLOCK) && !fchown(slave, account->pw_uid, account->pw_gid) &&
        !fchmod(slave, 0600))
        child = fork();
    if (!child)
    {
        close(master);
        if (setsid() < 0 || ioctl(slave, TIOCSCTTY) || dup2(slave, STDIN_FILENO) < 0 ||
            dup2(slave, STDOUT_FILENO) < 0 || dup2(slave, STDERR_FILENO) < 0)
            _exit(1);
        close(slave);
        _exit(start_session(account));
    }
    if (child < 0)
        perror("login: prepare session terminal");
    close(slave);
    int result = child > 0 ? relay_console(master, child) : 1;
    close(master);
    return result;
}

static int login(const char *requested)
{
    for (unsigned attempt = 0; attempt < 3 && !auth_terminal_interrupted(); ++attempt)
    {
        char name[AUTH_NAME_MAX + 1];
        const char *user = requested;
        if (!user)
        {
            if (auth_read_line("login: ", name, sizeof(name), true) < 0)
                return 1;
            user = name;
        }
        struct passwd entry;
        char account_buffer[4096];
        struct passwd *account = account_for(user, &entry, account_buffer);
        char password[AUTH_PASSWORD_MAX + 1] = {0};
        int bytes = auth_read_line("Password: ", password, sizeof(password), false);
        bool accepted = bytes >= 0 && auth_password_matches(account ? account->pw_name : NULL,
                                                            account ? account->pw_uid : 0, password,
                                                            (size_t)bytes);
        mbedtls_platform_zeroize(password, sizeof(password));
        if (accepted && !auth_terminal_interrupted())
            return auth_terminal_is_pty() ? start_session(account) : console_session(account);
        if (bytes < 0 || auth_terminal_interrupted())
            return 1;
        fputs("Login incorrect\n", stderr);
        sleep(1);
    }
    return 1;
}

int main(int argc, char **argv)
{
    if (!argc || !argv[0])
        return 2;
    const char *program = strrchr(argv[0], '/');
    program = program ? program + 1 : argv[0];
    if (!strcmp(program, "sudo-setup"))
    {
        if (argc != 1)
        {
            fputs("usage: sudo-setup\n", stderr);
            return 2;
        }
        if (auth_install_sudo())
        {
            perror("sudo-setup: install /data/bin/sudo");
            return 1;
        }
        puts("Installed /data/bin/sudo.");
        return 0;
    }
    bool passwd = !strcmp(program, "passwd");
    if (argc > 2 || (passwd && argc != 2))
    {
        fputs(passwd ? "usage: passwd account\n" : "usage: login [account]\n", stderr);
        return 2;
    }
    if (getuid() || geteuid() || !auth_terminal_ready())
    {
        fprintf(stderr, "%s: requires a privileged terminal session\n", program);
        return 1;
    }
    return passwd ? change_password(argv[1]) : login(argc == 2 ? argv[1] : NULL);
}
