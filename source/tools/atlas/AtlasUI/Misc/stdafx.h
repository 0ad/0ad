// Precompiled headers:

#ifdef _WIN32
# define HAVE_PCH
#endif

#ifdef HAVE_PCH

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN

// Include useful wx headers
#include "wx/wxprec.h"

#include "wx/artprov.h"
#include "wx/cmdproc.h"
#include "wx/colordlg.h"
#include "wx/config.h"
#include "wx/dialog.h"
#include "wx/dir.h"
#include "wx/dnd.h"
#include "wx/docview.h"
#include "wx/file.h"
#include "wx/filename.h"
#include "wx/filesys.h"
#include "wx/glcanvas.h"
#include "wx/image.h"
#include "wx/listctrl.h"
#include "wx/mstream.h"
#include "wx/progdlg.h"
#include "wx/regex.h"
#include "wx/sound.h"
#include "wx/spinctrl.h"
#include "wx/splitter.h"
#include "wx/tooltip.h"
#include "wx/treectrl.h"
#include "wx/wfstream.h"
#include "wx/zstream.h"

#include <vector>
#include <string>
#include <set>
#include <stack>
#include <map>
#include <limits>
#include <cassert>

#include <boost/signals.hpp>
#include <boost/bind.hpp>

// Nicer memory-leak detection:
#ifdef _DEBUG
# include <crtdbg.h>
# define new new(_NORMAL_BLOCK ,__FILE__, __LINE__)
#endif

#endif // HAVE_PCH

#ifndef HAVE_PCH
// If no PCH, just include a load of common headers anyway
# include "wx/wx.h"
#endif


#define ATLASDLLIMPEXP extern "C" __declspec(dllexport)
