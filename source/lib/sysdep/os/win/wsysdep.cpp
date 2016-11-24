/* Copyright (c) 2011 Wildfire Games
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

#include "lib/alignment.h"
#include "lib/sysdep/os/win/win.h"	// includes windows.h; must come before shlobj
#include <shlobj.h>	// pick_dir
#include <shellapi.h>	// open_url
#include <Wincrypt.h>
#include <WindowsX.h>	// message crackers
#include <winhttp.h>

#include "lib/sysdep/clipboard.h"
#include "lib/sysdep/os/win/error_dialog.h"
#include "lib/sysdep/os/win/wutil.h"

#if CONFIG_ENABLE_BOOST
# include <boost/algorithm/string.hpp>
#endif


#if MSC_VERSION
#pragma comment(lib, "shell32.lib")	// for sys_pick_directory SH* calls
#pragma comment(lib, "winhttp.lib")
#endif


bool sys_IsDebuggerPresent()
{
	return (IsDebuggerPresent() != 0);
}


std::wstring sys_WideFromArgv(const char* argv_i)
{
	// NB: despite http://cbloomrants.blogspot.com/2008/06/06-14-08-1.html,
	// WinXP x64 EN cmd.exe (chcp reports 437) encodes argv u-umlaut
	// (entered manually or via auto-complete) via cp1252. the same applies
	// to WinXP SP2 DE (where chcp reports 850).
	const UINT cp = CP_ACP;
	const DWORD flags = MB_PRECOMPOSED|MB_ERR_INVALID_CHARS;
	const int inputSize = -1;	// null-terminated
	std::vector<wchar_t> buf(strlen(argv_i)+1);	// (upper bound on number of characters)
	// NB: avoid mbstowcs because it may specify another locale
	const int ret = MultiByteToWideChar(cp, flags, argv_i, (int)inputSize, &buf[0], (int)buf.size());
	ENSURE(ret != 0);
	return std::wstring(&buf[0]);
}


void sys_display_msg(const wchar_t* caption, const wchar_t* msg)
{
	MessageBoxW(0, msg, caption, MB_ICONEXCLAMATION|MB_TASKMODAL|MB_SETFOREGROUND);
}


//-----------------------------------------------------------------------------
// "program error" dialog (triggered by ENSURE and exception)
//-----------------------------------------------------------------------------

// support for resizing the dialog / its controls (must be done manually)

static POINTS dlg_clientOrigin;
static POINTS dlg_prevClientSize;

static void dlg_OnMove(HWND UNUSED(hDlg), int x, int y)
{
	dlg_clientOrigin.x = (short)x;
	dlg_clientOrigin.y = (short)y;
}


static const size_t ANCHOR_LEFT   = 0x01;
static const size_t ANCHOR_RIGHT  = 0x02;
static const size_t ANCHOR_TOP    = 0x04;
static const size_t ANCHOR_BOTTOM = 0x08;
static const size_t ANCHOR_ALL    = 0x0F;

static void dlg_ResizeControl(HWND hDlg, int dlgItem, int dx, int dy, size_t anchors)
{
	HWND hControl = GetDlgItem(hDlg, dlgItem);
	RECT r;
	GetWindowRect(hControl, &r);

	int w = r.right - r.left, h = r.bottom - r.top;
	int x = r.left - dlg_clientOrigin.x, y = r.top - dlg_clientOrigin.y;

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


static void dlg_OnSize(HWND hDlg, UINT state, int clientSizeX, int clientSizeY)
{
	// 'minimize' was clicked. we need to ignore this, otherwise
	// dx/dy would reduce some control positions to less than 0.
	// since Windows clips them, we wouldn't later be able to
	// reconstruct the previous values when 'restoring'.
	if(state == SIZE_MINIMIZED)
		return;

	// NB: origin might legitimately be 0, but we know it is invalid
	// on the first call to this function, where dlg_prevClientSize is 0.
	const bool isOriginValid = (dlg_prevClientSize.y != 0);

	const int dx = clientSizeX - dlg_prevClientSize.x;
	const int dy = clientSizeY - dlg_prevClientSize.y;
	dlg_prevClientSize.x = (short)clientSizeX;
	dlg_prevClientSize.y = (short)clientSizeY;

	if(!isOriginValid)	// must not call dlg_ResizeControl
		return;

	dlg_ResizeControl(hDlg, IDC_CONTINUE, dx,dy, ANCHOR_LEFT|ANCHOR_BOTTOM);
	dlg_ResizeControl(hDlg, IDC_SUPPRESS, dx,dy, ANCHOR_LEFT|ANCHOR_BOTTOM);
	dlg_ResizeControl(hDlg, IDC_BREAK   , dx,dy, ANCHOR_LEFT|ANCHOR_BOTTOM);
	dlg_ResizeControl(hDlg, IDC_EXIT    , dx,dy, ANCHOR_LEFT|ANCHOR_BOTTOM);
	dlg_ResizeControl(hDlg, IDC_COPY    , dx,dy, ANCHOR_RIGHT|ANCHOR_BOTTOM);
	dlg_ResizeControl(hDlg, IDC_EDIT1   , dx,dy, ANCHOR_ALL);
}


static void dlg_OnGetMinMaxInfo(HWND UNUSED(hDlg), LPMINMAXINFO mmi)
{
	// we must make sure resize_control will never set negative coords -
	// Windows would clip them, and its real position would be lost.
	// restrict to a reasonable and good looking minimum size [pixels].
	mmi->ptMinTrackSize.x = 407;
	mmi->ptMinTrackSize.y = 159;	// determined experimentally
}


struct DialogParams
{
	const wchar_t* text;
	size_t flags;
};

static BOOL dlg_OnInitDialog(HWND hDlg, HWND UNUSED(hWndFocus), LPARAM lParam)
{
	const DialogParams* params = (const DialogParams*)lParam;
	HWND hWnd;

	// need to reset for new instance of dialog
	dlg_clientOrigin.x = dlg_clientOrigin.y = 0;
	dlg_prevClientSize.x = dlg_prevClientSize.y = 0;

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


static void dlg_OnCommand(HWND hDlg, int id, HWND UNUSED(hWndCtl), UINT UNUSED(codeNotify))
{
	switch(id)
	{
	case IDC_COPY:
	{
		std::vector<wchar_t> buf(128*KiB);	// (too big for stack)
		GetDlgItemTextW(hDlg, IDC_EDIT1, &buf[0], (int)buf.size());
		sys_clipboard_set(&buf[0]);
		break;
	}

	case IDC_CONTINUE:
		EndDialog(hDlg, ERI_CONTINUE);
		break;
	case IDC_SUPPRESS:
		EndDialog(hDlg, ERI_SUPPRESS);
		break;
	case IDC_BREAK:
		EndDialog(hDlg, ERI_BREAK);
		break;
	case IDC_EXIT:
		EndDialog(hDlg, ERI_EXIT);
		break;

	default:
		break;
	}
}


static void dlg_OnSysCommand(HWND hDlg, UINT cmd, int UNUSED(x), int UNUSED(y))
{
	switch(cmd & 0xFFF0)	// NB: lower 4 bits are reserved
	{
	// [X] clicked -> close dialog (doesn't happen automatically)
	case SC_CLOSE:
		EndDialog(hDlg, 0);
		break;

	default:
		break;
	}
}


static INT_PTR CALLBACK dlg_OnMessage(HWND hDlg, unsigned int msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		return HANDLE_WM_INITDIALOG(hDlg, wParam, lParam, dlg_OnInitDialog);

	case WM_SYSCOMMAND:
		return HANDLE_WM_SYSCOMMAND(hDlg, wParam, lParam, dlg_OnSysCommand);

	case WM_COMMAND:
		return HANDLE_WM_COMMAND(hDlg, wParam, lParam, dlg_OnCommand);

	case WM_MOVE:
		return HANDLE_WM_MOVE(hDlg, wParam, lParam, dlg_OnMove);

	case WM_GETMINMAXINFO:
		return HANDLE_WM_GETMINMAXINFO(hDlg, wParam, lParam, dlg_OnGetMinMaxInfo);

	case WM_SIZE:
		return HANDLE_WM_SIZE(hDlg, wParam, lParam, dlg_OnSize);

	default:
		// we didn't process the message; caller will perform default action.
		return FALSE;
	}
}


ErrorReactionInternal sys_display_error(const wchar_t* text, size_t flags)
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
	const BOOL isQuitPending = PeekMessage(&msg, 0, WM_QUIT, WM_QUIT, PM_REMOVE);

	const HINSTANCE hInstance = wutil_LibModuleHandle();
	LPCWSTR lpTemplateName = MAKEINTRESOURCEW(IDD_DIALOG1);
	const DialogParams params = { text, flags };
	// get the enclosing app's window handle. we can't just pass 0 or
	// the desktop window because the dialog must be modal (if the app
	// continues running, it may crash and take down the process before
	// we've managed to show the dialog).
	const HWND hWndParent = wutil_AppWindow();

	INT_PTR ret = DialogBoxParamW(hInstance, lpTemplateName, hWndParent, dlg_OnMessage, (LPARAM)&params);

	if(isQuitPending)
		PostQuitMessage((int)msg.wParam);

	// failed; warn user and make sure we return an ErrorReactionInternal.
	if(ret == 0 || ret == -1)
	{
		debug_DisplayMessage(L"Error", L"Unable to display detailed error dialog.");
		return ERI_CONTINUE;
	}
	return (ErrorReactionInternal)ret;
}


//-----------------------------------------------------------------------------
// misc
//-----------------------------------------------------------------------------

Status sys_StatusDescription(int user_err, wchar_t* buf, size_t max_chars)
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

	wchar_t message[400];
	{
		const LPCVOID source = 0;	// ignored (we're not using FROM_HMODULE etc.)
		const DWORD lang_id = 0;	// look for neutral, then current locale
		va_list* args = 0;			// we don't care about "inserts"
		const DWORD charsWritten = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, source, err, lang_id, message, (DWORD)ARRAY_SIZE(message), args);
		if(!charsWritten)
			WARN_RETURN(ERR::FAIL);
		ENSURE(charsWritten < max_chars);
		if(message[charsWritten-1] == '\n')
			message[charsWritten-1] = '\0';
		if(message[charsWritten-2] == '\r')
			message[charsWritten-2] = '\0';
	}

	const int charsWritten = swprintf_s(buf, max_chars, L"%d (%ls)", err, message);
	ENSURE(charsWritten != -1);
	return INFO::OK;
}


static Status GetModulePathname(HMODULE hModule, OsPath& pathname)
{
	wchar_t pathnameBuf[32768];	// NTFS limit
	const DWORD length = (DWORD)ARRAY_SIZE(pathnameBuf);
	const DWORD charsWritten = GetModuleFileNameW(hModule, pathnameBuf, length);
	if(charsWritten == 0)	// failed
		WARN_RETURN(StatusFromWin());
	ENSURE(charsWritten < length);	// why would the above buffer ever be exceeded?
	pathname = pathnameBuf;
	return INFO::OK;
}


Status sys_get_module_filename(void* addr, OsPath& pathname)
{
	MEMORY_BASIC_INFORMATION mbi;
	const SIZE_T bytesWritten = VirtualQuery(addr, &mbi, sizeof(mbi));
	if(!bytesWritten)
		WARN_RETURN(StatusFromWin());
	ENSURE(bytesWritten >= sizeof(mbi));
	return GetModulePathname((HMODULE)mbi.AllocationBase, pathname);
}


OsPath sys_ExecutablePathname()
{
	WinScopedPreserveLastError s;
	OsPath pathname;
	ENSURE(GetModulePathname(0, pathname) == INFO::OK);
	return pathname;
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
		// (MSDN: the return values for both of these BFFM_ notifications are ignored)
		(void)SendMessage(hWnd, BFFM_SETSELECTIONW, wParam, lpData);
	}

	return 0;
}

Status sys_pick_directory(OsPath& path)
{
	// (must not use multi-threaded apartment due to BIF_NEWDIALOGSTYLE)
	const HRESULT hr = CoInitialize(0);
	ENSURE(hr == S_OK || hr == S_FALSE);	// S_FALSE == already initialized

	// note: bi.pszDisplayName isn't the full path, so it isn't of any use.
	BROWSEINFOW bi;
	memset(&bi, 0, sizeof(bi));
	bi.ulFlags = BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE|BIF_NONEWFOLDERBUTTON;
	// for setting starting directory:
	bi.lpfn = (BFFCALLBACK)BrowseCallback;
	const Path::String initialPath = OsString(path);	// NB: BFFM_SETSELECTIONW can't deal with '/' separators
	bi.lParam = (LPARAM)initialPath.c_str();
	const LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
	if(!pidl)	// user canceled
		return INFO::SKIPPED;

	// translate ITEMIDLIST to string
	wchar_t pathBuf[MAX_PATH];	// mandated by SHGetPathFromIDListW
	const BOOL ok = SHGetPathFromIDListW(pidl, pathBuf);

	// free the ITEMIDLIST
	CoTaskMemFree(pidl);

	if(ok == TRUE)
	{
		path = pathBuf;
		return INFO::OK;
	}

	// Balance call to CoInitialize, which must have been successful
	CoUninitialize();

	WARN_RETURN(StatusFromWin());
}


Status sys_open_url(const std::string& url)
{
	HINSTANCE r = ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
	if ((int)(intptr_t)r > 32)
		return INFO::OK;

	WARN_RETURN(ERR::FAIL);
}


Status sys_generate_random_bytes(u8* buffer, size_t size)
{
	HCRYPTPROV hCryptProv = 0;
	if(!CryptAcquireContext(&hCryptProv, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		WARN_RETURN(StatusFromWin());

	memset(buffer, 0, size);
	if(!CryptGenRandom(hCryptProv, (DWORD)size, (BYTE*)buffer))
		WARN_RETURN(StatusFromWin());

	if(!CryptReleaseContext(hCryptProv, 0))
		WARN_RETURN(StatusFromWin());

	return INFO::OK;
}


#if CONFIG_ENABLE_BOOST

/*
 * Given a string of the form
 *   "example.com:80"
 * or
 *   "ftp=ftp.example.com:80;http=example.com:80;https=example.com:80"
 * separated by semicolons or whitespace,
 * return the string "example.com:80".
 */
