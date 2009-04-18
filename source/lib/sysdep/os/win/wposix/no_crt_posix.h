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
 * prevent subsequent includes of CRT headers (e.g. <io.h>) from
 * interfering with previous declarations made by wposix headers (e.g. open).
 * this is accomplished by #defining their include guards.
 * note: #include "crt_posix.h" can undo the effects of this header and
 * pull in those headers.
 **/

#ifndef _INC_IO	// <io.h> include guard
# define _INC_IO
# define WPOSIX_DEFINED_IO_INCLUDE_GUARD
#endif
#ifndef _INC_DIRECT	// <direct.h> include guard
# define _INC_DIRECT
# define WPOSIX_DEFINED_DIRECT_INCLUDE_GUARD
#endif
