#ifndef INCLUDED_SHARED_PTR
#define INCLUDED_SHARED_PTR

// adapter that allows calling page_aligned_free as a shared_ptr deleter.
class PageAlignedDeleter
{
public:
	PageAlignedDeleter(size_t size);
	void operator()(u8* p);

private:
	size_t m_size;
};

template<class T>
struct DummyDeleter
{
	void operator()(T*)
	{
	}
};

template<class T>
shared_ptr<T> DummySharedPtr(T* ptr)
{
	return shared_ptr<T>(ptr, DummyDeleter<T>());
}

LIB_API shared_ptr<u8> Allocate(size_t size);

#endif	// #ifndef INCLUDED_SHARED_PTR
