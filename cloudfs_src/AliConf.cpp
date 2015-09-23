

#include "AliConf.h"
#include "AliUtil.h"
#include <iostream>
#include <fstream>
#include <string>
#include "Ali.h"
using namespace std;
//using namespace afs;
//#define ALI_CONF_FILE "/etc/cloudfs/cloudfs.conf"
#define ALI_CONF_FILE "./conf/cloudfs.conf"

//将HOST/ID/KEY/BUCKET的值设置为空, 如果配置文件中未配置, 则直接在运行界面报错
string AliConf::HOST = "";
string AliConf::ID = "";
string AliConf::KEY = "";
string AliConf::BUCKET = "";

size_t AliConf::FILE_NAME_LEN = 0;
size_t AliConf::BLOCK_SIZE = DEFAULT_BLOCK_SIZE;
size_t AliConf::ONLINE_SYNC_CYCLE = DEFAULT_ONLINE_SYNC_CYCLE;
size_t AliConf::LOG_LEVEL = DEFAULT_LOG_LEVEL;
string AliConf::SYMLINK_TYPE_NAME = "1s2l3k";
size_t AliConf::IMMEDIATE_SYNC = DEFAULT_IMMEDIATE_SYNC;
size_t AliConf::ACCESS_MODE = 0666;
size_t AliConf::MAX_UPLOAD_THREADS = 1;
string AliConf::SQLITE_FILE_PATH = "/var/log/cloudfs/cloudfs_meta.db";
size_t AliConf::USE_SQLITE = 0;


void cloudfs_print_version_info()
{
	//打印产品信息
	cout << "---------------------------------------------------------------------------------" << endl;
    cout << "Welcome to use cloudfs. Cloudfs is developped and maintaind by Yunyu Technology" << endl;
    cout << "Version :" << CLOUDFS_VERSION_INFO << endl;
    cout << "---------------------------------------------------------------------------------" << endl;
}


void cloudfs_check_config()
{
	if ("" == AliConf::HOST)
	{
		cout << "ERROR: HOST is not configured, please check cloudfs.conf" << endl;
		exit(-1);
	}
	
	
	if ("" == AliConf::ID)
	{
		cout << "ERROR: ID is not configured, please check cloudfs.conf" << endl;
		exit(-1);			
	}
	
	if ("" == AliConf::KEY)
	{
		cout << "ERROR: KEY is not configured, please check cloudfs.conf" << endl;
		exit(-1);				
	}
	
	if ("" == AliConf::BUCKET)
	{
		cout << "ERROR: BUCKET is not configured, please check cloudfs.conf" << endl;
		exit(-1);				
	}		 

}


AliConf::AliConf() {
    // TODO Auto-generated constructor stub

}

AliConf::~AliConf() {
    // TODO Auto-generated destructor stub
}
void AliConf::INIT() {	
	cloudfs_print_version_info();
	
	string line;
    ifstream conf_file(ALI_CONF_FILE);
    if (conf_file.is_open()) {
    	cout << endl << "Configuration Information Start" << endl;
        while (conf_file.good()) {
            getline(conf_file, line);
            line = trim(line);
            //remove the comments and blank line
            if (line.size() == 0 || line.at(0) == '#' || line.at(0) == '[') {
                continue;
            }
            vector<string> results;
            //split(line, results, "=");
			//这里需要使用split_to_key_value, 如果value里面带有=符号，原来的split不能处理
			split_to_key_value(line, results, "=");
            //remove all the lines not in a=b format
            if (results.size() != 2) {
                continue;
            }
            results[0] = trim(results[0]);
            results[1] = trim(results[1]);
            AliConf::INIT_VARS(results[0], results[1]);
            cout << line << endl;
        }
        conf_file.close();
        cout << "Configuration Information End" << endl << endl;

        cloudfs_check_config();
    }
}
void AliConf::INIT_VARS(string &key, string &value) {
    if (key == "BUCKET") {
        AliConf::BUCKET = value;
    } else if (key == "ID") {
        AliConf::ID = value;
    } else if (key == "KEY") {
        AliConf::KEY = value;
    } else if (key == "HOST") {
        AliConf::HOST = value;
    } else if (key == "FILE_NAME_LEN") {
        AliConf::FILE_NAME_LEN = atoll(value.c_str());
    } else if (key == "BLOCK_SIZE") {
        AliConf::BLOCK_SIZE = atoll(value.c_str());
    } else if (key == "ONLINE_SYNC_CYCLE") {
    	AliConf::ONLINE_SYNC_CYCLE = atoi(value.c_str());
	} else if (key == "LOG_LEVEL") {
		AliConf::LOG_LEVEL = atoi(value.c_str());
	} else if (key == "SYMLINK_POSTFIX"){
		AliConf::SYMLINK_TYPE_NAME = value;
	} else if (key == "IMMEDIATE_SYNC") {
		AliConf::IMMEDIATE_SYNC = atoi(value.c_str());
	} else if (key == "ACCESS_MODE") {
		AliConf::ACCESS_MODE = atoo(value.c_str());
	} else if (key == "MAX_UPLOAD_THREADS") {
		AliConf::MAX_UPLOAD_THREADS = atoi(value.c_str());
	}
	else if (key == "USE_SQLITE") {
		AliConf::USE_SQLITE = atoi(value.c_str());
	}
	else if (key == "SQLITE_FILE_PATH") {
		AliConf::SQLITE_FILE_PATH = value;
	}
	else {
        cout << "Unknow configuration: [" << key << "=" << value << "] ignored"<<endl;
    }
}
