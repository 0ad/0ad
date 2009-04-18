/**
 * =========================================================================
 * File        : wxwidgets.h
 * Project     : 0 A.D.
 * Description : bring in wxWidgets headers, with compatibility fixes
 * =========================================================================
 */

// license: GPL; see lib/license.txt

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
