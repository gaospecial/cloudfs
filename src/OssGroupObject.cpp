

#include "OssGroupObject.h"
#include "OssSubObject.h"
#include "OssStats.h"
#include "OssFS.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string>
#include <iostream>
#include "Ali.h"
#include "OssFS.h"
#include "AliConf.h"
#include "log.h"
#include "util.h"
#include "Cond.h"
#include "cloudfs_sqlite.h"

using namespace std;

#define ONE_G		20971520		//20M
//#define ONE_G		104857600


void debug_error_multi_upload(map<int, long> &part_size)
{
	
	for (map<int, long>::iterator it = part_size.begin(); it != part_size.end(); it++)
	{
		log_error("Part Num[%d], Part Size[%ld]", it->first, it->second);
	}
}



//using namespace afs;
OssGroupObject::OssGroupObject(OssFS *fs, Oss *oss, const char *bucket,
		const char *pathname, const char *filename,
		std::vector<OssStats *> &stats) :
		OssObject(fs, oss, bucket, pathname, filename,
				new OssStats(0, time(0), OSS_REGULAR)) 
{
	m_readonly = true;

	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);	  	
	pthread_mutex_init(&m_open_mutex, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);
	
	for (size_t i = 0; i < stats.size(); i++) 
	{
		m_stats->size += stats[i]->size;
		add_subobject(i, stats[i], false);
	}

	m_refcount = 0;
	
}

OssGroupObject::~OssGroupObject() {
	// TODO Auto-generated destructor stub
	this->destory();

	//删除子对象
	for (size_t i = 0; i < m_sub_objects.size(); i++) {
		SAFE_DELETE(m_sub_objects[i]);
	}

	pthread_mutex_destroy(&m_open_mutex);
}


OssSubObject* OssGroupObject::add_subobject(size_t part_num, OssStats * stats, bool is_open) 
{

	OssSubObject *obj = new OssSubObject(m_fs, m_fs->get_oss(), AliConf::BUCKET.c_str(), NULL,
										 NULL, stats, part_num, this);
	m_sub_objects.push_back(obj);

	log_debug("part:[%d], size:[%d]", part_num, stats->size);
	return obj;
}


/*
 * write data by BLOCK_SIZE each
 */
OssSubObject* OssGroupObject::get_subobj_by_part_num(size_t part_num)
{

	//所有的块在创建时都是安装顺序完成的, part_num的值从0开始计数, part_num必须小于m_sub_objects.size()
	if (part_num >= m_sub_objects.size())
	{
		log_debug("error part_num = %Zd, m_sub_objects.size() = %Zd", part_num, m_sub_objects.size());
		return NULL;		
	}

	//此处是一个检查, 记录日志
	if (m_sub_objects[part_num]->get_part_num() != part_num)
	{
		log_error("error part_num = %Zd, m_sub_objects.size() = %Zd", part_num, m_sub_objects.size());
	}
	
	return m_sub_objects[part_num];
}


