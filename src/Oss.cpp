
#include "Ali.h"
#include "Oss.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include <string>
#include "oss_api.h"
#include "oss_err.h"
#include "log.h"
#include "client.h"
#include "AliConf.h"
#include "util.h"
//using namespace ali;
using namespace std;

Oss::Oss() {
	// TODO Auto-generated constructor stub
	//初始化OSS_C SDK鉴权信息
	oss_auth_init();
	xml_load_buffer_init();
}

Oss::~Oss() {
	// TODO Auto-generated destructor stub
	//析构OSS_C SDK鉴权
	oss_auth_destroy();	
	xml_load_buffer_destroy();
	SAFE_DELETE(m_mime);
	SAFE_DELETE(m_header);
}

int Oss::init(const char *host, const char *id, const char *key) 
{
	log_debug("enter ......");
	address.hostname = string(host);
	address.access_id = string(id);
	address.access_key = string(key);

	// 读取MIME信息
	m_mime = new OssMime();
	if (0 != m_mime->init())
	{
		log_error("get mime type info failed.");
		return -1;
	}

	m_header = new OssHeader();
	if (0 != m_header->init())
	{
		log_error("get header params failed.");
		return -1;
	}

	//log_debug("Cache-Control:[%s]", m_header->get_cache_control().c_str());
	//log_debug("Content-Disposition:[%s]", m_header->get_content_disposition().c_str());
	//log_debug("Content-Encoding:[%s]", m_header->get_content_encoding().c_str());
	//log_debug("Expires:[%s]", m_header->get_expires().c_str());

	
	log_debug("address.hostname:   %s", host);
	log_debug("address.access_id:  %s", id);
	log_debug("address.access_key: %s", key);

	return 0;
}


/*
oss api get bucket接口实现
prefix 指定的文件夹路径
delimiter 是否递归遍历

成功返回0
失败返回-1
*/

int Oss::get_bucket(const char *bucket, const char *prefix, const char *delimiter, vector<oss_object_desc_t> &object_list)
{
	oss_result_t res;
	oss_list_object_param_t params;
	params.prefix = prefix;
	params.max_keys = "1000";
	params.delimiter = delimiter;
	params.marker = "";
	string bucket_name(bucket);

	//struct timeval tv1, tv2;
	//gettimeofday(&tv1, NULL);
	res = list_object(address, bucket_name, params, object_list);
	if (res != OSS_OK)
	{
		log_error("list_object failed address:%s, bucket_name:%s, prefix:%s, delimiter:%s",
				  address.hostname.c_str(), bucket, prefix, delimiter);
		return -1;
	}
	//gettimeofday(&tv2, NULL);
	//log_error("[list_bucket] difftime:[%zd]", getDiffTime(&tv2, &tv1));
	
	return 0;	
}



/*
 * The meta should have the at least four items: size, mtime, type and mode
 * TYPE:
 * typedef enum OSS_OBJ_TYPE{
 *      OSS_REGULAR=0,
 *      OSS_DIR=1,
 *      OSS_LINK=2
 *  } oss_obj_type_t;
 *  mode is just same as OS defined value
 */
int Oss::get_object_list(const char* bucket, OSS_FILE_MAP &objects) {
	oss_result_t res;
	vector<oss_object_desc_t> object_list;
	oss_list_object_param_t params;
	params.prefix = "";
	params.max_keys = "1000";
	params.delimiter = "";
	params.marker = "";
	string bucket_name(bucket);
	res = list_object(address, bucket_name, params, object_list);
	if (res == OSS_OK) {
		vector<oss_object_desc_t>::iterator iter = object_list.begin();
		for (; iter != object_list.end(); iter++) {
			OSS_FILE_META meta;
			get_object_meta(bucket_name.c_str(),iter->name.c_str(), meta);
			objects[iter->name] = meta;
		}
	} else
		return -1;

	return 0;
}
/*
 *
 *
 */
int Oss::copy_object_data(const char *bucket, const char *old_name, const char *new_name) {
	string bucket_name = string(bucket);
    string source_copy = string(old_name);
    string dest_copy = string(new_name);
	oss_get_object_header_t header;

	oss_result_t ret = copy_object(address, bucket_name, source_copy, dest_copy, header, false);

	if (ret == OSS_OK)
		return 0;
	else
		return -1;
}

