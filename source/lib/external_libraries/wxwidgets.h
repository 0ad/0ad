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

/*
 * bring in wxWidgets headers, with compatibility fixes
 */

#ifndef INCLUDED_WXWIDGETS
#define INCLUDED_WXWIDGETS

// prevent wxWidgets from pulling in windows.h - it's mostly unnecessary
// and interferes with posix_sock's declarations.
#define _WINDOWS_	// <windows.h> include guard

// manually define what is actually needed from windows.h:
struct HINSTANCE__
{
	int unused;
};
typedef struct HINSTANCE__* HINSTANCE;	// definition as if STRICT were #defined


#include "wx/wxprec.h"

#include "wx/file.h"
#include "wx/ffile.h"
#include "wx/filename.h"
#include "wx/mimetype.h"
#include "wx/statline.h"
#include "wx/debugrpt.h"

#ifdef __WXMSW__
#include "wx/evtloop.h"     // for SetCriticalWindow()
#endif // __WXMSW__

// note: wxWidgets already does #pragma comment(lib) to add link targets.

#endif	// #ifndef INCLUDED_WXWIDGETS
