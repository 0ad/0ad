#include "precompiled.h"

#include "lib.h"

#include "oal.h"

#ifdef _MSC_VER
#pragma comment(lib, "openal32.lib")
#pragma comment(lib, "alut.lib")
#endif

int oal_Init()
{
	ONCE(alutInit(0, 0));
	return 0;
}


int oal_Shutdown()
{
	alutExit();
	return 0;
}