/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * =========================================================================
 * File        : debug_stl.h
 * Project     : 0 A.D.
 * Description : portable debugging helper functions specific to the STL.
 * =========================================================================
 */

#ifndef INCLUDED_DEBUG_STL
#define INCLUDED_DEBUG_STL


namespace ERR
{
	const LibError STL_CNT_UNKNOWN = -100500;
	// likely causes: not yet initialized or memory corruption.
	const LibError STL_CNT_INVALID = -100501;
}


/**
 * reduce complicated STL symbol names to human-readable form.
 *
 * algorithm: remove/replace undesired substrings in one pass (fast).
 * example: "std::basic_string<char, char_traits<char>,
 * std::allocator<char> >" => "string".
 *
 * @param buffer holding input symbol name; modified in-place.
 * there is no length limit; must be large enough to hold typical STL
 * strings. DBG_SYMBOL_LEN chars is a good measure.
 * @return name for convenience.
 **/
extern char* debug_stl_simplify_name(char* name);


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
 * @return LibError (ERR::STL_*)
 **/
extern LibError debug_stl_get_container_info(const char* type_name, const u8* p, size_t size,
	size_t el_size, size_t* el_count, DebugStlIterator* el_iterator, void* it_mem);

#endif	// #ifndef INCLUDED_DEBUG_STL
