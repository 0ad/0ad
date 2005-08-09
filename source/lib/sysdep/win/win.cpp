// Windows-specific code and program entry point
// Copyright (c) 2003 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "precompiled.h"

#include <stdio.h>
#include <stdlib.h>	// __argc

#include "win_internal.h"	// includes windows.h; must come before shlobj
#include <shlobj.h>	// pick_dir

#include "lib.h"
#include "posix.h"
#include "error_dialog.h"

#if MSC_VERSION
#pragma comment(lib, "shell32.lib")	// for pick_directory SH* calls
#endif


char win_sys_dir[MAX_PATH+1];
char win_exe_dir[MAX_PATH+1];



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
HWND win_get_app_main_window()
{
	HWND our_window = 0;
	DWORD ret = EnumWindows(is_this_our_window, (LPARAM)&our_window);
	UNUSED2(ret);
		// the callback returns FALSE when it has found the window
		// (so as not to waste time); EnumWindows then returns 0.
		// therefore, we can't check this; just return our_window.
	return our_window;
}


//-----------------------------------------------------------------------------

//
// safe allocator that may be used independently of libc malloc
// (in particular, before _cinit and while calling static dtors).
// used by wpthread critical section code.
//

void* win_alloc(size_t size)
{
	const DWORD flags = HEAP_ZERO_MEMORY;
	return HeapAlloc(GetProcessHeap(), flags, size);
}

void win_free(void* p)
{
	const DWORD flags = 0;
	HeapFree(GetProcessHeap(), flags, p);
}


//-----------------------------------------------------------------------------

//
// these override the portable versions in sysdep.cpp
// (they're more convenient)
//

void display_msg(const char* caption, const char* msg)
{
	MessageBoxA(0, msg, caption, MB_ICONEXCLAMATION|MB_TASKMODAL|MB_SETFOREGROUND);
}

void wdisplay_msg(const wchar_t* caption, const wchar_t* msg)
{
	MessageBoxW(0, msg, caption, MB_ICONEXCLAMATION|MB_TASKMODAL|MB_SETFOREGROUND);
}


inline int get_executable_name(char* n_path, size_t buf_size)
{
	DWORD nbytes = GetModuleFileName(0, n_path, (DWORD)buf_size);
	return nbytes? 0 : -1;
}


// return filename of the module which contains address <addr>,
// or L"" on failure. path holds the string and must be >= MAX_PATH chars.
wchar_t* get_module_filename(void* addr, wchar_t* path)
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

