/* Copyright (C) 2015 Wildfire Games.
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

/*
 * portable debugging helper functions specific to the STL.
 */

#include "precompiled.h"
#include "lib/debug_stl.h"

#include <deque>
#include <map>
#include <set>
#include <cassert>
#include <list>

#include "lib/regex.h"

static const StatusDefinition debugStlStatusDefinitions[] = {
	{ ERR::STL_CNT_UNKNOWN, L"Unknown STL container type_name" },
	{ ERR::STL_CNT_UNSUPPORTED, L"Unsupported STL container" },
	{ ERR::STL_CNT_INVALID, L"Container type is known but contents are invalid" }
};
STATUS_ADD_DEFINITIONS(debugStlStatusDefinitions);


// used in debug_stl_simplify_name.
// note: wcscpy_s is safe because replacement happens in-place and
// <what> is longer than <with> (otherwise, we wouldn't be replacing).
#define REPLACE(what, with)\
	else if(!wcsncmp(src, (what), ARRAY_SIZE(what)-1))\
	{\
		src += ARRAY_SIZE(what)-1-1; /* see preincrement rationale*/\
		wcscpy_s(dst, ARRAY_SIZE(with), (with));\
		dst += ARRAY_SIZE(with)-1;\
	}
#define STRIP(what)\
	else if(!wcsncmp(src, (what), ARRAY_SIZE(what)-1))\
	{\
		src += ARRAY_SIZE(what)-1-1;/* see preincrement rationale*/\
	}
#define STRIP_NESTED(what)\
	else if(!wcsncmp(src, (what), ARRAY_SIZE(what)-1))\
	{\
		/* remove preceding comma (if present) */\
		if(src != name && src[-1] == ',')\
			dst--;\
		src += ARRAY_SIZE(what)-1;\
		/* strip everything until trailing > is matched */\
		ENSURE(nesting == 0);\
		nesting = 1;\
	}

// reduce complicated STL names to human-readable form (in place).
// e.g. "std::basic_string<char, char_traits<char>, std::allocator<char> >" =>
//  "string". algorithm: strip undesired strings in one pass (fast).
// called from symbol_string_build.
//
// see http://www.bdsoft.com/tools/stlfilt.html and
// http://www.moderncppdesign.com/publications/better_template_error_messages.html
wchar_t* debug_stl_simplify_name(wchar_t* name)
{
	// used when stripping everything inside a < > to continue until
	// the final bracket is matched (at the original nesting level).
	int nesting = 0;

	const wchar_t* src = name-1;	// preincremented; see below.
	wchar_t* dst = name;

	// for each character: (except those skipped as parts of strings)
	for(;;)
	{
		wchar_t c = *(++src);
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
				ENSURE(nesting >= 0);
			}
			continue;
		}

		// start if chain (REPLACE and STRIP use else if)
		if(0) {}
		else if(!wcsncmp(src, L"::_Node", 7))
		{
			// add a space if not already preceded by one
			// (prevents replacing ">::_Node>" with ">>")
			if(src != name && src[-1] != ' ')
				*dst++ = ' ';
			src += 7;
		}
		REPLACE(L"unsigned short", L"u16")
		REPLACE(L"unsigned int", L"size_t")
		REPLACE(L"unsigned __int64", L"u64")
		STRIP(L",0> ")
		// early out: all tests after this start with s, so skip them
		else if(c != 's')
		{
			*dst++ = c;
			continue;
		}
		REPLACE(L"std::_List_nod", L"list")
		REPLACE(L"std::_Tree_nod", L"map")
		REPLACE(L"std::basic_string<char, ", L"string<")
		REPLACE(L"std::basic_string<__wchar_t, ", L"wstring<")
		REPLACE(L"std::basic_string<unsigned short, ", L"wstring<")
		STRIP(L"std::char_traits<char>, ")
		STRIP(L"std::char_traits<unsigned short>, ")
		STRIP(L"std::char_traits<__wchar_t>, ")
		STRIP(L"std::_Tmap_traits")
		STRIP(L"std::_Tset_traits")
		STRIP_NESTED(L"std::allocator<")
		STRIP_NESTED(L"std::less<")
		STRIP_NESTED(L"stdext::hash_compare<")
		STRIP(L"std::")
		STRIP(L"stdext::")
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
// them. we need to check if they are IsValid and only then use their contents.
// to that end, we derive a validator class from each container,
// cast the container's address to it, and call its IsValid() method.
//
// checks performed include: is size() realistic; does begin() come before
// end(), etc. we need to leverage all invariants because the values are
// random in release mode.
//
// we sometimes need to access protected members of the STL containers.
// granting access via friend is not possible since the system headers
// must not be changed. that leaves us with two alternatives:
// 1) write a 'shadow' class that has the same memory layout. this would
//    free us from the ugly Dinkumware naming conventions, but requires
//    more maintenance when the STL implementation changes.
// 2) derive from the container. while not entirely bulletproof due to the
//    lack of virtual dtors, this is safe in practice because pointers are
//    neither returned to users nor freed. the only requirement is that
//    classes must not include virtual functions, because a vptr would
//    change the memory layout in unknown ways.
//
// it is rather difficult to abstract away implementation details of various
// STL versions. we currently only really support Dinkumware due to
// significant differences in the implementations of set, map and string.


