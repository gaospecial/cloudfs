
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include "OssStats.h"
#include "AliConf.h"

//using namespace afs;
OssStats::OssStats(OSS_FILE_META &meta) {
	// TODO Auto-generated constructor stub
	OSS_FILE_META::iterator iter;
	for (iter = meta.begin(); iter != meta.end(); iter++) {
		string key = iter->first;
		string value = iter->second;
		if (key == "size") {
			size = atoll(value.c_str());
		} else if (key == "type") {
			//这里oss_obj_type_t是枚举类型, 必须使用atoi
			type = (oss_obj_type_t) atoi(value.c_str());
		} else if (key == "mtime") {
			mtime = atol(value.c_str());
		} else if (key == "mode") {
			mode = AliConf::ACCESS_MODE;
		}
	}
}

OssStats::OssStats(size_t size, time_t mtime, oss_obj_type_t type) {
	this->size = size;
	this->mtime = mtime;
	this->type = type;
	this->mode = AliConf::ACCESS_MODE;
}

OssStats::~OssStats() {
	// TODO Auto-generated destructor stub
}
int OssStats::to_meta(OSS_FILE_META & meta) const {
	stringstream s_size, s_type, s_mode, s_mtime;
	s_size << size;
	s_type << type;
	s_mode << mode;
	s_mtime << mtime;
	meta["size"] = s_size.str();
	meta["mtime"] = s_mtime.str();
	meta["type"] = s_type.str();
	meta["mode"] = s_mode.str();
	return 0;
}
