

#include "OssSubObject.h"
#include "OssStats.h"
#include "Ali.h"
#include "OssFS.h"
#include "AliConf.h"
#include "log.h"
#include "util.h"
#include <iostream>
using namespace std;
//using namespace afs;
OssSubObject::OssSubObject(OssFS *fs, Oss *oss, const char *bucket,
        const char *pathname, const char *filename, OssStats * stats,
        size_t part_num, OssGroupObject *group) /*:
        OssObject(fs, oss, bucket, pathname, filename, stats)*/ {
    // TODO Auto-generated constructor stub
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);	    
    pthread_mutex_init(&m_mutex, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);

    m_readonly = true;    
    m_read_pos = 0;
    m_write_pos = 0;
    
    //the buf size
    m_buf_size = 0;
    m_synced = true;
    m_data = NULL;
	m_part_num = part_num;
    m_group = group;
}

OssSubObject::~OssSubObject() {

    destory();
    pthread_mutex_destroy(&m_mutex);
}


/*
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

*/
int OssSubObject::write_data(off_t offset, size_t size, const char *buf) 
{
	log_debug("offset:%Zd, size:%Zd", offset, size);
    int written = (int) size;

    //注意 这里获取块大小需要父对象锁, 必须放在pthread_mutex_lock(&m_mutex) 之前
	size_t subobject_size = m_group->get_subobject_size(m_part_num);

    pthread_mutex_lock(&m_mutex);
    m_synced = false;
	m_readonly = false;			

	//获取当前块的大小
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
			pthread_mutex_unlock(&m_mutex);							
			return 0;							
										
		}
		
	}

	//offset 小于文件大小, 属于正常写入, 根据本次写入的结尾分配内存
    if (((size_t) offset) <= subobject_size) {
        RE_ALOCATE(m_data, m_buf_size, (offset+size), m_group->get_cloudfs())
    }
    //offset 大于文件大小, subobject当前尾部-offset之间填写0
    else 
	{
		log_error("file[%s] offset [%Zd] is bigger than fileSize [%Zd]", 
					get_path_name(), offset, subobject_size);
		log_error("subObject buff info: [m_read_pos: %Zd m_write_pos = %Zd m_buf_size: %Zd m_stats->size: %Zd]", 
									m_read_pos, m_write_pos, m_buf_size, subobject_size);

		//先分配内存
        RE_ALOCATE(m_data, m_buf_size, (offset+size), m_group->get_cloudfs())

        //subobject当前尾部-offset之间填写0
		memset((m_data+subobject_size), 0, (offset -  subobject_size));

    }

	//把本次需要写的数据填入m_data
    memcpy(m_data + offset, buf, size);
    m_write_pos = std::max(m_write_pos, (size_t)(offset + size));
    pthread_mutex_unlock(&m_mutex);
    return written;
}


int OssSubObject::read_data(off_t offset, size_t size, char *buf) {

    //注意 这里获取块大小需要父对象锁, 必须放在pthread_mutex_lock(&m_mutex) 之前
	size_t subobj_size = m_group->get_subobject_size(m_part_num);

    pthread_mutex_lock(&m_mutex);

	log_debug("offset:%d, size:%d", (int)offset, (int)size);

	// 如果起始偏移超过了文件总长度，不需再读取，直接返回
    if (subobj_size <= (size_t) offset) 
	{
    	pthread_mutex_unlock(&m_mutex);
        return 0;
    }    

	// 如果请求数据的最大偏移超过文件长度，超过文件部分丢弃
    if (subobj_size < (offset + size)) {
        size = subobj_size - offset;
	}


	//第一次加载, 一次性把内存分配好, OSS object数据全部加载到cloudfs中
	if ((subobj_size > 0) && (m_buf_size == 0))
	{
		RE_ALOCATE(m_data, m_buf_size, subobj_size, m_group->get_cloudfs())
        int res = m_group->get_cloudfs()->get_oss()->get_object_data_with_range(AliConf::BUCKET.c_str(), 
        					   			  m_group->get_path_name(), 
        					   			  m_data, 
        					   			  m_part_num * AliConf::BLOCK_SIZE, 
        					   			  subobj_size);
        if (res < 0)
        {
        	log_error("file:[%s] get_object_data_with_range failed, range[%Zd-%Zd]", m_part_num * AliConf::BLOCK_SIZE, (subobj_size-1));
        	return 0;
        }        
	}
	
    m_read_pos = std::max(m_read_pos, (size_t)(offset + size));    
    memcpy(buf, m_data + offset, size);
    on_read_check_block();
    pthread_mutex_unlock(&m_mutex);
    return (int) size;
}


int OssSubObject::drop_cache(bool lock) {
    pthread_mutex_lock(&m_mutex);
    int res = 0;
    log_debug("tttt file_name %s, m_synced %d", m_group->get_file_name(), m_synced);
    SAFE_DELETE_ARRAY(m_data);
    res = m_buf_size;
    m_group->get_cloudfs()->decrease_cache(m_buf_size);
    log_debug("tttt decrease_cache: file_name: %s decrease_size = %zd", m_group->get_file_name(), m_buf_size);
	m_read_pos = 0;
	m_write_pos = 0;    
    m_buf_size = 0;
    pthread_mutex_unlock(&m_mutex);
    return res;
}

