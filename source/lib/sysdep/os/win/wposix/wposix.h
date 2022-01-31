/* Copyright (C) 2022 Wildfire Games.
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
 * emulate a subset of POSIX on Win32.
 */

#ifndef INCLUDED_WPOSIX
#define INCLUDED_WPOSIX

/**
 * rationale: the Windows headers declare many POSIX functions (e.g. read).
 * unfortunately, these are often slightly incorrect (size_t vs. size_t).
 * to avert trouble in user code caused by these differences, we declare
 * all functions ourselves according to SUSv3 and do not use the headers.
 *
 * however, it does not end there. some other libraries (e.g. wxWidgets)
 * will want to pull in these headers, which would conflict with our
 * declarations. also, our implementation uses the actual CRT code,
 * so we want those functions (e.g. _read) to be declared correctly even
 * if switching compiler/CRT version.
 *
 * how can these conflicting requirements be reconciled? our headers \#include
 * "no_crt_posix.h" to \#define the CRT headers' include guards and thus
 * prevent them from declaring anything. the implementation files \#include
 * "crt_posix.h", which pulls in the CRT headers (even if "no_crt_posix.h"
 * was previously included, e.g. in the PCH). note that the CRT headers
 * would still cause conflicts with the POSIX function declarations,
 * but we are able to prevent this via __STDC__.
 **/

//
// <stdlib.h>
//

int setenv(const char* envname, const char* envval, int overwrite);


//
// <math.h>
//

// (missing on MSVC)
#ifndef M_PI
#define M_PI						3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2						1.57079632679489661923
#endif
#ifndef INFINITY
#define INFINITY					(std::numeric_limits<float>::infinity())
#endif
#ifndef NAN
#define NAN							(std::numeric_limits<float>::quiet_NaN())
#endif

#endif	// #ifndef INCLUDED_WPOSIX
