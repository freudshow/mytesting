/*
 ============================================================================
 Name        : test.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "list.h"
#include "basedef.h"
#include "oled.h"
#include "tcp.h"
#include "sqlite3/sqlite3.h"

int getBlob(sqlite3 *db, int unicode)
{
	char sql[128];
	sqlite3_stmt *stmt;

	snprintf(sql, 127, "select font from t_unicode_gbk where unicode=%d;",
			unicode);

	//读取数据
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	int result = sqlite3_step(stmt);
	int len = 0;
	while (result == SQLITE_ROW)
	{
		const char *pReadBolbData = sqlite3_column_blob(stmt, 0);
		len = sqlite3_column_bytes(stmt, 0);

		DEBUG_BUFF_FORMAT(pReadBolbData, len, "");
		result = sqlite3_step(stmt);
	}

	sqlite3_finalize(stmt);

	return 0;
}

int main(int argc, char **argv)
{
	char GetAppName[200]={'\0' } ;
	char topic[]="EndTerminal/set/request/ccoRouter/acqFiles";
	char *savedptr;
	char *tmp = NULL;

	for (tmp= strtok_r(topic,"/set/request", &savedptr); tmp != NULL; tmp = strtok_r(NULL,"/set/request", &savedptr))
	{
		strcpy(GetAppName,tmp);
	    printf("%s\n", GetAppName);
	}

	return 0;
}
