/**
 * =========================================================================
 * File        : debug_stl.cpp
 * Project     : 0 A.D.
 * Description : portable debugging helper functions specific to the STL.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "debug_stl.h"

#include <deque>
#include <map>
#include <set>
#include <cassert>

#include "lib.h"	// match_wildcard


AT_STARTUP(\
	error_setDescription(ERR::STL_CNT_UNKNOWN, "Unknown STL container type_name");\
	error_setDescription(ERR::STL_CNT_INVALID, "Container type is known but contents are invalid");\
)


// used in debug_stl_simplify_name.
// note: strcpy is safe because replacement happens in-place and
// src is longer than dst (otherwise, we wouldn't be replacing).
#define REPLACE(what, with)\
	else if(!strncmp(src, (what), sizeof(what)-1))\
	{\
		src += sizeof(what)-1-1; /* see preincrement rationale*/\
		strcpy(dst, (with)); /* safe - see above */\
		dst += sizeof(with)-1;\
	}
#define STRIP(what)\
	else if(!strncmp(src, (what), sizeof(what)-1))\
	{\
		src += sizeof(what)-1-1;/* see preincrement rationale*/\
	}
#define STRIP_NESTED(what)\
	else if(!strncmp(src, (what), sizeof(what)-1))\
	{\
		/* remove preceding comma (if present) */\
		if(src != name && src[-1] == ',')\
			dst--;\
		src += sizeof(what)-1;\
		/* strip everything until trailing > is matched */\
		debug_assert(nesting == 0);\
		nesting = 1;\
	}

// reduce complicated STL names to human-readable form (in place).
// e.g. "std::basic_string<char, char_traits<char>, std::allocator<char> >" =>
//  "string". algorithm: strip undesired strings in one pass (fast).
// called from symbol_string_build.
//
// see http://www.bdsoft.com/tools/stlfilt.html and
// http://www.moderncppdesign.com/publications/better_template_error_messages.html
char* debug_stl_simplify_name(char* name)
{
	// used when stripping everything inside a < > to continue until
	// the final bracket is matched (at the original nesting level).
	int nesting = 0;

	const char* src = name-1;	// preincremented; see below.
	char* dst = name;

	// for each character: (except those skipped as parts of strings)
	for(;;)
	{
		char c = *(++src);
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
		STRIP_NESTED("std::allocator<")
		STRIP_NESTED("std::less<")
		STRIP_NESTED("stdext::hash_compare<")
		STRIP("std::")
		STRIP("stdext::")
		else
			*dst++ = c;
	}

	return name;
}


//-----------------------------------------------------------------------------
// STL container debugging
//-----------------------------------------------------------------------------

// provide an iterator interface for arbitrary STL containers; this is
// used to display their contents in stack traces. their type and
// contents aren't known until runtime, so this is somewhat tricky.
//
// we assume STL containers aren't specialized on their content type and
// use their int instantiations's memory layout. vector<bool> will therefore
// not be displayed correctly, but it is frowned upon anyway (since
// address of its elements can't be taken).
// to be 100% correct, we'd have to write an Any_container_type__element_type
// class for each combination, but that is clearly infeasible.
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
	// empty, must not be reported as invalid
	if(!el_count)
		return true;

	// # elements is unbelievably high; assume it's invalid.
	if(el_count > 0x1000000)
		return false;

	if(debug_is_pointer_bogus(front))
		return false;

	return true;
}

//----------------------------------------------------------------------------



// vector iterators advance by sizeof(value_type) bytes; since we assume the
// int specialization, we have to do this ourselves.

// deque iterator operator* depends on el_size

// map iterator operator++ depends on el_size

/*
template<class T> class GenericContainer : public T
{
public:
	bool valid(size_t el_size) const
	{
		if(!container_valid(&front(), el_count(el_size)))
			return false;
		return true;
	}

	size_t el_count(size_t UNUSED(el_size)) const
	{
		return size();
	}

	class iter : public T::const_iterator
	{
	public:
		const u8* deref_and_advance(size_t el_size)
		{
			const u8* p = (const u8*)&operator*();
			++(*this);
			return p;
		}
	};
};
*/


//
// standard containers
//

