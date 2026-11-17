/* SPDX-License-Identifier: BSD-3-Clause */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 1)
    {
        int help = argc == 2 && !strcmp(argv[1], "--help");
        fputs("Usage: startwayland\nStart the XFCE Wayland desktop with GPU composition.\n",
              help ? stdout : stderr);
        return help ? 0 : 2;
    }
    const char *session = access("/data/bin/lume-xfce-session", X_OK) == 0
                              ? "/data/bin/lume-xfce-session"
                              : "/usr/bin/lume-xfce-session";
    const char *display = getenv("WAYLAND_DISPLAY");
    if (display && *display)
    {
        char *arguments[] = {"lume-xfce-session", NULL};
        execv(session, arguments);
    }
    else
    {
        char *arguments[] = {"wayland", "--renderer", "gl", "--", (char *)session, NULL};
        execv("/data/bin/wayland", arguments);
    }
    fprintf(stderr, "startwayland: cannot start desktop: %s\n", strerror(errno));
    return 1;
}
