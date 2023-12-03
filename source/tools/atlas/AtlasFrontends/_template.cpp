/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef _WIN32

# define WIN32_LEAN_AND_MEAN
# include <windows.h>

// Use WinXP-style controls
# if _MSC_VER >= 1400 // (can't be bothered to implement this for VC7.1...)
#  pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df'\"")
# endif

# define ATLASDLLIMPEXP extern "C" __declspec(dllimport)
#else
# define ATLASDLLIMPEXP extern "C"
#endif

#include "AtlasUI/Misc/DLLInterface.h"

#ifdef _WIN32
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
#else
int main()
#endif
{
	Atlas_StartWindow(L"$$WINDOW_NAME$$");
	return 0;
}
