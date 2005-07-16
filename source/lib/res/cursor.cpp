#include "precompiled.h"

#include <string.h>
#include <sstream>

#define USE_WINDOWS_CURSOR

#ifdef _WIN32
#include "lib/sysdep/win/win_internal.h"
#endif

#include "res.h"
#include "ogl_tex.h"
#include "lib/ogl.h"

struct ogl_cursor {
	Handle tex;
	int hotspotx, hotspoty;
	int w, h;
	void draw(int x, int y)
	{
		tex_bind(tex);
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		glBegin(GL_QUADS);
		glTexCoord2i(0, 0); glVertex2i( x-hotspotx,   y+hotspoty   );
		glTexCoord2i(1, 0); glVertex2i( x-hotspotx+w, y+hotspoty   );
		glTexCoord2i(1, 1); glVertex2i( x-hotspotx+w, y+hotspoty-h );
		glTexCoord2i(0, 1); glVertex2i( x-hotspotx,   y+hotspoty-h );
		glEnd();
	}
};

extern int g_mouse_x, g_mouse_y;

#ifdef _WIN32
// On Windows, allow runtime choice between Windows cursors and OpenGL
// cursors (Windows = more responsive, OpenGL = more consistent with what
// the game sees)
struct Cursor
{
	union {
		HICON wincursor; // Windows handle
		ogl_cursor* cursor; // font texture
	};
	char type; // 0 for OpenGL cursor, 1 for Windows cursor
};
#else // #ifdef _WIN32
struct Cursor
{
	ogl_cursor* cursor; // font texture
};
#endif // #ifdef _WIN32 / #else

H_TYPE_DEFINE(Cursor);

static void Cursor_init(Cursor* c, va_list)
{
#ifdef _WIN32
# ifdef USE_WINDOWS_CURSOR
	c->type = 1;
# else
	c->type = 0;
# endif
	c->wincursor = NULL;
#endif

	c->cursor = NULL;
}

static void Cursor_dtor(Cursor* c)
{
#ifdef _WIN32
	if (c->type == 1)
	{
		if (c->wincursor)
			DestroyIcon(c->wincursor);
	}
	else
#endif // _WIN32
	{
		if (c->cursor)
		{
			tex_free(c->cursor->tex);
			delete c->cursor;
			c->cursor = NULL;
		}
	}
}

static void* ptr_from_HICON(HICON hIcon)
{
	return (void*)(uintptr_t)hIcon;
}

static HICON HICON_from_ptr(void* p)
{
	return (HICON)(uintptr_t)p;
}