/*
 * TODO: need to add the meta data here in the put_object in order to make them
 * persistent
 */
int Oss::put_object_data(const char *bucket, const char *name, const char *buf,
		OSS_FILE_META & meta) {
	string bucket_name = string(bucket);
	oss_object_desc_t put_object_desc;

	put_object_desc.name = string(name);
	put_object_desc.size = atoll(meta["size"].c_str());
	put_object_desc.user_meta["x-oss-meta-type"] = meta["type"];
	put_object_desc.user_meta["x-oss-meta-mtime"] = meta["mtime"];
	put_object_desc.user_meta["x-oss-meta-mode"] = meta["mode"];
	oss_data_buffer_t put_data_buf = (oss_data_buffer_t) buf;

	// 根据参数设置content-type
	string suffix = getFileSuffix(name);
	log_debug("file:[%s],suffix:[%s]", name, suffix.c_str());

	string type = m_mime->find(suffix);
	if (type.empty())
		type.assign("application/octet-stream");
	
	log_debug("type:%s", type.c_str());
	put_object_desc.type = type;

	// 设置 header 参数
	if (m_header->get_cache_control() != "")
		put_object_desc.cache_control = m_header->get_cache_control();
	if (m_header->get_content_disposition() != "")
		put_object_desc.content_disposition = m_header->get_content_disposition();
	if (m_header->get_content_encoding() != "")
		put_object_desc.content_encoding = m_header->get_content_encoding();
	if (m_header->get_expires() != "")
		put_object_desc.expires = m_header->get_expires();
	
	oss_result_t ret = put_object(address, bucket_name, put_object_desc,
			put_data_buf);

	if (ret == OSS_OK)
		return 0;
	else
		return -1;
}

int Oss::put_object_data(const char *bucket, const char *name, OSS_FILE_META & meta) 
{
	//save the buf of length size to the oss remote object
	string bucket_name = string(bucket);
	string object_name = string(name);
	oss_object_desc_t put_object_desc;
	put_object_desc.name = object_name;
	put_object_desc.size = 0;
	put_object_desc.user_meta["x-oss-meta-type"] = meta["type"];
	put_object_desc.user_meta["x-oss-meta-mtime"] = meta["mtime"];
	put_object_desc.user_meta["x-oss-meta-mode"] = meta["mode"];

	string suffix = getFileSuffix(name);
	log_debug("file:[%s],suffix:[%s]", name, suffix.c_str());

	string type = m_mime->find(suffix);
	if (type.empty())
		type.assign("application/octet-stream");
	
	log_debug("type:%s", type.c_str());

	put_object_desc.type = type;

	//struct timeval tv1, tv2;
	//gettimeofday(&tv1, NULL);
	oss_data_buffer_t put_data_buf = NULL;
	oss_result_t ret = put_object(address, bucket_name, put_object_desc, put_data_buf);
	//gettimeofday(&tv2, NULL);
	//log_debug("[put_object] difftime:[%zd]", getDiffTime(&tv2, &tv1));

	if (ret == OSS_OK)
		return 0;
	else
		return -1;
}

int Oss::put_object_data(const char *bucket, oss_object_desc_t &put_object_desc, const char *buf) 
{
	//save the buf of length size to the oss remote object
	string bucket_name = string(bucket);

	oss_data_buffer_t put_data_buf = (oss_data_buffer_t)buf;
	oss_result_t ret = put_object(address, bucket_name, put_object_desc,
			put_data_buf);

	if (ret == OSS_OK)
		return 0;
	else
		return -1;
}





int Oss::get_object_data(const char *bucket, const char *name, char *buf,
		size_t size) {
	string bucket_name = string(bucket);
	string object_name = string(name);
	oss_get_object_header_t header;
	oss_object_desc_t response_header;
	oss_get_object_response_param_t response_params;
	oss_data_buffer_t object_buffer = 0;

	//ret = head_object(address, bucket_name, object_name, header, response_header);
	string range;
	oss_result_t ret = get_object(address, bucket_name, object_name, header,
			range, response_params, response_header, object_buffer);
	if (ret != OSS_OK){
		log_error("get_object failed, ret = %d", ret);
		return -1;
	}


	memcpy((void *) buf, (void *) object_buffer, response_header.size);
	delete[] object_buffer;

	return response_header.size;
}


