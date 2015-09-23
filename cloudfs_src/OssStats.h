

#ifndef OSSSTATS_H_
#define OSSSTATS_H_
#include <sys/types.h>
#include <time.h>
#include "Oss.h"
#include "Ali.h"
//namespace afs {
class OssStats {
public:
    OssStats(OSS_FILE_META &meta);
    OssStats(size_t size, time_t mtime, oss_obj_type_t type);
    virtual ~OssStats();
    int to_meta(OSS_FILE_META & meta) const;
    void set_size(size_t new_size){ size = new_size; }
	size_t get_size() { return size; }
public:
    size_t size;
    time_t mtime;
    mode_t mode;
    oss_obj_type_t type;
};
//}
#endif /* OSSSTATS_H_ */
