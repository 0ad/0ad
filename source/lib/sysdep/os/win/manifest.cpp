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

#include "precompiled.h"

/*
To use XP-style themed controls, we need to use the manifest to specify the
desired version. (This must be set in the game's .exe in order to affect Atlas.)

For VC7.1, we use manifest.rc to include a complete manifest file.
For VC8.0, which already generates its own manifest, we use the line below
to add the necessary parts to that generated manifest.

ICC 10.1 IPO considers this string to be an input file, hence this
is currently disabled there.
*/
#if MSC_VERSION >= 1400 && !ICC_VERSION && defined(LIB_STATIC_LINK)
# if ARCH_IA32
#  pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df'\"")
# elif ARCH_AMD64
#  pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df'\"")
# endif
#endif
