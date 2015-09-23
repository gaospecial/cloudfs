#include "Cond.h"
#include <sys/time.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCond::CCond()
{
	Enable();
	Lock();
	pthread_cond_init(&m_cond,NULL);
	Unlock();
}

CCond::~CCond()
{
	Lock();
	pthread_cond_destroy(&m_cond);
	Unlock();
}

bool CCond::Wait()
{
	int ret = 0;
	Lock();
	ret = pthread_cond_wait(&m_cond, &GetHandle());
	Unlock();
	return ret == 0;
}

bool CCond::Wait(unsigned int uMilliSecond)
{
	unsigned int tv_sec	=	uMilliSecond / 1000;						
	unsigned int tv_usec	=	(uMilliSecond % 1000) * 1000;	

//	cout<<tv_sec<<"+"<<tv_usec<<endl;
	
	struct timeval tv = {0};
	gettimeofday(&tv, NULL);

	struct timespec abstime = {0};

	abstime.tv_sec  = tv.tv_sec + tv_sec;					
    unsigned long long temp = tv.tv_usec + tv_usec ;

	abstime.tv_sec += temp / 1000000;
	abstime.tv_nsec = (temp % 1000000 ) * 1000;

    int nRet = 0;
    Lock();
	nRet = pthread_cond_timedwait(&m_cond, &GetHandle(), &abstime);
	Unlock();
	return nRet == 0;	
}

void CCond::Signal()
{
	Lock();
 	pthread_cond_signal(&m_cond);
 	Unlock();
}
void CCond::UnSignal()
{
}

void CCond::Broadcast()
{
	Lock();
 	pthread_cond_broadcast(&m_cond);
	Unlock();
}
pthread_cond_t &CCond::GetCondHandle()
{
	return m_cond;
}
