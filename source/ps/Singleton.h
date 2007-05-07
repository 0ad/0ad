/**
 * =========================================================================
 * File        : Singleton.h
 * Project     : 0 A.D.
 * Description : template base class for Singletons
 * =========================================================================
 */

/*
USAGE: class myClass : Singleton<myClass>{};

Modified from http://gamedev.net/reference/articles/article1954.asp
*/
            
#ifndef INCLUDED_SINGLETON
#define INCLUDED_SINGLETON

#include "lib/debug.h"

template<typename T>
class Singleton
{
	static T* ms_singleton;

public:
	Singleton()
	{
		debug_assert( !ms_singleton );
		ms_singleton = static_cast<T*>(this);
	}

	~Singleton()
	{
		debug_assert( ms_singleton );
		ms_singleton = 0;
	}

	static T& GetSingleton()
	{
		debug_assert( ms_singleton );
		return *ms_singleton;
	}

	static T* GetSingletonPtr()
	{
		debug_assert( ms_singleton );
		return ms_singleton;
	}

	static bool IsInitialised()
	{
		return (ms_singleton != 0);
	}
};

template <typename T>
T* Singleton<T>::ms_singleton = 0;

#endif
