#include "precompiled.h"
#include "lib/sysdep/clipboard.h"

#include "win.h"
#include "wutil.h"

// caller is responsible for freeing *hMem.
static LibError SetClipboardText(const wchar_t* text, HGLOBAL* hMem)
{
	const size_t len = wcslen(text);
	*hMem = GlobalAlloc(GMEM_MOVEABLE, (len+1) * sizeof(wchar_t));
	if(!*hMem)
		WARN_RETURN(ERR::NO_MEM);

	wchar_t* lockedText = (wchar_t*)GlobalLock(*hMem);
	if(!lockedText)
		WARN_RETURN(ERR::NO_MEM);
	SAFE_WCSCPY(lockedText, text);
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

	wchar_t* lockedText = (wchar_t*)GlobalLock(hMem);
	if(!lockedText)
		return 0;

	const SIZE_T size = GlobalSize(hMem);
	wchar_t* text = new wchar_t[size+1];
	SAFE_WCSCPY(text, lockedText);

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