// it is rather difficult to abstract away implementation details of various
// STL versions. we currently only support Dinkumware (that shipped with VC7)
// chiefly due to set/map- (i.e. tree) and string-specific code.
#if STL_DINKUMWARE

class Any_deque : public std::deque<int>
{
#if STL_DINKUMWARE
	// being declared as friend isn't enough;
	// our iterator still doesn't get access to std::deque.
	const u8* get_item(size_t i, size_t el_size) const
	{
		const u8** map = (const u8**)_Map;
		const size_t el_per_bucket = MAX(16 / el_size, 1);
		const size_t bucket_idx = i / el_per_bucket;
		const size_t idx_in_bucket = i - bucket_idx * el_per_bucket;
		const u8* bucket = map[bucket_idx];
		const u8* p = bucket + idx_in_bucket*el_size;
		return p;
	}
#endif

public:
	bool valid(size_t el_size) const
	{
#if STL_DINKUMWARE
		if(!container_valid(_Map, _Mysize))
			return false;
		const size_t el_per_bucket = MAX(16 / el_size, 1);	// see _DEQUESIZ
		// initial element is beyond end of first bucket
		if(_Myoff >= el_per_bucket)
			return false;
		// more elements reported than fit in all buckets
		if(_Mysize > _Mapsize * el_per_bucket)
			return false;
#endif
		return true;
	}

	size_t el_count(size_t UNUSED(el_size)) const
	{
		return size();
	}

	class iter;
	friend class iter;
	class iter : public const_iterator
	{
	public:
		const u8* deref_and_advance(size_t el_size)
		{
			const u8* p;
#if STL_DINKUMWARE
			Any_deque* d = (Any_deque*)_Mycont;
			p = d->get_item(_Myoff, el_size);
#else
			p = (const u8*)&operator*();
#endif
			++(*this);
			return p;
		}
	};
};


class Any_list : public std::list<int>
{
public:
	bool valid(size_t UNUSED(el_size)) const
	{
#if STL_DINKUMWARE
		if(!container_valid(_Myhead, _Mysize))
			return false;
#endif
		return true;
	}

	size_t el_count(size_t UNUSED(el_size)) const
	{
		return size();
	}

	class iter : public const_iterator
	{
	public:
		const u8* deref_and_advance(size_t UNUSED(el_size))
		{
			const u8* p = (const u8*)&operator*();
			++(*this);
			return p;
		}
	};
};


template<class _Traits> class Any_tree : public std::_Tree<_Traits>
{
	// return reference to the given node's nil flag.
	// reimplemented because this member is stored after _Myval, so it's
	// dependent on el_size.
	static _Charref _Isnil(_Nodeptr _Pnode, size_t el_size)
	{
		const u8* p = (const u8*)&_Pnode->_Isnil;	// ok for int specialization
		p += el_size - sizeof(value_type);
		// account for el_size difference
		assert(*p <= 1);	// bool value
		return (_Charref)*p;
	}

public:
	Any_tree() {}

	bool valid(size_t UNUSED(el_size)) const
	{
		if(!container_valid(_Myhead, _Mysize))
			return false;
		return true;
	}

	size_t el_count(size_t UNUSED(el_size)) const
	{
		return size();
	}

	class iter;
	friend class iter;
	class iter : public const_iterator
	{
	public:
		const u8* deref_and_advance(size_t el_size)
		{
			const u8* p = (const u8*)&operator*();

			// end() shouldn't be incremented, don't move
			if(_Isnil(_Ptr, el_size))
				return p;

			// return smallest (leftmost) node of right subtree
			_Nodeptr _Pnode = _Right(_Ptr);
			if(!_Isnil(_Pnode, el_size))
			{
				while(!_Isnil(_Left(_Pnode), el_size))
					_Pnode = _Left(_Pnode);
			}
			// climb looking for right subtree
			else
			{
				while (!_Isnil(_Pnode = _Parent(_Ptr), el_size)
					&& _Ptr == _Right(_Pnode))
					_Ptr = _Pnode;	// ==> parent while right subtree
			}
			_Ptr = _Pnode;

			return p;
		}
	};
};


class Any_map : public Any_tree<std::_Tmap_traits<int, int, std::less<int>, std::allocator<std::pair<const int, int> >, false> >
{
};


