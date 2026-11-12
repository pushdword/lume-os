/* SPDX-License-Identifier: BSD-3-Clause */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "third_party/sqlite/sqlite3.h"
#include "lume/abi.h"
#include "lume/sandbox.h"
#include "lume/syscall_native.h"

static void database_log(void *context, int code, const char *message)
{
    (void)context;
    printf("SQLITE_LOG code=%d errno=%d message=%s\n", code, errno, message);
    fflush(stdout);
}

static int query(const char *label, const char *path, int wal)
{
    sqlite3 *database = NULL;
    errno = 0;
    int result = sqlite3_open_v2(
        path, &database, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, NULL);
    printf("SQLITE_OPEN label=%s wal=%d result=%d extended=%d errno=%d message=%s\n", label, wal,
           result, database ? sqlite3_extended_errcode(database) : result, errno,
           database ? sqlite3_errmsg(database) : "No database handle");
    fflush(stdout);
    if (result == SQLITE_OK)
    {
        char *error = NULL;
        const char *sql = wal ? "PRAGMA journal_mode=WAL; CREATE TABLE IF NOT EXISTS item(value);"
                              : "CREATE TABLE IF NOT EXISTS item(value);";
        result = sqlite3_exec(database, sql, NULL, NULL, &error);
        printf("SQLITE_SCHEMA label=%s wal=%d result=%d extended=%d errno=%d message=%s\n", label,
               wal, result, sqlite3_extended_errcode(database), errno,
               error ? error : sqlite3_errmsg(database));
        sqlite3_free(error);
        if (result == SQLITE_OK)
        {
            result = sqlite3_exec(database, "INSERT INTO item VALUES(37);", NULL, NULL, &error);
            printf("SQLITE_WRITE label=%s wal=%d result=%d extended=%d errno=%d message=%s\n",
                   label, wal, result, sqlite3_extended_errcode(database), errno,
                   error ? error : sqlite3_errmsg(database));
            sqlite3_free(error);
        }
    }
    if (database)
    {
        int closed = sqlite3_close(database);
        if (result == SQLITE_OK)
            result = closed;
    }
    fflush(stdout);
    return result;
}

int main(void)
{
    char directory[128], path[160];
    snprintf(directory, sizeof(directory), "/data/tmp/sqlite-%ld", (long)getpid());
    if (mkdir("/data/tmp", 0700) && errno != EEXIST)
        return 1;
    if (mkdir(directory, 0700))
        return 1;
    puts("SQLITE_DIAGNOSTIC_START");
    fflush(stdout);
    sqlite3_config(SQLITE_CONFIG_LOG, database_log, NULL);
    int initialized = sqlite3_initialize();
    printf("SQLITE_INITIALIZE result=%d\n", initialized);
    fflush(stdout);
    if (initialized != SQLITE_OK)
    {
        puts("FAIL_SQLITE_DIAGNOSTIC_INITIALIZE");
        return 1;
    }
    snprintf(path, sizeof(path), "%s/plain.db", directory);
    int unrestricted = query("unrestricted", path, 0);
    snprintf(path, sizeof(path), "%s/wal.db", directory);
    int unrestricted_wal = query("unrestricted", path, 1);
    int descriptor = open(directory, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (descriptor < 0)
        return 1;
    int64_t minted = lume_native_call(LUME_SYS_SANDBOX, LUME_SANDBOX_DIRECTORY_MINT, descriptor, 0);
    int64_t entered = minted ? minted
                             : lume_native_call(LUME_SYS_SANDBOX, LUME_SANDBOX_ENTER,
                                                LUME_SANDBOX_NETWORK, LUME_SANDBOX_VERSION);
    int64_t activated = entered ? entered
                                : lume_native_call(LUME_SYS_SANDBOX,
                                                   LUME_SANDBOX_DIRECTORY_ACTIVATE, descriptor, 0);
    printf("SQLITE_AUTH mint=%lld enter=%lld activate=%lld\n", (long long)minted,
           (long long)entered, (long long)activated);
    if (activated)
        return 1;
    snprintf(path, sizeof(path), "%s/network.db", directory);
    int network = query("network", path, 0);
    snprintf(path, sizeof(path), "%s/network-wal.db", directory);
    int network_wal = query("network", path, 1);
    close(descriptor);
    printf("SQLITE_STORAGE_RESULT unrestricted=%d unrestricted_wal=%d network=%d network_wal=%d\n",
           unrestricted, unrestricted_wal, network, network_wal);
    fflush(stdout);
    puts("PASS_SQLITE_DIAGNOSTIC_COMPLETE");
    return unrestricted || unrestricted_wal || network || network_wal;
}
