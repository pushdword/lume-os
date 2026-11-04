/* SPDX-License-Identifier: BSD-3-Clause */
#include <glib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static GMutex lock;
static GCond condition;
static gboolean finished;

static gpointer worker(gpointer argument)
{
    g_mutex_lock(&lock);
    while (!finished)
        g_cond_wait(&condition, &lock);
    g_mutex_unlock(&lock);
    return argument;
}

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        char cwd[128];
        assert(getcwd(cwd, sizeof(cwd)) && !strcmp(cwd, "/data/gspawn-native"));
        if (!strcmp(argv[1], "sync"))
        {
            g_print("ação\n");
            g_printerr("ERR\n");
            return 17;
        }
        FILE *file = fopen("async.txt", "w");
        assert(file && fputs("GSPAWN_ASYNC_CHILD_PASS\n", file) >= 0 && !fclose(file));
        return 0;
    }
    assert(!mkdir("/data/gspawn-native", 0700));
    g_setenv("PATH", "/usr/libexec:/usr/bin:/bin", TRUE);
    GThread *thread = g_thread_new("spawn-worker", worker, &finished);
    gchar *arguments[] = {argv[0], "sync", NULL};
    gchar *output = NULL, *errors = NULL;
    gint status;
    GError *error = NULL;
    assert(g_spawn_sync("/data/gspawn-native", arguments, NULL, G_SPAWN_DEFAULT, NULL, NULL,
                        &output, &errors, &status, &error));
    assert(!error && WIFEXITED(status) && WEXITSTATUS(status) == 17);
    assert(!strcmp(output, "ação\n") && !strcmp(errors, "ERR\n"));
    g_free(output);
    g_free(errors);
    arguments[0] = strrchr(argv[0], '/') + 1;
    arguments[1] = "async";
    GPid pid;
    assert(g_spawn_async("/data/gspawn-native", arguments, NULL,
                         G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH, NULL, NULL, &pid,
                         &error));
    assert(!error && waitpid(pid, &status, 0) == pid && WIFEXITED(status) && !WEXITSTATUS(status));
    g_spawn_close_pid(pid);
    FILE *file = fopen("/data/gspawn-native/async.txt", "r");
    char text[64];
    assert(file && fgets(text, sizeof(text), file) && !strcmp(text, "GSPAWN_ASYNC_CHILD_PASS\n"));
    assert(!fclose(file) && !remove("/data/gspawn-native/async.txt"));
    assert(!g_spawn_async("/data/gspawn-native/absent", arguments, NULL,
                          G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH, NULL, NULL, &pid,
                          &error));
    assert(error && error->domain == G_SPAWN_ERROR);
    g_clear_error(&error);
    g_mutex_lock(&lock);
    finished = TRUE;
    g_cond_signal(&condition);
    g_mutex_unlock(&lock);
    assert(g_thread_join(thread) == &finished);
    assert(!rmdir("/data/gspawn-native"));
    puts("GSPAWN_NATIVE_SYNC_ASYNC_CWD_PIPES_WORKERS_PASS");
    return 0;
}
