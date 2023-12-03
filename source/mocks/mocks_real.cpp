/* Copyright (C) 2012 Wildfire Games.
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

#define CXXTEST_MOCK_REAL_SOURCE_FILE

// Pull in the common config headers from precompiled.h,
// but disable the actual precompiling (since we've only got
// one source file)
#ifdef USING_PCH
# undef USING_PCH
#endif
#include "lib/precompiled.h"

#if OS_LINUX || OS_BSD
#include "mocks/dlfcn.h"
#include "mocks/unistd.h"
#endif // OS_LINUX
