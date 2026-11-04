/* SPDX-License-Identifier: BSD-3-Clause */
#include <dbus/dbus.h>
#include <gio/gio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef LUME_TEST_XFCONF
#include <xfconf/xfconf.h>
#endif

static const char configuration[] =
    "<busconfig><type>session</type><listen>unix:path=/tmp/dbus-check</listen>"
    "<auth>EXTERNAL</auth><policy context=\"default\">"
    "<allow send_destination=\"*\"/><allow receive_sender=\"*\"/><allow own=\"*\"/>"
    "</policy></busconfig>";

static void directory(const char *path)
{
    assert(!mkdir(path, 0700) || errno == EEXIST);
}

static pid_t spawn(const char *name, const char *argument)
{
    pid_t child = fork();
    assert(child >= 0);
    if (!child)
    {
        execl(name, name, argument, (char *)NULL);
        _exit(127);
    }
    return child;
}

static void check_error(DBusError *error)
{
    if (dbus_error_is_set(error))
        fprintf(stderr, "%s: %s\n", error->name, error->message);
    assert(!dbus_error_is_set(error));
}

#ifdef LUME_TEST_XFCONF
static void settings(DBusConnection *connection, pid_t daemon)
{
    DBusError error = DBUS_ERROR_INIT;
    unsigned attempt;
    for (attempt = 0; attempt < 100; ++attempt)
    {
        if (dbus_bus_name_has_owner(connection, "org.xfce.Xfconf", &error))
            break;
        check_error(&error);
        usleep(20000);
    }
    assert(attempt < 100);
    GError *failure = NULL;
    assert(xfconf_init(&failure));
    g_assert_no_error(failure);
    XfconfChannel *channel = xfconf_channel_new("native-check");
    assert(channel);
    assert(xfconf_channel_set_string(channel, "/text", "ação, café e informação"));
    assert(xfconf_channel_set_int(channel, "/size", 128));
    char *value = xfconf_channel_get_string(channel, "/text", NULL);
    assert(value && !strcmp(value, "ação, café e informação"));
    g_free(value);
    assert(xfconf_channel_get_int(channel, "/size", 0) == 128);
    g_object_unref(channel);
    xfconf_shutdown();
    assert(!kill(daemon, SIGTERM));
    int status;
    assert(waitpid(daemon, &status, 0) == daemon && WIFEXITED(status) && !WEXITSTATUS(status));
    const char *path = "/data/xfconf-check/xfce4/xfconf/xfce-perchannel-xml/native-check.xml";
    char *contents;
    gsize length;
    assert(g_file_get_contents(path, &contents, &length, &failure));
    g_assert_no_error(failure);
    assert(length && strstr(contents, "ação, café e informação"));
    g_free(contents);
    puts("XFCONF_DBUS_PERSISTENCE_PASS");
    assert(!unlink(path));
}
#endif

static DBusConnection *connect_bus(void)
{
    DBusError error = DBUS_ERROR_INIT;
    DBusConnection *connection = NULL;
    for (unsigned attempt = 0; attempt < 100 && !connection; ++attempt)
    {
        connection = dbus_connection_open_private("unix:path=/tmp/dbus-check", &error);
        if (!connection)
        {
            dbus_error_free(&error);
            usleep(20000);
        }
    }
    check_error(&error);
    assert(connection);
    dbus_connection_set_exit_on_disconnect(connection, FALSE);
    assert(dbus_bus_register(connection, &error));
    check_error(&error);
    assert(dbus_connection_get_is_authenticated(connection));
    const char *name = dbus_bus_get_unique_name(connection);
    assert(name && name[0] == ':');
    return connection;
}