static int cursor_load(Handle ht, int hotspotx, int hotspoty, void** sysdep_cursor)
{
	int ret = -1;
	*sysdep_cursor = 0;

	void* bgra_mem = 0;

	{
	// get format
	int w, h, fmt, bpp;
	const u8* imgdata;
	CHECK_ERR(tex_info(ht, &w, &h, &fmt, &bpp, (void**)&imgdata));
	if(bpp != 32 || (fmt != GL_BGRA && fmt != GL_RGBA))
	{
		debug_warn("Cursor texture not 32-bit RGBA/BGRA");
		ret = ERR_TEX_FMT_INVALID;
		goto fail;
	}

	// convert to BGRA if not already in that format (required by BMP)
	if(fmt != GL_BGRA)
	{
		// don't convert in-place so we don't spoil someone else's
		// use of the texture (however unlikely that may be).
		bgra_mem = malloc(w*h*4);
		if(!bgra_mem)
		{
			ret = ERR_NO_MEM;
			goto fail;
		}
		const u8* src = imgdata;
		u8* dst = (u8*)bgra_mem;
		for(int i = 0; i < w*h; i++)
		{
			const u8 r = src[0], g = src[1], b = src[2], a = src[3];
			dst[0] = b; dst[1] = g; dst[2] = r; dst[3] = a;
			dst += 4;
			src += 4;
		}
		imgdata = (const u8*)bgra_mem;
	}
/*
	const size_t size = round_up(w*h, 8)/8;
	u8* mask = (u8*)malloc(size);
	if(!mask)
	{
		ret = ERR_NO_MEM;
		goto fail;
	}
*/
/*
	uint bit = 0;
	for(int i = 0; i < w*h; i++)
	{
		++bit;
		bit &= 7;
		if(imgdata[i*4+3])
			mask[i/8] |= bit;
	}
*/
//memset(mask, 0, size);

/*
	BITMAPINFO bi =
	{
		{
			sizeof(BITMAPINFOHEADER), // biSize
				w,	// biWidth
				-h,	// biHeight; negative to indicate top-down (required)
				1,	// biPlanes
				32,	// biBitCount
				BI_RGB,	// biCompression
				0,	// biSizeImage (not needed for BI_RGB)
				0,	// biXPelsPerMeter (don't care)
				0,	// biYPelsPerMeter    "
				0,	// biClrUser (not needed for BI_RGB)
				0	// biClrImportant   "
		},	// bmiHeader
		0 // bmiColors[]
	};
	const HDC hDC = wglGetCurrentDC();
	HBITMAP hBitmap = CreateDIBitmap(hDC, &bi.bmiHeader, CBM_INIT, imgdata, &bi, DIB_RGB_COLORS);
	if(!hBitmap)	// not INVALID_HANDLE_VALUE
	{
		debug_warn("cursor CreateDIBitmap failed");
		goto fail;
	}

//HANDLE hCursor = LoadImage(GetModuleHandle(0), "D:\\projects\\0ad\\svn\\binaries\\data\\mods\\official\\art\\textures\\cursors\\test.png", IMAGE_BITMAP, 32, 32, LR_LOADFROMFILE);

*/
/*
	ICONINFO info = { FALSE, hotspotx, hotspoty, hBitmap, hBitmap };
	HICON hIcon = CreateIconIndirect(&info);
	DeleteObject(hBitmap);
*/
/*
HICON hIcon = CreateIcon(GetModuleHandle(0), w,h,1,32,mask,imgdata);
ICONINFO info;
GetIconInfo(hIcon, &info);
info.fIcon = FALSE;	// cursor
info.xHotspot = hotspotx;
info.yHotspot = hotspoty;
HICON hIcon2 = CreateIconIndirect(&info);

*/
/*
	BITMAPV5HEADER bi;
	ZeroMemory(&bi,sizeof(BITMAPV5HEADER));
	bi.bV5Size           = sizeof(BITMAPV5HEADER);
	bi.bV5Width           = w;
	bi.bV5Height          = h;
	bi.bV5Planes = 1;
	bi.bV5BitCount = 32;
	bi.bV5Compression = BI_BITFIELDS;
	// The following mask specification specifies a supported 32 BPP
	// alpha format for Windows XP.
	bi.bV5RedMask   =  0x00FF0000;
	bi.bV5GreenMask =  0x0000FF00;
	bi.bV5BlueMask  =  0x000000FF;
	bi.bV5AlphaMask =  0xFF000000;
*/
//	HDC hDC = GetDC(NULL);
	HBITMAP hBitmap = CreateBitmap(w, h, 1, 32, imgdata);
		//(BITMAPINFO*)&bi, DIB_RGB_COLORS, &dst, 0,0);
//	ReleaseDC(0, hDC);

	// Create an empty mask bitmap.
	HBITMAP hMonoBitmap = CreateBitmap(w, h, 1, 1, 0);

	ICONINFO ii;
	ii.fIcon = FALSE;  // cursor
	ii.xHotspot = hotspotx;
	ii.yHotspot = hotspoty;
	ii.hbmMask = hMonoBitmap;
	ii.hbmColor = hBitmap;

	// Create the alpha cursor with the alpha DIB section.
	HICON hIcon2 = CreateIconIndirect(&ii);

	DeleteObject(hBitmap);
	DeleteObject(hMonoBitmap);

	if(!hIcon2)	// not INVALID_HANDLE_VALUE
	{
		debug_warn("cursor CreateIconIndirect failed");
		goto fail;
	}

	*sysdep_cursor = ptr_from_HICON(hIcon2);
	ret = 0;
	}

fail:
	free(bgra_mem);

	CHECK_ERR(ret);
	return 0;
}