int Oss::get_object_data_with_range(const char *bucket, const char *name, char *buf,
		size_t startPos, size_t size) {
	string bucket_name = string(bucket);
	string object_name = string(name);
	oss_get_object_header_t header;
	oss_object_desc_t response_header;
	oss_get_object_response_param_t response_params;
	oss_data_buffer_t object_buffer = 0;
	char rangeBuff[128];

	//ret = head_object(address, bucket_name, object_name, header, response_header);
	sprintf(rangeBuff, "bytes=%Zd-%Zd", startPos, (startPos + size -1));
	string range;
	range.assign(rangeBuff);
	
	log_debug("range content: %s", range.c_str());

	oss_result_t ret = get_object(address, bucket_name, object_name, header,
			range, response_params, response_header, object_buffer);
	if (ret != OSS_OK){
		log_error("get_object failed, ret = %d", ret);
		return -1;
	}

	// debug info
	if (response_header.size > size) {
		log_error("file[%s] request range[%s], response_header.size=%Zd, size=%u", name, rangeBuff, response_header.size, size);
	}
	
	uint64_t len = (response_header.size > size) ? size : response_header.size;

	log_debug("response_header.size = %Zd", response_header.size);
	memcpy((void *) buf, (void *) object_buffer, len);
	delete[] object_buffer;

	return response_header.size;
}




unsigned int Oss::get_object_data(const char *bucket, const char *name, char *buf,
			int start_pos, size_t size)
{
	unsigned int ret = 0;
	unsigned short retcode = -1;			//保存服务器http返回码的解析结果;
	const char *retinfo = NULL;            //保存通过retcode获得的错误信息
	void *buffer = NULL;
	
	oss_client_t *client = client_initialize_with_endpoint(
		address.access_id.c_str(), address.access_key.c_str(), address.hostname.c_str());
	oss_get_object_request_t *request = get_object_request_initialize(bucket, name);
	request->set_range(request, start_pos, size);

	oss_object_metadata_t *metadata =
		client_get_object_to_buffer_2nd(client, request, &buffer, &ret, &retcode);

	if (retcode == OK) 
	{
		memcpy(buf, buffer, size);
		log_debug("Get object to buffer successfully.");
		log_debug("File length: %d", ret);
	}
	else 
	{
		retinfo = oss_get_error_message_from_retcode(retcode);
		log_error("%s", retinfo);
	}

	if (buffer != NULL) free(buffer);
	if (request != NULL) get_object_request_finalize(request);
	if (metadata != NULL) object_metadata_finalize(metadata);
	client_finalize(client);

	return ret;
}

/*
int Oss::del_object(const char *bucket, const char *name, bool dir) {
	string bucket_name = string(bucket);
	string object_name = string(name);
	oss_result_t res;
	if (!dir) {
		res = delete_object(address, bucket_name, object_name);
		return res;
	} else {
		// delete all children first
		vector<oss_object_desc_t> object_list;
		oss_list_object_param_t params;
		params.prefix = object_name;
		params.max_keys = "1000";
		params.delimiter = "";
		params.marker = "";
		res = list_object(address, bucket_name, params, object_list);
		if (res == OSS_OK) {
			vector<oss_object_desc_t>::iterator iter = object_list.begin();
			for (; iter != object_list.end(); iter++) {
				res = this->del_object(bucket, iter->name.c_str(), false);				
			}
		}
		// delete itself
		this->del_object(bucket, name, false);
	}
	return 0;
}*/


int Oss::del_object(const char *bucket, const char *name, bool dir) {
	string bucket_name = string(bucket);
	string object_name = string(name);
	oss_result_t res;

	res = delete_object(address, bucket_name, object_name);
	if (res != OSS_OK)
	{
		log_error("delete file [%s] failed.", name);
		return -1;
	}
	
	return 0;
}


int Oss::get_object_meta(const char* bucket, const char* name,
		oss_object_desc_t & obj_desc)
{
	string bucket_name = string(bucket);
	string object_name = string(name);
	oss_get_object_header_t header;


	if (object_name.at(0) == '/')
		object_name = object_name.substr(1);

	oss_result_t ret = head_object(address, bucket_name, object_name, header,
			obj_desc);
	if (ret != OSS_OK)
		return -1;

	return 0;	
}