int OssGroupObject::write_data(off_t offset, size_t size, const char *buf) 
{
	log_debug("offset:%d, size:%d", (int)offset, (int)size);
	size_t part_start = offset / AliConf::BLOCK_SIZE;		//本次要写入数据的第一个part
	size_t index_offset = offset % AliConf::BLOCK_SIZE;		//第一个块需要写入的数据偏移位置
	size_t data_offset = 0;									//记录当前已经写的数据数量
	size_t end = offset + size;								//本次要写入的数据结尾
	size_t part_end = (end - 1) / AliConf::BLOCK_SIZE;		//本次要写入数据的最后一个块
	size_t index = part_start;								//块循环遍历变量
	size_t written = 0;										//记录当前实际已经写入的数据数量

	pthread_mutex_lock(&m_open_mutex);
	m_readonly = false;
	//两种情况需要创建multipart_upload并进行upload_part_copy
	//1) 文件当前总大小小于1个块, 本次写入后会大于或等于1个块;
	//2) 文件当前总大小大于1个块, 无论本次写入在哪里
	if ((end >= AliConf::BLOCK_SIZE) || (m_stats->size > AliConf::BLOCK_SIZE))
	{
		if (m_uploadId.empty())
		{
			string uploadId;
			// initial multipart load
			uploadId = m_fs->get_oss()->initiate_multipart_upload(AliConf::BUCKET.c_str(), m_pathname);
			if (uploadId.empty())
			{
				log_error("initial multipart upload failed.");
				pthread_mutex_unlock(&m_open_mutex);
				return -1;
			}

			// 保存到内存中
			m_uploadId.assign(uploadId);

			// 保存到文件中
			m_fs->AddUploadId(m_uploadId, m_pathname);		
			log_debug("upload id:[%s]", m_uploadId.c_str());

		}
	}

	//写入时分为两种情况:
	//本次写入数据在同一个块内: 直接写入即可
	//本次写入数据跨了块: 先写第一块, 然后写中间块, 然后写最后一块(中间块可能是不存在的, 除非把块的大小设置到很小
	//这里实现先不考虑中间块的场景?
	


	//准备把数据写入块
	while (index <= part_end) 
	{
		OssSubObject* obj = NULL;		
		obj = get_subobj_by_part_num(index);
		if (obj != NULL) 
		{//subobject已经创建
			
			size_t index_size = 0;
			const char *index_data = NULL;
			size_t part_offset = 0;
			if (index == part_start)
			{
				//第一个块的写入
				part_offset = index_offset;
				index_size = std::min((AliConf::BLOCK_SIZE - part_offset), size);
			}
			else
			{
				//后面块的写入
				part_offset = 0;
				index_size = std::min(AliConf::BLOCK_SIZE, size);							
			}

			index_data = buf + data_offset;
			written += obj->write_data(part_offset, index_size, index_data);
			if (written != index_size)
			{
				log_error("written: %Zd, index_size: %Zd", written, index_size);
			}
			data_offset += index_size;
			size -= index_size;
			index += 1;
			
			if (((size_t)(offset + written)) > m_stats->size) {
				m_stats->size = offset + written;
			}

			obj->on_write_check_block();			
			if (size == 0)
				break;
		} 
		else 
		{	
			OssSubObject* tmpObj = NULL;		
			OssStats tmpStats(0, time(0), OSS_REGULAR);
			for (size_t i = 0; i < index; i++)
			{			
				tmpObj = get_subobj_by_part_num(i);
				if (tmpObj == NULL)
				{
					char* tmpBuf = new char[AliConf::BLOCK_SIZE * sizeof(char)];
					if (tmpBuf == NULL) {
						pthread_mutex_unlock(&m_open_mutex);
						exit(1);
					}
					
					memset(tmpBuf, 0, AliConf::BLOCK_SIZE * sizeof(char));
					tmpObj = add_subobject(i, &tmpStats);
					if (tmpObj) {
						written += tmpObj->write_data(0, AliConf::BLOCK_SIZE, tmpBuf);
						if (((size_t)(offset + written)) > m_stats->size) {
							m_stats->size = offset + written;
						}
						tmpObj->on_write_check_block();
					}
					else
					{
						log_error("add_subobject failed, part_num = %Zd", i);
					}

					SAFE_DELETE_ARRAY(tmpBuf);
				}
			}
			
			tmpObj = add_subobject(index, &tmpStats);
			if (!tmpObj) {
				log_error("add_subobject failed, part_num = %Zd", index);
			}
		}
	}

	//更新修改时间
	m_stats->mtime = time(0);
	if (end > m_stats->size) {
		m_stats->size = end;
	}

	pthread_mutex_unlock(&m_open_mutex);
	return written;
}