void CreateAlphaCursor(void)
{
	HDC hMemDC;
	DWORD dwWidth, dwHeight;
	BITMAPV5HEADER bi;
	HBITMAP hBitmap, hOldBitmap;
	void *lpBits;
	DWORD x,y;
	HCURSOR hAlphaCursor = NULL;

	dwWidth  = 32;  // width of cursor
	dwHeight = 32;  // height of cursor

	ZeroMemory(&bi,sizeof(BITMAPV5HEADER));
	bi.bV5Size           = sizeof(BITMAPV5HEADER);
	bi.bV5Width           = dwWidth;
	bi.bV5Height          = dwHeight;
	bi.bV5Planes = 1;
	bi.bV5BitCount = 32;
	bi.bV5Compression = BI_BITFIELDS;
	// The following mask specification specifies a supported 32 BPP
	// alpha format for Windows XP.
	bi.bV5RedMask   =  0x00FF0000;
	bi.bV5GreenMask =  0x0000FF00;
	bi.bV5BlueMask  =  0x000000FF;
	bi.bV5AlphaMask =  0xFF000000;

	HDC hdc;
	hdc = GetDC(NULL);

	// Create the DIB section with an alpha channel.
	hBitmap = CreateDIBSection(hdc, (BITMAPINFO *)&bi, DIB_RGB_COLORS,
		(void **)&lpBits, NULL, (DWORD)0);

	hMemDC = CreateCompatibleDC(hdc);
	ReleaseDC(NULL,hdc);

	// Draw something on the DIB section.
	hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
	PatBlt(hMemDC,0,0,dwWidth,dwHeight,WHITENESS);
	SetTextColor(hMemDC,RGB(0,0,0));
	SetBkMode(hMemDC,TRANSPARENT);
	TextOut(hMemDC,0,9,"rgba",4);
	SelectObject(hMemDC, hOldBitmap);
	DeleteDC(hMemDC);

	// Create an empty mask bitmap.
	HBITMAP hMonoBitmap = CreateBitmap(dwWidth,dwHeight,1,1,NULL);

	// Set the alpha values for each pixel in the cursor so that
	// the complete cursor is semi-transparent.
	DWORD *lpdwPixel;
	lpdwPixel = (DWORD *)lpBits;
	for (x=0;x<dwWidth;x++)
		for (y=0;y<dwHeight;y++)
		{
			// Clear the alpha bits
			*lpdwPixel &= 0x00FFFFFF;
			// Set the alpha bits to 0x9F (semi-transparent)
			*lpdwPixel |= 0x9F000000;
			lpdwPixel++;
		}

		ICONINFO ii;
		ii.fIcon = FALSE;  // Change fIcon to TRUE to create an alpha icon
		ii.xHotspot = 0;
		ii.yHotspot = 0;
		ii.hbmMask = hMonoBitmap;
		ii.hbmColor = hBitmap;

		// Create the alpha cursor with the alpha DIB section.
		hAlphaCursor = CreateIconIndirect(&ii);

		DeleteObject(hBitmap);
		DeleteObject(hMonoBitmap);

		SetCursor(hAlphaCursor);
//		return hAlphaCursor;
}



static int Cursor_reload(Cursor* c, const char* name, Handle)
{
	char filename[VFS_MAX_PATH];

	// Load the .txt file containing the pixel offset of
	// the cursor's hotspot (the bit of it that's
	// drawn at (g_mouse_x,g_mouse_y) )
	snprintf(filename, ARRAY_SIZE(filename), "art/textures/cursors/%s.txt", name);
	int hotspotx, hotspoty;
	{
		void* p;
		size_t size;
		Handle hm = vfs_load(filename, p, size);
		RETURN_ERR(hm);

		std::stringstream s(std::string((const char*)p, size));
		s >> hotspotx >> hotspoty;

		mem_free_h(hm);
	}

	snprintf(filename, ARRAY_SIZE(filename), "art/textures/cursors/%s.png", name);

	Handle ht = tex_load(filename);
	CHECK_ERR(ht);

#ifdef _WIN32
	if (c->type == 1)
	{
		int err = cursor_load(ht, hotspotx, hotspoty, (void**)&c->wincursor);
		if(err < 0)
		{
			tex_free(ht);
			return err;
		}
	}
	else
#endif // _WIN32
	{
		int err = tex_upload(ht, GL_NEAREST);
		CHECK_ERR(err);

		c->cursor = new ogl_cursor;

		c->cursor->tex = ht;
		c->cursor->hotspotx = hotspotx;
		c->cursor->hotspoty = hotspoty;
		// Get the width/height
		tex_info(c->cursor->tex, &c->cursor->w, &c->cursor->h, NULL, NULL, NULL);
	}

	return 0;
}

Handle cursor_load(const char* name)
{
	return h_alloc(H_Cursor, name, 0);
}

int cursor_free(Handle& h)
{
	return h_free(h, H_Cursor);
}

extern int g_yres; // from main.cpp. Required because GL's (0,0) is in the bottom-left

void cursor_draw(const char* name)
{
	// Use 'null' to disable the cursor
	if (!name)
	{
#ifdef _WIN32
		SetCursor(LoadCursor(NULL, IDC_ARROW));
#endif
		return;
	}

	Handle h = cursor_load(name);
	if (h <= 0)
		return;

	Cursor* c = (Cursor*) h_user_data(h, H_Cursor);
	if (!c)
		return;

#ifdef _WIN32
	if (c->type == 1)
	{
		SetCursor(c->wincursor);
	}
	else
#endif // _WIN32
	{
		c->cursor->draw(g_mouse_x, g_yres - g_mouse_y);
	}

	cursor_free(h);
}
