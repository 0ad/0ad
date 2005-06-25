#include "precompiled.h"

#include "debug_stl.h"



// used in stl_simplify_name.
// TODO: check strcpy safety
#define REPLACE(what, with)\
	else if(!strncmp(src, (what), sizeof(what)-1))\
	{\
	src += sizeof(what)-1-1;/* see preincrement rationale*/\
	strcpy(dst, (with));\
	dst += sizeof(with)-1;\
	}
#define STRIP(what)\
	else if(!strncmp(src, (what), sizeof(what)-1))\
	{\
	src += sizeof(what)-1-1;/* see preincrement rationale*/\
	}

// reduce complicated STL names to human-readable form (in place).
// e.g. "std::basic_string<char, char_traits<char>, std::allocator<char> >" =>
//  "string". algorithm: strip undesired strings in one pass (fast).
// called from symbol_string_build.
//
// see http://www.bdsoft.com/tools/stlfilt.html and
// http://www.moderncppdesign.com/publications/better_template_error_messages.html
void stl_simplify_name(char* name)
{
	// used when stripping everything inside a < > to continue until
	// the final bracket is matched (at the original nesting level).
	int nesting = 0;

	const char* src = name-1;	// preincremented; see below.
	char* dst = name;

	// for each character: (except those skipped as parts of strings)
	for(;;)
	{
		int c = *(++src);
		// preincrement rationale: src++ with no further changes would
		// require all comparisons to subtract 1. incrementing at the
		// end of a loop would require a goto, instead of continue
		// (there are several paths through the loop, for speed).
		// therefore, preincrement. when skipping strings, subtract
		// 1 from the offset (since src is advanced directly after).

		// end of string reached - we're done.
		if(c == '\0')
		{
			*dst = '\0';
			break;
		}

		// we're stripping everything inside a < >; eat characters
		// until final bracket is matched (at the original nesting level).
		if(nesting)
		{
			if(c == '<')
				nesting++;
			else if(c == '>')
			{
				nesting--;
				assert(nesting >= 0);
			}
			continue;
		}

		// start if chain (REPLACE and STRIP use else if)
		if(0) {}
		else if(!strncmp(src, "::_Node", 7))
		{
			// add a space if not already preceded by one
			// (prevents replacing ">::_Node>" with ">>")
			if(src != name && src[-1] != ' ')
				*dst++ = ' ';
			src += 7;
		}
		REPLACE("unsigned short", "u16")
			REPLACE("unsigned int", "uint")
			REPLACE("unsigned __int64", "u64")
			STRIP(",0> ")
			// early out: all tests after this start with s, so skip them
		else if(c != 's')
		{
			*dst++ = c;
			continue;
		}
		REPLACE("std::_List_nod", "list")
			REPLACE("std::_Tree_nod", "map")
			REPLACE("std::basic_string<char,", "string<")
			REPLACE("std::basic_string<unsigned short,", "wstring<")
			STRIP("std::char_traits<char>,")
			STRIP("std::char_traits<unsigned short>,")
			STRIP("std::_Tmap_traits")
			STRIP("std::_Tset_traits")
		else if(!strncmp(src, "std::allocator<", 15))
		{
			// remove preceding comma (if present)
			if(src != name && src[-1] == ',')
				dst--;
			src += 15;
			// strip everything until trailing > is matched
			assert(nesting == 0);
			nesting = 1;
		}
		else if(!strncmp(src, "std::less<", 10))
		{
			// remove preceding comma (if present)
			if(src != name && src[-1] == ',')
				dst--;
			src += 10;
			// strip everything until trailing > is matched
			assert(nesting == 0);
			nesting = 1;
		}
		STRIP("std::")
		else
		*dst++ = c;
	}
}

//-----------------------------------------------------------------------------


/*
// queue - same as deque
// stack - same as deque
*/

// if a container reports more elements than this, assume it's invalid.
// (only used for containers where we can't check anything else, e.g. map)
static const size_t MAX_CONTAINER_ELEMENTS = 0x1000000;