int OssGroupObject::read_data(off_t offset, size_t size, char *buf) 
{
	log_debug("offset:%d, size:%d", (int)offset, (int)size);
	pthread_mutex_lock(&m_open_mutex);

	//先检查本次读取数据是否在文件范围内
	if ((size_t)offset >= m_stats->size)
	{
		pthread_mutex_unlock(&m_open_mutex);
		return 0;
	}

	size_t end = offset + size;
	
	//检查offset + size 是否大于文件的最大长度, 如果大于文件的最大长度, 则截取为文件的当前实际长度
	if (end > m_stats->size)
	{
		end = m_stats->size;
		size = (end - offset);
	}

	size_t part_start = offset / AliConf::BLOCK_SIZE;
	size_t index_offset = offset % AliConf::BLOCK_SIZE;
	size_t part_end = (end - 1) / AliConf::BLOCK_SIZE;
	
	OssSubObject* obj = NULL;
	size_t data_offset = 0;
	
	/* 本次读没有跨块 */
	if (part_start == part_end)
	{
		obj = get_subobj_by_part_num(part_start);
		if (obj != NULL)
		{
			/* 记录本次读写的数据 */
			data_offset += obj->read_data(index_offset, size, (buf + data_offset));
		}
		else
		{
			log_error("get_subobj_by_part_num failed, part_start = %zd, current sub num = %zd", part_start, m_sub_objects.size());
			pthread_mutex_unlock(&m_open_mutex);
			return data_offset;
			
		}
	}
	/* 本次读跨了块, 先读第一个块的后半部分, 再读最后一个块的开始
	   如果中间有整个跨块, 则读全部块数据 */
	else if ((part_end - part_start) >= 1)
	{
		int read_size = 0;
			
		obj = get_subobj_by_part_num(part_start);
		if (obj != NULL)
		{			
			read_size = obj->read_data(index_offset, (AliConf::BLOCK_SIZE - index_offset), (buf + data_offset));
			data_offset += read_size;
		}
		else
		{
			log_error("get_subobj_by_part_num failed, part_start = %zd, current sub num = %zd", part_start, m_sub_objects.size());
			pthread_mutex_unlock(&m_open_mutex);
			return data_offset;			
		}

		/* 如果中间有块, 每次读整个块 */
		for (size_t part_index = part_start+1; part_index < part_end; part_index++)
		{
			obj = get_subobj_by_part_num(part_index);
			if (obj != NULL)
			{			
				read_size = obj->read_data(0, AliConf::BLOCK_SIZE, (buf + data_offset));
				data_offset += read_size;
			}
			else
			{
				log_error("get_subobj_by_part_num failed, part_index = %zd, current sub num = %zd", part_index, m_sub_objects.size());
				pthread_mutex_unlock(&m_open_mutex);
				return data_offset;
			}

		}


		/* 读最后一个块 */
		obj = get_subobj_by_part_num(part_end);
		if (obj != NULL)
		{			
			read_size = obj->read_data(0, (size - data_offset),(buf + data_offset));
			data_offset += read_size;
		}
		else
		{
			log_error("get_subobj_by_part_num failed, part_end = %zd, current sub num = %zd", part_end, m_sub_objects.size());
			pthread_mutex_unlock(&m_open_mutex);
			return data_offset;
		}		
		
	}

	pthread_mutex_unlock(&m_open_mutex);
	return (int) (data_offset);
}


int OssGroupObject::sync(bool dc) 
{
	log_debug("object size:%zd, file size: %Zd", m_sub_objects.size(), m_stats->size);

	//文件大小为0, 属于第一次创建的场景
	if (m_stats->size == 0)
	{
		log_debug("file size 0, try put_object_data");
		OSS_FILE_META meta;
		m_stats->to_meta(meta);
	    m_fs->get_oss()->put_object_data(AliConf::BUCKET.c_str(), get_path_name(), meta);
					
	}
	//只有一个块, 且小于BLOCK_SIZE
	else if (m_stats->size < AliConf::BLOCK_SIZE)
	{
		//此处的true指示sync调用put_object接口上传对象
		m_sub_objects[0]->sync(true);
	}
	else
	{
		//大于或等于1个块, 遍历所有的块, 将那些过程中未ready块sync
		for (size_t i = 0; i < m_sub_objects.size(); i++)
		{	
			//对于被改写过的subobject, 且subobject写的数量程度为not_ready, 需要在结束时完成upload part操作
			if ((!m_sub_objects[i]->is_readonly()) && (!m_sub_objects[i]->is_write_ready()))
			{
				m_sub_objects[i]->smart_upload_part();
			}

			//对于只读的subobject, cloudfs中模块的内存已经释放, 但是需要补上upload_part_copy
			m_sub_objects[i]->on_fclose_check_block(m_readonly);
		}
	}

	return 0;
}
int OssGroupObject::drop_cache(bool lock) {
	int res = 0;
	for (size_t i = 0; i < m_sub_objects.size(); i++) {
		res = m_sub_objects[i]->drop_cache(lock);
	}
	return res;
}
const OssStats *OssGroupObject::get_stats() {
	return m_stats;
}


