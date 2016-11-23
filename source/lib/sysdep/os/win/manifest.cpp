/* Copyright (c) 2015 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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

/*
NOTE: vcbuild.exe (as used by the autobuilder) seems to ignore these linker
comments, so we have to duplicate these commands into premake.lua too.
Remember to keep them in sync with this file.
(Duplicate entries appear to get omitted from the .manifest file so there
should be no harmful effects.)
*/

#endif
