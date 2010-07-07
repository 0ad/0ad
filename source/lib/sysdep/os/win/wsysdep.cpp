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

/*
 * Windows backend of the sysdep interface
 */

#include "precompiled.h"
#include "lib/sysdep/sysdep.h"

#include "lib/sysdep/os/win/win.h"	// includes windows.h; must come before shlobj
#include <shlobj.h>	// pick_dir
#include <shellapi.h>	// open_url
#include <Wincrypt.h>

#include "lib/sysdep/clipboard.h"
#include "lib/sysdep/os/win/error_dialog.h"
#include "lib/sysdep/os/win/wutil.h"



#if MSC_VERSION
#pragma comment(lib, "shell32.lib")	// for sys_pick_directory SH* calls
#endif


void sys_display_msg(const wchar_t* caption, const wchar_t* msg)
{
	MessageBoxW(0, msg, caption, MB_ICONEXCLAMATION|MB_TASKMODAL|MB_SETFOREGROUND);
}


//-----------------------------------------------------------------------------
// "program error" dialog (triggered by debug_assert and exception)
//-----------------------------------------------------------------------------

// support for resizing the dialog / its controls
// (have to do this manually - grr)

static POINTS dlg_client_origin;
static POINTS dlg_prev_client_size;

static const size_t ANCHOR_LEFT   = 0x01;
static const size_t ANCHOR_RIGHT  = 0x02;
static const size_t ANCHOR_TOP    = 0x04;
static const size_t ANCHOR_BOTTOM = 0x08;
static const size_t ANCHOR_ALL    = 0x0f;

static void dlg_resize_control(HWND hDlg, int dlg_item, int dx,int dy, size_t anchors)
{
	HWND hControl = GetDlgItem(hDlg, dlg_item);
	RECT r;
	GetWindowRect(hControl, &r);

	int w = r.right - r.left, h = r.bottom - r.top;
	int x = r.left - dlg_client_origin.x, y = r.top - dlg_client_origin.y;

	if(anchors & ANCHOR_RIGHT)
	{
		// right only
		if(!(anchors & ANCHOR_LEFT))
			x += dx;
		// horizontal (stretch width)
		else
			w += dx;
	}

	if(anchors & ANCHOR_BOTTOM)
	{
		// bottom only
		if(!(anchors & ANCHOR_TOP))
			y += dy;
		// vertical (stretch height)
		else
			h += dy;
	}

	SetWindowPos(hControl, 0, x,y, w,h, SWP_NOZORDER);
}


static void dlg_resize(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	// 'minimize' was clicked. we need to ignore this, otherwise
	// dx/dy would reduce some control positions to less than 0.
	// since Windows clips them, we wouldn't later be able to
	// reconstruct the previous values when 'restoring'.
	if(wParam == SIZE_MINIMIZED)
		return;

	// first call for this dialog instance. WM_MOVE hasn't been sent yet,
	// so dlg_client_origin are invalid => must not call resize_control().
	// we need to set dlg_prev_client_size for the next call before exiting.
	bool first_call = (dlg_prev_client_size.y == 0);

	POINTS dlg_client_size = MAKEPOINTS(lParam);
	int dx = dlg_client_size.x - dlg_prev_client_size.x;
	int dy = dlg_client_size.y - dlg_prev_client_size.y;
	dlg_prev_client_size = dlg_client_size;

	if(first_call)
		return;

	dlg_resize_control(hDlg, IDC_CONTINUE, dx,dy, ANCHOR_LEFT|ANCHOR_BOTTOM);
	dlg_resize_control(hDlg, IDC_SUPPRESS, dx,dy, ANCHOR_LEFT|ANCHOR_BOTTOM);
	dlg_resize_control(hDlg, IDC_BREAK   , dx,dy, ANCHOR_LEFT|ANCHOR_BOTTOM);
	dlg_resize_control(hDlg, IDC_EXIT    , dx,dy, ANCHOR_LEFT|ANCHOR_BOTTOM);
	dlg_resize_control(hDlg, IDC_COPY    , dx,dy, ANCHOR_RIGHT|ANCHOR_BOTTOM);
	dlg_resize_control(hDlg, IDC_EDIT1   , dx,dy, ANCHOR_ALL);
}



struct DialogParams
{
	const wchar_t* text;
	size_t flags;
};


