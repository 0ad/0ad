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

#ifndef SYSDEP_H__
#define SYSDEP_H__

// STL_HASH_MAP, STL_HASH_MULTIMAP
#ifdef __GNUC__
// GCC
# include <ext/hash_map>
# define STL_HASH_MAP __gnu_cxx::hash_map
# define STL_HASH_MULTIMAP __gnu_cxx::hash_multimap
#else	// !__GNUC__
# include <hash_map>
# if defined(_MSC_VER) && (_MSC_VER >= 1300)
// VC7 or above
#  define STL_HASH_MAP stdext::hash_map
#  define STL_HASH_MULTIMAP stdext::hash_multimap
# else
// VC6 and anything else (most likely name)
#  define STL_HASH_MAP std::hash_map
#  define STL_HASH_MULTIMAP std::hash_multimap
# endif	// defined(_MSC_VER) && (_MSC_VER >= 1300)
#endif	// !__GNUC__


#endif	// #ifndef SYSDEP_H__
