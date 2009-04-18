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
 * =========================================================================
 * File        : wxwidgets.h
 * Project     : 0 A.D.
 * Description : bring in wxWidgets headers, with compatibility fixes
 * =========================================================================
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
