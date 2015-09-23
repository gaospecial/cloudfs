#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <utime.h>
#include <getopt.h>
#include <libgen.h>
#include <openssl/buffer.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/bio.h>
#include "util.h"
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <sys/stat.h>


using namespace std;

static const char hexAlphabet[] = "0123456789ABCDEF";

string lower(string s) {
	// change each character of the string to lower case
	for(unsigned int i = 0; i < s.length(); i++)
		s[i] = tolower(s[i]);

	return s;
}

string int_to_str(int n) {
	stringstream result;
	result << n;
	return result.str();
}


string trim_left(const string &s, const string &t /* = SPACES */) {
	string d(s);
	return d.erase(0, s.find_first_not_of(t));
}

string trim_right(const string &s, const string &t /* = SPACES */) {
	string d(s);
	string::size_type i(d.find_last_not_of(t));
	if (i == string::npos)
		return "";
	else
		return d.erase(d.find_last_not_of(t) + 1);
}

string trim(const string &s, const string &t /* = SPACES */) {
	string d(s);
	return trim_left(trim_right(d, t), t);
}

string url_encode(const string &s) {
//在调试包含中文字符的object name时发现此处不需要进行URL编码
//需要继续跟进分析

	string result;
	for (unsigned i = 0; i < s.length(); ++i) {
		if (s[i] == '/') // Note- special case for fuse paths...
			result += s[i];
		else if (isalnum(s[i]))
			result += s[i];
		else if (s[i] == '.' || s[i] == '-' || s[i] == '*' || s[i] == '_')
			result += s[i];
		else if (s[i] == ' ')
			result += '+';
		else {
			result += "%";
			result += hexAlphabet[static_cast<unsigned char>(s[i]) / 16];
			result += hexAlphabet[static_cast<unsigned char>(s[i]) % 16];
		}
	}

	return result;

/*
	return s;
*/
}

string prepare_url(const char* url, string bucket)
{
	// string url_str = str(url);
	// string first_part = url_str.substr(0, 7);
	// string last_part = url_str.substr(7, url_str.size() - 1);
	// url_str = first_part + bucket + "." + last_part;

	return str(bucket + "." + url);
}

string get_date()
{
	char buf[100];
	struct tm gmt;
	time_t t = time(0);
	gmtime_r(&t, &gmt);
	strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &gmt);
	return buf;
}

string calc_signature(const string & method, const string & access_id, const string & access_key,
		const string & date, const string & resource, map<string, string> & headers)
{
	string signature = "";
	string str_to_sign = "";

	str_to_sign += method + "\n";
	str_to_sign += "\n"; // emtpy md5
	str_to_sign += headers["Content-Type"] + "\n"; // content-type
	str_to_sign += date + "\n";

	map<string, string>::iterator headers_map_iter = headers.begin();
	vector<string> oss_header_keys;
	for (; headers_map_iter != headers.end(); headers_map_iter++)
	{
		if (headers_map_iter->first.find("x-oss-") == 0)
		{
			oss_header_keys.push_back(headers_map_iter->first);
		}
	}
	// Sort Oss headers to be canonicalized
	sort(oss_header_keys.begin(), oss_header_keys.end());
	vector<string>::iterator sorted_header_keys_iter = oss_header_keys.begin();
	for (; sorted_header_keys_iter != oss_header_keys.end(); sorted_header_keys_iter++)
	{
		str_to_sign += *sorted_header_keys_iter + ":" + headers[*sorted_header_keys_iter] + "\n";
	}
	str_to_sign += resource;

	// Finish prepare string to sign
	// Start to call HMAC and Base64

	const void * key = access_key.data();
	int key_len = access_key.size();
	const unsigned char * d = (const unsigned char *) str_to_sign.data();
	int n = str_to_sign.size();
	unsigned int md_len;
	unsigned char md[EVP_MAX_MD_SIZE];
	int ret, bytes_written, offset, write_attempts = 0;

	HMAC(evp_md, key, key_len, d, n, md, &md_len);

	BIO* b64 = BIO_new(BIO_f_base64());
	BIO* bmem = BIO_new(BIO_s_mem());
	b64 = BIO_push(b64, bmem);

	offset = 0;
	for (;;) {
		bytes_written = BIO_write(b64, &(md[offset]), md_len);
		write_attempts++;
		//  -1 indicates that an error occurred, or a temporary error, such as
		//  the server is busy, occurred and we need to retry later.
		//  BIO_write can do a short write, this code addresses this condition
		if (bytes_written <= 0) {
			//  Indicates whether a temporary error occurred or a failure to
			//  complete the operation occurred
			if ((ret = BIO_should_retry(b64))) {

				// Wait until the write can be accomplished
				if(write_attempts <= 10) {
					continue;
				} else {
					// Too many write attempts
					BIO_free_all(b64);
					signature.clear();
					return signature;
				}
			} else {
				// If not a retry then it is an error
				BIO_free_all(b64);
				signature.clear();
				return signature;
			}
		}

		// The write request succeeded in writing some Bytes
		offset += bytes_written;
		md_len -= bytes_written;

		// If there is no more data to write, the request sending has been
		// completed
		if (md_len <= 0) {
			break;
		}
	}

	// Flush the data
	ret = BIO_flush(b64);
	if ( ret <= 0) {
		BIO_free_all(b64);
		signature.clear();
		return signature;
	}

	BUF_MEM *bptr;

	BIO_get_mem_ptr(b64, &bptr);

	signature.resize(bptr->length - 1);
	memcpy(&signature[0], bptr->data, bptr->length-1);

	BIO_free_all(b64);
	return signature;
}


