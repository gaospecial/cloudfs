// LockBase.cpp: implementation of the CLockBase class.
//
//////////////////////////////////////////////////////////////////////

#include "LockBase.h"
#include <assert.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLockBase::CLockBase()
{
	m_bLock = false;
	m_bInit = false;
}
CLockBase::CLockBase(bool bInit)
{
	m_bLock = false;
	m_bInit = false;
	Enable(bInit);
}

CLockBase::~CLockBase()
{
	Clean();
}

void CLockBase::Enable(bool bLock)
{

	m_bLock = bLock;
	if(bLock)
	{
		if(!m_bInit)
		{
			pthread_mutexattr_t mutexattr;
			pthread_mutexattr_init(&mutexattr);
			pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
			pthread_mutex_init(&m_sec,&mutexattr);
			m_bInit = true;

			pthread_mutexattr_destroy(&mutexattr);

		}
	}else
	{
		if(m_bInit)
		{
			pthread_mutex_destroy(&m_sec);
			m_bInit = false;
		}
	}

}

void CLockBase::Lock()
{
	if(m_bLock)
	{
		assert(m_bInit);
		pthread_mutex_lock(&m_sec);
	}
}

void CLockBase::Unlock()
{
	if(m_bLock)
	{
		assert(m_bInit);
		pthread_mutex_unlock(&m_sec);
	}
}

void CLockBase::Clean()
{
	if(m_bInit)
	{
		pthread_mutex_destroy(&m_sec);
		m_bInit = false;
	}	
}
