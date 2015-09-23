

#ifndef OSSFS_H_
#define OSSFS_H_

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <limits.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include "Oss.h"
#include "AliConf.h"
#include "Ali.h"
#include <vector>
#include "util.h"
#include "Cond.h"
#include "OssThreadPool.h"

//namespace afs {
class OssStats;
class OssObject;
class OssSubObject;
class OssCache;
typedef std::map<const char *, OssObject *, map_str_comparer> OSS_OBJS_MAP;

typedef struct multipart_upload_node_s
{
	int type; //0:multipart_upload_copy 1:multipart_upload_complete
	string bucket;
	string object;
	string s_object;		// multi_part_copy 时使用
	string uploadid;
	int part_num;
	size_t start_pos;
	size_t end_pos;
	CCond  *sync_cond;			//用于multipart_upload_complete提交后的进程状态同步
}multipart_upload_node_s;


typedef struct tagUploadPart
{
	OssSubObject* subObj;
	bool isDropCache;
}UploadPart;

typedef struct tagUploadPartCopy
{
	Oss *oss;
	string bucket;
	string object;
	string s_object;	
	string uploadid;
	int part_num;
	size_t start_pos;
	size_t end_pos;

}UploadPartCopy;


class OssFS {
public:
    OssFS();
    virtual ~OssFS();
protected:
    Oss *m_oss;
    //the file tree just like the dir
    map<const char *, OSS_OBJS_MAP *, map_str_comparer> m_files;


	//the mutex for the online sync;
	pthread_mutex_t m_online_sync_mutex;

    //the root object for this bucket, just '/'
    OssObject * m_root;

    //uploadid 文件管理实例
    UploadIdFile m_uploadid_file;

    unsigned long long m_file_count;

	threadpool m_pool;
protected:
    OssObject * add_file(const char* path, std::vector<OssStats *> &stats,
            oss_obj_type_t type);
    int del_file(const char *path);
    int init_file();

	int load_files(vector<oss_object_desc_t>& objects, bool initial_load, size_t sync_flag);
    OssObject *get_file(const char *path);
	void uplodid_undone_proc();

	OssObject* find_file(const char* path);
protected:

    unsigned long long m_cache_size;
    pthread_t m_sync_thread;
    bool m_thread_running_flag;

public:
	inline Oss *get_oss() {
		return m_oss;
	}

	OssObject* get_parent(const char* path);
    int AddUploadId(string uploadId, string filename);
    int RemoveUploadId(string uploadId, string filename);


    void * sync_routine();

    unsigned long long increase_file_count() {
        return __sync_fetch_and_add(&m_file_count, 1);
    }

    unsigned long long increase_cache(unsigned long long size) {
        return __sync_fetch_and_add(&m_cache_size, size);
    }
    unsigned long long decrease_cache(unsigned long long size) {
        return __sync_fetch_and_sub(&m_cache_size, size);
    }

	int threadpool_add_task(UploadPart* part);
	int threadpool_add_upload_part_copy_task(UploadPartCopy* part);
	bool threadpool_is_busy();

public:
    int getattr(const char *path, struct stat *statbuf);
    int readlink(const char *path, char *link, size_t size);
    int mkdir(const char *path, mode_t mode);
    int unlink(const char *path);
    int rmdir(const char *path);
    int symlink(const char *path, const char *link);
    int rename(const char *path, const char *newpath);
    int link(const char *path, const char *newpath);
    int chmod(const char *path, mode_t mode);
    int chown(const char *path, uid_t uid, gid_t gid);
    int truncate(const char *path, off_t newSize);
    int utime(const char *path, struct utimbuf *ubuf);
    int open(const char *path, struct fuse_file_info *fileInfo);
    int create(const char *path, mode_t mode, struct fuse_file_info *fi);
    int read(const char *path, char *buf, size_t size, off_t offset,
            struct fuse_file_info *fileInfo);
    int write(const char *path, const char *buf, size_t size, off_t offset,
            struct fuse_file_info *fileInfo);
    int statfs(const char *path, struct statvfs *statInfo);
    int flush(const char *path, struct fuse_file_info *fileInfo);
    int release(const char *path, struct fuse_file_info *fileInfo);
    int fsync(const char *path, int datasync, struct fuse_file_info *fi);
    int readdir(const char *path, void *buf, fuse_fill_dir_t filler,
            off_t offset, struct fuse_file_info *fileInfo);
    int fsyncdir(const char *path, int datasync,
            struct fuse_file_info *fileInfo);
    int access(const char *path, int mode);         
    void init(struct fuse_conn_info *conn);
    void destroy();
    OssObject* find_certain_path(const char* path);
};
//}
#endif /* OSSFS_H_ */
