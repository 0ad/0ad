/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_DEBUG_STL
#define INCLUDED_DEBUG_STL


namespace ERR
{
	const Status STL_CNT_UNKNOWN = -100500;
	const Status STL_CNT_UNSUPPORTED = -100501;
	// likely causes: not yet initialized or memory corruption.
	const Status STL_CNT_INVALID = -100502;
}


/**
 * reduce complicated STL symbol names to human-readable form.
 *
 * algorithm: remove/replace undesired substrings in one pass (fast).
 * example: "std::basic_string\<char, char_traits\<char\>,
 * std::allocator\<char\> \>" => "string".
 *
 * @param name Buffer holding input symbol name; modified in-place.
 *		  There is no length limit; must be large enough to hold typical STL
 *		  strings. DEBUG_SYMBOL_CHARS chars is a good measure.
 * @return name for convenience.
 **/
extern wchar_t* debug_stl_simplify_name(wchar_t* name);


/**
 * abstraction of all STL iterators used by debug_stl.
 **/
typedef const u8* (*DebugStlIterator)(void* internal, size_t el_size);

/**
 * no STL iterator is larger than this; see below.
 **/
const size_t DEBUG_STL_MAX_ITERATOR_SIZE = 64;

/**
 * prepare to enumerate the elements of arbitrarily typed STL containers.
 *
 * works by retrieving element count&size via debug information and hiding
 * the container's iterator implementation behind a common interface.
 * a basic sanity check is performed to see if the container memory is
 * valid and appears to be initialized.
 *
 * @param type_name exact type of STL container (see example above)
 * @param p raw memory holding the container
 * @param size sizeof(container)
 * @param el_size sizeof(value_type)
 * @param el_count out; number of valid elements in container
 * @param el_iterator out; callback function that acts as an iterator
 * @param it_mem out; buffer holding the iterator state. must be
 * at least DEBUG_STL_MAX_ITERATOR_SIZE bytes.
 * @return Status (ERR::STL_*)
 **/
extern Status debug_stl_get_container_info(const wchar_t* type_name, const u8* p, size_t size,
	size_t el_size, size_t* el_count, DebugStlIterator* el_iterator, void* it_mem);

#endif	// #ifndef INCLUDED_DEBUG_STL
