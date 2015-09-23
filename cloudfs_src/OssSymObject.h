/*
 * OssSymObject.h
 *
 *  Created on: Sep 30, 2012
 *      Author: weil
 */

#ifndef OSSSYMOBJECT_H_
#define OSSSYMOBJECT_H_

#include "OssObject.h"
#include <stdlib.h>
//namespace afs {
class OssSymObject: public OssObject {
public:
    OssSymObject(OssFS *fs, Oss *oss, const char *bucket,
            const char *pathname, const char *filename, OssStats * stat);
            
    OssSymObject(OssFS *fs, Oss *oss, const char *bucket,
            const char *pathname, const char *filename, const char *link, OssStats * stat);            
    virtual ~OssSymObject();
protected:
    virtual int destory();
public:
    void set_link_obj(const char *link,size_t size);
    void get_link_obj(char *link, size_t size);
    int delete_oss_object();
    //virtual const OssStats *get_stats();
protected:
    char *m_link_name;
};
//}
#endif /* OSSSYMOBJECT_H_ */
