

#ifndef OSSOBJECT_H_
#define OSSOBJECT_H_
#include "Ali.h"
#include "Oss.h"
#include "OssStats.h"
#include "OssFS.h"
#include <fuse.h>
#include "cloudfs_sqlite.h"


class OssObject {
public:
	OssObject(OssFS *fs, Oss *oss, const char *bucket, const char *pathname,
			const char *filename, OssStats * stats);
	virtual ~OssObject();
protected:
	char *m_pathname;
	char *m_filename;
	OssStats *m_stats;
	OssFS *m_fs;
	size_t m_sync_flag;

	virtual int destory() {
		return 0;
	}
public:
	inline OssFS * get_cloudfs(){
		return m_fs;
	}
	virtual int fopen(struct fuse_file_info *fi);
	virtual int fclose(struct fuse_file_info *fi);
	virtual int write_data(off_t offset, size_t size, const char *buf) {
		return 0;
	}
	virtual int read_data(off_t offset, size_t size, char *buf) {
		return 0;
	}
    virtual void copy(const char *new_path_name);
	virtual int sync(bool dc = true) {
		return 0;
	}
	virtual bool is_ready() {
		return true;
	}
	virtual bool is_part_ready() {
		return true;
	}
	virtual int get_flags() {
		return 0;
	}

	virtual bool is_readonly() {
		return true;
	}
	virtual int drop_cache(bool lock) {
		return 0;
	}
	virtual int set_mode(mode_t mode) {
		m_stats->mode = mode;
		string tmpPath = get_path_name();
		stFileMetaRecord tmpMeta;
		tmpMeta.mode = m_stats->mode;
		tmpMeta.m_time = m_stats->mtime;
		
		cloudfs_sqlite_set_file_meta(tmpPath,
									 &tmpMeta);		
		return 0;
	}

	virtual int set_write() {
		return 0;
	}

	virtual bool is_open() { return false; }
	

	virtual int set_ftime(time_t actime, time_t modtime) {
		//此处需要更新meta_db
		m_stats->mtime = modtime;
		string tmpPath = get_path_name();
		stFileMetaRecord tmpMeta;
		tmpMeta.mode = m_stats->mode;
		tmpMeta.m_time = m_stats->mtime;
		
		cloudfs_sqlite_set_file_meta(tmpPath,
									 &tmpMeta);
		return 0;
	}
	/*
	inline double get_weight() {
		return m_weight;
	}
	*/
	virtual const char *get_file_name() const {
		return m_filename;
	}
	virtual const char *get_path_name() const {
		return m_pathname;
	}
	virtual std::map<const char *, OssObject *, map_str_comparer> *get_subs()
	{
		//useless implementation
		return 0;
	} 

	int sync_meta();
	virtual const OssStats *get_stats() {
		return m_stats;
	}
	virtual const size_t get_sync_flag() {
		return m_sync_flag;
	}
	virtual void set_sync_flag(size_t sync_flag) {
		if (sync_flag == 0)
			sync_flag++;
		m_sync_flag = sync_flag;
	}

	virtual int delete_oss_object() = 0;
	
};

#endif /* OSSOBJECT_H_ */
