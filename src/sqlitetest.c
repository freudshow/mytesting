#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "sqlite3.h"
#include "basedef.h"

#define SQLITE_IN_MEMORY_MODE   ":memory:"
#define CURVE_DB_NAME   "curve.db"
#define ATTACH_DB_NAME  "curve"
#define DEFAULT_MEMORY_NAME "main"
#define CURVE_TABLE_NAME    "realdata"

typedef unsigned char boolean; /* bool value, 0-false, 1-true       */
typedef unsigned char u8; /* Unsigned  8 bit quantity          */
typedef char s8; /* Signed    8 bit quantity          */
typedef unsigned short u16; /* Unsigned 16 bit quantity          */
typedef signed short s16; /* Signed   16 bit quantity          */
typedef unsigned int u32; /* Unsigned 32 bit quantity          */
typedef signed int s32; /* Signed   32 bit quantity          */
typedef unsigned long long u64; /* Unsigned 64 bit quantity        */
typedef signed long long s64; /* Unsigned 64 bit quantity          */
typedef float fp32; /* Single precision floating point   */
typedef double fp64; /* Double precision floating point   */

void get_time_stamp(char *buf, u32 bufLen)
{
    struct timeval systime;
    struct tm timeinfo;

    gettimeofday(&systime, NULL);
    localtime_r(&systime.tv_sec, &timeinfo);
    snprintf(buf, bufLen, "%04d-%02d-%02d %02d:%02d:%02d.%03ld", (timeinfo.tm_year + 1900), timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, systime.tv_usec / 1000);
}

int sqlite_db_create(char *dbname, sqlite3 **db)
{
    if (sqlite3_open(dbname, db))
    {
        DEBUG_TIME_LINE("Can't open database_file\n");
        return -1;
    }

    DEBUG_TIME_LINE("Opened database_file successfully\n");

    return 0;
}

/**
 * ---------------------------------------------
 * The max_page_count setting is a runtime
 * option that doesn't persist across
 * connections to a database
 * , so once the dbhandler is closed,
 * the parameter is set to default.
 * ---------------------------------------------
 * @param db
 * @param alias
 * ---------------------------------------------
 * @return
 */
int limitDbSize(sqlite3 *db, char *alias)
{
    char *errmsg;
    char sql[128] = { 0 };
    /* 限值数据库大小5M，默认每页4096，最大页数20*/

    snprintf(sql, sizeof(sql) - 1, "PRAGMA %s.max_page_count = 50;", alias);
    if (sqlite3_exec(db, sql, NULL, NULL, &errmsg))
    {
        DEBUG_TIME_LINE("limit file size error\n");
        return -1;
    }
    else
    {
        DEBUG_TIME_LINE("limit file size success\n");
    }

    return 0;
}

int sqlite_table_create(char *tablename, sqlite3 *dbhandle, char *alias)
{
    char sql[328];
    char *errmsg;

    memset(sql, '\0', sizeof(sql));
    snprintf(sql, sizeof(sql) - 1, "CREATE TABLE if not exists %s.%s (\
                            RtdbNo INT,\
                            Name   STRING,\
                            Value  FLOAT,\
                            Time   TEXT\
                        );", alias, tablename);

    if (sqlite3_exec(dbhandle, sql, NULL, NULL, &errmsg))
    {
        DEBUG_TIME_LINE("create table error\n");
    }
    else
    {
        DEBUG_TIME_LINE("create table success\n");
    }

    return 0;
}

/*------------------------------------------------------------------------
 函数：sqlite_table_AttachDb
 目的：将文件数据库作为内存数据库的附加数据库
 输入参数：
 输出参数：无
 返回值： 0成功，-1失败
 ------------------------------------------------------------------------*/
int sqlite_table_AttachDb(sqlite3 *db, char *filename, char *attachname)
{
    int rc = 0;
    char *errMsg = NULL;
    char sqlcmd[512] = { 0 };
    snprintf(sqlcmd, sizeof(sqlcmd), "ATTACH '%s' AS %s", filename, attachname);
    rc = sqlite3_exec(db, sqlcmd, NULL, NULL, &errMsg);
    if (SQLITE_OK != rc)
    {
        DEBUG_TIME_LINE("ERR: can't attach database\r\n");
        sqlite3_close(db);
        return -1;
    }

    return 0;
}

/*------------------------------------------------------------------------
 函数：sqlite_table_read_from_file
 目的：数据中心启动时，将文件数据库中的内容复制到内存数据库
 输入参数：无
 输出参数：无
 返回值： 0成功，-1失败
 ------------------------------------------------------------------------*/
