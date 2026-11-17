/* SPDX-License-Identifier: BSD-3-Clause */
#include <gio/gio.h>
#include <langinfo.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "--terminal"))
    {
        if (!isatty(0) || !isatty(1))
            return 1;
        puts("\033[32mTerminal PTY ready\033[0m");
        fflush(stdout);
        struct timespec delay = {.tv_sec = 2};
        return nanosleep(&delay, NULL) != 0;
    }
    int failures = 0;
    char text[128];
    snprintf(text, sizeof(text), "%u %llu %s", 7u, 123456789012345ULL, "format");
    int formatted = strcmp(text, "7 123456789012345 format") == 0;
    failures += !formatted;
    printf("DESKTOP_RUNTIME printf=%d text=%s\n", formatted, text);
    struct timespec now = {0};
    int timed = clock_gettime(CLOCK_MONOTONIC, &now) == 0 && now.tv_sec >= 0 && now.tv_nsec >= 0 &&
                now.tv_nsec < 1000000000;
    failures += !timed;
    printf("DESKTOP_RUNTIME time=%d sec=%lld ns=%ld size=%u\n", timed, (long long)now.tv_sec,
           now.tv_nsec, (unsigned)sizeof(now));
    timed = clock_gettime(CLOCK_REALTIME, &now) == 0 && now.tv_sec >= 0 && now.tv_nsec >= 0 &&
            now.tv_nsec < 1000000000;
    failures += !timed;
    printf("DESKTOP_RUNTIME realtime=%d sec=%lld ns=%ld\n", timed, (long long)now.tv_sec,
           now.tv_nsec);
    const char *charset;
    g_get_charset(&charset);
    printf("DESKTOP_RUNTIME locale=%s codeset=%s charset=%s\n", setlocale(LC_CTYPE, NULL),
           nl_langinfo(CODESET), charset);
    GError *error = g_error_new(G_IO_ERROR, G_IO_ERROR_FAILED, "error %u", 42u);
    int errors = error && error->code == G_IO_ERROR_FAILED && !strcmp(error->message, "error 42");
    failures += !errors;
    printf("DESKTOP_RUNTIME gerror=%d\n", errors);
    g_clear_error(&error);
    GSocketAddress *address = g_unix_socket_address_new("/data/tmp/desktop-runtime-absent");
    GSocketClient *client = g_socket_client_new();
    GSocketConnection *connection =
        g_socket_client_connect(client, G_SOCKET_CONNECTABLE(address), NULL, &error);
    int connected = !connection && error != NULL;
    failures += !connected;
    printf("DESKTOP_RUNTIME connect=%d code=%d message=%s\n", connected, error ? error->code : -1,
           error ? error->message : "none");
    if (connection)
        g_object_unref(connection);
    g_clear_error(&error);
    g_object_unref(client);
    g_object_unref(address);
    printf("DESKTOP_RUNTIME_%s failures=%d\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}
