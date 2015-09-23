
#ifndef ALI_UTIL_H_
#define ALI_UTIL_H_
#include<unistd.h>
#include<vector>
#include<string>
using namespace std;
//namespace afs{
string trim(const string& str);
int split(const string& str, vector<string>& ret_, string sep = ",");
string replace(const string& str, const string& src, const string& dest);
size_t sys_read_link(const char *path, char *link, size_t size);
int split_to_key_value(const string& str, vector<string>& ret_, string sep);
int atoo(const char* p);
//}
#endif
