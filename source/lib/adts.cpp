#include "precompiled.h"

#include "adts.h"


template<class T, int max_items> class DynArray
{
public:
	DynArray() {}
	~DynArray() {}

	T* operator[](uint n)
	{
		T*& page = pages[n / items_per_page];
		if(!page)
		{
			page = (T*)calloc(PAGE_SIZE, 1);
			if(!page)
				return 0;
		}
		return page + (n % items_per_page);
	}

private:
	enum { PAGE_SIZE = 4096 };

//	const size_t items_per_page = sizeof(T) / PAGE_SIZE;
//	const size_t num_pages = (max_items + items_per_page-1) / items_per_page;
	T* pages[(max_items + (sizeof(T) / PAGE_SIZE)-1) / (sizeof(T) / PAGE_SIZE)];
};
