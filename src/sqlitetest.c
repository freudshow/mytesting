#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "sqlite3.h"
#include "basedef.h"

#define CURVE_DB_NAME   "curve.db"
#define ATTACH_DB_NAME  "curve"

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
    snprintf(buf, bufLen, "%04d-%02d-%02d %02d:%02d:%02d %03ld", (timeinfo.tm_year + 1900),
            timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour,
            timeinfo.tm_min, timeinfo.tm_sec, systime.tv_usec/1000);
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

int limitDbSize(sqlite3 *db)
{
    char *errmsg;
    /* 限值数据库大小5M，默认每页4096，最大页数20*/

    if (sqlite3_exec(db, "PRAGMA page_size = 4096;", NULL, NULL, &errmsg))
    {
        DEBUG_TIME_LINE("limit file size error\n");
        return -1;
    }
    else
    {
        DEBUG_TIME_LINE("limit file size success\n");
    }


    if (sqlite3_exec(db, "PRAGMA max_page_count = 1500;", NULL, NULL, &errmsg))
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

int sqlite_table_create(char *tablename, sqlite3 *dbhandle)
{
    char sql[328];
    char *errmsg;

    memset(sql, '\0', sizeof(sql));
    snprintf(sql, sizeof(sql) - 1, "CREATE TABLE if not exists realdata (\
                            RtdbNo INT,\
                            Name   STRING,\
                            Value  FLOAT,\
                            Time   TEXT\
                        );");

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
int sqlite_table_load_data_from_file(sqlite3 *db_memory, sqlite3 *db_file, char *filedbAttechedName)
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

    snprintf(sql, sizeof(sql) - 1, "SELECT name FROM sqlite_master WHERE type='table'");
    ret = sqlite3_get_table(db_file, sql, &db_result, &nrow, &ncolumn, &s);
    if (ret)
    {
        DEBUG_TIME_LINE("select error\n");
        return -1;
    }

    DEBUG_TIME_LINE("table count : %d\r\n", nrow);
    for (i = 0; i < nrow; i++)
    {
        snprintf(sqlcmd, sizeof(sqlcmd), "create table %s as select * from %s.%s", db_result[1 + i], filedbAttechedName, db_result[1 + i]);
        DEBUG_TIME_LINE("read table from file :%s\r\n", db_result[1 + i]);
        rc = sqlite3_exec(db_memory, sqlcmd, NULL, NULL, &errMsg);
        if (SQLITE_OK != rc)
        {
            DEBUG_TIME_LINE("create table<%s> error: %s", db_result[1 + i], errMsg);
            sqlite3_close(db_memory);
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
int sqlite_table_Flush(sqlite3 *db, char *filedbAttechedName)
{
    int i = 0;
    int rc = 0;
    char *errMsg = NULL;
    char sqlcmd[512] = { 0 };
    int ret = 0;
    int nrow, ncolumn;
    char **db_result;

    snprintf(sqlcmd, sizeof(sqlcmd) - 1, "SELECT name FROM sqlite_master WHERE type='table'");
    ret = sqlite3_get_table(db, sqlcmd, &db_result, &nrow, &ncolumn, &errMsg);
    if (ret)
    {
        DEBUG_TIME_LINE("select error: %s\n", errMsg);
        return -1;
    }

    DEBUG_TIME_LINE("table count : %d\r\n", nrow);

    for (i = 0; i < nrow; i++)
    {
        snprintf(sqlcmd, sizeof(sqlcmd), "INSERT OR REPLACE INTO %s.%s SELECT * FROM %s;", filedbAttechedName, db_result[1 + i], db_result[1 + i]);
        DEBUG_TIME_LINE("flush table:%s\r\n", db_result[1 + i]);
        rc = sqlite3_exec(db, sqlcmd, NULL, NULL, &errMsg);
        if (SQLITE_OK != rc)
        {
            sqlite3_close(db);
            return -1;
        }
    }

    return 0;
}

int insertData(sqlite3 *db)
{
    char sql[128] = { 0 };
    char time[64] = { 0 };
    char *errMsg;

    get_time_stamp(time, sizeof(time));
    snprintf(sql, sizeof(sql) - 1, "insert into realdata values (1, 'A_voltage', 220.1, '%s')", time);
    if (SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, &errMsg))
    {
        sqlite3_close(db);
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    sqlite3 *filedb; //db in hard-drive
    sqlite3 *memdb; //db in memory

    int iTime = 0;

    /* 创建实时数据库（内存库和文件库） */
    if (sqlite_db_create(":memory:", &memdb) < 0)
    {
        return -1;
    }

    if (sqlite_db_create(CURVE_DB_NAME, &filedb) < 0)
    {
        sqlite3_close(memdb);
        return -1;
    }

    if (sqlite_table_create(CURVE_DB_NAME, filedb) < 0)
    {
        sqlite3_close(memdb);
        sqlite3_close(filedb);
        return -1;
    }

    limitDbSize(filedb);

    /* 将内存实时库和文件实时库绑定 */
    if (sqlite_table_AttachDb(memdb, CURVE_DB_NAME, ATTACH_DB_NAME) < 0)
    {
        sqlite3_close(memdb);
        sqlite3_close(filedb);
        return -1;
    }

    /* 从文件实时库中表复制到内存库中 */
    sqlite_table_load_data_from_file(memdb, filedb, ATTACH_DB_NAME);

    while (1)
    {
        insertData(memdb);
        usleep(10000);
        iTime++;

        /* 定时将内存实时库数据复制到文件实时库中 */
        if (iTime >= 600)
        {
            sqlite_table_Flush(memdb, ATTACH_DB_NAME);
            iTime = 0;
        }
    }

    return 0;
}
