/**
 * =========================================================================
 * File        : wxwidgets.h
 * Project     : 0 A.D.
 * Description : pulls in wxWidgets headers, with compatibility fixes
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


// automatically link against the required library
#if MSC_VERSION
# ifdef NDEBUG
# else
#  pragma comment(lib, "wxmsw28ud_core.lib")
#  pragma comment(lib, "wxmsw28ud_qa.lib")
#  pragma comment(lib, "wxbase28ud.lib")
#  pragma comment(lib, "wxbase28ud_xml.lib")


//#  pragma comment(lib, "wxbase28ud_net.lib")
//#  pragma comment(lib, "wxbase28ud_odbc.lib")
//#  pragma comment(lib, "wxmsw28ud_adv.lib")
//#  pragma comment(lib, "wxmsw28ud_aui.lib")
//#  pragma comment(lib, "wxmsw28ud_dbgrid.lib")
//#  pragma comment(lib, "wxmsw28ud_gl.lib")
//#  pragma comment(lib, "wxmsw28ud_html.lib")
//#  pragma comment(lib, "wxmsw28ud_media.lib")
//#  pragma comment(lib, "wxmsw28ud_richtext.lib")
//#  pragma comment(lib, "wxmsw28ud_xrc.lib")
//#  pragma comment(lib, "wxexpatd.lib")
//#  pragma comment(lib, "wxpngd.lib")
//#  pragma comment(lib, "wxjpegd.lib")
//#  pragma comment(lib, "wxtiffd.lib")
//#  pragma comment(lib, "wxzlibd.lib")
//#  pragma comment(lib, "wxregexd.lib")

#  pragma comment(lib, "Rpcrt4.lib")	// Uuid
#  pragma comment(lib, "comctl32.lib")	// ImageList_*


# endif	// NDEBUG
#endif	// MSC_VERSION


#endif	// #ifndef INCLUDED_WXWIDGETS