// we need a complete class for each container type and cannot
// templatize them, because the valid() function needs access to
// protected members of the containers. since we can't make it a friend
// without the cooperation of the system headers, they need to be
// in a derived class.

// can't placement-new on top of the actual data because Any* will include
// vtbl ptr -> size is different and we trash the in-mem contents.
// actually we need to cast or placement-new (to validate the contents),
// so the validator class must not be virtual.

// templates don't help; we need a separate class for each container
// (since there's a special implementation). templates don't simplify
// the dispatch (given name, call correct validator function) since type name is
// only known at runtime.

template<class T> const u8* stl_iterator(void* it_mem, size_t el_size)
{
	UNUSED(el_size);
	T::iterator* const it = (T::iterator*)it_mem;
	T::reference el = *(*it);
	const u8* p = (const u8*)&el;
	++(*it);
	return p;
}

class Any_deque : public std::deque<int>
{
public:
	bool valid(size_t el_size) const
	{
		const size_t el_per_bucket = 16 / el_size;	// see _DEQUESIZ
		// offset of initial element beyond end of first bucket
		if(_Myoff >= el_per_bucket)
			return false;
		// more elements reported than fit in all buckets
		if(_Mysize > _Mapsize * el_per_bucket)
			return false;
		return true;
	}
};

class Any_list: public std::list<int>
{
public:
	bool valid(size_t el_size) const
	{
		// way too many elements, must be garbage in _Mysize field
		if(_Mysize > MAX_CONTAINER_ELEMENTS)
			return false;
		return !debug_is_bogus_pointer(_Myhead);
	}
};

class Any_map : public std::map<int,int>
{
public:
	bool valid(size_t el_size) const
	{
		// way too many elements, must be garbage in _Mysize field
		if(_Mysize > MAX_CONTAINER_ELEMENTS)
			return false;
		return !debug_is_bogus_pointer(_Myhead);
	}
};

class Any_set: public std::set<int>
{
public:
	bool valid(size_t el_size) const
	{
		// way too many elements, must be garbage in _Mysize field
		if(_Mysize > MAX_CONTAINER_ELEMENTS)
			return false;
		return !debug_is_bogus_pointer(_Myhead);
	}
};

class Any_vector: public std::vector<int>
{
public:
	bool valid(size_t el_size) const
	{
		// way too many elements, must be garbage in _Mylast
		if(_Mylast - _Myfirst > MAX_CONTAINER_ELEMENTS)
			return false;
		// pointers are incorrectly ordered
		if(_Myfirst > _Mylast || _Mylast > _Myend)
			return false;
		return !debug_is_bogus_pointer(_Myfirst);
	}
};

// size of container in bytes
// not a ctor because we need to indicate if type_name was recognized

template<class T> bool get_container_info(T* t, size_t size, size_t el_size,
	size_t* el_count, DebugIterator* el_iterator, void* it_mem)
{
	assert(sizeof(T) == size);
	assert(sizeof(T::iterator) < DEBUG_STL_MAX_ITERATOR_SIZE);

	*el_count = t->size();
	*el_iterator = stl_iterator<T>;
	*(T::iterator*)it_mem = t->begin();
	return t->valid(el_size);
}


int stl_get_container_info(wchar_t* type_name, const u8* p, size_t size,
	size_t el_size, size_t* el_count, DebugIterator* el_iterator, void* it_mem)
{
	bool valid;

#define CONTAINER(name)\
	else if(!wcsncmp(type_name, L"std::" L###name, wcslen(L###name)+5))\
		valid = get_container_info<Any_##name>((Any_##name*)p, size, el_size, el_count, el_iterator, it_mem);

	if(0) {}	// kickoff
	CONTAINER(deque)
	CONTAINER(list)
	CONTAINER(map)
	CONTAINER(set)
	CONTAINER(vector)
	// unknown type, can't handle it
	else
		return 1;

	if(!valid)
		return -1;

	return 0;
}
