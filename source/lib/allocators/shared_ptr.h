/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef INCLUDED_ALLOCATORS_SHARED_PTR
#define INCLUDED_ALLOCATORS_SHARED_PTR

#include "lib/alignment.h"
#include "lib/sysdep/rtl.h" // rtl_AllocateAligned

struct DummyDeleter
{
	template<class T>
	void operator()(T*)
	{
	}
};

template<class T>
inline shared_ptr<T> DummySharedPtr(T* ptr)
{
	return shared_ptr<T>(ptr, DummyDeleter());
}

struct ArrayDeleter
{
	template<class T>
	void operator()(T* p)
	{
		delete[] p;
	}
};

// (note: uses CheckedArrayDeleter)
LIB_API shared_ptr<u8> Allocate(size_t size);


struct AlignedDeleter
{
	template<class T>
	void operator()(T* t)
	{
		rtl_FreeAligned(t);
	}
};

template<class T>
static inline Status AllocateAligned(shared_ptr<T>& p, size_t size, size_t alignment = cacheLineSize)
{
	void* mem = rtl_AllocateAligned(size, alignment);
	if(!mem)
		WARN_RETURN(ERR::NO_MEM);
	p.reset((T*)mem, AlignedDeleter());
	return INFO::OK;
}

#endif	// #ifndef INCLUDED_ALLOCATORS_SHARED_PTR