int IsFolderExist(const char* path)
{
    DIR *dp;
    if ((dp = opendir(path)) == NULL)
    {
        return 0;
    }

    closedir(dp);
    return 1;
}


size_t getDiffTime(struct timeval* tv1, struct timeval* tv2)
{
	size_t t1 = tv1->tv_sec * 1000000 + tv1->tv_usec;
	size_t t2 = tv2->tv_sec * 1000000 + tv2->tv_usec;

	return t1 - t2;
}


int create_dir(const char* pathname)
{
	int i;

	if (1 == IsFolderExist(pathname))
		return 0;
	
	char* dirname = strdup(pathname);
	int len = strlen(dirname);
	
	if (dirname[len - 1] != '/')
        strcat(dirname, "/");

	len = strlen(dirname);
	for(i = 1; i < len; i++)
	{
        if(dirname[i] == '/')
        {
            dirname[i] = 0;
            if(access(dirname, 0) != 0)
            {
                if(mkdir(dirname, 0755) == -1)
                {
                    perror("mkdir   error");
					free(dirname);
                    return -1;
                }
            }
            dirname[i] = '/';
        }
	}

	free(dirname);
	return 0;
}


string getFileSuffix(const char* name)
{
	string filename;
	string suffix;
	string error = "";

	string path = name;
	if (path.empty())
		return error;

	string::size_type pos = path.rfind("/");
	if (pos == string::npos) {
		filename = path;
	}
	else {
		filename = path.substr(pos + 1);
	}

	if (filename.empty())
		return error;

	pos = filename.rfind(".");
	if (pos == string::npos) {
		return error;
	}

	suffix = filename.substr(pos + 1);
	if (suffix.empty())
		return error;

	return suffix;;
}


UploadIdFile::UploadIdFile(const char *path)
{

	/* 初始化操作锁 */
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_uploadid_mutex,&mutexattr);
	pthread_mutexattr_destroy(&mutexattr);


    m_FilePath.assign(path);

    /* 打开文件, 并load文件中的数据 */
    FILE *tmpFile  = fopen(path, "r");
    if (NULL == tmpFile)
    {
        log_debug("UploadId File not exist");		
        return;
    }

    /* 加载数据 */
    char TmpBuf[1024];
    while(fgets(TmpBuf, 1024, tmpFile) != NULL)
    {
        m_uploadid_list.push_back(string(TmpBuf, 0, (strlen(TmpBuf)-1)));
    }

    /* 关闭文件 */
    fclose(tmpFile);
}


UploadIdFile::~UploadIdFile()
{
	/* 删除线程锁资源 */
	pthread_mutex_destroy(&m_uploadid_mutex);
	
    m_uploadid_list.clear();
}

int UploadIdFile::AddUploadId(string uploadId, string filename)
{
    string value(uploadId + ":" + filename);

    pthread_mutex_lock(&m_uploadid_mutex);
    m_uploadid_list.push_back(value);
    sync2file();
    pthread_mutex_unlock(&m_uploadid_mutex);
    return 0;
}

int UploadIdFile::RemoveUploadId(string uploadId, string filename)
{
	string value(uploadId + ":" + filename);
	pthread_mutex_lock(&m_uploadid_mutex);
    m_uploadid_list.remove(value);
    sync2file();
	pthread_mutex_unlock(&m_uploadid_mutex);
    return 0;
}

list<string>& UploadIdFile::GetUploadList()
{
    return m_uploadid_list;
}

void UploadIdFile::ClearUploadList()
{
	pthread_mutex_lock(&m_uploadid_mutex);
	m_uploadid_list.clear();
	sync2file();
	pthread_mutex_unlock(&m_uploadid_mutex);
}


void UploadIdFile::sync2file()
{
    /* 打开文件, 并load文件中的数据, 使用"w"选项打开, 会清空原有数据 */
    FILE *tmpFile = fopen(m_FilePath.c_str(), "w");
    if (NULL == tmpFile)
    {
            log_error("open file %s failed", m_FilePath.c_str());
            return;
    }

    list<string>::iterator i;
    string tmp;
    for (i = m_uploadid_list.begin(); i != m_uploadid_list.end(); ++i)
    {
            tmp = *i + "\n";
            fputs(tmp.c_str(), tmpFile);

    }

    fflush(tmpFile);
    fclose(tmpFile);
}

/*
使用代码范例
int main()
{
        UploadIdFile test;

        test.AddUploadId("1234567");
        list<string> str = test.GetUploadList();
        for (list<string>::iterator i = str.begin(); i != str.end(); i++)
        {
                cout << *i << endl;
        }
        test.RemoveUploadId("1234567");

        return 0;
}

*/



