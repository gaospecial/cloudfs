

#ifndef OSSDIROBJECT_H_
#define OSSDIROBJECT_H_

#include "OssObject.h"
//namespace afs {
class OssDirObject: public OssObject {
public:
    OssDirObject(OssFS *fs,Oss *oss,const char *bucket,const char *pathname,const char *filename,OssStats * stat);
    virtual ~OssDirObject();
	void rdlock();
	void wrlock();
	void unlock();
	int delete_oss_object();
	inline std::map<const char *, OssObject *, map_str_comparer> *get_subs() 
	{
		return &m_subs;
	}

	int add_record(const char *path, OssObject *obj)
	{
		wrlock();
		m_subs.insert(std::pair<const char *, OssObject *>(path, obj));
		unlock();
		return 0;
	}

	int remove_record(const char *path)
	{
		std::map<const char *, OssObject *, map_str_comparer>::iterator it;
		wrlock();
		it = m_subs.find(path);
		if (it == m_subs.end())
		{
			//ц╩спур╣╫
			unlock();
			return -1;
		}
		OssObject *pTmp = it->second;
		m_subs.erase(it);
		SAFE_DELETE(pTmp);
		unlock();
		return 0;		
	}

	OssObject * get_record(const char *path)
	{
		std::map<const char *, OssObject *, map_str_comparer>::iterator it;
		OssObject *pResult = NULL;
		rdlock();
		it = m_subs.find(path);
		if (it == m_subs.end())
		{
			pResult = NULL;
		}
		else
		{
			pResult = it->second;
		}
		unlock();
		return pResult;		
	}

	
	
protected:
    virtual int destory();


protected:
    std::map<const char *, OssObject *, map_str_comparer> m_subs;
	pthread_rwlock_t m_rwlock;
};
//}
#endif /* OSSDIROBJECT_H_ */
