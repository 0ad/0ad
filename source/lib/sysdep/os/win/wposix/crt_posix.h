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
 * see rationale in wposix.h.
 * we need to have CRT functions (e.g. _open) declared correctly
 * but must prevent POSIX functions (e.g. open) from being declared -
 * they would conflict with our wrapper function.
 *
 * since the headers only declare POSIX stuff #if !__STDC__, a solution
 * would be to set this flag temporarily. note that MSDN says that
 * such predefined macros cannot be redefined nor may they change during
 * compilation, but this works and seems to be a fairly common practice.
 **/

// undefine include guards set by no_crt_posix
#if defined(_INC_IO) && defined(WPOSIX_DEFINED_IO_INCLUDE_GUARD)
# undef _INC_IO
#endif
#if defined(_INC_DIRECT) && defined(WPOSIX_DEFINED_DIRECT_INCLUDE_GUARD)
# undef _INC_DIRECT
#endif


#define __STDC__ 1

#include <io.h>             // _open etc.
#include <direct.h>			// _rmdir

#undef __STDC__
#define __STDC__ 0
