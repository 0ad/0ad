#ifndef INCLUDED_SHARED_PTR
#define INCLUDED_SHARED_PTR

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

#endif	// #ifndef INCLUDED_SHARED_PTR
