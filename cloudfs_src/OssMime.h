#ifndef _OSS_MIME__H__
#define _OSS_MIME__H__

#include <map>
#include <string>
using namespace std;

typedef map<string, string> SS_Map;
typedef map<string, string>::iterator SS_Map_Iter;


class OssMime
{
public:
	OssMime(const char* mimeCfgPath = "./conf/mime.types");
	int ReadFile(string& text);
	int init();
	string find(string key);
private:
	string m_mime_path;
	SS_Map m_mimes;
};

#endif

