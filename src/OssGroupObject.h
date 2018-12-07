

#ifndef OSSGROUPOBJECT_H_
#define OSSGROUPOBJECT_H_

#include "OssObject.h"
#include <vector>

//namespace afs {
class OssSubObject;
class OssGroupObject: public OssObject {
public:
    OssGroupObject(OssFS *fs, Oss *oss, const char *bucket,
            const char *pathname, const char *filename,
            std::vector<OssStats *> &stats);
    virtual ~OssGroupObject();
protected:
    OssSubObject* add_subobject(size_t part_num, OssStats * stats,bool is_open=true);
	OssSubObject* get_subobj_by_part_num(size_t part_num);
    int destory();
public:
	int truncate(off_t new_size);
    int fopen(struct fuse_file_info *fi);
    int fclose(struct fuse_file_info *fi);
    int write_data(off_t offset, size_t size, const char *buf);
    int read_data(off_t offset, size_t size, char *buf);
    const OssStats *get_stats();
    int sync(bool dc = true);
    int drop_cache(bool lock);
    //virtual int set_mode(mode_t mode); 
    
	bool is_readonly() {
		return m_readonly;
	}
	
	inline string get_uploadId() const {
		return m_uploadId;
	}
	int multi_copy(const char *new_path_name);
	
	size_t get_subobj_num(){
		pthread_mutex_lock(&m_open_mutex);
		size_t res = m_sub_objects.size();
		pthread_mutex_unlock(&m_open_mutex);
		return res;
	}

	int delete_oss_object();

	int set_write() {
		m_readonly = false;
		return 0;
	}

	int get_refcnt() {
		return m_refcount;
	}

	bool is_open();

	size_t get_subobject_size(size_t part_num);

protected:
	int release_subobject();
	

protected:
	string m_uploadId;
    std::vector<OssSubObject *> m_sub_objects;
    pthread_mutex_t m_open_mutex;
    bool m_readonly;
    int m_refcount;
};
//}
#endif /* OSSGROUPOBJECT_H_ */
