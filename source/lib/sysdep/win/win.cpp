// Windows-specific code
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

#include "lib.h"
#include "win_internal.h"

#include <crtdbg.h>	// malloc debug

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>	// __argc
#include <malloc.h>

#include "lib/res/vfs.h"
#include "lib/res/ogl_tex.h"
#include "lib/ogl.h"

void sle(int x)
{
	SetLastError((DWORD)x);
}


//
// these override the portable stdio versions in sysdep.cpp
// (they're more convenient)
//


void check_heap()
{
	_heapchk();
}


void display_msg(const char* caption, const char* msg)
{
	MessageBoxA(0, msg, caption, MB_ICONEXCLAMATION);
}

void wdisplay_msg(const wchar_t* caption, const wchar_t* msg)
{
	MessageBoxW(0, msg, caption, MB_ICONEXCLAMATION);
}


void win_debug_break()
{
	DebugBreak();
}



// need to shoehorn printf-style variable params into
// the OutputDebugString call.
// - don't want to split into multiple calls - would add newlines to output.
// - fixing Win32 _vsnprintf to return # characters that would be written,
//   as required by C99, looks difficult and unnecessary. if any other code
//   needs that, implement GNU vasprintf.
// - fixed size buffers aren't nice, but much simpler than vasprintf-style
//   allocate+expand_until_it_fits. these calls are for quick debug output,
//   not loads of data, anyway.


static const int MAX_CNT = 512;
	// max output size of 1 call of (w)debug_out (including \0)



void debug_out(const char* fmt, ...)
{
	char buf[MAX_CNT];
	buf[MAX_CNT-1] = '\0';

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, MAX_CNT-1, fmt, ap);
	va_end(ap);

	OutputDebugString(buf);
}


void wdebug_out(const wchar_t* fmt, ...)
{
	wchar_t buf[MAX_CNT];
	buf[MAX_CNT-1] = L'\0';

	va_list ap;
	va_start(ap, fmt);
	vsnwprintf(buf, MAX_CNT-1, fmt, ap);
	va_end(ap);

	OutputDebugStringW(buf);
}










int clipboard_set(const wchar_t* text)
{
	int err = -1;

	if(!OpenClipboard(0))
		return err;
	EmptyClipboard();

	const size_t len = wcslen(text);

	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (len+1) * sizeof(wchar_t));
	if(!hMem)
		goto fail;

	wchar_t* copy = (wchar_t*)GlobalLock(hMem);
	if(copy)
	{
		wcscpy(copy, text);

		GlobalUnlock(hMem);

		if(SetClipboardData(CF_TEXT, hMem) != 0)
			err = 0;	// success
	}

fail:
	CloseClipboard();
	return err;
}