int OssGroupObject::fopen(struct fuse_file_info *fi) 
{
	pthread_mutex_lock(&m_open_mutex);
	m_refcount++;
	pthread_mutex_unlock(&m_open_mutex);
	return 0;
}

int OssGroupObject::release_subobject()
{
	if (m_refcount > 0)
	{
		return 0;
	}

	//删除子对象
	for (size_t i = 0; i < m_sub_objects.size(); i++) {
		SAFE_DELETE(m_sub_objects[i]);
	}

	//清除m_sub_objects vector中的元素
	m_sub_objects.clear();
	return 0;
}

int OssGroupObject::fclose(struct fuse_file_info *fi) 
{
	log_debug("path:%s, m_readonly", m_pathname, m_readonly);
	
	pthread_mutex_lock(&m_open_mutex);
	
	//文件为只读状态
	if (m_readonly)
	{
		m_refcount--;
		drop_cache(true);
		pthread_mutex_unlock(&m_open_mutex);
		return 0;
	}

	//文件被修改过	
	sync();
	m_readonly = true;
	pthread_mutex_unlock(&m_open_mutex);
	
	int count = 1000;
	int totalPart = 0;
	map<int, string> part_etag;
	map<int, long> part_size;

	
	if (m_uploadId != "")
	{
		do
		{
			usleep(100 * 1000);
			part_etag.clear();
			part_size.clear();
			totalPart = m_fs->get_oss()->list_part(AliConf::BUCKET.c_str(), m_pathname, 
												   m_uploadId.c_str(), part_etag, part_size);
			if (totalPart < 0) {
				log_error("file:[%s] list part failed.", m_pathname);
				break;
			}
			
			if (totalPart == (int)m_sub_objects.size()) 
			{
				int i = 1;
				for (; i < totalPart; i++)
				{
					if (part_size[i] < (long)AliConf::BLOCK_SIZE)
					{
						log_error("file[%s:part%d] size:%ld", m_pathname,i, part_size[i]);
						break;
					}
				}

				if (i != totalPart)
				{
					continue;
				}
				
				log_error("totalpart:%d, subobjects:%d, fclose count: %d", totalPart, m_sub_objects.size(), (1000 - count));

				//  完成 complete操作
				string ret = m_fs->get_oss()->complete_multipart_upload(AliConf::BUCKET.c_str(), 
										 			   			 		m_pathname,
											 		   			 		m_uploadId.c_str());
				if (ret == "") 
				{
					log_error("[%s::%s]complete_multipart_upload failed.", m_pathname, m_uploadId.c_str());
					debug_error_multi_upload(part_size);
					m_fs->get_oss()->abort_multipart_upload(AliConf::BUCKET.c_str(), m_pathname, m_uploadId.c_str());
				}
				else 
				{
					log_debug("[%s::%s]complete_multipart_upload successful.", m_pathname, m_uploadId.c_str());
				}

				m_fs->RemoveUploadId(m_uploadId, m_pathname);
				m_uploadId = "";
				break;
			}
		} while(--count);

		log_debug("totalpart:%zd,subobject:%zd", totalPart, m_sub_objects.size());
	}

	if (count == 0) {
		log_error("filename:[%s] fclose timeout.", m_pathname);
		debug_error_multi_upload(part_size);
		m_fs->get_oss()->abort_multipart_upload(AliConf::BUCKET.c_str(), m_pathname, m_uploadId.c_str());
	}
	
	//此处需要更新meta_db
	string tmpPath = get_path_name();
	stFileMetaRecord tmpMeta;
	tmpMeta.mode = m_stats->mode;
	tmpMeta.m_time = m_stats->mtime;
	
	cloudfs_sqlite_set_file_meta(tmpPath,
								 &tmpMeta);	

	//保护m_refcount
	pthread_mutex_lock(&m_open_mutex);
	m_refcount--;
	OssObject::fclose(fi);
	pthread_mutex_unlock(&m_open_mutex);
	return 0;
}

