#include <sqlite3.h>
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>

int gettimeofday(struct timeval *value, void *zone)
{
    (void)value;
    (void)zone;
    errno = ENOSYS;
    return -1;
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return 1;
    sqlite3 *first = NULL, *second = NULL;
    if (sqlite3_open(argv[1], &first) != SQLITE_OK)
        return 1;
    if (sqlite3_exec(
            first, "CREATE TABLE item(value INTEGER); BEGIN IMMEDIATE; INSERT INTO item VALUES(41)",
            NULL, NULL, NULL) != SQLITE_OK)
        return 1;
    if (sqlite3_open(argv[1], &second) != SQLITE_OK)
        return 1;
    if (sqlite3_exec(second, "INSERT INTO item VALUES(99)", NULL, NULL, NULL) != SQLITE_BUSY)
        return 1;
    if (sqlite3_exec(first, "COMMIT", NULL, NULL, NULL) != SQLITE_OK)
        return 1;
    if (sqlite3_exec(second, "INSERT INTO item VALUES(1)", NULL, NULL, NULL) != SQLITE_OK)
        return 1;
    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(second, "SELECT sum(value), datetime('now') FROM item", -1, &statement,
                           NULL) != SQLITE_OK)
        return 1;
    if (sqlite3_step(statement) != SQLITE_ROW || sqlite3_column_int(statement, 0) != 42 ||
        sqlite3_column_type(statement, 1) != SQLITE_NULL)
        return 1;
    sqlite3_finalize(statement);
    if (sqlite3_close(first) != SQLITE_OK || sqlite3_close(second) != SQLITE_OK)
        return 1;
    puts("PASS transactions, directory locking and unavailable clock");
    return 0;
}