static std::wstring parse_proxy(const std::wstring& input)
{
	if(input.find('=') == input.npos)
		return input;

	std::vector<std::wstring> parts;
	split(parts, input, boost::algorithm::is_any_of("; \t\r\n"), boost::algorithm::token_compress_on);

	for(size_t i = 0; i < parts.size(); ++i)
		if(boost::algorithm::starts_with(parts[i], "http="))
			return parts[i].substr(5);

	// If we got this far, proxies were only set for non-HTTP protocols
	return L"";
}

Status sys_get_proxy_config(const std::wstring& url, std::wstring& proxy)
{
	WINHTTP_AUTOPROXY_OPTIONS autoProxyOptions;
	memset(&autoProxyOptions, 0, sizeof(autoProxyOptions));
	autoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
	autoProxyOptions.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
	autoProxyOptions.fAutoLogonIfChallenged = TRUE;

	WINHTTP_PROXY_INFO proxyInfo;
	memset(&proxyInfo, 0, sizeof(proxyInfo));

	WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ieConfig;
	memset(&ieConfig, 0, sizeof(ieConfig));

	HINTERNET hSession = NULL;

	Status err = INFO::SKIPPED;

	bool useAutoDetect;

	if(WinHttpGetIEProxyConfigForCurrentUser(&ieConfig))
	{
		if(ieConfig.lpszAutoConfigUrl)
		{
			// Use explicit auto-config script if specified
			useAutoDetect = true;
			autoProxyOptions.dwFlags |= WINHTTP_AUTOPROXY_CONFIG_URL;
			autoProxyOptions.lpszAutoConfigUrl = ieConfig.lpszAutoConfigUrl;
		}
		else
		{
			// Use auto-discovery if enabled
			useAutoDetect = (ieConfig.fAutoDetect == TRUE);
		}
	}
	else
	{
		// Can't find IE config settings - fall back to auto-discovery
		useAutoDetect = true;
	}

	if(useAutoDetect)
	{
		hSession = WinHttpOpen(NULL, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
			WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

		if(hSession && WinHttpGetProxyForUrl(hSession, url.c_str(), &autoProxyOptions, &proxyInfo) && proxyInfo.lpszProxy)
		{
			proxy = parse_proxy(proxyInfo.lpszProxy);
			if(!proxy.empty())
			{
				err = INFO::OK;
				goto done;
			}
		}
	}

	// No valid auto-config; try explicit proxy instead
	if(ieConfig.lpszProxy)
	{
		proxy = parse_proxy(ieConfig.lpszProxy);
		if(!proxy.empty())
		{
			err = INFO::OK;
			goto done;
		}
	}

done:
	if(ieConfig.lpszProxy)
		GlobalFree(ieConfig.lpszProxy);
	if(ieConfig.lpszProxyBypass)
		GlobalFree(ieConfig.lpszProxyBypass);
	if(ieConfig.lpszAutoConfigUrl)
		GlobalFree(ieConfig.lpszAutoConfigUrl);
	if(proxyInfo.lpszProxy)
		GlobalFree(proxyInfo.lpszProxy);
	if(proxyInfo.lpszProxyBypass)
		GlobalFree(proxyInfo.lpszProxyBypass);
	if(hSession)
		WinHttpCloseHandle(hSession);

	return err;
}

#endif

FILE* sys_OpenFile(const OsPath& pathname, const char* mode)
{
	FILE* f = 0;
	const std::wstring wmode(mode, mode+strlen(mode));
	(void)_wfopen_s(&f, OsString(pathname).c_str(), wmode.c_str());
	return f;
}
