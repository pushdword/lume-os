/* SPDX-License-Identifier: BSD-3-Clause */
#include "auth_files.h"
#include "password.h"
#include "sudo_policy.h"
#include "terminal.h"
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mbedtls/platform_util.h>

static bool authenticate(const struct passwd *user)
{
    if (!auth_terminal_ready())
    {
        fputs("sudo: authentication requires a terminal\n", stderr);
        return false;
    }
    for (unsigned attempt = 0; attempt < 3 && !auth_terminal_interrupted(); ++attempt)
    {
        char password[AUTH_PASSWORD_MAX + 1] = {0};
        int bytes = auth_read_line("[sudo] Password: ", password, sizeof(password), false);
        bool accepted = bytes >= 0 &&
                        auth_password_matches(user->pw_name, user->pw_uid, password, (size_t)bytes);
        mbedtls_platform_zeroize(password, sizeof(password));
        if (bytes < 0 || auth_terminal_interrupted())
            return false;
        if (accepted)
            return true;
        fputs("sudo: incorrect password\n", stderr);
        sleep(1);
    }
    return false;
}

int main(int argc, char **argv)
{
    int first = argc > 1 && !strcmp(argv[1], "--") ? 2 : 1;
    if (argc <= first || argv[first][0] != '/')
    {
        fputs("usage: sudo [--] /absolute/command [argument ...]\n", stderr);
        fputs("Rules in /data/etc/sudoers use user or %group, ALL=(root), and a command.\n",
              stderr);
        return 2;
    }
    if (geteuid())
    {
        fputs("sudo: requires an installation owned by root with set-user-ID permissions\n",
              stderr);
        return 1;
    }
    struct passwd user, *found;
    char buffer[4096];
    uid_t caller = getuid();
    char **command = argv + first;
    if (caller && (getpwuid_r(caller, &user, buffer, sizeof(buffer), &found) || !found ||
                   !sudo_policy_allows(user.pw_name, getgid(), command)))
    {
        fputs("sudo: command denied by policy\n", stderr);
        return 1;
    }
    if (!sudo_trusted_command(command[0]))
    {
        fputs("sudo: command path must be owned by root and protected from modification\n", stderr);
        return 1;
    }
    if (caller && !authenticate(&user))
        return 1;
    /* Recheck policy and the executable after the terminal wait. */
    if ((caller && !sudo_policy_allows(user.pw_name, getgid(), command)) ||
        !sudo_trusted_command(command[0]))
    {
        fputs("sudo: command authorization changed\n", stderr);
        return 1;
    }
    if (!auth_close_descriptors() || setgroups(0, NULL) || setgid(0) || setuid(0) || chdir("/"))
    {
        fputs("sudo: cannot prepare command credentials\n", stderr);
        return 1;
    }
    umask(022);
    char *const environment[] = {
        "HOME=/",     "USER=root", "LOGNAME=root", "SHELL=/bin/sh", "PATH=/bin:/usr/bin",
        "TERM=vt100", NULL};
    execve(command[0], command, environment);
    perror("sudo: execute command");
    return 126;
}
