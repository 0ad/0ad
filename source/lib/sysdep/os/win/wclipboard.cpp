/* Copyright (c) 2010 Wildfire Games
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
#include "lib/sysdep/clipboard.h"

#include "lib/sysdep/os/win/win.h"
#include "lib/sysdep/os/win/wutil.h"

// caller is responsible for freeing *hMem.
static LibError SetClipboardText(const wchar_t* text, HGLOBAL* hMem)
{
	const size_t numChars = wcslen(text);
	*hMem = GlobalAlloc(GMEM_MOVEABLE, (numChars+1) * sizeof(wchar_t));
	if(!*hMem)
		WARN_RETURN(ERR::NO_MEM);

	wchar_t* lockedText = (wchar_t*)GlobalLock(*hMem);
	if(!lockedText)
		WARN_RETURN(ERR::NO_MEM);
	wcscpy_s(lockedText, numChars+1, text);
	GlobalUnlock(*hMem);

	HANDLE hData = SetClipboardData(CF_UNICODETEXT, *hMem);
	if(!hData)	// failed
		WARN_RETURN(ERR::FAIL);

	return INFO::OK;
}

// "copy" text into the clipboard. replaces previous contents.
LibError sys_clipboard_set(const wchar_t* text)
{
	// note: MSDN claims that the window handle must not be 0;
	// that does actually work on WinXP, but we'll play it safe.
	if(!OpenClipboard(wutil_AppWindow()))
		WARN_RETURN(ERR::FAIL);
	EmptyClipboard();

	HGLOBAL hMem;
	LibError ret = SetClipboardText(text, &hMem);

	CloseClipboard();

	// note: MSDN's SetClipboardData documentation says hMem must not be
	// freed until after CloseClipboard. however, GlobalFree still fails
	// after the successful completion of both. we'll leave it in to avoid
	// memory leaks, but ignore its return value.
	(void)GlobalFree(hMem);

	return ret;
}


static wchar_t* CopyClipboardContents()
{
	// Windows NT/2000+ auto convert UNICODETEXT <-> TEXT
	HGLOBAL hMem = GetClipboardData(CF_UNICODETEXT);
	if(!hMem)
		return 0;

	const wchar_t* lockedText = (const wchar_t*)GlobalLock(hMem);
	if(!lockedText)
		return 0;

	const size_t numChars = GlobalSize(hMem)/sizeof(wchar_t) - 1;
	wchar_t* text = new wchar_t[numChars+1];
	wcscpy_s(text, numChars+1, lockedText);

	GlobalUnlock(hMem);

	return text;
}

// allow "pasting" from clipboard. returns the current contents if they
// can be represented as text, otherwise 0.
// when it is no longer needed, the returned pointer must be freed via
// sys_clipboard_free. (NB: not necessary if zero, but doesn't hurt)
wchar_t* sys_clipboard_get()
{
	if(!OpenClipboard(wutil_AppWindow()))
		return 0;
	wchar_t* const ret = CopyClipboardContents();
	CloseClipboard();
	return ret;
}


// frees memory used by <copy>, which must have been returned by
// sys_clipboard_get. see note above.
LibError sys_clipboard_free(wchar_t* text)
{
	delete[] text;
	return INFO::OK;
}
