// Windows backend of the sysdep interface
// Copyright (c) 2003-2005 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.x
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "precompiled.h"

#include "win_internal.h"	// includes windows.h; must come before shlobj
#include <shlobj.h>	// pick_dir

#include "lib.h"
#include "posix.h"
#include "error_dialog.h"
#include "lib/res/graphics/tex.h"


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



// workaround: apparently PCH contents may be included after the headers.
// malloc.h is defining alloca to _alloca and then VC complains about us
// redefining an intrinsic. so, stomp on the macro here.
#undef alloca

void* alloca(size_t size)
{
	void* ret;
	__try
	{
		ret = _alloca(size);
	}
	// if stack overflow, handle it; otherwise, continue handler search.
	__except(GetExceptionCode() == STATUS_STACK_OVERFLOW)
	{
		// restore guard page - necessary on XP, works everywhere.
		_resetstkoflw();
		ret = 0;
	}
	return ret;
}


//-----------------------------------------------------------------------------
// "program error" dialog (triggered by debug_assert and exception)
//-----------------------------------------------------------------------------

// we need to know the app's main window for the error dialog, so that
// it is modal and actually stops the app. if it keeps running while
// we're reporting an error, it'll probably crash and take down the
// error window before it is seen (since we're in the same process).

static BOOL CALLBACK is_this_our_window(HWND hWnd, LPARAM lParam)
{
	DWORD pid;
	DWORD tid = GetWindowThreadProcessId(hWnd, &pid);
	UNUSED2(tid);	// the function can't fail

	if(pid == GetCurrentProcessId())
	{
		*(HWND*)lParam = hWnd;
		return FALSE;	// done
	}

	return TRUE;	// keep calling
}

// try to determine the app's main window by enumerating all
// top-level windows and comparing their PIDs.
// returns 0 if not found, e.g. if the app doesn't have one yet.
static HWND get_app_main_window()
{
	HWND our_window = 0;
	DWORD ret = EnumWindows(is_this_our_window, (LPARAM)&our_window);
	UNUSED2(ret);
	// the callback returns FALSE when it has found the window
	// (so as not to waste time); EnumWindows then returns 0.
	// therefore, we can't check this; just return our_window.
	return our_window;
}


// support for resizing the dialog / its controls
// (have to do this manually - grr)

static POINTS dlg_client_origin;
static POINTS dlg_prev_client_size;

const int ANCHOR_LEFT   = 0x01;
const int ANCHOR_RIGHT  = 0x02;
const int ANCHOR_TOP    = 0x04;
const int ANCHOR_BOTTOM = 0x08;
const int ANCHOR_ALL    = 0x0f;

static void dlg_resize_control(HWND hDlg, int dlg_item, int dx,int dy, int anchors)
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


static int CALLBACK error_dialog_proc(HWND hDlg, unsigned int msg, WPARAM wParam, LPARAM lParam)
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
				// (allocating on the stack would be easier+safer, but this is
				// too big.)
				const size_t max_chars = 128*KiB;
				wchar_t* buf = (wchar_t*)malloc(max_chars*sizeof(wchar_t));
				if(buf)
				{
					GetDlgItemTextW(hDlg, IDC_EDIT1, buf, max_chars);
					sys_clipboard_set(buf);
					free(buf);
				}
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
	// they may be holding systemwide locks (e.g. heap or loader) that
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

	const HINSTANCE hInstance = GetModuleHandle(0);
	LPCSTR lpTemplateName = MAKEINTRESOURCE(IDD_DIALOG1);
	const DialogParams params = { text, flags };
	// get the enclosing app's window handle. we can't just pass 0 or
	// the desktop window because the dialog must be modal (the app
	// must not crash/continue to run before it has been displayed).
	const HWND hWndParent = get_app_main_window();

	INT_PTR ret = DialogBoxParam(hInstance, lpTemplateName, hWndParent, error_dialog_proc, (LPARAM)&params);

	if(quit_pending)
		PostQuitMessage((int)msg.wParam);

	// failed; warn user and make sure we return an ErrorReaction.
	if(ret == 0 || ret == -1)
	{
		sys_display_msgw(L"Error", L"Unable to display detailed error dialog.");
		// TODO: i18n
		return ER_CONTINUE;
	}
	return (ErrorReaction)ret;
}


