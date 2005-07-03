// portable debugging helper functions specific to the STL.
// Copyright (c) 2004-2005 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#ifndef DEBUG_STL_H_INCLUDED
#define DEBUG_STL_H_INCLUDED

// reduce complicated STL names to human-readable form (in place).
// e.g. "std::basic_string<char, char_traits<char>, std::allocator<char> >" =>
//  "string". algorithm: strip undesired strings in one pass (fast).
// returns <name> for convenience.
extern char* stl_simplify_name(char* name);


// no STL iterator is larger than this; see below.
const size_t DEBUG_STL_MAX_ITERATOR_SIZE = 64;

enum StlContainerError
{
	// type_name is not that of a known STL container.
	STL_CNT_UNKNOWN = -100,

	// the container is of a known type but its contents are invalid.
	// likely causes: not yet initialized or memory corruption.
	STL_CNT_INVALID = -101
};

// if <wtype_name> indicates the object <p, size> to be an STL container,
// and given the size of its value_type (retrieved via debug information),
// return number of elements and an iterator (any data it needs is stored in
// it_mem, which must hold DEBUG_STL_MAX_ITERATOR_SIZE bytes).
// returns 0 on success or an StlContainerError.
extern int stl_get_container_info(const char* type_name, const u8* p, size_t size,
	size_t el_size, size_t* el_count, DebugIterator* el_iterator, void* it_mem);

#endif	// #ifndef DEBUG_STL_H_INCLUDED
