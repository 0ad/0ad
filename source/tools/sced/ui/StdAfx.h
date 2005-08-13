// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__806A78B8_0008_491A_9FB6_5FF892838693__INCLUDED_)
#define AFX_STDAFX_H__806A78B8_0008_491A_9FB6_5FF892838693__INCLUDED_

#undef new // as defined by precompiled.h

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

// Fix nasty conflicts between various header files.
#define _SIZE_T_DEFINED
#include "posix.h"
#include "CStr.h"
#pragma push_macro("UNUSED") // remember the original version, so we can revert to it later
#undef UNUSED

#undef _WINDOWS_

#define WINVER 0x0500		// Windows 2000

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#pragma pop_macro("UNUSED")

#include "resource.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__806A78B8_0008_491A_9FB6_5FF892838693__INCLUDED_)
