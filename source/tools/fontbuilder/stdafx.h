// $Id: stdafx.h,v 1.5 2004/06/19 13:46:11 philip Exp $

// Precompiled headers

#ifdef _WIN32
# define HAVE_PCH
#endif

#ifdef HAVE_PCH

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN

// Disable some complaints in wx headers
#pragma warning (push)
#pragma warning (disable: 4267 4311)

#ifdef __INTEL_COMPILER
// Disable some of the excessively pedantic icl /W4 warnings (remarks)
#pragma warning (disable: 193 373 444 981 383 1125 1418)
#endif

// Include relevant wx headers
#include "wx/wxprec.h"
#include "wx/wx.h"
#include "wx/menu.h"
#include "wx/filedlg.h"
#include "wx/button.h"
#include "wx/image.h"
#include "wx/spinctrl.h"
#include "wx/regex.h"
#include "wx/ffile.h"
#include "wx/utils.h"
#include "wx/progdlg.h"
#include "wx/wxexpr.h"
#include "wx/docview.h"
#include "wx/config.h"
#include "wx/filename.h"
#include "wx/config.h"
#include "wx/log.h"

#pragma warning (pop)

#include <set>
#include <list>


//// Things other than precompiled headers which
  // are just useful to include everywhere:

// For nicer memory-leak detection
#ifdef _DEBUG
 #include <crtdbg.h>
 #define new new(_NORMAL_BLOCK ,__FILE__, __LINE__)
#endif

// Don't care about copy constructors / assignment operators
#pragma warning (disable: 4511 4512)

#ifdef __INTEL_COMPILER
// Disable some of the excessively pedantic warnings again
#pragma warning (disable: 193 373 444 981 383 1418)
#endif

#endif // HAVE_PCH

#include "wx/defs.h"