int main(void)
{
    assert(dbus_threads_init_default());
    directory("/data/var");
    directory("/data/var/lib");
    directory("/data/var/lib/dbus");
    pid_t generator = spawn("/usr/libexec/dbus-uuidgen", "--ensure");
    int status;
    assert(waitpid(generator, &status, 0) == generator && WIFEXITED(status) &&
           !WEXITSTATUS(status));
    puts("DBUS_NATIVE_MACHINE_ID_PASS");
    FILE *config = fopen("/data/dbus-check.conf", "w");
    assert(config);
    assert(fwrite(configuration, 1, sizeof(configuration) - 1, config) ==
           sizeof(configuration) - 1);
    assert(!fclose(config));
    pid_t daemon = spawn("/usr/libexec/dbus-daemon", "--config-file=/data/dbus-check.conf");
#ifdef LUME_TEST_XFCONF
    assert(!setenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/tmp/dbus-check", 1));
    assert(!setenv("XDG_CONFIG_HOME", "/data/xfconf-check", 1));
    pid_t xfconf = spawn("/usr/libexec/xfconfd", NULL);
#endif
    DBusConnection *first = connect_bus();
    DBusError error = DBUS_ERROR_INIT;
    char *id = dbus_bus_get_id(first, &error);
    check_error(&error);
    assert(id && strlen(id) == 32);
    dbus_free(id);
    const char *owned = "org.lume.NativeCheck";
    assert(dbus_bus_request_name(first, owned, DBUS_NAME_FLAG_DO_NOT_QUEUE, &error) ==
           DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);
    check_error(&error);
    assert(dbus_bus_name_has_owner(first, owned, &error));
    check_error(&error);
    unsigned long uid = dbus_bus_get_unix_user(first, dbus_bus_get_unique_name(first), &error);
    check_error(&error);
    assert(uid == (unsigned long)geteuid());
    puts("DBUS_EXTERNAL_AUTH_PASS");
    GError *gio_error = NULL;
    GDBusConnection *gio =
        g_dbus_connection_new_for_address_sync("unix:path=/tmp/dbus-check",
                                               G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
                                                   G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
                                               NULL, NULL, &gio_error);
    if (gio_error)
        fprintf(stderr, "GDBus: %s\n", gio_error->message);
    g_assert_no_error(gio_error);
    assert(gio && g_dbus_connection_get_unique_name(gio));
    GVariant *reply = g_dbus_connection_call_sync(
        gio, "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "GetId", NULL,
        G_VARIANT_TYPE("(s)"), G_DBUS_CALL_FLAGS_NONE, 3000, NULL, &gio_error);
    g_assert_no_error(gio_error);
    assert(reply);
    const char *gio_id;
    g_variant_get(reply, "(&s)", &gio_id);
    assert(strlen(gio_id) == 32);
    g_variant_unref(reply);
    assert(g_dbus_connection_close_sync(gio, NULL, &gio_error));
    g_assert_no_error(gio_error);
    g_object_unref(gio);
    puts("DBUS_GIO_CLIENT_PASS");
    DBusConnection *second = connect_bus();
    assert(strcmp(dbus_bus_get_unique_name(first), dbus_bus_get_unique_name(second)));
    assert(dbus_bus_request_name(second, owned, DBUS_NAME_FLAG_DO_NOT_QUEUE, &error) ==
           DBUS_REQUEST_NAME_REPLY_EXISTS);
    check_error(&error);
    assert(dbus_bus_release_name(first, owned, &error) == DBUS_RELEASE_NAME_REPLY_RELEASED);
    check_error(&error);
    assert(dbus_bus_request_name(second, owned, DBUS_NAME_FLAG_DO_NOT_QUEUE, &error) ==
           DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);
    check_error(&error);
    dbus_connection_close(first);
#ifdef LUME_TEST_XFCONF
    settings(second, xfconf);
#endif
    dbus_connection_unref(first);
    dbus_connection_close(second);
    dbus_connection_unref(second);
    dbus_shutdown();
    puts("DBUS_NAME_OWNERSHIP_PASS");
    assert(!kill(daemon, SIGTERM));
    assert(waitpid(daemon, &status, 0) == daemon);
    assert((WIFEXITED(status) && !WEXITSTATUS(status)) ||
           (WIFSIGNALED(status) && WTERMSIG(status) == SIGTERM));
    assert(!unlink("/data/dbus-check.conf"));
    puts("DBUS_NATIVE_PASS");
    return 0;
}
