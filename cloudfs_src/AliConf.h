

#ifndef ALICONF_H_
#define ALICONF_H_
#include <string>
using namespace std;
//namespace afs {
class AliConf {
public:
    AliConf();
    virtual ~AliConf();
public:
    static string BUCKET;
    static string HOST;
    static string KEY;
    static string ID;
    static size_t FILE_NAME_LEN;
    static size_t BLOCK_SIZE;
	static size_t ONLINE_SYNC_CYCLE;
	static size_t LOG_LEVEL;
	static string SYMLINK_TYPE_NAME;
	static size_t IMMEDIATE_SYNC;
	static size_t ACCESS_MODE;
	static size_t MAX_UPLOAD_THREADS;
	static size_t USE_SQLITE;
	static string SQLITE_FILE_PATH;
	
public:
    static void INIT();
protected:
    static void INIT_VARS(string &key, string &value);
};
//}
#endif /* ALICONF_H_ */
