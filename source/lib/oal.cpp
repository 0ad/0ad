#include "precompiled.h"

#include "lib.h"

#include "oal.h"

#ifdef _MSC_VER
#pragma comment(lib, "openal32.lib")
#pragma comment(lib, "alut.lib")
#endif



// called as late as possible, i.e. the first time sound/music is played
// (either from module init there, or from the play routine itself).
// this delays library load, leading to faster perceived app startup.
// registers an atexit routine for cleanup.
// no harm if called more than once.
int oal_Init()
{
	ONCE({
		alutInit(0, 0);
		atexit(alutExit);
	});

	return 0;
}