static INT_PTR CALLBACK error_dialog_proc(HWND hDlg, unsigned int msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			const DialogParams* params = (const DialogParams*)lParam;
			HWND hWnd;

			// need to reset for new instance of dialog
			dlg_client_origin.x = dlg_client_origin.y = 0;
			dlg_prev_client_size.x = dlg_prev_client_size.y = 0;

			if(!(params->flags & DE_ALLOW_SUPPRESS))
			{
				hWnd = GetDlgItem(hDlg, IDC_SUPPRESS);
				EnableWindow(hWnd, FALSE);
			}

			// set fixed font for readability
			hWnd = GetDlgItem(hDlg, IDC_EDIT1);
			HGDIOBJ hObj = (HGDIOBJ)GetStockObject(SYSTEM_FIXED_FONT);
			LPARAM redraw = FALSE;
			SendMessage(hWnd, WM_SETFONT, (WPARAM)hObj, redraw);

			SetDlgItemTextW(hDlg, IDC_EDIT1, params->text);
			return TRUE;	// set default keyboard focus
		}

	case WM_SYSCOMMAND:
		// close dialog if [X] is clicked (doesn't happen automatically)
		// note: lower 4 bits are reserved
		if((wParam & 0xFFF0) == SC_CLOSE)
		{
			EndDialog(hDlg, 0);
			return 0;	// processed
		}
		break;

		// return 0 if processed, otherwise break
	case WM_COMMAND:
		switch(wParam)
		{
		case IDC_COPY:
			{
				// note: allocating on the stack would be easier+safer,
				// but this is too big.
				const size_t max_chars = 128*KiB;
				wchar_t* buf = new wchar_t[max_chars];
				GetDlgItemTextW(hDlg, IDC_EDIT1, buf, max_chars);
				sys_clipboard_set(buf);
				delete[] buf;
				return 0;
			}

		case IDC_CONTINUE:
			EndDialog(hDlg, ER_CONTINUE);
			return 0;
		case IDC_SUPPRESS:
			EndDialog(hDlg, ER_SUPPRESS);
			return 0;
		case IDC_BREAK:
			EndDialog(hDlg, ER_BREAK);
			return 0;
		case IDC_EXIT:
			EndDialog(hDlg, ER_EXIT);
			return 0;

		default:
			break;
		}
		break;

	case WM_MOVE:
		dlg_client_origin = MAKEPOINTS(lParam);
		break;

	case WM_GETMINMAXINFO:
		{
			// we must make sure resize_control will never set negative coords -
			// Windows would clip them, and its real position would be lost.
			// restrict to a reasonable and good looking minimum size [pixels].
			MINMAXINFO* mmi = (MINMAXINFO*)lParam;
			mmi->ptMinTrackSize.x = 407;
			mmi->ptMinTrackSize.y = 159;	// determined experimentally
			return 0;
		}

	case WM_SIZE:
		dlg_resize(hDlg, wParam, lParam);
		break;

	default:
		break;
	}

	// we didn't process the message; caller will perform default action.
	return FALSE;
}


ErrorReaction sys_display_error(const wchar_t* text, size_t flags)
{
	// note: other threads might still be running, crash and take down the
	// process before we have a chance to display this error message.
	// ideally we would suspend them all and resume when finished; however,
	// they may be holding system-wide locks (e.g. heap or loader) that
	// are potentially needed by DialogBoxParam. in that case, deadlock
	// would result; this is much worse than a crash because no error
	// at all is displayed to the end-user. therefore, do nothing here.

	// temporarily remove any pending quit message from the queue because
	// it would prevent the dialog from being displayed (DialogBoxParam
	// returns IDOK without doing anything). will be restored below.
	// notes:
	// - this isn't only relevant at exit - Windows also posts one if
	//   window init fails. therefore, it is important that errors can be
	//   displayed regardless.
	// - by passing hWnd=0, we check all windows belonging to the current
	//   thread. there is no reason to use hWndParent below.
	MSG msg;
	BOOL quit_pending = PeekMessage(&msg, 0, WM_QUIT, WM_QUIT, PM_REMOVE);

	const HINSTANCE hInstance = wutil_LibModuleHandle;
	LPCWSTR lpTemplateName = MAKEINTRESOURCEW(IDD_DIALOG1);
	const DialogParams params = { text, flags };
	// get the enclosing app's window handle. we can't just pass 0 or
	// the desktop window because the dialog must be modal (if the app
	// continues running, it may crash and take down the process before
	// we've managed to show the dialog).
	const HWND hWndParent = wutil_AppWindow();

	INT_PTR ret = DialogBoxParamW(hInstance, lpTemplateName, hWndParent, error_dialog_proc, (LPARAM)&params);

	if(quit_pending)
		PostQuitMessage((int)msg.wParam);

	// failed; warn user and make sure we return an ErrorReaction.
	if(ret == 0 || ret == -1)
	{
		debug_DisplayMessage(L"Error", L"Unable to display detailed error dialog.");
		return ER_CONTINUE;
	}
	return (ErrorReaction)ret;
}


//-----------------------------------------------------------------------------
// misc
//-----------------------------------------------------------------------------

