// Precompiled headers:

#ifdef _WIN32
# define HAVE_PCH
#endif

#ifdef HAVE_PCH

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN

// Include useful wx headers
#include "wx/wxprec.h"
#include "wx/listctrl.h"

#include <vector>

// Nicer memory-leak detection:
#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK ,__FILE__, __LINE__)
#endif

#endif // HAVE_PCH