//----------------------------------------------------------------------------
// standard containers

// base class (slightly simplifies code by providing default implementations
// that can be used for most containers).
// Container is the complete type of the STL container (we can't pass this
// as a template because Dinkumware _Tree requires different parameters)
template<class Container>
struct ContainerBase : public Container
{
	bool IsValid(size_t UNUSED(el_size)) const
	{
		return true;
	}

	size_t NumElements(size_t UNUSED(el_size)) const
	{
		return this->size();
	}

	static const u8* DereferenceAndAdvance(typename Container::iterator& it, size_t UNUSED(el_size))
	{
		const u8* p = (const u8*)&*it;
		++it;
		return p;
	}
};


struct Any_deque : public ContainerBase<std::deque<int> >
{
#if STL_DINKUMWARE == 405

	bool IsValid(size_t el_size) const
	{
		const size_t el_per_bucket = ElementsPerBucket(el_size);
		// initial element is beyond end of first bucket
		if(_Myoff >= el_per_bucket)
			return false;
		// more elements reported than fit in all buckets
		if(_Mysize > _Mapsize * el_per_bucket)
			return false;

		return true;
	}

	static const u8* DereferenceAndAdvance(iterator& stl_it, size_t el_size)
	{
		struct Iterator : public iterator
		{
			Any_deque& Container() const
			{
				return *(Any_deque*)_Mycont;
			}

			size_t CurrentIndex() const
			{
				return _Myoff;
			}
		};

		Iterator& it = *(Iterator*)&stl_it;
		Any_deque& container = it.Container();
		const size_t currentIndex = it.CurrentIndex();
		const u8* p = container.GetNthElement(currentIndex, el_size);
		++it;
		return p;
	}

private:
	static size_t ElementsPerBucket(size_t el_size)
	{
		return std::max(16u / el_size, (size_t)1u);	// see _DEQUESIZ
	}

	const u8* GetNthElement(size_t i, size_t el_size) const
	{
		const size_t el_per_bucket = ElementsPerBucket(el_size);
		const size_t bucket_idx = i / el_per_bucket;
		ENSURE(bucket_idx < _Mapsize);
		const size_t idx_in_bucket = i - bucket_idx * el_per_bucket;
		ENSURE(idx_in_bucket < el_per_bucket);
		const u8** map = (const u8**)_Map;
		const u8* bucket = map[bucket_idx];
		const u8* p = bucket + idx_in_bucket*el_size;
		return p;
	}

#endif
};


struct Any_list : public ContainerBase<std::list<int> >
{
};


#if STL_DINKUMWARE == 405

template<class _Traits>
struct Any_tree : public std::_Tree<_Traits>
{
	Any_tree()	// (required because default ctor cannot be generated)
	{
	}

	bool IsValid(size_t UNUSED(el_size)) const
	{
		return true;
	}

	size_t NumElements(size_t UNUSED(el_size)) const
	{
		return size();
	}