int Oss::get_object_meta(const char* bucket, const char* name,
		OSS_FILE_META & meta) {
	string bucket_name = string(bucket);
	string object_name = string(name);
	oss_get_object_header_t header;
	oss_object_desc_t response_header;

	oss_result_t ret = head_object(address, bucket_name, object_name, header,
			response_header);
	if (ret != OSS_OK)
		return -1;

	stringstream ss, oss_type;
	oss_object_desc_t * iter = &response_header;

	/* 获取对象的Size */
	ss << iter->size;
	meta["size"] = ss.str();

	log_debug("meta size: %s", meta["size"].c_str());

	/* 判断对象的类型 
	文件夹的name形如: a/b/c/
	其他文件的name形如: a.txt a/b/c.tar
	*/
	if (object_name[object_name.size()-1] == '/')
	{
		/* 最后一个字符为'/', 说明该对象为目录对象 */
		oss_type << OSS_DIR;
		meta["type"] = oss_type.str();
	}
	else
	{
		//不是文件夹, 则为其他类型, 先看是否存在x-oss-meta-type, 如果存在则直接赋值即可
		if (iter->user_meta.find("x-oss-meta-type") != iter->user_meta.end())
		{
			//存在x-oss-meta-type字段, 说明该文件时通过cloudfs上传的, 直接赋值
			meta["type"] = iter->user_meta["x-oss-meta-type"];
		}
		else
		{
			//无x-oss-meta-type, 判断该object为OSS_REGULAR, 该操作为了兼容在OSS控制台上直接创建的文件
			oss_type << OSS_REGULAR;
			meta["type"] = oss_type.str();
		}				
	}
	
	log_debug("meta type: %s", meta["type"].c_str());

	// 处理modified time属性, 如果对象本身携带了x-oss-meta-mtime, 则直接赋值
	// 否则把文件时间设置为当前时间
	if (iter->user_meta.find("x-oss-meta-mtime") != iter->user_meta.end())
	{
		meta["mtime"] = iter->user_meta["x-oss-meta-mtime"];
	}
	else
	{
		stringstream oss_time;
		oss_time << time(0);
		meta["mtime"] = oss_time.str();
	}
	
	log_debug("meta mtime: %s", meta["mtime"].c_str());

	// 处理mode属性, 如果对象本身携带了x-oss-meta-mode, 则直接赋值
	// 否则把mode设置为0666
	if (iter->user_meta.find("x-oss-meta-mode") != iter->user_meta.end())
	{
		meta["mode"] = iter->user_meta["x-oss-meta-mode"];
	}
	else
	{
		stringstream oss_mode;
		oss_mode << AliConf::ACCESS_MODE;
		meta["mode"] = oss_mode.str();
	}
	log_debug("meta mode: %s", meta["mode"].c_str());

	return 0;
}

int Oss::put_object_meta(const char * bucket, const char * name,
		OSS_FILE_META & meta) {
	string bucket_name = string(bucket);
	string object_name = string(name);
	oss_object_desc_t put_object_desc;

	put_object_desc.name = object_name;
	put_object_desc.size = 0;
	put_object_desc.user_meta["x-oss-meta-type"] = meta["type"];
	put_object_desc.user_meta["x-oss-meta-mtime"] = meta["mtime"];
	put_object_desc.user_meta["x-oss-meta-mode"] = meta["mode"];
	oss_data_buffer_t put_data_buf = NULL;
	oss_result_t ret = put_object(address, bucket_name, put_object_desc,
			put_data_buf);

	if (ret == OSS_OK)
		return 0;
	else
		return -1;
}

