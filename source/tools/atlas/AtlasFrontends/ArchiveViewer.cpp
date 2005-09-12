#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define ATLASDLLIMPEXP extern "C" __declspec(dllimport)

#include "AtlasUI/Misc/DLLInterface.h"

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPTSTR, int) {
	Atlas_StartWindow(L"ArchiveViewer");
	return 0;
}