//-----------------------------------------------------------------------------
// clipboard
//-----------------------------------------------------------------------------

// "copy" text into the clipboard. replaces previous contents.
int sys_clipboard_set(const wchar_t* text)
{
	int err = -1;

	const HWND new_owner = 0;
	// MSDN: passing 0 requests the current task be granted ownership;
	// there's no need to pass our window handle.
	if(!OpenClipboard(new_owner))
		return err;
	EmptyClipboard();

	err = 0;

	const size_t len = wcslen(text);

	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (len+1) * sizeof(wchar_t));
	if(!hMem)
		goto fail;

	wchar_t* copy = (wchar_t*)GlobalLock(hMem);
	if(copy)
	{
		wcscpy(copy, text);

		GlobalUnlock(hMem);

		if(SetClipboardData(CF_UNICODETEXT, hMem) != 0)
			err = 0;	// success
	}

fail:
	CloseClipboard();
	return err;
}


// allow "pasting" from clipboard. returns the current contents if they
// can be represented as text, otherwise 0.
// when it is no longer needed, the returned pointer must be freed via
// sys_clipboard_free. (NB: not necessary if zero, but doesn't hurt)
wchar_t* sys_clipboard_get()
{
	wchar_t* ret = 0;

	const HWND new_owner = 0;
	// MSDN: passing 0 requests the current task be granted ownership;
	// there's no need to pass our window handle.
	if(!OpenClipboard(new_owner))
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
int sys_clipboard_free(wchar_t* copy)
{
	free(copy);
	return 0;
}


//-----------------------------------------------------------------------------
// mouse cursor
//-----------------------------------------------------------------------------

static void* ptr_from_HICON(HICON hIcon)
{
	return (void*)(uintptr_t)hIcon;
}

static void* ptr_from_HCURSOR(HCURSOR hCursor)
{
	return (void*)(uintptr_t)hCursor;
}

static HICON HICON_from_ptr(void* p)
{
	return (HICON)(uintptr_t)p;
}

static HCURSOR HCURSOR_from_ptr(void* p)
{
	return (HCURSOR)(uintptr_t)p;
}


// creates a cursor from the given image.
// w, h specify image dimensions [pixels]. limit is implementation-
//   dependent; 32x32 is typical and safe.
// bgra_img is the cursor image (BGRA format, bottom-up).
//   it is no longer needed and can be freed after this call returns.
// hotspot (hx,hy) is the offset from its upper-left corner to the
//   position where mouse clicks are registered.
// return: negative error code, or 0 on success. cursor is filled with
//   a pointer and undefined on failure. it must be sys_cursor_free-ed
//   when no longer needed.
int sys_cursor_create(uint w, uint h, void* bgra_img,
	uint hx, uint hy, void** cursor)
{
	// MSDN says selecting this HBITMAP into a DC is slower since we use
	// CreateBitmap; bpp/format must be checked against those of the DC.
	// this is the simplest way and we don't care about slight performance
	// differences because this is typically only called once.
	HBITMAP hbmColour = CreateBitmap(w, h, 1, 32, bgra_img);

	// CreateIconIndirect doesn't access this; we just need to pass
	// an empty bitmap.
	HBITMAP hbmMask = CreateBitmap(w, h, 1, 1, 0);

	// create the cursor (really an icon; they differ only in
	// fIcon and the hotspot definitions).
	ICONINFO ii;
	ii.fIcon = FALSE;  // cursor
	ii.xHotspot = hx;
	ii.yHotspot = hy;
	ii.hbmMask  = hbmMask;
	ii.hbmColor = hbmColour;
	HICON hIcon = CreateIconIndirect(&ii);

	// CreateIconIndirect makes copies, so we no longer need these.
	DeleteObject(hbmMask);
	DeleteObject(hbmColour);

	if(!hIcon)	// not INVALID_HANDLE_VALUE
		return -1;

	*cursor = ptr_from_HICON(hIcon);
	return 0;
}



// replaces the current system cursor with the one indicated. need only be
// called once per cursor; pass 0 to restore the default.
int sys_cursor_set(void* cursor)
{
	// restore default cursor.
	if(!cursor)
		cursor = ptr_from_HCURSOR(LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW)));

	(void)SetCursor(HCURSOR_from_ptr(cursor));
	// return value (previous cursor) is useless.

	return 0;
}


