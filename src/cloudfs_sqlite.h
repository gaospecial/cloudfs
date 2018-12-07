
#ifndef CLOUDFS_SQLITE_H_
#define CLOUDFS_SQLITE_H_

#include <string>

typedef struct tagFileMetaRecord
{
	time_t m_time;
	time_t c_time;
	time_t a_time;
	size_t mode;
}stFileMetaRecord;



int cloudfs_sqlite_init(const char *pDbFilePath);

int cloudfs_sqlite_set_file_meta(std::string &filePath, stFileMetaRecord *pMeta);

int cloudfs_sqlite_remove_file_meta(std::string &filePath);

int cloudfs_sqlite_add_file_meta(std::string &filePath, stFileMetaRecord *pMeta);


#endif
