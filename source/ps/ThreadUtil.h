/*
ThreadUtil.h - Thread Utility Functions

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

#include "lib/posix/posix_pthread.h"

#ifdef DEBUG_LOCKS

#define LOCK_MUTEX(_mutex) STMT( \
	printf("pthread_mutex_lock: 1 %p [pid:%d]\n", _mutex, pthread_self()); \
	pthread_mutex_lock(_mutex); \
	printf("pthread_mutex_lock: 2 %p [pid:%d]\n", _mutex, pthread_self()) \
)
#define UNLOCK_MUTEX(_mutex) STMT( \
	pthread_mutex_unlock(_mutex); \
	printf("pthread_mutex_unlock: %p [pid:%d]\n", _mutex, pthread_self()) \
)

#else

#define LOCK_MUTEX(_mutex) pthread_mutex_lock(_mutex)
#define UNLOCK_MUTEX(_mutex) pthread_mutex_unlock(_mutex)

#endif

//-------------------------------------------------
// Types
//-------------------------------------------------

//-------------------------------------------------
// Declarations
//-------------------------------------------------

/**
 * A Mutual Exclusion lock.
 */
class CMutex
{
public:
	inline CMutex()
	{
		pthread_mutex_init(&m_Mutex, NULL);
	}
	
	inline ~CMutex()
	{
		if (pthread_mutex_destroy(&m_Mutex) != 0)
		{
			Unlock();
			pthread_mutex_destroy(&m_Mutex);
		}
	}

	/**
	 * Atomically wait for the mutex to become unlocked, then lock it.
	 */
	inline void Lock()
	{
		LOCK_MUTEX(&m_Mutex);
	}

	/**
	 * Unlock the mutex.
	 */
	inline void Unlock()
	{
		UNLOCK_MUTEX(&m_Mutex);
	}

	pthread_mutex_t m_Mutex;
};

// CScopeLock
// ---------------------------------------------------------------------| Class
// Locks a CMutex over the objects lifetime
class CScopeLock
{
	NONCOPYABLE(CScopeLock);
public:
	inline CScopeLock(pthread_mutex_t &mutex): m_Mutex(mutex)
	{
		LOCK_MUTEX(&m_Mutex);
	}
	
	inline CScopeLock(CMutex &mutex): m_Mutex(mutex.m_Mutex)
	{
		LOCK_MUTEX(&m_Mutex);
	}
	
	inline ~CScopeLock()
	{
		UNLOCK_MUTEX(&m_Mutex);
	}

private:
	pthread_mutex_t &m_Mutex;
};

// CLocker
// ---------------------------------------------------------------------| Class
// This will not give access to the wrapped class constructors directly,
// to use them you have to do CLocker(CWrappedClass(args))
template <typename _T>
struct CLocker: public CMutex, public _T
{
public: 
	inline CLocker()
	{}

	/*
	GCC doesn't take these... I don't understand what the problem is! // Simon
	
	inline CLocker(const _T &arg): _T(arg)
	{}
	
	inline CLocker(_T &arg): _T(arg)
	{}*/
};

#endif
