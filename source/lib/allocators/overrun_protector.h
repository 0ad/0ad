/* Copyright (c) 2011 Wildfire Games
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

#ifndef INCLUDED_ALLOCATORS_OVERRUN_PROTECTOR
#define INCLUDED_ALLOCATORS_OVERRUN_PROTECTOR

#include "lib/config2.h"	// CONFIG2_ALLOCATORS_OVERRUN_PROTECTION
#include "lib/sysdep/vm.h"

/**
OverrunProtector wraps an arbitrary object in isolated page(s) and
can detect inadvertent writes to it. this is useful for
tracking down memory overruns.

the basic idea is to require users to request access to the object and
notify us when done; memory access permission is temporarily granted.
(similar in principle to Software Transaction Memory).

since this is quite slow, the protection is disabled unless
CONFIG2_ALLOCATORS_OVERRUN_PROTECTION == 1; this avoids having to remove the
wrapper code in release builds and re-write when looking for overruns.

example usage:
OverrunProtector\<T\> wrapper;
..
T* p = wrapper.get();   // unlock, make ready for use
if(!p)                  // wrapper's one-time alloc of a T-
	abort();            // instance had failed - can't continue.
DoSomethingWith(p);     // (read/write access)
wrapper.lock();         // disallow further access until next .get()
..
**/
template<class T> class OverrunProtector
{
	NONCOPYABLE(OverrunProtector);	// const member
public:
	OverrunProtector()
#if CONFIG2_ALLOCATORS_OVERRUN_PROTECTION
		: object(new(vm::Allocate(sizeof(T))) T())
#else
		: object(new T())
#endif
	{
		lock();
	}

	~OverrunProtector()
	{
		unlock();
#if CONFIG2_ALLOCATORS_OVERRUN_PROTECTION
		object->~T();	// call dtor (since we used placement new)
		vm::Free(object, sizeof(T));
#else
		delete object;
#endif
	}

	T* get() const
	{
		unlock();
		return object;
	}

	void lock() const
	{
#if CONFIG2_ALLOCATORS_OVERRUN_PROTECTION
		vm::Protect(object, sizeof(T), PROT_NONE);
#endif
	}

private:
	void unlock() const
	{
#if CONFIG2_ALLOCATORS_OVERRUN_PROTECTION
		vm::Protect(object, sizeof(T), PROT_READ|PROT_WRITE);
#endif
	}

	T* const object;
};

#endif	// #ifndef INCLUDED_ALLOCATORS_OVERRUN_PROTECTOR