int sqlite_table_load_data_from_file(sqlite3 *db, char *memoryAlias, char *filedbAlias)
{
    int i = 0;
    int rc = 0;
    char *errMsg = NULL;
    char sqlcmd[512] = { 0 };
    char sql[328] = { 0 };
    int ret = 0;
    char *s;
    int nrow, ncolumn;
    char **db_result;

    snprintf(sql, sizeof(sql) - 1, "SELECT name FROM %s.sqlite_master WHERE type='table'", filedbAlias);
    ret = sqlite3_get_table(db, sql, &db_result, &nrow, &ncolumn, &s);
    if (ret)
    {
        DEBUG_TIME_LINE("select error\n");
        return -1;
    }

    DEBUG_TIME_LINE("table count : %d\r\n", nrow);
    for (i = 0; i < nrow; i++)
    {
        snprintf(sqlcmd, sizeof(sqlcmd), "create table %s.%s as select * from %s.%s", memoryAlias, db_result[1 + i], filedbAlias, db_result[1 + i]);
        DEBUG_TIME_LINE("read table from file :%s\r\n", db_result[1 + i]);
        rc = sqlite3_exec(db, sqlcmd, NULL, NULL, &errMsg);
        if (SQLITE_OK != rc)
        {
            DEBUG_TIME_LINE("create table<%s> error: %s", db_result[1 + i], errMsg);
            sqlite3_close(db);
            return -1;
        }
    }

    return 0;
}

/*------------------------------------------------------------------------
 函数：sqlite_table_Flush
 目的：定时调用此函数将内存数据中的内容同步到文件数据库
 输入参数：无
 输出参数：无
 返回值： 0成功，-1失败
 ------------------------------------------------------------------------*/
int sqlite_table_Flush(sqlite3 *db, char *memoryAlias, char *filedbAlias)
{
    int i = 0;
    int rc = 0;
    char *errMsg = NULL;
    char sqlcmd[512] = { 0 };
    int ret = 0;
    int nrow, ncolumn;
    char **db_result;

    snprintf(sqlcmd, sizeof(sqlcmd) - 1, "SELECT name FROM %s.sqlite_master WHERE type='table'", memoryAlias);
    ret = sqlite3_get_table(db, sqlcmd, &db_result, &nrow, &ncolumn, &errMsg);
    if (ret)
    {
        DEBUG_TIME_LINE("select error: %s\n", errMsg);
        return -1;
    }

    DEBUG_TIME_LINE("table count : %d\r\n", nrow);

    for (i = 0; i < nrow; i++)
    {
        snprintf(sqlcmd, sizeof(sqlcmd), "INSERT OR REPLACE INTO %s.%s SELECT * FROM %s.%s;", filedbAlias, db_result[1 + i], memoryAlias, db_result[1 + i]);
        DEBUG_TIME_LINE("flush table:%s\r\n", db_result[1 + i]);
        rc = sqlite3_exec(db, sqlcmd, NULL, NULL, &errMsg);
        if (SQLITE_OK != rc)
        {
            sqlite3_close(db);
            return -1;
        }
    }

    sqlite3_free_table(&db_result);

    return 0;
}

int delData(sqlite3 *db)
{
    char sql[128] = { 0 };
    char *errMsg;

    snprintf(sql, sizeof(sql) - 1, "delete from realdata where rowid < (select max(rowid)/2  from realdata);vacuum;");
    int res = sqlite3_exec(db, sql, NULL, NULL, &errMsg);
    if (SQLITE_OK != res)
    {
        DEBUG_TIME_LINE("res: %d, error: %s", res, errMsg);
        return -1;
    }
    else
    {
        DEBUG_TIME_LINE("delete successfully");
    }

    return 0;
}

int insertData(sqlite3 *db, char *alias)
{
    char sql[128] = { 0 };
    char time[64] = { 0 };
    char *errMsg;

    get_time_stamp(time, sizeof(time));
    snprintf(sql, sizeof(sql) - 1, "insert into %s.realdata values (1, 'A_voltage', 220.1, '%s')", alias, time);
    int res = sqlite3_exec(db, sql, NULL, NULL, &errMsg);
    if (SQLITE_OK != res)
    {
        DEBUG_TIME_LINE("res: %d, error: %s", res, errMsg);
        if (res == SQLITE_FULL)
        {
            delData(db);
        }

        return -1;
    }

    return 0;
}

