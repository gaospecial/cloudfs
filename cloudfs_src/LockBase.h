// LockBase.h: interface for the CLockBase class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __YUNYU_LOCK_BALSE_H__
#define __YUNYU_LOCK_BALSE_H__


#include "pthread.h"

class CLockBase  
{
public:
	void Unlock();
	void Lock();
	void Enable(bool bLock = true);
	void Clean();
	CLockBase();
	CLockBase(bool bInit);
	virtual ~CLockBase();
protected:
	bool m_bLock;
	bool m_bInit;
#ifdef WIN32
	CRITICAL_SECTION m_sec;
#else
	pthread_mutex_t m_sec;
	pthread_mutex_t &GetHandle(){return m_sec;};
#endif
};
#endif 