	static const u8* DereferenceAndAdvance(iterator& stl_it, size_t el_size)
	{
		struct Iterator : public const_iterator
		{
			_Nodeptr Node() const
			{
				return _Ptr;
			}

			void SetNode(_Nodeptr node)
			{
				_Ptr = node;
			}
		};

		Iterator& it = *(Iterator*)&stl_it;
		_Nodeptr node = it.Node();
		const u8* p = (const u8*)&*it;

		// end() shouldn't be incremented, don't move
		if(_Isnil(node, el_size))
			return p;

		// return smallest (leftmost) node of right subtree
		_Nodeptr _Pnode = _Right(node);
		if(!_Isnil(_Pnode, el_size))
		{
			while(!_Isnil(_Left(_Pnode), el_size))
				_Pnode = _Left(_Pnode);
		}
		// climb looking for right subtree
		else
		{
			while (!_Isnil(_Pnode = _Parent(node), el_size) && node == _Right(_Pnode))
				node = _Pnode;	// ==> parent while right subtree
		}
		it.SetNode(_Pnode);
		return p;
	};

private:
	// return reference to the given node's nil flag.
	// reimplemented because this member is stored after _Myval, so it's
	// dependent on el_size.
	static _Charref _Isnil(_Nodeptr _Pnode, size_t el_size)
	{
		const u8* p = (const u8*)&_Pnode->_Isnil;	// correct for int specialization
		p += el_size - sizeof(value_type);	// adjust for difference in el_size
		assert(*p <= 1);	// bool value
		return (_Charref)*p;
	}
};


struct Any_map : public Any_tree<std::_Tmap_traits<int, int, std::less<int>, std::allocator<std::pair<const int, int> >, false> >
{
};


struct Any_multimap : public Any_map
{
};


struct Any_set: public Any_tree<std::_Tset_traits<int, std::less<int>, std::allocator<int>, false> >
{
};


struct Any_multiset: public Any_set
{
};

#endif


struct Any_vector: public ContainerBase<std::vector<int> >
{
	bool IsValid(size_t UNUSED(el_size)) const
	{
		// more elements reported than reserved
		if(size() > capacity())
			return false;
		// front/back pointers incorrect
		if(&front() > &back())
			return false;
		return true;
	}

#if STL_DINKUMWARE == 405

	size_t NumElements(size_t el_size) const
	{
		// vectors store front and back pointers and calculate their
		// element count as the difference between them. since we are
		// derived from a template specialization, the pointer arithmetic
		// is incorrect. we fix it by taking el_size into account.
		return ((u8*)_Mylast - (u8*)_Myfirst) * el_size;
	}

	static const u8* DereferenceAndAdvance(iterator& stl_it, size_t el_size)
	{
		struct Iterator : public const_iterator
		{
			void Advance(size_t numBytes)
			{
				(u8*&)_Myptr += numBytes;
			}
		};

		Iterator& it = *(Iterator*)&stl_it;
		const u8* p = (const u8*)&*it;
		it.Advance(el_size);
		return p;
	}

#endif
};


#if STL_DINKUMWARE == 405

struct Any_basic_string : public ContainerBase<std::string>
{
	bool IsValid(size_t el_size) const
	{
		// less than the small buffer reserved - impossible
		if(_Myres < (16/el_size)-1)
			return false;
		// more elements reported than reserved
		if(_Mysize > _Myres)
			return false;
		return true;
	}
};

#endif


//
// standard container adapters
//

// debug_stl_get_container_info makes sure this was actually instantiated with
// container = deque as we assume.
struct Any_queue : public Any_deque
{
};


// debug_stl_get_container_info makes sure this was actually instantiated with
// container = deque as we assume.
struct Any_stack : public Any_deque
{
};


//-----------------------------------------------------------------------------

// generic iterator - returns next element. dereferences and increments the
// specific container iterator stored in it_mem.
template<class T> const u8* stl_iterator(void* it_mem, size_t el_size)
{
	typedef typename T::iterator iterator;
	iterator& stl_it = *(iterator*)it_mem;
	return T::DereferenceAndAdvance(stl_it, el_size);
}


