#ifndef _OSS_HEADER__H__
#define _OSS_HEADER__H__

#include <map>
#include <string>
using namespace std;


class OssHeader
{
public:
	OssHeader(const char* headerCfgPath = "./conf/header_params");
	int ReadFile(string& text);
	int init();

	string get_cache_control();
	string get_content_disposition();
	string get_content_encoding();
	string get_expires();
private:
	string m_header_path;
	
	string m_cache_control;
	string m_content_disposition;
	string m_content_encoding;
	string m_expires;
};

#endif

