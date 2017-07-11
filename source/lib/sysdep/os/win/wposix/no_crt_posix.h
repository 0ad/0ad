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
 * prevent subsequent includes of CRT headers (e.g. <io.h>) from
 * interfering with previous declarations made by wposix headers (e.g. open).
 * this is accomplished by \#defining their include guards.
 * note: \#include "crt_posix.h" can undo the effects of this header and
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
