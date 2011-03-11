// for each project that builds a shared-library, include this "header" in
// one source file that is on the linker command line.
// (avoids the ICC remark "Main entry point was not seen")

#if OS_WIN
#include "lib/sysdep/os/win/win.h"

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD UNUSED(reason), LPVOID UNUSED(reserved))
{
	// avoid unnecessary DLL_THREAD_ATTACH/DETACH calls
	WARN_IF_FALSE(DisableThreadLibraryCalls(hInstance));
	return TRUE;	// success (ignored unless reason == DLL_PROCESS_ATTACH)
}

#endif