int loadOrSaveDb(sqlite3 *pInMemory, const char *zFilename, int isSave)
{
    int rc = -1; /* Function return code */
    sqlite3 *pFile; /* Database connection opened on zFilename */
    sqlite3_backup *pBackup; /* Backup object used to copy data */
    sqlite3 *pTo; /* Database to copy to (pFile or pInMemory) */
    sqlite3 *pFrom; /* Database to copy from (pFile or pInMemory) */

    /* Open the database file identified by zFilename. Exit early if this fails
     ** for any reason. */
    rc = sqlite3_open(zFilename, &pFile);
    if (rc == SQLITE_OK)
    {

        /* If this is a 'load' operation (isSave==0), then data is copied
         ** from the database file just opened to database pInMemory.
         ** Otherwise, if this is a 'save' operation (isSave==1), then data
         ** is copied from pInMemory to pFile.  Set the variables pFrom and
         ** pTo accordingly. */
        pFrom = (isSave ? pInMemory : pFile);
        pTo = (isSave ? pFile : pInMemory);

        /* Set up the backup procedure to copy from the "main" database of
         ** connection pFile to the main database of connection pInMemory.
         ** If something goes wrong, pBackup will be set to NULL and an error
         ** code and message left in connection pTo.
         **
         ** If the backup object is successfully created, call backup_step()
         ** to copy data from pFile to pInMemory. Then call backup_finish()
         ** to release resources associated with the pBackup object.  If an
         ** error occurred, then an error code and message will be left in
         ** connection pTo. If no error occurred, then the error code belonging
         ** to pTo is set to SQLITE_OK.
         */
        pBackup = sqlite3_backup_init(pTo, "main", pFrom, "main");
        if (pBackup)
        {
            (void) sqlite3_backup_step(pBackup, -1);
            (void) sqlite3_backup_finish(pBackup);
        }

        rc = sqlite3_errcode(pTo);
        DEBUG_TIME_LINE("rc: %d", rc);
    }

    /* Close the database connection opened on database file zFilename
     ** and return the result of this function. */
    (void) sqlite3_close(pFile);
    return rc;
}

/*
 ** Perform an online backup of database pDb to the database file named
 ** by zFilename. This function copies 5 database pages from pDb to
 ** zFilename, then unlocks pDb and sleeps for 250 ms, then repeats the
 ** process until the entire database is backed up.
 **
 ** The third argument passed to this function must be a pointer to a progress
 ** function. After each set of 5 pages is backed up, the progress function
 ** is invoked with two integer parameters: the number of pages left to
 ** copy, and the total number of pages in the source file. This information
 ** may be used, for example, to update a GUI progress bar.
 **
 ** While this function is running, another thread may use the database pDb, or
 ** another process may access the underlying database file via a separate
 ** connection.
 **
 ** If the backup process is successfully completed, SQLITE_OK is returned.
 ** Otherwise, if an error occurs, an SQLite error code is returned.
 */
int backupDb(sqlite3 *pDb, /* Database to back up */
const char *zFilename, /* Name of file to back up to */
void (*xProgress)(int, int) /* Progress function to invoke */
)
{
    int rc; /* Function return code */
    sqlite3 *pFile; /* Database connection opened on zFilename */
    sqlite3_backup *pBackup; /* Backup handle used to copy data */

    /* Open the database file identified by zFilename. */
    rc = sqlite3_open(zFilename, &pFile);
    if (rc == SQLITE_OK)
    {

        /* Open the sqlite3_backup object used to accomplish the transfer */
        pBackup = sqlite3_backup_init(pFile, "main", pDb, "main");
        if (pBackup)
        {

            /* Each iteration of this loop copies 5 database pages from database
             ** pDb to the backup database. If the return value of backup_step()
             ** indicates that there are still further pages to copy, sleep for
             ** 250 ms before repeating. */
            do
            {
                rc = sqlite3_backup_step(pBackup, 5);
                xProgress(sqlite3_backup_remaining(pBackup), sqlite3_backup_pagecount(pBackup));
                if (rc == SQLITE_OK || rc == SQLITE_BUSY || rc == SQLITE_LOCKED)
                {
                    sqlite3_sleep(250);
                }
            } while (rc == SQLITE_OK || rc == SQLITE_BUSY || rc == SQLITE_LOCKED);

            /* Release resources allocated by backup_init(). */
            (void) sqlite3_backup_finish(pBackup);
        }
        rc = sqlite3_errcode(pFile);
    }

    /* Close the database connection opened on database file zFilename
     ** and return the result of this function. */
    (void) sqlite3_close(pFile);
    return rc;
}

int fileExist(char *filename)
{
    struct stat st;

    if (stat(filename, &st) == 0)
    {
        return 1;
    }

    return 0;
}

int sql3main(int argc, char *argv[])
{
    sqlite3 *memdb;

    /* 创建实时数据库（内存库和文件库） */
    if (sqlite_db_create(SQLITE_IN_MEMORY_MODE, &memdb) < 0)
    {
        return -1;
    }

    /* 从文件实时库中表复制到内存库中 */
    if (fileExist(CURVE_DB_NAME))
    {
        loadOrSaveDb(memdb, CURVE_DB_NAME, 0);
    }

    limitDbSize(memdb, DEFAULT_MEMORY_NAME);

    if (sqlite_table_create(CURVE_TABLE_NAME, memdb, DEFAULT_MEMORY_NAME) < 0)
    {
        sqlite3_close(memdb);
        return -1;
    }

    u32 rows = 0;
    int iTime = 0;
    while (1)
    {
        insertData(memdb, DEFAULT_MEMORY_NAME);
        usleep(10000);
        iTime++;
        rows++;

        /* 定时将内存实时库数据复制到文件实时库中 */
        if (iTime >= 600)
        {
            loadOrSaveDb(memdb, CURVE_DB_NAME, 1);
            iTime = 0;
        }
    }

    return 0;
}