// destroys the indicated cursor and frees its resources. if it is
// currently the system cursor, the default cursor is restored first.
int sys_cursor_free(void* cursor)
{
	// bail now to prevent potential confusion below; there's nothing to do.
	if(!cursor)
		return 0;

	// if the cursor being freed is active, restore the default arrow
	// (just for safety).
	if(ptr_from_HCURSOR(GetCursor()) == cursor)
		WARN_ERR(sys_cursor_set(0));

	BOOL ok = DestroyIcon(HICON_from_ptr(cursor));
	return ok? 0 : -1;
}


//-----------------------------------------------------------------------------
// misc
//-----------------------------------------------------------------------------

// OS-specific backend for error_description_r.
// NB: it is expected to be rare that OS return/error codes are actually
// seen by user code, but we still translate them for completeness.
int sys_error_description_r(int err, char* buf, size_t max_chars)
{
	// not in our range (Win32 error numbers are positive)
	if(err < 0)
		return -1;

	const LPCVOID source = 0;	// ignored (we're not using FROM_HMODULE etc.)
	const DWORD lang_id = 0;	// look for neutral, then current locale
	va_list* args = 0;			// we don't care about "inserts"
	DWORD ret = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, source, (DWORD)err,
		lang_id, buf, (DWORD)max_chars, args);
	if(!ret)
		return -1;
	debug_assert(ret < max_chars);	// ret = #chars output
	return 0;
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
// returns 0 or a negative error code.
// useful for determining installation directory, e.g. for VFS.
inline int sys_get_executable_name(char* n_path, size_t buf_size)
{
	DWORD nbytes = GetModuleFileName(0, n_path, (DWORD)buf_size);
	return nbytes? 0 : -1;
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
// returns 0 on success or a negative error code.
int sys_pick_directory(char* path, size_t buf_size)
{
	// bring up dialog; set starting directory to current working dir.
	WARN_IF_FALSE(GetCurrentDirectory((DWORD)buf_size, path));
	BROWSEINFOA bi;
	memset(&bi, 0, sizeof(bi));
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.lpfn = (BFFCALLBACK)browse_cb;
	bi.lParam = (LPARAM)path;
	ITEMIDLIST* pidl = SHBrowseForFolderA(&bi);

	// translate ITEMIDLIST to string. note: SHGetPathFromIDList doesn't
	// support a user-specified char limit *sigh*
	debug_assert(buf_size >= MAX_PATH);	//
	BOOL ok = SHGetPathFromIDList(pidl, path);

	// free the ITEMIDLIST
	IMalloc* p_malloc;
	SHGetMalloc(&p_malloc);
	p_malloc->Free(pidl);
	p_malloc->Release();

	return ok? 0 : -1;
}


// execute the specified function once on each CPU.
// this includes logical HT units and proceeds serially (function
// is never re-entered) in order of increasing OS CPU ID.
// note: implemented by switching thread affinity masks and forcing
// a reschedule, which is apparently not possible with POSIX.
// return 0 on success or a negative error code on failure
// (e.g. if OS is preventing us from running on some CPUs).
// called from ia32.cpp get_cpu_count
int sys_on_each_cpu(void(*cb)())
{
	const HANDLE hProcess = GetCurrentProcess();
	DWORD process_affinity, system_affinity;
	if(!GetProcessAffinityMask(hProcess, &process_affinity, &system_affinity))
		return -1;
	// our affinity != system affinity: OS is limiting the CPUs that
	// this process can run on. fail (cannot call back for each CPU).
	if(process_affinity != system_affinity)
		return -1;

	for(DWORD cpu_bit = 1; cpu_bit != 0 && cpu_bit <= process_affinity; cpu_bit *= 2)
	{
		// check if we can switch to target CPU
		if(!(process_affinity & cpu_bit))
			continue;
		// .. and do so.
		if(!SetProcessAffinityMask(hProcess, process_affinity))
		{
			debug_warn("SetProcessAffinityMask failed");
			continue;
		}

		// reschedule, to make sure we switch CPUs
		Sleep(1);

		cb();
	}

	// restore to original value
	SetProcessAffinityMask(hProcess, process_affinity);

	return 0;
}