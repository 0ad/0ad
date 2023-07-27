/* Copyright (C) 2021 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_THREADING
#define INCLUDED_THREADING

#include "ThreadUtil.h"

#if OS_WIN
#include "lib/sysdep/os/win/wseh.h"
#endif

namespace Threading
{

/**
 * Wrap a function to handle exceptions.
 * This currently deals with Windows Structured Exception Handling (see wseh.cpp)
 */
template<auto F, class T>
struct HandleExceptionsBase {};
template<auto functionPtr>
struct HandleExceptions : public HandleExceptionsBase<functionPtr, decltype(functionPtr)> {};

template<auto functionPtr, typename... Types>
struct HandleExceptionsBase<functionPtr, void(*)(Types...)>
{
	static void Wrapper(Types... args)
	{
#if OS_WIN
		__try
		{
			functionPtr(args...);
		}
		__except(wseh_ExceptionFilter(GetExceptionInformation()))
		{
			// Nothing particular to do.
		}
#else
		functionPtr(args...);
#endif
	}
};

}

#endif // INCLUDED_THREADING