int OssGroupObject::destory() {

	pthread_mutex_lock(&m_open_mutex);
	
	//m_fs->remove_sub_files(m_sub_objects);
	drop_cache(true);
	
	pthread_mutex_unlock(&m_open_mutex);
	return 0;
}


int OssGroupObject::multi_copy(const char *new_path_name)
{
	size_t start_pos;
	size_t end_pos;
	string uploadId;
	string ret;
	size_t count;
	size_t partSize;
	uploadId = m_fs->get_oss()->initiate_multipart_upload(AliConf::BUCKET.c_str(), new_path_name);
	if (uploadId.empty())
	{
		log_error("initial multipart upload failed.");
		return -1;
	}

	log_debug("stats size:%u", m_stats->size);
	if (m_stats->size > 0)
	{
		partSize = m_stats->size % AliConf::BLOCK_SIZE;
		count    = m_stats->size / AliConf::BLOCK_SIZE;

		size_t i;

		UploadPartCopy *pNode = NULL;
		for (i = 0; i < count; ++i)
		{
			//分配part copy节点内存
			pNode = new UploadPartCopy;

			start_pos = i * AliConf::BLOCK_SIZE;
			end_pos = start_pos + AliConf::BLOCK_SIZE - 1;
			
			pNode->bucket = AliConf::BUCKET;
			pNode->object = new_path_name;
			pNode->s_object = get_path_name();
			pNode->uploadid = uploadId;
			pNode->part_num = (i + 1);
			pNode->start_pos = start_pos;
			pNode->end_pos = end_pos;
			pNode->oss = get_cloudfs()->get_oss();					
			m_fs->threadpool_add_upload_part_copy_task(pNode);
		}

		if (partSize)
		{
			
			pNode = new UploadPartCopy;
			start_pos = count * AliConf::BLOCK_SIZE;
			end_pos = start_pos + partSize - 1;

			count += 1;
			pNode->bucket = AliConf::BUCKET;
			pNode->object = new_path_name;
			pNode->s_object = get_path_name();
			pNode->uploadid = uploadId;
			pNode->part_num = count;
			pNode->start_pos = start_pos;
			pNode->end_pos = end_pos;
			pNode->oss = get_cloudfs()->get_oss();	
			m_fs->threadpool_add_upload_part_copy_task(pNode);
			
		}		
	}

	map<int, string> part_etag;
	map<int, long> part_size;
	int totalPart = 0;

	int loop_count = 5*count;
	do
	{
		part_etag.clear();
		part_size.clear();
		totalPart = m_fs->get_oss()->list_part(AliConf::BUCKET.c_str(), new_path_name, 
											   uploadId.c_str(), part_etag, part_size);
		if (totalPart < 0) {
			log_error("file:[%s] list part failed.", m_pathname);
			break;
		}

		log_debug("totalPart = %d, count = %d", totalPart, count);
		if (totalPart == (int)(count)) 
		{
			if (part_size[1] < (long)AliConf::BLOCK_SIZE)
			{
				log_error("part 1 size:%ld", part_size[1]);
				usleep(60 * 1000);
				continue;
			}
			
			log_debug("totalpart: %d, subobjects: %d, loop_count: %d", totalPart, count, (1000-loop_count));
	
			//	完成 complete操作
			string ret = m_fs->get_oss()->complete_multipart_upload(AliConf::BUCKET.c_str(), 
																	new_path_name,
																	uploadId.c_str());
			if (ret == "") 
			{
				log_error("[%s::%s]complete_multipart_upload failed.", new_path_name, uploadId.c_str());
			}
			else 
			{
				log_debug("[%s::%s]complete_multipart_upload successful.", new_path_name, uploadId.c_str());
			}
	
			break;
		}
		else {
			usleep(100 * 1000); // 10 ms
		}
	} while(--loop_count);
	
	return 0;
}


