/**
 * =========================================================================
 * File        : debug_stl.h
 * Project     : 0 A.D.
 * Description : portable debugging helper functions specific to the STL.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2004-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DEBUG_STL_H_INCLUDED
#define DEBUG_STL_H_INCLUDED

// reduce complicated STL names to human-readable form (in place).
// e.g. "std::basic_string<char, char_traits<char>, std::allocator<char> >" =>
//  "string". algorithm: strip undesired strings in one pass (fast).
// returns <name> for convenience.
extern char* stl_simplify_name(char* name);


// no STL iterator is larger than this; see below.
const size_t DEBUG_STL_MAX_ITERATOR_SIZE = 64;

// if <wtype_name> indicates the object <p, size> to be an STL container,
// and given the size of its value_type (retrieved via debug information),
// return number of elements and an iterator (any data it needs is stored in
// it_mem, which must hold DEBUG_STL_MAX_ITERATOR_SIZE bytes).
// returns 0 on success or an StlContainerError.
extern LibError stl_get_container_info(const char* type_name, const u8* p, size_t size,
	size_t el_size, size_t* el_count, DebugIterator* el_iterator, void* it_mem);

#endif	// #ifndef DEBUG_STL_H_INCLUDED