class Any_multimap : public Any_map
{
};


class Any_set: public Any_tree<std::_Tset_traits<int, std::less<int>, std::allocator<int>, false> >
{
};


class Any_multiset: public Any_set
{
};


class Any_vector: public std::vector<int>
{
public:
	bool valid(size_t UNUSED(el_size)) const
	{
#if STL_DINKUMWARE
		if(!container_valid(_Myfirst, _Mylast-_Myfirst))
			return false;
#endif
		// more elements reported than reserved
		if(size() > capacity())
			return false;
		// front/back pointers incorrect
		if(&front() > &back())
			return false;
		return true;
	}

	size_t el_count(size_t el_size) const
	{
		// vectors store front and back pointers and calculate
		// element count as the difference between them. since we are
		// derived from the int specialization, the pointer arithmetic is
		// off. we fix it by taking el_size into account.
		return size() * 4 / el_size;
	}

	class iter : public const_iterator
	{
	public:
		const u8* deref_and_advance(size_t el_size)
		{
			const u8* p = (const u8*)&operator*();
			_Myptr = (_Tptr)((u8*)_Myptr + el_size);
			return p;
		}
	};
};


class Any_basic_string : public std::string
{
	const void* ptr(size_t el_size) const
	{
		return _Myres <= (16/el_size)-1? _Bx._Buf : _Bx._Ptr;
	}

public:
	bool valid(size_t el_size) const
	{
		if(!container_valid(ptr(el_size), _Mysize))
			return false;
#if STL_DINKUMWARE
		// less than the small buffer reserved - impossible
		if(_Myres < (16/el_size)-1)
			return false;
		// more elements reported than reserved
		if(_Mysize > _Myres)
			return false;
#endif
		return true;
	}

	size_t el_count(size_t UNUSED(el_size)) const
	{
		return size();
	}

	class iter : public const_iterator
	{
	public:
		const u8* deref_and_advance(size_t UNUSED(el_size))
		{
			const u8* p = (const u8*)&operator*();
			++(*this);
			return p;
		}
	};
};


//
// standard container adapters
//

// debug_stl_get_container_info makes sure this was actually instantiated with
// container = deque as we assume.
class Any_queue : public Any_deque
{
};


// debug_stl_get_container_info makes sure this was actually instantiated with
// container = deque as we assume.
class Any_stack : public Any_deque
{
};


//
// nonstandard containers (will probably be part of C++0x)
//

#if HAVE_STL_HASH


class Any_hash_map: public STL_HASH_MAP<int,int>
{
public:
	bool valid(size_t el_size) const
	{
		Any_list* list = (Any_list*)&_List;
		if(!list->valid(el_size))
			return false;
		return true;
	}

	size_t el_count(size_t UNUSED(el_size)) const
	{
		return size();
	}

	class iter : public const_iterator
	{
	public:
		const u8* deref_and_advance(size_t UNUSED(el_size))
		{
			const u8* p = (const u8*)&operator*();
			++(*this);
			return p;
		}
	};
};


class Any_hash_multimap : public Any_hash_map
{
};


class Any_hash_set: public STL_HASH_SET<int>
{
public:
	bool valid(size_t el_size) const
	{
		Any_list* list = (Any_list*)&_List;
		if(!list->valid(el_size))
			return false;
		return true;
	}

	size_t el_count(size_t UNUSED(el_size)) const
	{
		return size();
	}

	class iter : public const_iterator
	{
	public:
		const u8* deref_and_advance(size_t UNUSED(el_size))
		{
			const u8* p = (const u8*)&operator*();
			++(*this);
			return p;
		}
	};
};


class Any_hash_multiset : public Any_hash_set
{
};

#endif	// HAVE_STL_HASH

#if HAVE_STL_SLIST


class Any_slist: public Any_list
{
};

#endif	// HAVE_STL_SLIST

//-----------------------------------------------------------------------------

// generic iterator - returns next element. dereferences and increments the
// specific container iterator stored in it_mem.
template<class T> const u8* stl_iterator(void* it_mem, size_t el_size)
{
	T::iter* pi = (T::iter*)it_mem;
	return pi->deref_and_advance(el_size);
}


