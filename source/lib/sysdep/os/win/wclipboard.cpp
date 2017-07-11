/* Copyright (C) 2010 Wildfire Games.
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


// caller is responsible for freeing hMem.
static Status SetClipboardText(const wchar_t* text, HGLOBAL& hMem)
{
	const size_t numChars = wcslen(text);
	hMem = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, (numChars + 1) * sizeof(wchar_t));
	if(!hMem)
		WARN_RETURN(ERR::NO_MEM);

	wchar_t* lockedText = (wchar_t*)GlobalLock(hMem);
	if(!lockedText)
		WARN_RETURN(ERR::NO_MEM);
	wcscpy_s(lockedText, numChars+1, text);
	GlobalUnlock(hMem);

	HANDLE hData = SetClipboardData(CF_UNICODETEXT, hMem);
	if(!hData)	// failed
		WARN_RETURN(ERR::FAIL);

	return INFO::OK;
}


// @return INFO::OK iff text has been assigned a pointer (which the
// caller must free via sys_clipboard_free) to the clipboard text.
static Status GetClipboardText(wchar_t*& text)
{
	// NB: Windows NT/2000+ auto convert CF_UNICODETEXT <-> CF_TEXT.

	if(!IsClipboardFormatAvailable(CF_UNICODETEXT))
		return INFO::CANNOT_HANDLE;

	HGLOBAL hMem = GetClipboardData(CF_UNICODETEXT);
	if(!hMem)
		WARN_RETURN(ERR::FAIL);

	const wchar_t* lockedText = (const wchar_t*)GlobalLock(hMem);
	if(!lockedText)
		WARN_RETURN(ERR::NO_MEM);

	const size_t size = GlobalSize(hMem);
	text = (wchar_t*)malloc(size);
	if(!text)
		WARN_RETURN(ERR::NO_MEM);
	wcscpy_s(text, size/sizeof(wchar_t), lockedText);

	(void)GlobalUnlock(hMem);

	return INFO::OK;
}


// OpenClipboard parameter.
// NB: using wutil_AppWindow() causes GlobalLock to fail.
static const HWND hWndNewOwner = 0;	// MSDN: associate with "current task"

Status sys_clipboard_set(const wchar_t* text)
{
	if(!OpenClipboard(hWndNewOwner))
		WARN_RETURN(ERR::FAIL);

	WARN_IF_FALSE(EmptyClipboard());

	// NB: to enable copy/pasting something other than text, add
	// message handlers for WM_RENDERFORMAT and WM_RENDERALLFORMATS.
	HGLOBAL hMem;
	Status ret = SetClipboardText(text, hMem);

	WARN_IF_FALSE(CloseClipboard());	// must happen before GlobalFree

	ENSURE(GlobalFree(hMem) == 0);	// (0 indicates success)

	return ret;
}


wchar_t* sys_clipboard_get()
{
	if(!OpenClipboard(hWndNewOwner))
		return 0;

	wchar_t* text;
	Status ret = GetClipboardText(text);

	WARN_IF_FALSE(CloseClipboard());

	return (ret == INFO::OK)? text : 0;
}


Status sys_clipboard_free(wchar_t* text)
{
	free(text);
	return INFO::OK;
}
