#include "precompiled.h"

#include "debug_stl.h"
#include "lib.h"	// match_wildcardw

// portable debugging helper functions specific to the STL.

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
				debug_assert(nesting >= 0);
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
			debug_assert(nesting == 0);
			nesting = 1;
		}
		else if(!strncmp(src, "std::less<", 10))
		{
			// remove preceding comma (if present)
			if(src != name && src[-1] == ',')
				dst--;
			src += 10;
			// strip everything until trailing > is matched
			debug_assert(nesting == 0);
			nesting = 1;
		}
		STRIP("std::")
		else
		*dst++ = c;
	}
}


//-----------------------------------------------------------------------------
// STL container debugging
//-----------------------------------------------------------------------------

// provide an iterator interface for arbitrary STL containers; this is
// used to display their contents in stack traces. their type and
// contents aren't known until runtime, so this is somewhat tricky.
//
// we assume STL containers aren't specialized on their content
// type and use container<int>'s memory layout. vector<bool> will therefore
// not be displayed correctly, but it is frowned upon anyway (since
// address of its elements can't be taken).
// to be 100% correct, we'd have to write an Any_container_type__element_type
// class for each combination, but that is clearly infeasible.


// generic iterator - returns next element. dereferences and increments the
// specific container iterator in it_mem.
template<class T> const u8* stl_iterator(void* it_mem, size_t el_size)
{
	UNUSED(el_size);
	T::iterator* const it = (T::iterator*)it_mem;
	T::reference el = *(*it);
	const u8* p = (const u8*)&el;
	++(*it);
	return p;
}


//-----------------------------------------------------------------------------

// validator classes for all STL containers
//
// containers might still be uninitialized when we call get_container_info on
// them. we need to check if they are valid and only then use their contents.
// to that end, we derive a validator class from each container,
// cast the container's address to it, and call its valid() method.
//
// checks performed include: is size() realistic; does begin() come before
// end(), etc. we need to leverage all invariants because the values are
// random in release mode.
//
// rationale:
// - we need a complete class for each container type because the
//   valid() function sometimes needs access to protected members of
//   the containers. since we can't grant access via friend without the
//   cooperation of the system headers, it needs to be in a derived class.
// - since we cast our validator on top of the actual container,
//   it must not contain virtual functions (the vptr would shift addresses;
//   we can't really correct for this because it's totally non-portable).
// - we don't bother with making this a template because there are enough
//   variations that we'd have to specialize everything anyway.

// basic sanity checks shared by all containers.
static bool container_valid(const void* front, size_t el_count)
{
	// # elements is unbelievably high; assume it's invalid.
	if(el_count > 0x1000000)
		return false;
	if(debug_is_bogus_pointer(front))
		return false;
	return true;
}

// shared by deque, queue and stack. this is specific to
// the Dinkumware implementation but portable.
static bool deque_valid(size_t el_count, size_t el_size,
	size_t num_skipped, size_t num_buckets)
{
	const size_t el_per_bucket = 16 / el_size;	// see _DEQUESIZ
	// initial element is beyond end of first bucket
	if(num_skipped >= el_per_bucket)
		return false;
	// more elements reported than fit in all buckets
	if(el_count > num_buckets * el_per_bucket)
		return false;
	return true;
}

class Any_deque : public std::deque<int>
{
public:
	bool valid(size_t el_size) const
	{
		if(!container_valid(&front(), size()))
			return false;
#if STL_DINKUMWARE != 0
		if(!deque_valid(_Mysize, el_size, _Myoff, _Mapsize))
			return false;
#endif
		return true;
	}
};

class Any_list: public std::list<int>
{
public:
	bool valid(size_t el_size) const
	{
		UNUSED(el_size);
		if(!container_valid(&front(), size()))
			return false;
		return true;
	}
};

class Any_map : public std::map<int,int>
{
public:
	bool valid(size_t el_size) const
	{
		UNUSED(el_size);
		const_iterator it = begin();
		if(!container_valid(&*it, size()))
			return false;
		return true;
	}
};

// we assume this adapter was instantiated with container=deque!
class Any_queue : public std::deque<int>
{
public:
	bool valid(size_t el_size) const
	{
		if(!container_valid(&front(), size()))
			return false;
#if STL_DINKUMWARE != 0
		if(!deque_valid(_Mysize, el_size, _Myoff, _Mapsize))
			return false;
#endif
		return true;
	}
};