// check if the container is valid and return # elements and an iterator;
// this is instantiated once for each type of container.
// we don't do this in the Any_* ctors because we need to return bool valid and
// don't want to throw an exception (may confuse the debug code).
template<class T> bool get_container_info(T* t, size_t size, size_t el_size,
	size_t* el_count, DebugIterator* el_iterator, void* it_mem)
{
	debug_assert(sizeof(T) == size);
	debug_assert(sizeof(T::iterator) < DEBUG_STL_MAX_ITERATOR_SIZE);

	// bail if the container is uninitialized/invalid.
	// check this before calling el_count etc. because they may crash.
	if(!t->valid(el_size))
		return false;

	*el_count = t->el_count(el_size);
	*el_iterator = stl_iterator<T>;
	*(T::const_iterator*)it_mem = t->begin();
	return true;
}


// if <wtype_name> indicates the object <p, size> to be an STL container,
// and given the size of its value_type (retrieved via debug information),
// return number of elements and an iterator (any data it needs is stored in
// it_mem, which must hold DEBUG_STL_MAX_ITERATOR_SIZE bytes).
// returns 0 on success or an StlContainerError.
LibError debug_stl_get_container_info(const char* type_name, const u8* p, size_t size,
	size_t el_size, size_t* el_count, DebugIterator* el_iterator, void* it_mem)
{
	// HACK: The debug_stl code breaks VS2005's STL badly, causing crashes in
	// later pieces of code that try to manipulate the STL containers. Presumably
	// it needs to be altered/rewritten to work happily with the new STL debug iterators.
#if MSC_VERSION >= 1400
	return ERR::FAIL;
#endif

	bool handled = false, valid = false;
#define CONTAINER(name, type_name_pattern)\
	else if(match_wildcard(type_name, type_name_pattern))\
	{\
		handled = true;\
		valid = get_container_info<Any_##name>((Any_##name*)p, size, el_size, el_count, el_iterator, it_mem);\
	}
#define STD_CONTAINER(name) CONTAINER(name, "std::" #name "<*>")

	// workaround for preprocessor limitation: what we're trying to do is
	// stringize the defined value of a macro. prepending and pasting L
	// apparently isn't possible because macro args aren't expanded before
	// being pasted; we therefore compare as chars[].
#define STRINGIZE2(id) # id
#define STRINGIZE(id) STRINGIZE2(id)

	if(0) {}	// kickoff
	// standard containers
	STD_CONTAINER(deque)	// deref provided
	STD_CONTAINER(list)		// ok
	STD_CONTAINER(map)		// advance provided
	STD_CONTAINER(multimap)	// advance provided
	STD_CONTAINER(set)		// TODO use map impl?
	STD_CONTAINER(multiset)	// TODO use map impl?
	STD_CONTAINER(vector)	// special-cased
	STD_CONTAINER(basic_string)	// ok
	// standard container adapter
	// (note: Any_queue etc. assumes the underlying container is a deque.
	// we make sure of that here and otherwise refuse to display it, because
	// doing so is lots of work for little gain.)
	CONTAINER(queue, "std::queue<*,std::deque<*> >")
	CONTAINER(stack, "std::stack<*,std::deque<*> >")
	// nonstandard containers (will probably be part of C++0x)
#if HAVE_STL_HASH
	CONTAINER(hash_map, STRINGIZE(STL_HASH_MAP) "<*>")
	CONTAINER(hash_multimap, STRINGIZE(STL_HASH_MULTIMAP) "<*>")
	CONTAINER(hash_set, STRINGIZE(STL_HASH_SET) "<*>")
	CONTAINER(hash_multiset, STRINGIZE(STL_HASH_MULTISET) "<*>")
#endif
#if HAVE_STL_SLIST
	CONTAINER(slist, STRINGIZE(STL_SLIST) "<*>")
#endif

	// note: do not raise warnings - these can happen for new
	// STL classes or if the debuggee's memory is corrupted.
	if(!handled)
		return ERR::STL_CNT_UNKNOWN;	// NOWARN
	if(!valid)
		return ERR::STL_CNT_INVALID;	// NOWARN
	return INFO::OK;
}

#endif
