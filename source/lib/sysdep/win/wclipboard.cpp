#include "precompiled.h"
#include "../clipboard.h"

#include "win.h"
#include "wutil.h"

// caller is responsible for freeing *hMem.
static LibError SetClipboardText(const wchar_t* text, HGLOBAL* hMem)
{
	const size_t len = wcslen(text);
	*hMem = GlobalAlloc(GMEM_MOVEABLE, (len+1) * sizeof(wchar_t));
	if(!*hMem)
		WARN_RETURN(ERR::NO_MEM);

	wchar_t* lockedMemory = (wchar_t*)GlobalLock(*hMem);
	if(!lockedMemory)
		WARN_RETURN(ERR::NO_MEM);
	SAFE_WCSCPY(lockedMemory, text);
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

	// note: SetClipboardData says hMem must not be freed until after
	// CloseClipboard. however, GlobalFree still fails after the successful
	// completion of both. to avoid memory leaks when one of the calls fails,
	// we'll leave it in and just ignore the return value.
	(void)GlobalFree(hMem);

	return ret;
}


// allow "pasting" from clipboard. returns the current contents if they
// can be represented as text, otherwise 0.
// when it is no longer needed, the returned pointer must be freed via
// sys_clipboard_free. (NB: not necessary if zero, but doesn't hurt)
wchar_t* sys_clipboard_get()
{
	wchar_t* ret = 0;

	const HWND newOwner = 0;
	// MSDN: passing 0 requests the current task be granted ownership;
	// there's no need to pass our window handle.
	if(!OpenClipboard(newOwner))
		return 0;

	// Windows NT/2000+ auto convert UNICODETEXT <-> TEXT
	HGLOBAL hMem = GetClipboardData(CF_UNICODETEXT);
	if(hMem != 0)
	{
		wchar_t* text = (wchar_t*)GlobalLock(hMem);
		if(text)
		{
			SIZE_T size = GlobalSize(hMem);
			wchar_t* copy = (wchar_t*)malloc(size);	// unavoidable
			if(copy)
			{
				wcscpy(copy, text);
				ret = copy;
			}

			GlobalUnlock(hMem);
		}
	}

	CloseClipboard();

	return ret;
}


// frees memory used by <copy>, which must have been returned by
// sys_clipboard_get. see note above.
LibError sys_clipboard_free(wchar_t* copy)
{
	free(copy);
	return INFO::OK;
}
