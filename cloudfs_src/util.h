#ifndef _OSS_API_UTIL_H_
#define _OSS_API_UTIL_H_

#include <string.h>
#include <openssl/evp.h>
#include <pwd.h>
#include <grp.h>
#include <string>
#include <sstream>
#include <map>
#include <list>
#include "log.h"
#include <sys/time.h>

#define SPACES " \t\r\n"

using namespace std;

template<typename T> string str(T value) {
      stringstream s;
      s << value;
      return s.str();
}

static const EVP_MD* evp_md = EVP_sha1();

string trim_left(const string &s, const string &t = SPACES);
string trim_right(const string &s, const string &t = SPACES);
string trim(const string &s, const string &t = SPACES);
string lower(string s);
string int_to_str(int);
string get_date();
string url_encode(const string &s);
string prepare_url(const char* url, string bucket_name);
string calc_signature(const string & method, const string & access_id, const string & access_key,
		const string & date, const string & resource, map<string, string> & headers);

int create_dir(const char* pathname);
string getFileSuffix(const char* name);

size_t getDiffTime(struct timeval* tv1, struct timeval* tv2);

class UploadIdFile
{
public:
    UploadIdFile(const char *pPath="/var/log/cloudfs/uploadid_list");
    ~UploadIdFile();
    int AddUploadId(string uploadId, string filename);
    int RemoveUploadId(string uploadId, string filename);
    list<string>& GetUploadList();
    void ClearUploadList();
protected:
    void sync2file();
private:
    string m_FilePath;
    list<string> m_uploadid_list;
    pthread_mutex_t m_uploadid_mutex;		//多线程增加或删除uploadid时的线程锁
};



#endif // _OSS_API_UTIL_H_