class Any_set: public std::set<int>
{
public:
	bool valid(size_t el_size) const
	{
		UNUSED(el_size);
		const_iterator it = begin();
		if(!container_valid(&*it, size()))
			return false;
		return true;
	}
};

// we assume this adapter was instantiated with container=deque!
class Any_stack : public std::deque<int>
{
public:
	bool valid(size_t el_size) const
	{
		if(!container_valid(&front(), size()))
			return false;
#if STL_DINKUMWARE != 0
		if(!deque_valid(_Mysize, el_size, _Myoff, _Mapsize))
			return false;
#endif
		return true;
	}
};

class Any_vector: public std::vector<int>
{
public:
	bool valid(size_t el_size) const
	{
		UNUSED(el_size);
		if(!container_valid(&front(), size()))
			return false;
		// more elements reported than reserved
		if(size() > capacity())
			return false;
		// front/back pointers incorrect
		if(&front() > &back())
			return false;
		return true;
	}
};

class Any_string : public std::string
{
public:
	bool valid(size_t el_size) const
	{
		debug_assert(el_size == sizeof(char));
		if(!container_valid(c_str(), size()))
			return false;
#if STL_DINKUMWARE != 0
		// less than the small buffer reserved - impossible
		if(_Myres < 15)
			return false;
		// more elements reported than reserved
		if(_Mysize > _Myres)
			return false;
#endif
		return true;
	}
};

class Any_wstring : public std::wstring
{
public:
	bool valid(size_t el_size) const
	{
		debug_assert(el_size == sizeof(wchar_t));
		if(!container_valid(c_str(), size()))
			return false;
#if STL_DINKUMWARE != 0
		// less than the small buffer reserved - impossible
		if(_Myres < 15)
			return false;
		// more elements reported than reserved
		if(_Mysize > _Myres)
			return false;
#endif
		return true;
	}
};

//-----------------------------------------------------------------------------

// check if the container is valid and return # elements and an iterator;
// this is instantiated once for each type of container.
// we don't do this in the Any_* ctors because we need to return bool valid and
// don't want to throw an exception (may confuse the debug code).
template<class T> bool get_container_info(T* t, size_t size, size_t el_size,
	size_t* el_count, DebugIterator* el_iterator, void* it_mem)
{
	if(sizeof(T) != size)
		debug_assert(sizeof(T) == size);
	debug_assert(sizeof(T::iterator) < DEBUG_STL_MAX_ITERATOR_SIZE);

	*el_count = t->size();
	*el_iterator = stl_iterator<T>;
	*(T::iterator*)it_mem = t->begin();
	return t->valid(el_size);
}


// if <type_name> indicates the object <p, size> to be an STL container,
// and given the size of its value_type (retrieved via debug information),
// return number of elements and an iterator (any data it needs is stored in
// it_mem, which must hold DEBUG_STL_MAX_ITERATOR_SIZE bytes).
// returns 0 on success, 1 if type_name is unknown, or -1 if the contents
// are invalid (most likely due to being uninitialized).
int stl_get_container_info(const wchar_t* type_name, const u8* p, size_t size,
	size_t el_size, size_t* el_count, DebugIterator* el_iterator, void* it_mem)
{
	bool valid;

#define CONTAINER(name)\
	else if(match_wildcardw(type_name, L"std::" L###name L"<*>"))\
		valid = get_container_info<Any_##name>((Any_##name*)p, size, el_size, el_count, el_iterator, it_mem);

	if(0) {}	// kickoff
	CONTAINER(deque)
	CONTAINER(list)
	CONTAINER(map)
	CONTAINER(queue)
	CONTAINER(set)
	CONTAINER(stack)
	CONTAINER(vector)
	else if(match_wildcardw(type_name, L"std::basic_string<char*>"))
		valid = get_container_info<Any_string>((Any_string*)p, size, el_size, el_count, el_iterator, it_mem);
	else if(match_wildcardw(type_name, L"std::basic_string<unsigned short*>"))
		valid = get_container_info<Any_wstring>((Any_wstring*)p, size, el_size, el_count, el_iterator, it_mem);
	// unknown type, can't handle it
	else
		return 1;

	if(!valid)
		return -1;

	return 0;
}

