/*
ThreadUtil.h - Thread Utility Functions
by Simon Brenner
simon.brenner@home.se

--Overview-- 

Contains three classes: CMutex, CRWLock and CLocker.
CMutex is a Mutual Exclusion lock that can be locked and unlocked. The Lock
function waits for the current lock holder (if any) to release the lock and
then acquires it.

CRWLock is a lock where any number of readers *or* one writer can hold the lock.

CLocker is a generic wrapper class that can wrap any data class with any locker
class.

--Example--

CMutex usage:

	CMutex protectData;

	void function()
	{
		protectData.Lock();
		.. do stuff with data ..
		protectData.Unlock();
	}

CLocker usage:

	class CDataClass {};
	CLocker<CDataClass> instance;
	
	void function()
	{
		instance.Lock();
		.. do stuff with instance ..
		instance.Unlock();
	}

CLocker usage 2:

	class CDataClass {};
	class CCustomLockerClass { void MyOwnLock(); void MyOwnUnlock(); };
	
	CLocker<CDataClass, CCustomLockerClass> instance;
	
	void function()
	{
		instance.MyOwnLock();
		.. do stuff with instance ..
		instance.MyOwnUnlock();
	}

--More Info--


*/

#ifndef _ThreadUtil_h
#define _ThreadUtil_h

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------

#include "posix.h"

//-------------------------------------------------
// Types
//-------------------------------------------------

//-------------------------------------------------
// Declarations
//-------------------------------------------------

// CScopeLock
// ---------------------------------------------------------------------| Class
// Locks a CMutex over the objects lifetime
class CScopeLock
{
public:
	inline CScopeLock(pthread_mutex_t &mutex): m_Mutex(mutex)
	{
		pthread_mutex_lock(&m_Mutex);
	}
	inline ~CScopeLock()
	{
		pthread_mutex_unlock(&m_Mutex);
	}

private:
	pthread_mutex_t &m_Mutex;
};

// CLocker
// ---------------------------------------------------------------------| Class
// This will not give access to the wrapped class constructors directly,
// to use them you have to do CLocker(CWrappedClass(args))

template <typename _T>
struct CLocker: public _T
{
	inline CLocker()
	{}

	inline CLocker(const _T &arg): _T(arg)
	{}
	
	inline CLocker(_T &arg): _T(arg)
	{}

	inline void Lock()
	{
		pthread_mutex_lock(&m_Mutex);
	}

	inline void Unlock()
	{
		pthread_mutex_unlock(&m_Mutex);
	}

	pthread_mutex_t m_Mutex;
};

#endif
