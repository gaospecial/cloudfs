

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/time.h>
#include "sqlite3.h"
#include "log.h"
#include "cloudfs_sqlite.h"
#include "AliConf.h"

using namespace std;

class SqlHandle
{
	public:
		SqlHandle(){m_db=0;}
		~SqlHandle(){sqlite3_close(m_db);}
		void Init(sqlite3 *db){m_db = db;}
		sqlite3 * get_db(){return m_db;}
	private:	
		sqlite3 *m_db;
};

SqlHandle gDbHanle;



int cloudfs_sqlite_create_file_meta_table(void *pDbHandle);




int cloudfs_sqlite_create_table(void *pDbHandle, string &pStatement)
{
	sqlite3 *db = (sqlite3 *)pDbHandle;
	char *zErrMsg = NULL;
	if (NULL == db)
	{
		log_error("Invalid input, 0x%x 0x%x\n", pDbHandle);
		return -1;
	}

	log_debug("sql_statement [%s]", pStatement.c_str());
	int rc = sqlite3_exec(db, pStatement.c_str(), NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK )
	{
		log_error("SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
		return -1;
	}

	return 0;
}



/*
file meta table

file_path varchar(1024)    a_time INTEGER	m_time INTEGER	c_time INTEGER	mode INTEGER		

create table t_fs_file_meta (file_path TEXT, a_time INTEGER, m_time INTEGER, c_time INTEGER, mode INTEGER)	

*/
int cloudfs_sqlite_create_file_meta_table(void *pDbHandle)
{
	string file_meta_table = "create table t_fs_file_meta (file_path TEXT PRIMARY KEY, a_time INTEGER, m_time INTEGER, c_time INTEGER, mode INTEGER)";
	return cloudfs_sqlite_create_table(pDbHandle, file_meta_table);
}



int cloudfs_sqlite_get_file_meta(void *pDbHandle, string &filePath, stFileMetaRecord *pMeta)
{
	if ((NULL == pDbHandle) || (NULL == pMeta))
	{
		log_error("Invalid input, 0x%x 0x%x\n", pDbHandle, pMeta);
		return -1;
	}

	sqlite3 *db = (sqlite3 *)pDbHandle;
	int rc = 0;

	char pStatement[] = "select m_time, mode from t_fs_file_meta where file_path=?";
	
	sqlite3_stmt *pStmt = NULL;
	rc = sqlite3_prepare_v2(db, pStatement, strlen(pStatement), &pStmt, NULL);
	if (rc != SQLITE_OK)
	{
		log_error("sqlite3_prepare_v2 failed, rc = %d, stmt = %s, sql error: %s", rc, pStatement, sqlite3_errmsg(db));
		return -1;
	}

	rc = sqlite3_bind_text(pStmt, 1, filePath.c_str(), filePath.length(), SQLITE_STATIC);
	if (rc != SQLITE_OK)
	{
		log_error("sqlite3_bind_text failed, rc = %d, stmt = %s, sql error: %s", rc, filePath.c_str(), sqlite3_errmsg(db));
		sqlite3_finalize(pStmt);
		return -1;	
	
	}

	//以主键查询, 只可能有一个返回记录
	int s = sqlite3_step(pStmt);
	if (s != SQLITE_ROW)
	{
		log_debug("no record found, stmt = %s, sql error: %s", pStatement, sqlite3_errmsg(db));
		sqlite3_finalize(pStmt);
		return -1;
	}

	//获取返回结果, 存在
	pMeta->m_time = sqlite3_column_int(pStmt, 0);
	pMeta->mode = sqlite3_column_int(pStmt, 1);
	sqlite3_finalize(pStmt);
	return 0;
	
}


int cloudfs_sqlite_set_file_meta_ex(void *pDbHandle, string &filePath, stFileMetaRecord *pMeta)
{
	if ((NULL == pDbHandle) || (NULL == pMeta))
	{
		log_error("Invalid input, 0x%x 0x%x\n", pDbHandle, pMeta);
		return -1;
	}

	sqlite3 *db = (sqlite3 *)pDbHandle;
	int rc = 0;
	sqlite3_stmt *pStmt = NULL;

	//插入, 此处使用replace语法
	char pStatement[256];
	sprintf(pStatement, "replace into t_fs_file_meta values (?, 0, %Zd, 0, %Zd)", pMeta->m_time, pMeta->mode);
	
	rc = sqlite3_prepare_v2(db, pStatement, strlen(pStatement), &pStmt, NULL);
	if (rc != SQLITE_OK)
	{
		log_error("sqlite3_prepare_v2 failed, rc = %d, stmt = %s, sql error: %s", rc, pStatement, sqlite3_errmsg(db));
		return -1;
	}

	rc = sqlite3_bind_text(pStmt, 1, filePath.c_str(), filePath.length(), SQLITE_STATIC);
	if (rc != SQLITE_OK)
	{
		log_error("sqlite3_bind_text failed, rc = %d, stmt = %s, sql error: %s", rc, filePath.c_str(), sqlite3_errmsg(db));
		sqlite3_finalize(pStmt);
		return -1;	
	
	}

	int s = sqlite3_step(pStmt);
	if (s != SQLITE_DONE)
	{
		log_error("error, stmt = %s, sql error: %s", pStatement, sqlite3_errmsg(db));
		sqlite3_finalize(pStmt);
		return -1;	
	}	

	sqlite3_finalize(pStmt);	
	return 0;
}


int cloudfs_sqlite_remove_file_meta_ex(void *pDbHandle, string &filePath)
{
	if (NULL == pDbHandle)
	{
		log_error("Invalid input, 0x%x 0x%x\n", pDbHandle);
		return -1;
	}

	sqlite3 *db = (sqlite3 *)pDbHandle;
	int rc = 0;

	char pStatement[] = "delete from t_fs_file_meta where file_path=?";
	
	sqlite3_stmt *pStmt = NULL;
	rc = sqlite3_prepare_v2(db, pStatement, strlen(pStatement), &pStmt, NULL);
	if (rc != SQLITE_OK)
	{
		log_error("sqlite3_prepare_v2 failed, rc = %d, stmt = %s, sql error: %s", rc, pStatement, sqlite3_errmsg(db));
		return -1;
	}

	rc = sqlite3_bind_text(pStmt, 1, filePath.c_str(), filePath.length(), SQLITE_STATIC);
	if (rc != SQLITE_OK)
	{
		log_error("sqlite3_bind_text failed, rc = %d, stmt = %s, sql error: %s", rc, filePath.c_str(), sqlite3_errmsg(db));
		sqlite3_finalize(pStmt);
		return -1;	
	
	}

	//以主键查询, 只可能有一个返回记录
	int s = sqlite3_step(pStmt);
	if (s != SQLITE_DONE)
	{
		log_debug("no record found, stmt = %s, sql error: %s", pStatement, sqlite3_errmsg(db));
		sqlite3_finalize(pStmt);
		return -1;
	}
	
	sqlite3_finalize(pStmt);
	return 0;
	


}


//add的逻辑是先查询是否存在, 如果存在直接把相关字段返回给调用者
//如果不存在, 则插入输入的数据
int cloudfs_sqlite_add_file_meta_ex(void *pDbHandle, string &filePath, stFileMetaRecord *pMeta)
{
	int rc = 0;
	if ((NULL == pDbHandle) || (NULL == pMeta))
	{
		log_error("Invalid input, 0x%x 0x%x\n", pDbHandle, pMeta);
		return -1;
	}
	
	rc = cloudfs_sqlite_get_file_meta(pDbHandle, filePath, pMeta);
	if (rc == 0)
	{
		//记录已经存在, 直接return;
		return 0;
	}

	rc = cloudfs_sqlite_set_file_meta_ex(pDbHandle, filePath, pMeta);	
	return rc;	
}


//程序启动时, 打开数据库文件, 获得数据库访问句柄
int cloudfs_sqlite_init(const char *pDbFilePath)
{

	if (0 == AliConf::USE_SQLITE)
	{
		return 0;
	}


	sqlite3 *db = NULL;
	int rc = 0;
	rc = sqlite3_open(pDbFilePath, &db);
	if(rc != 0)
	{
		log_error("Can't open database[%s]: %s", pDbFilePath, sqlite3_errmsg(db));
		sqlite3_close(db);
		return -1;
	}

	//创建表, 如果表已经存在, 则无需关心	
	cloudfs_sqlite_create_file_meta_table(db);

	gDbHanle.Init(db);
	//sqlite3_close(db);
	
	return 0;
}



int cloudfs_sqlite_set_file_meta(string &filePath, stFileMetaRecord *pMeta)
{
/*
	sqlite3 *db = NULL;
	int rc = 0;
	rc = sqlite3_open(AliConf::SQLITE_FILE_PATH.c_str(), &db);
	if(rc != 0)
	{
		log_error("Can't open database[%s]: %s", AliConf::SQLITE_FILE_PATH.c_str(), sqlite3_errmsg(db));
		sqlite3_close(db);
		return -1;
	}
*/
	if (0 == AliConf::USE_SQLITE)
	{
		return 0;
	}

	sqlite3 *db = gDbHanle.get_db();
	int rc = 0;
	
#ifdef __CLOUDFS_COUNT_TIME__	
	struct timeval tv_start, tv_end;
	gettimeofday(&tv_start, NULL);
#endif	

	rc = cloudfs_sqlite_set_file_meta_ex(db, filePath, pMeta);
#ifdef __CLOUDFS_COUNT_TIME__	
	gettimeofday(&tv_end, NULL);
	printf("tv_start.tv_sec:%ld tv_start.tv_usec:%ld\n", tv_start.tv_sec, tv_start.tv_usec);	
	printf("tv_end.tv_sec:%ld tv_end.tv_usec:%ld\n", tv_end.tv_sec, tv_end.tv_usec);
#endif

	//sqlite3_close(db);
	
	return rc;

}

int cloudfs_sqlite_remove_file_meta(string &filePath)
{
/*
	sqlite3 *db = NULL;
	int rc = 0;
	rc = sqlite3_open(AliConf::SQLITE_FILE_PATH.c_str(), &db);
	if(rc != 0)
	{
		log_error("Can't open database[%s]: %s", AliConf::SQLITE_FILE_PATH.c_str(), sqlite3_errmsg(db));
		sqlite3_close(db);
		return -1;
	}
*/

	if (0 == AliConf::USE_SQLITE)
	{
		return 0;
	}


	sqlite3 *db = gDbHanle.get_db();
	int rc = 0;
#ifdef __CLOUDFS_COUNT_TIME__	
	struct timeval tv_start, tv_end;
	gettimeofday(&tv_start, NULL);
#endif	
	rc = cloudfs_sqlite_remove_file_meta_ex(db, filePath);
#ifdef __CLOUDFS_COUNT_TIME__	
	gettimeofday(&tv_end, NULL);
	printf("tv_start.tv_sec:%ld tv_start.tv_usec:%ld\n", tv_start.tv_sec, tv_start.tv_usec);	
	printf("tv_end.tv_sec:%ld tv_end.tv_usec:%ld\n", tv_end.tv_sec, tv_end.tv_usec);
#endif

	//sqlite3_close(db);
	
	return rc;	
}

int cloudfs_sqlite_add_file_meta(string &filePath, stFileMetaRecord *pMeta)
{/*
	sqlite3 *db = NULL;
	int rc = 0;
	rc = sqlite3_open(AliConf::SQLITE_FILE_PATH.c_str(), &db);
	if(rc != 0)
	{
		log_error("Can't open database[%s]: %s", AliConf::SQLITE_FILE_PATH.c_str(), sqlite3_errmsg(db));
		sqlite3_close(db);
		return -1;
	}
*/	

	if (0 == AliConf::USE_SQLITE)
	{
		return 0;
	}

	sqlite3 *db = gDbHanle.get_db();
	int rc = 0;
	
#ifdef __CLOUDFS_COUNT_TIME__	
		struct timeval tv_start, tv_end;
		gettimeofday(&tv_start, NULL);
#endif	

	rc = cloudfs_sqlite_add_file_meta_ex(db, filePath, pMeta);
#ifdef __CLOUDFS_COUNT_TIME__	
		gettimeofday(&tv_end, NULL);
		printf("tv_start.tv_sec:%ld tv_start.tv_usec:%ld\n", tv_start.tv_sec, tv_start.tv_usec);	
		printf("tv_end.tv_sec:%ld tv_end.tv_usec:%ld\n", tv_end.tv_sec, tv_end.tv_usec);
#endif


	//sqlite3_close(db);
	
	return rc;	

}