int pick_directory(char* path, size_t buf_size)
{
	debug_assert(buf_size >= PATH_MAX);
	IMalloc* p_malloc;
	SHGetMalloc(&p_malloc);

	GetCurrentDirectory(PATH_MAX, path);

///	ShowWindow(hWnd, SW_HIDE);

	BROWSEINFOA bi;
	memset(&bi, 0, sizeof(bi));
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.lpfn = (BFFCALLBACK)browse_cb;
	bi.lParam = (LPARAM)path;
	ITEMIDLIST* pidl = SHBrowseForFolderA(&bi);
	BOOL ok = SHGetPathFromIDList(pidl, path);

///	ShowWindow(hWnd, SW_SHOW);

	p_malloc->Free(pidl);
	p_malloc->Release();

	return ok? 0 : -1;
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
			const size_t max_chars = 128*KiB;
			wchar_t* buf = (wchar_t*)malloc(max_chars*sizeof(wchar_t));
			if(buf)
			{
				GetDlgItemTextW(hDlg, IDC_EDIT1, buf, max_chars);
				clipboard_set(buf);
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
ErrorReaction display_error_impl(const wchar_t* text, int flags)
{
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
	const HWND hWndParent = win_get_app_main_window();

	INT_PTR ret = DialogBoxParam(hInstance, lpTemplateName, hWndParent, error_dialog_proc, (LPARAM)&params);

	if(quit_pending)
		PostQuitMessage((int)msg.wParam);

	// failed; warn user and make sure we return an ErrorReaction.
	if(ret == 0 || ret == -1)
	{
		wdisplay_msg(L"Error", L"Unable to display detailed error dialog.");
			// TODO: i18n
		return ER_CONTINUE;
	}
	return (ErrorReaction)ret;
}


//-----------------------------------------------------------------------------
// clipboard
//-----------------------------------------------------------------------------

int clipboard_set(const wchar_t* text)
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


wchar_t* clipboard_get()
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
			wchar_t* copy = (wchar_t*)malloc(size);
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


int clipboard_free(wchar_t* copy)
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


// creates a cursor from the given 32 bpp RGBA texture. hotspot (hx,hy) is
// the offset from its upper-left corner to the position where mouse clicks
// are registered.
// the cursor must be cursor_free-ed when no longer needed.
int cursor_create(int w, int h, void* img, int hx, int hy,
	void** cursor)
{
	*cursor = 0;

	// convert to BGRA (required by BMP).
	// don't do this in-place so we don't spoil someone else's
	// use of the texture (however unlikely that may be).
	void* img_bgra = malloc(w*h*4);
	if(!img_bgra)
		return ERR_NO_MEM;
	const u8* src = (const u8*)img;
	u8* dst = (u8*)img_bgra;
	for(int i = 0; i < w*h; i++)
	{
		const u8 r = src[0], g = src[1], b = src[2], a = src[3];
		dst[0] = b; dst[1] = g; dst[2] = r; dst[3] = a;
		dst += 4;
		src += 4;
	}
	img = img_bgra;

	// MSDN says selecting this HBITMAP into a DC is slower since we use
	// CreateBitmap; bpp/format must be checked against those of the DC.
	// this is the simplest way and we don't care about slight performance
	// differences because this is typically only called once.
	HBITMAP hbmColor = CreateBitmap(w, h, 1, 32, img_bgra);

	free(img_bgra);

	// CreateIconIndirect doesn't access it; we just need to pass
	// an empty bitmap.
	HBITMAP hbmMask = CreateBitmap(w, h, 1, 1, 0);

	// create the cursor (really an icon; they differ only in
	// fIcon and the hotspot definitions).
	ICONINFO ii;
	ii.fIcon = FALSE;  // cursor
	ii.xHotspot = hx;
	ii.yHotspot = hy;
	ii.hbmMask  = hbmMask;
	ii.hbmColor = hbmColor;
	HICON hIcon = CreateIconIndirect(&ii);

	// CreateIconIndirect makes copies, so we no longer need these.
	DeleteObject(hbmMask);
	DeleteObject(hbmColor);

	if(!hIcon)	// not INVALID_HANDLE_VALUE
	{
		debug_warn("cursor CreateIconIndirect failed");
		return -1;
	}

	*cursor = ptr_from_HICON(hIcon);
	return 0;
}


// replaces the current system cursor with the one indicated. need only be
// called once per cursor; pass 0 to restore the default.
int cursor_set(void* cursor)
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
int cursor_free(void* cursor)
{
	// bail now to prevent potential confusion below; there's nothing to do.
	if(!cursor)
		return 0;

	// if the cursor being freed is active, restore the default arrow
	// (just for safety).
	if(ptr_from_HCURSOR(GetCursor()) == cursor)
		WARN_ERR(cursor_set(0));

	BOOL ok = DestroyIcon(HICON_from_ptr(cursor));
	return ok? 0 : -1;
}


///////////////////////////////////////////////////////////////////////////////
//
// module init and shutdown mechanism
//
///////////////////////////////////////////////////////////////////////////////


// init and shutdown mechanism: register a function to be called at
// pre-libc init, pre-main init or shutdown.
//
// each module has the linker add a pointer to its init or shutdown
// function to a table (at a user-defined position).
// zero runtime overhead, and there's no need for a central dispatcher
// that knows about all the modules.
//
// disadvantage: requires compiler support (MS VC-specific).
//
// alternatives:
// - initialize via constructor. however, that would leave the problem of
//   shutdown order and timepoint, which is also taken care of here.
// - register init/shutdown functions from a NLSO constructor:
//   clunky, and setting order is more difficult.
// - on-demand initialization: complicated; don't know in what order
//   things happen. also, no way to determine how long init takes.
//
// the "segment name" determines when and in what order the functions are
// called: "LIB$W{type}{group}", where {type} is C for pre-libc init,
// I for pre-main init, or T for terminators (last of the atexit handlers).
// {group} is [B, Y]; groups are called in alphabetical order, but
// call order within the group itself is unspecified.
//
// define the segment via #pragma data_seg(name), register any functions
// to be called via WIN_REGISTER_FUNC, and then restore the previous segment
// with #pragma data_seg() .
//
// note: group must be [B, Y]. data declared in groups A or Z may
// be placed beyond the table start/end by the linker, since the linker's
// ordering WRT other source files' data is undefined within a segment.

typedef int(*_PIFV)(void);

// pointers to start and end of function tables.
// note: COFF throws out empty segments, so we have to put in one value
// (zero, because call_func_tbl has to ignore NULL entries anyway).
#pragma data_seg(WIN_CALLBACK_PRE_LIBC(a))
_PIFV pre_libc_begin[] = { 0 };
#pragma data_seg(WIN_CALLBACK_PRE_LIBC(z))
_PIFV pre_libc_end[] = { 0 };
#pragma data_seg(WIN_CALLBACK_PRE_MAIN(a))
_PIFV pre_main_begin[] = { 0 };
#pragma data_seg(WIN_CALLBACK_PRE_MAIN(z))
_PIFV pre_main_end[] = { 0 };
#pragma data_seg(WIN_CALLBACK_POST_ATEXIT(a))
_PIFV shutdown_begin[] = { 0 };
#pragma data_seg(WIN_CALLBACK_POST_ATEXIT(z))
_PIFV shutdown_end[] = { 0 };
#pragma data_seg()

#pragma comment(linker, "/merge:.LIB=.data")

// call all non-NULL function pointers in [begin, end).
// note: the range may be larger than expected due to section padding.
// that (and the COFF empty section problem) is why we need to ignore zeroes.
static void call_func_tbl(_PIFV* begin, _PIFV* end)
{
	for(_PIFV* p = begin; p < end; p++)
		if(*p)
			(*p)();
}


//-----------------------------------------------------------------------------
// locking for win-specific code
//-----------------------------------------------------------------------------

// several init functions are before called before _cinit.
// POSIX static mutex init may not have been done by then,
// so we need our own lightweight functions.

static CRITICAL_SECTION cs[NUM_CS];
static bool cs_valid;

void win_lock(uint idx)
{
	debug_assert(idx < NUM_CS && "win_lock: invalid critical section index");
	if(cs_valid)
		EnterCriticalSection(&cs[idx]);
}

void win_unlock(uint idx)
{
	debug_assert(idx < NUM_CS && "win_unlock: invalid critical section index");
	if(cs_valid)
		LeaveCriticalSection(&cs[idx]);
}

int win_is_locked(uint idx)
{
	debug_assert(idx < NUM_CS && "win_is_locked: invalid critical section index");
	if(!cs_valid)
		return -1;
	BOOL got_it = TryEnterCriticalSection(&cs[idx]);
	if(got_it)
		LeaveCriticalSection(&cs[idx]);
	return !got_it;
}


static void cs_init()
{
	for(int i = 0; i < NUM_CS; i++)
		InitializeCriticalSection(&cs[i]);

	cs_valid = true;
}

static void cs_shutdown()
{
	cs_valid = false;

	for(int i = 0; i < NUM_CS; i++)
		DeleteCriticalSection(&cs[i]);
	memset(cs, 0, sizeof(cs));
}


//-----------------------------------------------------------------------------
// startup
//-----------------------------------------------------------------------------

// entry -> pre_libc -> WinMainCRTStartup -> WinMain -> pre_main -> main
// at_exit is called as the last of the atexit handlers
// (assuming, as documented in lib.cpp, constructors don't use atexit!)
//
// rationale: we need to gain control after _cinit and before main() to
// complete initialization.
// note: this way of getting control before main adds overhead
// (setting up the WinMain parameters), but is simpler and safer than
// SDL-style #define main SDL_main.

// explained where used.
static HMODULE hUser32Dll;

static void at_exit(void)
{
	call_func_tbl(shutdown_begin, shutdown_end);

	cs_shutdown();

	// free the reference taken in win_pre_libc_init;
	// this avoids Boundschecker warnings at exit.
	FreeLibrary(hUser32Dll);
}


#ifndef NO_MAIN_REDIRECT
static
#endif
void win_pre_main_init()
{
	// enable FPU exceptions
#if CPU_IA32
	const int bits = _EM_INVALID|_EM_DENORMAL|_EM_ZERODIVIDE|_EM_OVERFLOW|_EM_UNDERFLOW|_EM_INEXACT;
	_control87(bits, _MCW_EM);
#endif

	// enable memory tracking and leak detection;
	// no effect if !HAVE_VC_DEBUG_ALLOC.
#if CONFIG_PARANOIA
	debug_heap_enable(DEBUG_HEAP_ALL);
#elif !defined(NDEBUG)
	debug_heap_enable(DEBUG_HEAP_NORMAL);
#endif

	call_func_tbl(pre_main_begin, pre_main_end);

	atexit(at_exit);

	// no point redirecting stdout yet - the current directory
	// may be incorrect (file_set_root not yet called).
	// (w)sdl will take care of it anyway.
}


#ifndef NO_MAIN_REDIRECT

#undef main
extern int app_main(int argc, char* argv[]);
int main(int argc, char* argv[])
{
	win_pre_main_init();
	return app_main(argc, argv);
}
#endif


// perform all initialization that needs to run before _cinit
// (which calls C++ ctors).
// be very careful to avoid non-stateless libc functions!
static inline void pre_libc_init()
{
	// enable low-fragmentation heap
#if WINVER >= 0x0501
	HMODULE hKernel32Dll = LoadLibrary("kernel32.dll");
	if(hKernel32Dll)
	{
		BOOL (WINAPI* pHeapSetInformation)(HANDLE, HEAP_INFORMATION_CLASS, void*, size_t);
		*(void**)&pHeapSetInformation = GetProcAddress(hKernel32Dll, "HeapSetInformation");
		if(pHeapSetInformation)
		{
			ULONG flags = 2;	// enable LFH
			pHeapSetInformation(GetProcessHeap(), HeapCompatibilityInformation, &flags, sizeof(flags));
		}

		FreeLibrary(hKernel32Dll);
	}
#endif	// #if WINVER >= 0x0501

	cs_init();

	GetSystemDirectory(win_sys_dir, sizeof(win_sys_dir));

	if(GetModuleFileName(GetModuleHandle(0), win_exe_dir, MAX_PATH) != 0)
	{
		char* slash = strrchr(win_exe_dir, '\\');
		if(slash)
			*slash = '\0';
	}

	// HACK: make sure a reference to user32 is held, even if someone
	// decides to delay-load it. this fixes bug #66, which was the
	// Win32 mouse cursor (set via user32!SetCursor) appearing as a
	// black 32x32(?) rectangle. underlying cause was as follows:
	// powrprof.dll was the first client of user32, causing it to be
	// loaded. after we were finished with powrprof, we freed it, in turn
	// causing user32 to unload. later code would then reload user32,
	// which apparently terminally confused the cursor implementation.
	//
	// since we hold a reference here, user32 will never unload.
	// of course, the benefits of delay-loading are lost for this DLL,
	// but that is unavoidable. it is safer to force loading it, rather
	// than documenting the problem and asking it not be delay-loaded.
	hUser32Dll = LoadLibrary("user32.dll");

	call_func_tbl(pre_libc_begin, pre_libc_end);
}


int entry()
{
	int ret = -1;
	__try
	{
		pre_libc_init();
#ifdef USE_WINMAIN
		ret = WinMainCRTStartup();	// calls _cinit and then our main
#else
		ret = mainCRTStartup();	// calls _cinit and then our main
#endif
	}
	__except(wdbg_exception_filter(GetExceptionInformation()))
	{
	}
	return ret;
}