/*sync will include the drop cache*/
//pub_obj  true: 使用put_object上传本块
//		   false: 使用upload part上传本块
int OssSubObject::sync(bool put_obj) 
{

	//注意 这里获取块大小需要父对象锁, 必须放在pthread_mutex_lock(&m_mutex) 之前
	size_t subobj_size = m_group->get_subobject_size(m_part_num);
    pthread_mutex_lock(&m_mutex);
	log_debug("enter ...");
    int res = 0;
    
	log_debug("m_synced:%d", m_synced);
    if (!m_synced) 
	{
        m_synced = true;


		// 提交 Multipart 数据到内存
		if (put_obj)
		{
			OSS_FILE_META meta;

			if (m_group->get_stats()->size > m_buf_size)
			{
				log_error("m_buf_size[%Zd] is smaller than file size[%Zd]", m_buf_size, m_group->get_stats()->size);
			}
			m_group->get_stats()->to_meta(meta);
        	m_group->get_cloudfs()->get_oss()->put_object_data(AliConf::BUCKET.c_str(), 
															   m_group->get_path_name(), 
															   m_data, 
															   meta);
		}
		else
		{
			if (subobj_size > m_buf_size)
			{
				log_error("m_buf_size[%Zd] is smaller than sub size[%Zd]", m_buf_size, subobj_size);
			}
			string ret = m_group->get_cloudfs()->get_oss()->upload_part(AliConf::BUCKET.c_str(), 
	        				   m_group->get_path_name(),
	        				   m_group->get_uploadId().c_str(), 
	        				   m_part_num + 1, 
	        				   m_data, 
	        				   subobj_size);
	        if (ret == "") {
	        	log_error("upload_part failed, m_bucket[%s], objectname[%s], uploadid[%s], partnum[%d]", 
	        			   AliConf::BUCKET.c_str(),m_group->get_path_name(),m_group->get_uploadId().c_str(),(m_part_num + 1)); 
	        }
	        else {
	        	log_debug("upload_part succeed, m_bucket[%s], objectname[%s], uploadid[%s], partnum[%d]", 
	        			   AliConf::BUCKET.c_str(),m_group->get_path_name(),m_group->get_uploadId().c_str(),(m_part_num + 1));         
	        }
		}
		
        
    }


	//内存无条件释放
	SAFE_DELETE_ARRAY(m_data);
	res = m_buf_size;
	m_group->get_cloudfs()->decrease_cache(m_buf_size);
	log_debug("tttt decrease_cache: file_name: %s decrease_size = %zd", m_group->get_file_name(), m_buf_size);
	m_read_pos = 0;
	m_write_pos = 0;
	m_buf_size = 0;
    
    pthread_mutex_unlock(&m_mutex);
    return res;
}

int OssSubObject::destory() {
    return 0;
}

//本函数并不真正执行upload part copy操作, 由于该操作比较费时, 写入操作线程异步进行
int OssSubObject::upload_part_copy()
{

	//注意 这里获取块大小需要父对象锁, 必须放在pthread_mutex_lock(&m_mutex) 之前
	size_t subobj_size = m_group->get_subobject_size(m_part_num);

	pthread_mutex_lock(&m_mutex);
	if (subobj_size > 0)
	{
		UploadPartCopy *pNode = new UploadPartCopy;
		pNode->bucket = AliConf::BUCKET;
		pNode->object = m_group->get_path_name();
		pNode->s_object = m_group->get_path_name();
		pNode->uploadid = m_group->get_uploadId();
		pNode->part_num = m_part_num + 1;
		pNode->start_pos = m_part_num * AliConf::BLOCK_SIZE;
		pNode->end_pos = pNode->start_pos + subobj_size - 1;
		pNode->oss = m_group->get_cloudfs()->get_oss();
		
		m_group->get_cloudfs()->threadpool_add_upload_part_copy_task(pNode);
	}	
	pthread_mutex_unlock(&m_mutex);

	return 0;
}


void OssSubObject::on_write_check_block()
{
	pthread_mutex_lock(&m_mutex);
	//先判断write_pos是否满块, 如果write_pos已经满块, 直接启动upload part
	if (m_write_pos == AliConf::BLOCK_SIZE)
	{
		smart_upload_part();
		pthread_mutex_unlock(&m_mutex);
		return;
	}

	pthread_mutex_unlock(&m_mutex);
	return;

}



void OssSubObject::on_read_check_block()
{
	pthread_mutex_lock(&m_mutex);

	//判断read_pos是否满块, 同时write_pos是否为0, 如果满足条件, 直接释放内存
	if ((m_read_pos == AliConf::BLOCK_SIZE) && (m_write_pos == 0))
	{
		drop_cache(true);
		pthread_mutex_unlock(&m_mutex);
		return;
	}

	//其他情况不在本函数处理
	pthread_mutex_unlock(&m_mutex);
	return;

}

//本函数
void OssSubObject::on_fclose_check_block(bool bGroupReadOnly)
{
	//Group只读, 则直接返回
	pthread_mutex_lock(&m_mutex);
	if (bGroupReadOnly)
	{
		m_readonly = true;
		pthread_mutex_unlock(&m_mutex);
		return;
	}
	
	
	if (m_readonly)
	{
		//块处于只读状态, copy一份
		upload_part_copy();
	}
	m_readonly = true;
	pthread_mutex_unlock(&m_mutex);
	return;

}

void OssSubObject::smart_upload_part()
{
	//如果该块只读, 什么也不做, 直接返回
	pthread_mutex_lock(&m_mutex);
	if (m_readonly)
	{
		pthread_mutex_unlock(&m_mutex);
		return;
	}
	
	if (m_group->get_cloudfs()->threadpool_is_busy())
	{
		//thread pool已经满载, 此处在本线程进行upload part操作
		sync(false);
	}
	else
	{
		//thread pool未满载, 将upload part 发送给thread pool进行操作
		UploadPart *pNode = new UploadPart;
		pNode->isDropCache = true;
		pNode->subObj = this;
		m_group->get_cloudfs()->threadpool_add_task(pNode);
	}
	pthread_mutex_unlock(&m_mutex);
}


