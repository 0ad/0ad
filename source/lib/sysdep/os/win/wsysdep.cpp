/**
 * =========================================================================
 * File        : wsysdep.cpp
 * Project     : 0 A.D.
 * Description : Windows backend of the sysdep interface
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "lib/sysdep/sysdep.h"

#include "win.h"	// includes windows.h; must come before shlobj
#include <shlobj.h>	// pick_dir

#include "lib/sysdep/clipboard.h"
#include "error_dialog.h"
#include "wutil.h"



#if MSC_VERSION
#pragma comment(lib, "shell32.lib")	// for sys_pick_directory SH* calls
#endif


void sys_display_msg(const char* caption, const char* msg)
{
	MessageBoxA(0, msg, caption, MB_ICONEXCLAMATION|MB_TASKMODAL|MB_SETFOREGROUND);
}

void sys_display_msgw(const wchar_t* caption, const wchar_t* msg)
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
	int flags;
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


// show error dialog with the given text and return user's reaction.
// exits directly if 'exit' is clicked.
ErrorReaction sys_display_error(const wchar_t* text, int flags)
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
	LPCSTR lpTemplateName = MAKEINTRESOURCE(IDD_DIALOG1);
	const DialogParams params = { text, flags };
	// get the enclosing app's window handle. we can't just pass 0 or
	// the desktop window because the dialog must be modal (if the app
	// continues running, it may crash and take down the process before
	// we've managed to show the dialog).
	const HWND hWndParent = wutil_AppWindow();

	INT_PTR ret = DialogBoxParam(hInstance, lpTemplateName, hWndParent, error_dialog_proc, (LPARAM)&params);

	if(quit_pending)
		PostQuitMessage((int)msg.wParam);

	// failed; warn user and make sure we return an ErrorReaction.
	if(ret == 0 || ret == -1)
	{
		debug_display_msgw(L"Error", L"Unable to display detailed error dialog.");
		return ER_CONTINUE;
	}
	return (ErrorReaction)ret;
}


//-----------------------------------------------------------------------------
// misc
//-----------------------------------------------------------------------------

LibError sys_error_description_r(int user_err, char* buf, size_t max_chars)
{
	// validate user_err - Win32 doesn't have negative error numbers
	if(user_err < 0)
		return ERR::FAIL;	// NOWARN

	const DWORD err = user_err? (DWORD)user_err : GetLastError();

	// no one likes to see "The operation completed successfully" in
	// error messages, so return more descriptive text instead.
	if(err == 0)
	{
		strcpy_s(buf, max_chars, "0 (no error code was set)");
		return INFO::OK;
	}

	const LPCVOID source = 0;	// ignored (we're not using FROM_HMODULE etc.)
	const DWORD lang_id = 0;	// look for neutral, then current locale
	va_list* args = 0;			// we don't care about "inserts"
	const DWORD chars_output = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, source, err, lang_id, buf, (DWORD)max_chars, args);
	if(!chars_output)
		WARN_RETURN(ERR::FAIL);
	debug_assert(chars_output < max_chars);
	return INFO::OK;
}


// determine filename of the module to whom the given address belongs.
// useful for handling exceptions in other modules.
// <path> receives full path to module; it must hold at least MAX_PATH chars.
// on error, it is set to L"".
// return path for convenience.
wchar_t* sys_get_module_filename(void* addr, wchar_t* path)
{
	path[0] = '\0';	// in case either API call below fails
	wchar_t* module_filename = path;

	MEMORY_BASIC_INFORMATION mbi;
	if(VirtualQuery(addr, &mbi, sizeof(mbi)))
	{
		HMODULE hModule = (HMODULE)mbi.AllocationBase;
		if(GetModuleFileNameW(hModule, path, MAX_PATH))
			module_filename = wcsrchr(path, '\\')+1;
		// note: GetModuleFileName returns full path => a '\\' exists
	}

	return module_filename;
}


// store full path to the current executable.
// useful for determining installation directory, e.g. for VFS.
inline LibError sys_get_executable_name(char* n_path, size_t buf_size)
{
	DWORD nbytes = GetModuleFileName(0, n_path, (DWORD)buf_size);
	return nbytes? INFO::OK : ERR::FAIL;
}


// callback for shell directory picker: used to set
// starting directory to the current directory (for user convenience).
static int CALLBACK browse_cb(HWND hWnd, unsigned int msg, LPARAM UNUSED(lParam), LPARAM ldata)
{
	if(msg == BFFM_INITIALIZED)
	{
		const char* cur_dir = (const char*)ldata;
		SendMessage(hWnd, BFFM_SETSELECTIONA, 1, (LPARAM)cur_dir);
		return 1;
	}

	return 0;
}

// have the user specify a directory via OS dialog.
// stores its full path in the given buffer, which must hold at least
// PATH_MAX chars.
LibError sys_pick_directory(char* path, size_t buf_size)
{
	// bring up dialog; set starting directory to current working dir.
	WARN_IF_FALSE(GetCurrentDirectory((DWORD)buf_size, path));
	BROWSEINFOA bi;
	memset(&bi, 0, sizeof(bi));
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.lpfn = (BFFCALLBACK)browse_cb;
	bi.lParam = (LPARAM)path;
	LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);

	// translate ITEMIDLIST to string. note: SHGetPathFromIDList doesn't
	// support a user-specified char limit *sigh*
	debug_assert(buf_size >= MAX_PATH);	//
	BOOL ok = SHGetPathFromIDList(pidl, path);

	// free the ITEMIDLIST
	IMalloc* p_malloc;
	SHGetMalloc(&p_malloc);
	p_malloc->Free(pidl);
	p_malloc->Release();

	return LibError_from_win32(ok);
}