int OssGroupObject::truncate(off_t new_size)
{
	//truncate 有两种逻辑: 
	//第1种: 如果newSize < 当前对象的size, 则把newSize-size的数据清掉
	//第2种: 如果newSize > 当前对象的size, 则制造出来一个空洞文件, 暂时先不支持第2种操作	

	//cloudfs实现当前只考虑支持open后直接调用truncate的场景, open/write/truncate的场景无法支持
	//因为一旦存在write操作, 则可能存在部分part已经提交给oss, 这种场景需要先abort掉原来的multi-part-upload
	//在truncate中重新进行上传操作

	pthread_mutex_lock(&m_open_mutex);
	if (new_size > (off_t)this->get_stats()->size)
	{
		log_error("truncate extend function not implemented");
		pthread_mutex_unlock(&m_open_mutex);
		return -1;
	}

	//truncate操作未改变文件大小, 直接返回
	if (new_size == (off_t)this->get_stats()->size)
	{
		pthread_mutex_unlock(&m_open_mutex);
		return 0;
	}

	// 计算new_size需要的sub_object的数量
	unsigned int part_num = new_size / AliConf::BLOCK_SIZE;
	unsigned int last_part_size = new_size % AliConf::BLOCK_SIZE;

	if (last_part_size > 0)
	{
		part_num += 1; 
	}
	

	unsigned int origin_size = (unsigned int)m_sub_objects.size();
	for (unsigned int i = part_num; i < origin_size; i++)
	{
		//删除释放对应的sub_object对象	
		SAFE_DELETE(m_sub_objects[i]);
	}

	//循环删除m_sub_objects的元素, 因为我们要保留的是前面的part_num个元素, 因此
	//将part_num元素后面的元素全部pop_back可以达到删除的目的
	for (unsigned int i = 0; i < (origin_size - part_num); i++)
	{	
		m_sub_objects.pop_back();
	}

	//重新设置GroupObject的size
	OssStats *pTmpStat = const_cast<OssStats *>(this->get_stats());
	pTmpStat->set_size((size_t)new_size);

	pthread_mutex_unlock(&m_open_mutex);	
	return 0;
}


int OssGroupObject::delete_oss_object()
{
	int res = m_fs->get_oss()->del_object(AliConf::BUCKET.c_str(), m_pathname, false);
	if (res != 0)
	{
		log_error("delete object %s failed", m_pathname);
	}
	
	return res;
}


size_t OssGroupObject::get_subobject_size(size_t part_num)
{
	size_t res = 0;
	pthread_mutex_lock(&m_open_mutex);
	size_t sub_obj_size = m_sub_objects.size();
	if ((part_num + 1) < sub_obj_size)
	{
		res = AliConf::BLOCK_SIZE;
	}
	else
	{
		res = m_stats->size - ((sub_obj_size-1) * AliConf::BLOCK_SIZE);
	}

	pthread_mutex_unlock(&m_open_mutex);

	return res;
}

bool OssGroupObject::is_open()
{
	   bool res = false;
	   pthread_mutex_lock(&m_open_mutex);
	   if (m_refcount == 0)
	   {
 	         	
		   res = false; 		   
	   }
	   else if (m_refcount > 0)
	   {
		   res = true;
	   }
	   else
	   {//m_refcount < 0 异常, 打印日志作为参考, 返回false
	       	res = false;
	       	log_error("file[%s] m_refcount abnoral %d", m_pathname, m_refcount);
	   }
	   pthread_mutex_unlock(&m_open_mutex); 
	   return res;
   }

