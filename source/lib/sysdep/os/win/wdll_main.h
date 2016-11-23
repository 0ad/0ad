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
// for each project that builds a shared-library, include this "header" in
// one source file that is on the linker command line.
// (avoids the ICC remark "Main entry point was not seen")

#if OS_WIN
#include "lib/sysdep/os/win/win.h"

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD UNUSED(reason), LPVOID UNUSED(reserved))
{
	// avoid unnecessary DLL_THREAD_ATTACH/DETACH calls
	// (ignore failure - happens if the DLL uses TLS)
	(void)DisableThreadLibraryCalls(hInstance);
	return TRUE;	// success (ignored unless reason == DLL_PROCESS_ATTACH)
}

#endif
