#include "lib/precompiled.h"	// common precompiled header

// "engine"-specific PCH:

#include "ps/Pyrogenesis.h"	// MICROLOG and old error system

#if HAVE_PCH

// some other external libraries that are used in several places:
// .. CStr is included very frequently, so a reasonable amount of time is
//    saved by including it here. (~10% in a full rebuild, as of r2365)
#include "ps/CStr.h"
#include "scripting/SpiderMonkey.h"
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#endif // HAVE_PCH