// basic sanity checks that apply to all containers.
template<class T>
static bool IsContainerValid(const T& t, size_t el_count)
{
	// note: don't test empty() because vector's implementation of it
	// depends on el_size.

	// size must be reasonable
	if(el_count > 0x1000000)
		return false;

	if(el_count != 0)
	{
		// valid pointer
		const u8* front = (const u8*)&*t.begin();	// (note: map doesn't have front)
		if(debug_IsPointerBogus(front))
			return false;

		// note: don't test back() because that depends on el_size and
		// requires container-specific code.
	}

	return true;
}

// check if the container is IsValid and return # elements and an iterator;
// this is instantiated once for each type of container.
// we don't do this in the Any_* ctors because we need to return bool IsValid and
// don't want to throw an exception (may confuse the debug code).
template<class T> bool get_container_info(const T& t, size_t size, size_t el_size,
	size_t& el_count, DebugStlIterator& el_iterator, void* it_mem)
{
	typedef typename T::iterator iterator;
	typedef typename T::const_iterator const_iterator;

	ENSURE(sizeof(T) == size);
	ENSURE(sizeof(iterator) < DEBUG_STL_MAX_ITERATOR_SIZE);

	el_count = t.NumElements(el_size);

	// bail if the container is uninitialized/invalid.
	if(!IsContainerValid(t, el_count))
		return false;

	el_iterator = stl_iterator<T>;

	// construct a copy of begin() at it_mem. placement new is necessary
	// because VC8's secure copy ctor apparently otherwise complains about
	// invalid values in the (uninitialized) destination memory.
	new(it_mem) const_iterator(t.begin());
	return true;
}


// if <wtype_name> indicates the object <p, size> to be an STL container,
// and given the size of its value_type (retrieved via debug information),
// return number of elements and an iterator (any data it needs is stored in
// it_mem, which must hold DEBUG_STL_MAX_ITERATOR_SIZE bytes).
// returns 0 on success or an StlContainerError.
Status debug_stl_get_container_info(const wchar_t* type_name, const u8* p, size_t size,
	size_t el_size, size_t* el_count, DebugStlIterator* el_iterator, void* it_mem)
{
#if MSC_VERSION
	UNUSED2(type_name);
	UNUSED2(p);
	UNUSED2(size);
	UNUSED2(el_size);
	UNUSED2(el_count);
	UNUSED2(el_iterator);
	UNUSED2(it_mem);
	return ERR::STL_CNT_UNSUPPORTED;	// NOWARN
#else

	bool handled = false, IsValid = false;
#define CONTAINER(name, type_name_pattern)\
	else if(match_wildcard(type_name, type_name_pattern))\
	{\
		handled = true;\
		IsValid = get_container_info<Any_##name>(*(Any_##name*)p, size, el_size, *el_count, *el_iterator, it_mem);\
	}
#define STD_CONTAINER(name) CONTAINER(name, L"std::" WIDEN(#name) L"<*>")

	// workaround for preprocessor limitation: what we're trying to do is
	// stringize the defined value of a macro. prepending and pasting L
	// apparently isn't possible because macro args aren't expanded before
	// being pasted; we therefore compare as chars[].
#define STRINGIZE2(id) # id
#define STRINGIZE(id) STRINGIZE2(id)

	if(0) {}	// kickoff
	// standard containers
	STD_CONTAINER(deque)
	STD_CONTAINER(list)
	STD_CONTAINER(vector)
#if STL_DINKUMWARE == 405
	STD_CONTAINER(map)
	STD_CONTAINER(multimap)
	STD_CONTAINER(set)
	STD_CONTAINER(multiset)
	STD_CONTAINER(basic_string)
#endif
	// standard container adapters
	// (note: Any_queue etc. assumes the underlying container is a deque.
	// we make sure of that here and otherwise refuse to display it, because
	// doing so is lots of work for little gain.)
	CONTAINER(queue, L"std::queue<*,std::deque<*> >")
	CONTAINER(stack, L"std::stack<*,std::deque<*> >")

	// note: do not raise warnings - these can happen for new
	// STL classes or if the debuggee's memory is corrupted.
	if(!handled)
		return ERR::STL_CNT_UNKNOWN;	// NOWARN
	if(!IsValid)
		return ERR::STL_CNT_INVALID;	// NOWARN
	return INFO::OK;

#endif
}
