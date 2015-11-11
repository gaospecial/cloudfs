

#include "OssObject.h"
#include "OssStats.h"
#include "Ali.h"
#include "OssFS.h"
#include <iostream>
using namespace std;
//using namespace afs;
OssObject::OssObject(OssFS *fs,Oss *oss,const char *bucket,const char *pathname,const char *filename,OssStats * stats) {
	// TODO Auto-generated constructor stub
    m_fs=fs;
    //m_oss=oss;
	oss = oss;
    bucket=bucket;
	
	if (pathname != NULL)
	{
	    int path_len = strlen(pathname);    
	    m_pathname = new char[path_len + 1];    
	    strncpy(m_pathname, pathname, path_len + 1);
    }
    else
    {
    	m_pathname = NULL;
    }
    
	//此处的filename不能节省
    if (filename != NULL)
    {
	    int file_len = strlen(filename);
		m_filename = new char[file_len + 1];
	    strncpy(m_filename, filename, file_len + 1);
    }
    else
    {
    	m_filename = NULL;
    }
    
    m_stats=stats;

	//增加文件成功, 此处往meta_db中写入数据
	string tmpPath = m_pathname;
	stFileMetaRecord tmpMeta;
	tmpMeta.m_time = m_stats->mtime;
	tmpMeta.mode = m_stats->mode;
	cloudfs_sqlite_add_file_meta(tmpPath, &tmpMeta);
	m_stats->mtime = tmpMeta.m_time;
	m_stats->mode = tmpMeta.mode;

	//如果当前对象不是根对象, 需要将该对象的上一级目录的m_sync_flag赋给当前对象
	if (strcmp(pathname, "/") != 0) 
	{
		m_sync_flag = m_fs->get_parent(pathname)->get_sync_flag();
	}
	else
	{
		m_sync_flag = 0;
	}
	

}

OssObject::~OssObject() {
	// TODO Auto-generated destructor stub
    SAFE_DELETE(m_stats);
    m_filename = NULL;
    SAFE_DELETE_ARRAY(m_pathname);    
}
int OssObject::fopen(struct fuse_file_info *fi){
    //m_flags=fi->flags;
    return 0;
}
int OssObject::fclose(struct fuse_file_info *fi){
    return 0;
}
int OssObject::sync_meta(){
	OSS_FILE_META meta;
	m_stats->to_meta(meta);
	return 0;//m_oss->put_object_meta(m_bucket, m_pathname,meta);
}
void OssObject::copy(const char *new_path_name) {
    m_fs->get_oss()->copy_object_data(AliConf::BUCKET.c_str(), m_pathname, new_path_name);
}

