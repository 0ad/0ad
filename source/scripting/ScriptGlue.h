
#ifndef _SCRIPTGLUE_H_
#define _SCRIPTGLUE_H_

#include "ScriptingHost.h"

#include "LightEnv.h"	// required by g_LightEnv declaration below.

// referenced by ScriptingHost.cpp
extern JSFunctionSpec ScriptFunctionTable[];
extern JSPropertySpec ScriptGlobalTable[];

// dependencies (moved to header to avoid L4 warnings)
// .. from main.cpp:
extern "C" int fps;
extern void kill_mainloop();
extern CStr g_CursorName;
extern void StartGame();
extern void EndGame();
// .. other
extern CLightEnv g_LightEnv;
#ifdef _WIN32
extern int GetVRAMInfo(int&, int&);
#endif

#endif	// #ifndef _SCRIPTGLUE_H_
