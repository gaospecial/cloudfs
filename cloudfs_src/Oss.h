
#ifndef OSS_H_
#define OSS_H_
#include <map>
#include "oss_api.h"
#include "OssMime.h"
#include "OssHeader.h"


using namespace std;
typedef map<string, string> OSS_FILE_META;
typedef map<string, OSS_FILE_META> OSS_FILE_MAP;

#define MAX_PART_NUM 10000
#define MAX_ETAG_LENGTH 1024

//namespace ali {
class Oss {
public:
	Oss();
	virtual ~Oss();
public:
	int init(const char *host, const char *id, const char *key);
	int get_bucket(const char *bucket, const char *prefix, const char *delimiter, vector<oss_object_desc_t> &object_list);

	int get_object_list(const char *bucket, OSS_FILE_MAP &objects);
    int copy_object_data(const char *bucket, const char *old_name, const char *new_name);
	int put_object_data(const char *bucket, const char *name, const char *buf,
			OSS_FILE_META & meta);
	int put_object_data(const char *bucket, const char *name,
			OSS_FILE_META & meta);

	int put_object_data(const char *bucket, oss_object_desc_t &put_object_desc, const char *buf);
	
	int get_object_data(const char *bucket, const char *name, char *buf,
			size_t size);

	unsigned int get_object_data(const char *bucket, const char *name, char *buf,
			int start_pos, size_t size);
			
	int del_object(const char *bucket, const char *name, bool dir);
	
	int get_object_meta(const char *bucket, const char *name,
			OSS_FILE_META & meta);
			
	int get_object_meta(const char* bucket, const char* name,
			oss_object_desc_t & obj_desc);
			
	int put_object_meta(const char *bucket, const char *name,
			OSS_FILE_META & meta);

	//成功返回正常uploadId,失败返回空("")uploadId
	string initiate_multipart_upload(const char *bucket, const char *name);

	//成功返回正常etag,失败返回空("")etag
	string upload_part(const char *bucket, const char *name, const char *upload_id, 
		int part_num, char *buf, size_t data_len);

	//成功返回正常etag,失败返回空("")etag
	//如果end_pos为0则copy整个object为一个part
	string upload_part_copy(const char *bucket, const char *name, const char* s_name, 
							const char *upload_id, int part_num, size_t start_pos, size_t end_pos);

	//成功返回正常etag,失败返回空("")etag
	string complete_multipart_upload(const char *bucket, const char *name, const char *upload_id);

	//成功返回正在multiupload的任务个数,uploadId保存在upload_id_array中
	int list_multipart_upload(const char *bucket, const char *name, char *upload_id_array[]);

	//成功返回part的个数,part_num和etag分别保存在part_num_array和part_etag中
	int list_part(const char *bucket, const char *name, const char *upload_id, 
				  map<int, string>& part_etag, map<int, long>& part_size);

	int get_object_data_with_range(const char *bucket, const char *name, char *buf,
										size_t startPos, size_t size);

	void abort_multipart_upload(const char *bucket, const char *name, const char *upload_id);


private:
	oss_address_t address;
	OssMime* m_mime;
	OssHeader* m_header;
};
//}
#endif /* OSS_H_ */