wchar_t* clipboard_get()
{
	wchar_t* ret = 0;

	if(!OpenClipboard(0))
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




// Slightly hacky resource-loading code, but it doesn't
// need to do anything particularly clever.
// Pass name==NULL to unset the cursor. Pass name=="xyz"
// to load "art/textures/cursors/xyz.txt" (containing e.g. "10 10"
// for hotspot position) and "art/textures/cursors/xyz.png".
void cursor_set(const char* name)
{
	static HICON last_cursor = NULL;
	static char* last_name = NULL;

	if (name == NULL)
	{
		// Clean up and go back to the standard Windows cursor

		SetCursor(LoadCursor(NULL, IDC_ARROW));

		if (last_cursor)
			DestroyIcon(last_cursor);
		last_cursor = NULL;

		delete[] last_name;
		last_name = NULL;

		return;
	}

	// Don't do much if it's the same as last time
	if (last_name && !strcmp(name, last_name))
	{
		if (GetCursor() != last_cursor)
			SetCursor(last_cursor);
		return;
	}

	// Store the name, so that the code can tell when it's
	// being called several times with the same cursor
	delete[] last_name;
	last_name = new char[strlen(name)+1];
	strcpy(last_name, name);

	char filename[VFS_MAX_PATH];

	// Load the .txt file containing the pixel offset of
	// the cursor's hotspot (the bit of it that's
	// drawn at (mouse_x,mouse_y) )
	sprintf(filename, "art/textures/cursors/%s.txt", name);

	int hotspotx, hotspoty;
	{
		void* data;
		size_t size;
		Handle h = vfs_load(filename, data, size);
		if (h <= 0)
		{
			// TODO: Handle errors
			assert(! "Error loading cursor hotspot .txt file");
			return;
		}
		if (sscanf((const char*)data, "%d %d", &hotspotx, &hotspoty) != 2)
		{
			// TODO: Handle errors
			assert(! "Invalid contents of cursor hotspot .txt file (should be like \"123 456\")");
			return;
		}
		// TODO: Do I have to unload the file?
	}

	sprintf(filename, "art/textures/cursors/%s.png", name);

	Handle tex = tex_load(filename);
	if (tex <= 0)
	{
		// TODO: Handle errors
		assert(! "Error loading cursor texture");
		return;
	}

	// Convert the image data into a DIB

	int w, h, fmt, bpp;
	u32* imgdata;
	tex_info(tex, &w, &h, &fmt, &bpp, (void**)&imgdata);
	u32* imgdata_bgra;
	if (fmt == GL_BGRA)
	{
		// No conversion needed
		imgdata_bgra = imgdata;
	}
	else if (fmt == GL_RGBA)
	{
		// Convert ABGR -> ARGB (assume little-endian)
		imgdata_bgra = new u32[w*h];
		for (int i=0; i<w*h; ++i)
		{
			imgdata_bgra[i] = (
				(imgdata[i] & 0xff00ff00) // G and A
			| ( (imgdata[i] << 16) & 0x00ff0000) // R
			| ( (imgdata[i] >> 16) & 0x000000ff) // B
			);
		}
	}
	else
	{
		// TODO: Handle errors
		assert(! "Cursor texture not 32-bit RGBA/BGRA");

		tex_free(tex);

		return;
	}

	BITMAPINFOHEADER dibheader = {
		sizeof(BITMAPINFOHEADER), // biSize
		w,	// biWidth
		-h,	// biHeight (negative for top-down)
		1,	// biPlanes
		32,	// biBitCount
		BI_RGB,	// biCompression
		0,	// biSizeImage (not needed for BI_RGB)
		0,	// biXPelsPerMeter (I really don't care how many pixels are in a meter)
		0,	// biYPelsPerMeter
		0,	// biClrUser (0 == maximum for this biBitCount. I hope we're not going to run in paletted display modes.)
		0	// biClrImportant
	};
	BITMAPINFO dibinfo = {
		dibheader,	// bmiHeader
		NULL		// bmiColors[]
	};

	HDC hDC = wglGetCurrentDC();
	HBITMAP iconbitmap = CreateDIBitmap(hDC, &dibheader, CBM_INIT, imgdata_bgra, &dibinfo, 0);
	if (! iconbitmap)
	{
		// TODO: Handle errors
		assert(! "Error creating icon DIB");

		if (imgdata_bgra != imgdata) delete[] imgdata_bgra;
		tex_free(tex);

		return;
	}

	ICONINFO info = { FALSE, hotspotx, hotspoty, iconbitmap, iconbitmap };
	HICON cursor = CreateIconIndirect(&info);
	DeleteObject(iconbitmap);

	if (! cursor)
	{
		// TODO: Handle errors
		assert(! "Error creating cursor");

		if (imgdata_bgra != imgdata) delete[] imgdata_bgra;
		tex_free(tex);

		return;
	}

	if (last_cursor)
		DestroyIcon(last_cursor);
	SetCursor(cursor);
	last_cursor = cursor;

	if (imgdata_bgra != imgdata) delete[] imgdata_bgra;
	tex_free(tex);
}





///////////////////////////////////////////////////////////////////////////////
//
// init and shutdown mechanism
//
///////////////////////////////////////////////////////////////////////////////


// each module has the linker add a pointer to its init and shutdown
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
//   clunky, and setting order is more complicated.
// - on-demand initialization: complicated; don't know in what order
//   things happen. also, no way to determine how long init takes.

typedef int(*_PIFV)(void);

// pointers to start and end of function tables.
// note: COFF throws out empty segments, so we have to put in one value
// (zero, because call_func_tbl has to ignore NULL entries anyway).
#pragma data_seg(".LIB$WIA")
_PIFV init_begin[] = { 0 };
#pragma data_seg(".LIB$WIZ")
_PIFV init_end[] = { 0 };
#pragma data_seg(".LIB$WTA")
_PIFV shutdown_begin[] = { 0 };
#pragma data_seg(".LIB$WTZ")
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


///////////////////////////////////////////////////////////////////////////////


// locking for win-specific code
// several init functions are called on-demand, possibly from constructors.
// can't guarantee POSIX static mutex init has been done by then.



static CRITICAL_SECTION cs[NUM_CS];

void win_lock(uint idx)
{
	assert(idx < NUM_CS && "win_lock: invalid critical section index");
	EnterCriticalSection(&cs[idx]);
}

void win_unlock(uint idx)
{
	assert(idx < NUM_CS && "win_unlock: invalid critical section index");
	LeaveCriticalSection(&cs[idx]);
}


///////////////////////////////////////////////////////////////////////////////


// entry -> pre_libc -> WinMainCRTStartup -> WinMain -> pre_main -> main
// at_exit is called as the last of the atexit handlers
// (assuming, as documented in lib.cpp, constructors don't use atexit!)
//
// note: this way of getting control before main adds overhead
// (setting up the WinMain parameters), but is simpler and safer than
// SDL-style #define main SDL_main.

static void at_exit(void)
{
	call_func_tbl(shutdown_begin, shutdown_end);

	for(int i = 0; i < NUM_CS; i++)
		DeleteCriticalSection(&cs[i]);
}


// be very careful to avoid non-stateless libc functions!
static inline void pre_libc_init()
{
	for(int i = 0; i < NUM_CS; i++)
		InitializeCriticalSection(&cs[i]);
}


static inline void pre_main_init()
{
#ifdef HAVE_DEBUGALLOC
	uint flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	// Always enable leak detection in debug builds
	flags |= _CRTDBG_LEAK_CHECK_DF;
#ifdef PARANOIA
	// force malloc et al. to check the heap every call.
	// slower, but reports errors closer to where they occur.
	flags |= _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_DELAY_FREE_MEM_DF;
#endif // PARANOIA
	_CrtSetDbgFlag(flags);
#endif // HAVE_DEBUGALLOC

	call_func_tbl(init_begin, init_end);

	atexit(at_exit);

	// no point redirecting stdout yet - the current directory
	// may be incorrect (file_set_root not yet called).
	// (w)sdl will take care of it anyway.
}

// Enable heap corruption checking after every allocation. Has the same
// effect as PARANOIA in pre_main_init, but lets you switch it on anywhere
// so that you can skip checking the whole of the initialisation code.
// The debugger will break in the allocation just after the one that
// corrupted the heap, so check its ID and then _CrtSetBreakAlloc(...)
// on the previous one and try again.
// Warning: This makes things rather slow.
void memory_debug_extreme_turbo_plus()
{
	_CrtSetDbgFlag( _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_DELAY_FREE_MEM_DF );
}

extern u64 rdtsc();
extern u64 PREVTSC;
u64 PREVTSC;


int entry()
{

#ifdef _MSC_VER
u64 TSC=rdtsc();
debug_out(
"----------------------------------------\n"\
"ENTRY\n"\
"----------------------------------------\n");
PREVTSC=TSC;
#endif

	pre_libc_init();
	return WinMainCRTStartup();	// calls _cinit, and then WinMain
}


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	pre_main_init();
	return main(__argc, __argv);
}
