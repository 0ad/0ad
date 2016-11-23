/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * template base class for Singletons
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
		ENSURE( !ms_singleton );
		ms_singleton = static_cast<T*>(this);
	}

	~Singleton()
	{
		ENSURE( ms_singleton );
		ms_singleton = 0;
	}

	static T& GetSingleton()
	{
		ENSURE( ms_singleton );
		return *ms_singleton;
	}

	static T* GetSingletonPtr()
	{
		ENSURE( ms_singleton );
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
