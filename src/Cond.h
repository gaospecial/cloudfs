#ifndef _cond_h
#define _cond_h


#include "LockBase.h"
class CCond : public CLockBase 
{
public:
	bool Wait();
	bool Wait(unsigned int uMilliSecond);
	void Signal();
	void UnSignal();
	void Broadcast();
	pthread_cond_t &GetCondHandle();
public:
	CCond();
	virtual ~CCond();
protected:
	pthread_cond_t m_cond;
};

#endif
