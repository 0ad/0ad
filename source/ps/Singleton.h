//////////////////////////////////////////////////////////////////////////////
//    AUTHOR: Michael Reiland
//  FILENAME: Singleton.h
//   PURPOSE: Provides a base template class for Singletons
//
//    USEAGE: class myClass : Singleton<myClass>{};
//
//      INFO: This implementation was originally copied from: 
//
//                           Enginuity, Part II 
//            Memory Management, Error Logging, and Utility Classes; 
//                 or, How To Forget To Explode Your Underwear 
//                        by Richard "superpig" Fine
//
//            hosted at Gamedev.net at
//            http://gamedev.net/reference/articles/article1954.asp
//
//  MODIFIED: 07.09.2003 mreiland
#ifndef _TEMPLATE_SINGLETON
#define _TEMPLATE_SINGLETON

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
