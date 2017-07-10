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

/**
 * see rationale in wposix.h.
 * we need to have CRT functions (e.g. _open) declared correctly
 * but must prevent POSIX functions (e.g. open) from being declared -
 * they would conflict with our wrapper function.
 *
 * since the headers only declare POSIX stuff \#if !__STDC__, a solution
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