LibError sys_error_description_r(int user_err, wchar_t* buf, size_t max_chars)
{
	// validate user_err - Win32 doesn't have negative error numbers
	if(user_err < 0)
		return ERR::FAIL;	// NOWARN

	const DWORD err = user_err? (DWORD)user_err : GetLastError();

	// no one likes to see "The operation completed successfully" in
	// error messages, so return more descriptive text instead.
	if(err == 0)
	{
		wcscpy_s(buf, max_chars, L"0 (no error code was set)");
		return INFO::OK;
	}

	wchar_t message[200];
	{
		const LPCVOID source = 0;	// ignored (we're not using FROM_HMODULE etc.)
		const DWORD lang_id = 0;	// look for neutral, then current locale
		va_list* args = 0;			// we don't care about "inserts"
		const DWORD charsWritten = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, source, err, lang_id, message, (DWORD)ARRAY_SIZE(message), args);
		if(!charsWritten)
			WARN_RETURN(ERR::FAIL);
		debug_assert(charsWritten < max_chars);
	}

	const int charsWritten = swprintf_s(buf, max_chars, L"%d (%ls)", err, message);
	debug_assert(charsWritten != -1);
	return INFO::OK;
}


LibError sys_get_module_filename(void* addr, fs::wpath& pathname)
{
	MEMORY_BASIC_INFORMATION mbi;
	const SIZE_T bytesWritten = VirtualQuery(addr, &mbi, sizeof(mbi));
	if(!bytesWritten)
		return LibError_from_GLE();
	debug_assert(bytesWritten >= sizeof(mbi));
	const HMODULE hModule = (HMODULE)mbi.AllocationBase;

	wchar_t pathnameBuf[MAX_PATH+1];
	const DWORD charsWritten = GetModuleFileNameW(hModule, pathnameBuf, (DWORD)ARRAY_SIZE(pathnameBuf));
	if(charsWritten == 0)
		return LibError_from_GLE();
	debug_assert(charsWritten < ARRAY_SIZE(pathnameBuf));
	pathname = pathnameBuf;
	return INFO::OK;
}


LibError sys_get_executable_name(fs::wpath& pathname)
{
	wchar_t pathnameBuf[MAX_PATH+1];
	const DWORD charsWritten = GetModuleFileNameW(0, pathnameBuf, (DWORD)ARRAY_SIZE(pathnameBuf));
	if(charsWritten == 0)
		return LibError_from_GLE();
	debug_assert(charsWritten < ARRAY_SIZE(pathnameBuf));
	pathname = pathnameBuf;
	return INFO::OK;
}

std::wstring sys_get_user_name()
{
	wchar_t usernameBuf[256];
	DWORD size = ARRAY_SIZE(usernameBuf);
	if(!GetUserNameW(usernameBuf, &size))
		return L"";
	return usernameBuf;
}

// callback for shell directory picker: used to set starting directory
// (for user convenience).
static int CALLBACK BrowseCallback(HWND hWnd, unsigned int msg, LPARAM UNUSED(lParam), LPARAM lpData)
{
	if(msg == BFFM_INITIALIZED)
	{
		const WPARAM wParam = TRUE;	// lpData is a Unicode string, not PIDL.
		LRESULT ret = SendMessage(hWnd, BFFM_SETSELECTIONW, wParam, lpData);
		return 1;
	}

	return 0;
}

LibError sys_pick_directory(fs::wpath& path)
{
	// (must not use multi-threaded apartment due to BIF_NEWDIALOGSTYLE)
	const HRESULT hr = CoInitialize(0);
	debug_assert(hr == S_OK || hr == S_FALSE);	// S_FALSE == already initialized

	// the above BFFM_SETSELECTIONW can't deal with '/' separators,
	// which is what string() returns.
	const std::wstring initialPath = path.external_directory_string();

	// note: bi.pszDisplayName isn't the full path, so it isn't of any use.
	BROWSEINFOW bi;
	memset(&bi, 0, sizeof(bi));
	bi.ulFlags = BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE|BIF_NONEWFOLDERBUTTON;
	// for setting starting directory:
	bi.lpfn = (BFFCALLBACK)BrowseCallback;
	bi.lParam = (LPARAM)initialPath.c_str();
	const LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
	if(!pidl)	// user canceled
		return INFO::SKIPPED;

	// translate ITEMIDLIST to string
	wchar_t pathBuf[MAX_PATH];
	const BOOL ok = SHGetPathFromIDListW(pidl, pathBuf);

	// free the ITEMIDLIST
	IMalloc* p_malloc;
	SHGetMalloc(&p_malloc);
	p_malloc->Free(pidl);
	p_malloc->Release();

	if(ok == TRUE)
	{
		path = pathBuf;
		return INFO::OK;
	}

	return LibError_from_GLE();
}

LibError sys_open_url(const std::string& url)
{
	HINSTANCE r = ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
	if ((int)r > 32)
		return INFO::OK;

	WARN_RETURN(ERR::FAIL);
}

LibError sys_generate_random_bytes(u8* buf, size_t count)
{
	HCRYPTPROV hCryptProv;

	if(!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, 0))
		WARN_RETURN(ERR::FAIL);

	if(!CryptGenRandom(hCryptProv, count, buf))
		WARN_RETURN(ERR::FAIL);

	if (!CryptReleaseContext(hCryptProv, 0))
		WARN_RETURN(ERR::FAIL);

	return INFO::OK;
}