string Oss::initiate_multipart_upload(const char *bucket, const char *name)
{
	string ret = "";
	unsigned short retcode = -1;			//保存服务器http返回码的解析结果;
	const char *retinfo = NULL;            //保存通过retcode获得的错误信息
	oss_client_t *client = client_initialize_with_endpoint(
		address.access_id.c_str(), address.access_key.c_str(), address.hostname.c_str());

	log_debug("access id:[%s]", address.access_id.c_str());
	log_debug("access key:[%s]", address.access_key.c_str());
	log_debug("hostname url:[%s]", address.hostname.c_str());
#if 1 /* 设置带元信息的Multipart Upload 上传请求 */

	oss_object_metadata_t *metadata = object_metadata_initialize();

	string suffix = getFileSuffix(name);
	log_debug("file:[%s],suffix:[%s]", name, suffix.c_str());

	string type = m_mime->find(suffix);
	if (type.empty())
		type.assign("application/octet-stream");
	
	log_debug("type:%s", type.c_str());
	
	/* 设置上传对象的元信息 */
	metadata->set_content_type(metadata, type.c_str());

	if (m_header->get_cache_control() != "") {
		metadata->set_cache_control(metadata, m_header->get_cache_control().c_str());
		log_debug("Cache-Control:[%s]", m_header->get_cache_control().c_str());
	}

	if (m_header->get_content_disposition() != "") {
		metadata->set_content_disposition(metadata, m_header->get_content_disposition().c_str());
		log_debug("Content-Disposition:[%s]", m_header->get_content_disposition().c_str());
	}

	if (m_header->get_content_encoding() != "") {
		metadata->set_content_encoding(metadata, m_header->get_content_encoding().c_str());
		log_debug("Content-Encoding:[%s]", m_header->get_content_encoding().c_str());
	}

	if (m_header->get_expires() != "") {
		metadata->set_expiration_time(metadata, m_header->get_expires().c_str());
		log_debug("Expires:[%s]", m_header->get_expires().c_str());
	}
	//metadata->set_cache_control(metadata, "no-cache");
	//metadata->set_content_encoding(metadata, "utf-8");
	//metadata->set_content_disposition(metadata, "attachment;");
	//metadata->set_expiration_time(metadata, "Thu, 31 Oct 2016 21:08:42 GMT");

	oss_initiate_multipart_upload_request_t *request = 
		initiate_multipart_upload_request_initialize_with_metadata(bucket, name, metadata);
#else /* 不带元信息的Multipart Upload上传请求 */
	oss_initiate_multipart_upload_request_t *request = 
		initiate_multipart_upload_request_initialize(bucket_name, key, NULL);
#endif

	//struct timeval tv1, tv2;
	//gettimeofday(&tv1, NULL);
	oss_initiate_multipart_upload_result_t *result = client_initiate_multipart_upload(client, request, &retcode);
	//gettimeofday(&tv2, NULL);
	//log_error("[initiate_multipart] timediff:[%zd]", getDiffTime(&tv2, &tv1));

	if (retcode == OK) 
	{
		ret = result->get_upload_id(result);
		log_debug("Upload ID:[%s]", result->get_upload_id(result));
	} 
	else 
	{
		retinfo = oss_get_error_message_from_retcode(retcode);
		log_error("result message:[%s]", retinfo);
	}

	if (result != NULL)
	{
		initiate_multipart_upload_result_finalize(result);
	}

	if (request != NULL)
	{
		initiate_multipart_upload_request_finalize(request);
	}

	if (metadata != NULL)
	{
		object_metadata_finalize(metadata);
	}

	if (client != NULL)
	{
		client_finalize(client);
	}	
	
	return ret;
}

string Oss::upload_part(const char *bucket, const char *name, const char *upload_id, 
		int part_num, char *buf, size_t data_len)
{
	string ret = "";
	unsigned short retcode = -1;			//保存服务器http返回码的解析结果;
	const char *retinfo = NULL;            //保存通过retcode获得的错误信息
	oss_client_t *client = client_initialize_with_endpoint(
		address.access_id.c_str(), address.access_key.c_str(), address.hostname.c_str());

	oss_upload_part_request_t *request = upload_part_request_initialize();
	request->set_bucket_name(request, bucket);
	request->set_key(request, name);
	request->set_upload_id(request, upload_id);

	request->set_part_number(request, part_num);
	request->set_input_stream(request, buf, data_len);
	request->set_part_size(request, data_len);

	//struct timeval tv1, tv2;
	//gettimeofday(&tv1, NULL);
	oss_upload_part_result_t *result = client_upload_part(client, request, &retcode);
	//gettimeofday(&tv2, NULL);
	//log_error("[upload_part] timediff:[%zd]", getDiffTime(&tv2, &tv1));
			
	if (retcode == OK) 
	{ 
		/* 打印返回信息，包括 Part Number 和 ETag 值 */
		log_debug("PartNumber:%d, ETag:%s", result->get_part_number(result), result->get_etag(result));
		ret = result->get_etag(result);
	} 
	else 
	{ 
		/* 打印出错信息 */
		retinfo = oss_get_error_message_from_retcode(retcode);
		log_error("%s", retinfo);
	}
	/* 最好及时释放 result 空间，否则回造成内存泄漏 */
	if (result != NULL)
	{
		upload_part_result_finalize(result);
	}

	if (request != NULL)
	{
		upload_part_request_finalize(request);
	}

	if (client != NULL)
	{
		client_finalize(client);
	}
	return ret;
}

