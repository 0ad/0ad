#ifndef INCLUDED_SHARED_PTR
#define INCLUDED_SHARED_PTR

#include "lib/sysdep/arch/x86_x64/x86_x64.h"
#include "lib/sysdep/rtl.h" // rtl_AllocateAligned

struct DummyDeleter
{
	template<class T>
	void operator()(T*)
	{
	}
};

template<class T>
shared_ptr<T> DummySharedPtr(T* ptr)
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
		_mm_free(t);
	}
};

template<class T>
shared_ptr<T> AllocateAligned(size_t size)
{
	return shared_ptr<T>((T*)rtl_AllocateAligned(size, x86_x64_L1CacheLineSize()), AlignedDeleter());
}

#endif	// #ifndef INCLUDED_SHARED_PTR
