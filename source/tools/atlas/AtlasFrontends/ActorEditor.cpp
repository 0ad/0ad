/* Copyright (C) 2011 Wildfire Games.
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

/* Don't use pragmas here to set the manifests, instead do this in premake.lua
 * since pragmas get ignored by the autobuilder
 * (see comments in lib\sysdep\os\win\manifest.cpp)
 */

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
	Atlas_StartWindow(L"ActorEditor");
	return 0;
}
