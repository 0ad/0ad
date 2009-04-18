/**
 * =========================================================================
 * File        : openal.h
 * Project     : 0 A.D.
 * Description : bring in OpenAL header+library, with compatibility fixes
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_OPENAL
#define INCLUDED_OPENAL

#if OS_MACOSX
# include <OpenAL/al.h>
# include <OpenAL/alc.h>
#else
# include <AL/al.h>
# include <AL/alc.h>
#endif

#if MSC_VERSION
# pragma comment(lib, "openal32.lib")
#endif

#endif	// #ifndef INCLUDED_OPENAL
