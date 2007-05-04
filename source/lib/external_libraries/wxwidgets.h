/**
 * =========================================================================
 * File        : wxw.h
 * Project     : 0 A.D.
 * Description : pulls in wxWidgets headers, with compatibility fixes
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2007 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef INCLUDED_WXW
#define INCLUDED_WXW

// prevent wxWidgets from pulling in windows.h - it's mostly unnecessary
// and interferes with posix_sock's declarations.
#define _WINDOWS_	// <windows.h> include guard

// manually define what is actually needed from windows.h:
struct HINSTANCE__
{
	int unused;
};
typedef struct HINSTANCE__* HINSTANCE;	// definition as if STRICT were #defined


#include "wx/wx.h"

#include "wx/file.h"
#include "wx/ffile.h"
#include "wx/filename.h"
#include "wx/mimetype.h"

#include "wx/statline.h"

#include "wx/debugrpt.h"

#ifdef __WXMSW__
#include "wx/evtloop.h"     // for SetCriticalWindow()
#endif // __WXMSW__


#endif	// #ifndef INCLUDED_WXW
