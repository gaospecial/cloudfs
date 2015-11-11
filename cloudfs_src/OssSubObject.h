

#ifndef OSSSUBOBJECT_H_
#define OSSSUBOBJECT_H_

#include "OssObject.h"
#include "OssGroupObject.h"
#include "AliConf.h"
#include <pthread.h>


/*
如果文件本身存在,write_data第一次写入时，将文件全部加载出来, 将读POS置为块大小

写pos   满          满              满    
读pos   满          未满            0

结果   write_data   write_data      write_data
        upload      upload          upload         
		
写pos   未满         未满            未满                       
读pos   满           未满            0             
结果   fclose        fclose          fclose       (upload的块内容以pos指较大的为准)
       upload        upload          upload
	   

写pos   0            0                0
读pos   满           未满             0 	   
结果:   read_data    fclose
        释放内存     释放内存         什么都不做


    写数据逻辑分析:
		写数据时分为两种情况: 
		情况1) 当前写入时, 本块已有数据
			   如果已经有了数据, 则需要将已有的数据一次性加载到内存中, 再进行写入;
			   写入时有两种情况: 
			   情况1: 写入的offset <= 当前块实际大小, 这种是普通的写入
			   情况2: 写入的offset > 当前块实际大小, 在当前块与offset之间填充0
		
		情况2) 当前写入时, 本块并无数据
				本块无数据, 则无需从OSS加载数据, 直接写入
				写入时也分两种情况:
			   情况1: 写入的offset 从0开始, 这种是普通的写入
			   情况2: 写入的offset > 大于0, 在0与offset之间填充0	
    

	//获取当前块的大小
	size_t subobject_size = get_size();
	if ((subobject_size > 0) && (m_buf_size == 0))
	{
		//从OSS加载数据数据到cloudfs中
		RE_ALOCATE(m_data, m_buf_size, subobject_size, m_group->get_cloudfs())
		int res = m_group->get_cloudfs()->get_oss()->get_object_data_with_range(AliConf::BUCKET.c_str(), 
										  m_group->get_path_name(), 
										  m_data, 
										  m_part_num * AliConf::BLOCK_SIZE, 
										  subobject_size);
		if (res < 0)
		{
			log_error("file: [%s] get_object_data_with_range range[%Zd-%Zd] failed", 
										m_group->get_path_name(), 
										(m_part_num * AliConf::BLOCK_SIZE), 
										(m_part_num * AliConf::BLOCK_SIZE+subobject_size-1));
			return 0;							
										
		}
		
	}
*/



//class OssGroupObject;
//namespace afs {
//class OssSubObject: public OssObject {
class OssSubObject{
public:
    OssSubObject(OssFS *fs, Oss *oss, const char *bucket, const char *pathname,
            const char *filename, OssStats * stats, size_t part_num, OssGroupObject *group);
    virtual ~OssSubObject();
protected:
    bool m_synced;
    bool m_readonly;
    size_t m_buf_size;
    size_t m_read_pos;
    size_t m_write_pos;
	size_t m_part_num;
    char *m_data;

    pthread_mutex_t m_mutex;
    OssGroupObject *m_group;	
    int destory();
	
    
public:	
    int drop_cache(bool lock);
    bool is_write_ready() {
    	pthread_mutex_lock(&m_mutex);
        bool write_ready = (m_write_pos == AliConf::BLOCK_SIZE);
        pthread_mutex_unlock(&m_mutex);
        return write_ready;
    }

    //the flags of sub files should comes from the above group object
    int get_flags() {
        return ((OssObject *)m_group)->get_flags();
    }
    //the is readonly of sub files should comes from the above group object
    bool is_readonly() {
        return m_readonly;
    }

	const char *get_file_name() const {
		
		return ((OssObject *)m_group)->get_file_name();
	}
	const char *get_path_name() const {
		return ((OssObject *)m_group)->get_path_name();
	}

	size_t get_part_num() {
		return m_part_num;
	}
	
	inline size_t get_buf_size() {
		return m_buf_size;
	}

	void set_synced(bool bSynced) { m_synced = bSynced;}
		
    int read_data(off_t offset, size_t size, char *buf);
    int write_data(off_t offset, size_t size, const char *buf);
    int sync(bool dc = true);
    int upload_part_copy();
    void on_fclose_check_block(bool bGroupReadOnly);
	void smart_upload_part();
	void on_read_check_block();
	void on_write_check_block();
};
//}
#endif /* OSSSUBOBJECT_H_ */