string Oss::upload_part_copy(const char *bucket, const char *name, const char* s_name, 
							 const char *upload_id, int part_num, size_t start_pos, size_t end_pos)
{
	string ret = "";
	unsigned short retcode = -1;			//保存服务器http返回码的解析结果;
	const char *retinfo = NULL;            //保存通过retcode获得的错误信息
	oss_client_t *client = client_initialize_with_endpoint(
		address.access_id.c_str(), address.access_key.c_str(), address.hostname.c_str());

	oss_upload_part_request_t *request = upload_part_request_initialize();
	request->set_bucket_name(request, bucket);
	request->set_key(request, name);
	request->set_upload_id(request, upload_id);
	request->set_part_number(request, part_num);

	oss_upload_part_result_t *result = client_upload_part_copy(client, request, s_name, start_pos, end_pos, &retcode);
			
	if (retcode == OK) 
	{ 
		/* 打印返回信息，包括 Part Number 和 ETag 值 */
		log_debug("PartNumber:%d, ETag:%s", result->get_part_number(result), result->get_etag(result));
		ret = result->get_etag(result);
	}
	else
	{ 
		/* 打印出错信息 */
		retinfo = oss_get_error_message_from_retcode(retcode);
		log_error("errorinfo:%s, name:%s", retinfo, s_name);
	}
	/* 最好及时释放 result 空间，否则回造成内存泄漏 */
	if (result != NULL)
	{
		upload_part_result_finalize(result);
	}

	if (request != NULL)
	{
		upload_part_request_finalize(request);
	}

	if (client != NULL)
	{
		client_finalize(client);
	}


	return ret;
}

//成功返回正常etag,失败返回空("")etag
string Oss::complete_multipart_upload(const char *bucket, const char *name, const char *upload_id)
{
	string ret = "";
	
	unsigned short retcode = -1;			//保存服务器http返回码的解析结果;
	const char *retinfo = NULL; 		   //保存通过retcode获得的错误信息
	// int part_num_array[MAX_PART_NUM];
	// char part_etag[MAX_PART_NUM][MAX_ETAG_LENGTH + 1]; // 栈越界
	int total_part = 0;
	map<int, string> part_etag;
	map<int, long> part_size;
	
	// memset(part_num_array, 0, sizeof(part_num_array));
	// memset(part_etag, 0, sizeof(part_etag));
	//struct timeval tv1, tv2;
	//gettimeofday(&tv1, NULL);
	total_part = list_part(bucket, name, upload_id, part_etag, part_size);
	//gettimeofday(&tv2, NULL);
	//log_error("[list_part] difftime:[%zd]", getDiffTime(&tv2, &tv1));

	if (total_part < 0)
	{
		log_error("total_part = %d, upload_id = %s", total_part, upload_id);
		return ret;
	}

	oss_part_etag_t **oss_part_etag = NULL;
	if (total_part > 0)
	{
		oss_part_etag = (oss_part_etag_t **)malloc(sizeof(oss_part_etag_t *) * total_part);
		if (oss_part_etag == NULL) {
			log_debug("alloc memory failed.");
			return ret;
		}
	}
	
	int i = 0;
	map<int, string>::iterator iter;
	for (iter=part_etag.begin(); iter!=part_etag.end(); ++iter, ++i) 
	{
		//*(part_etag + i) = part_etag_initialize(i + 1, etags[i]);
		*(oss_part_etag + i) = part_etag_initialize(iter->first, iter->second.c_str());
		log_debug("part number:%d, etag:%s", iter->first, iter->second.c_str());
	}
	
	oss_client_t *client = client_initialize_with_endpoint(
		address.access_id.c_str(), address.access_key.c_str(), address.hostname.c_str());

	oss_complete_multipart_upload_request_t *request = 
		complete_multipart_upload_request_initialize(bucket, name, upload_id, oss_part_etag, total_part);

	//struct timeval tv3, tv4;
	//gettimeofday(&tv3, NULL);
	/* 完成一次Multipart Upload上传操作 */
	oss_complete_multipart_upload_result_t *result = 
		client_complete_multipart_upload(client, request, &retcode);
	//gettimeofday(&tv4, NULL);
	//log_error("[complete_multi_part] difftime:[%zd]", getDiffTime(&tv4, &tv3));

	if (retcode == OK) 
	{
		log_debug("Complete multipart upload successfully.");
		ret = result->get_etag(result);
		log_debug("ETag: %s", result->get_etag(result));
	} 
	else 
	{
		retinfo = oss_get_error_message_from_retcode(retcode);
		log_error("result message:%s", retinfo);
	}

	/* 释放空间 */
	for (int i = 0; i < total_part; i++) 
	{
		part_etag_finalize(*(oss_part_etag + i));
	}

	if (oss_part_etag) {
	 	free(oss_part_etag);
	}
	
	if (request != NULL)
	{
		complete_multipart_upload_request_finalize(request);
	}	
	if (result != NULL) 
		complete_multipart_upload_result_finalize(result);

	if (client != NULL)
	{
		client_finalize(client);
	}
	return ret;
}


