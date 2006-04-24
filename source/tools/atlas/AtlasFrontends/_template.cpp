#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Use WinXP-style controls
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df'\"")

#define ATLASDLLIMPEXP extern "C" __declspec(dllimport)

#include "AtlasUI/Misc/DLLInterface.h"

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPTSTR, int) {
	Atlas_StartWindow(L"$$WINDOW_NAME$$");
	return 0;
}