void Oss::abort_multipart_upload(const char *bucket, const char *name, const char *upload_id)
{

	oss_client_t *client = client_initialize_with_endpoint(
		address.access_id.c_str(), address.access_key.c_str(), address.hostname.c_str());
	if (NULL == client)
	{
		log_error("client_initialize_with_endpoint failed");
		return;
	}

	oss_abort_multipart_upload_request_s *request = abort_multipart_upload_request_initialize(bucket, name, upload_id);
	if (NULL == request)
	{
		log_error("[%s] abort_multipart_upload_request_initialize failed, uploadid = %s", name, upload_id);
		client_finalize(client);
		return;
	}

	unsigned short retcode = 0;
	client_abort_multipart_upload(client, request, &retcode);
	if (retcode != OK)
	{
		log_error("result message:%s", oss_get_error_message_from_retcode(retcode));		
	}
	
	abort_multipart_upload_request_finalize(request);
	client_finalize(client);

	
}




int Oss::list_part(const char *bucket, const char *name, const char *upload_id, 
				   map<int, string>& part_etag, map<int, long>& part_size)
{
	int ret = 0;
	unsigned short retcode = -1;			//保存服务器http返回码的解析结果;
	const char *retinfo = NULL;            //保存通过retcode获得的错误信息
	bool truncated = true;
	unsigned int nextPartNumberMarker = 0;
	
	// map <int, string> part_num_etag;
	oss_client_t *client = client_initialize_with_endpoint(
		address.access_id.c_str(), address.access_key.c_str(), address.hostname.c_str());

	oss_list_parts_request_t *request = 
		list_parts_request_initialize(bucket, name, upload_id);
	request->set_max_parts(request, 1000);

	while (truncated)
	{
		request->set_part_number_marker(request, nextPartNumberMarker);
		
		oss_part_listing_t *listing = client_list_parts(client, request, &retcode);
		if (retcode == OK) 
		{
			int part_counts = 0;
			int i = 0;
			oss_part_summary_t **parts = listing->get_parts(listing, &part_counts);
			ret += part_counts;

			for (i = 0; i < part_counts; i++) 
			{
				log_debug("PARTNUMBER: [%d] ETAG: [%s] LENGTH[%ld]", (int)(*(parts + i))->get_part_number(*(parts + i))
																	,(*(parts + i))->get_etag(*(parts + i))
																	,(*(parts + i))->get_size(*(parts + i)));
				part_etag[(*(parts + i))->get_part_number(*(parts + i))] = (*(parts + i))->get_etag(*(parts + i));
				part_size[(*(parts + i))->get_part_number(*(parts + i))] = (*(parts + i))->get_size(*(parts + i));
			}
		}
		else 
		{
			retinfo = oss_get_error_message_from_retcode(retcode);
			log_error("%s", retinfo);

			if (listing != NULL) {
				part_listing_finalize(listing);
			}

			ret = -1;
			break;
		}

		truncated = listing->is_truncated;
		if (truncated)
			nextPartNumberMarker = listing->get_next_part_number_marker(listing);

		if (listing != NULL) {
			part_listing_finalize(listing);
		}
	}
	
	if (request != NULL) {
		list_parts_request_finalize(request);
	}

	if (client != NULL) {
		client_finalize(client);
	}
	
	return ret;
}


