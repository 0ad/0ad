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

static int Cursor_reload(Cursor* c, const char* name, Handle)
{
	char filename[VFS_MAX_PATH];

	// Load the .txt file containing the pixel offset of
	// the cursor's hotspot (the bit of it that's
	// drawn at (g_mouse_x,g_mouse_y) )
	sprintf(filename, "art/textures/cursors/%s.txt", name);

	int hotspotx, hotspoty;

	{
		void* p;
		size_t size;
		Handle hm = vfs_load(filename, p, size);
		CHECK_ERR(hm);

		std::stringstream s(std::string((const char*)p, size));
		s >> hotspotx >> hotspoty;

		mem_free_h(hm);
	}

	sprintf(filename, "art/textures/cursors/%s.png", name);

	Handle tex = tex_load(filename);
	CHECK_ERR(tex);

#ifdef _WIN32
	if (c->type == 1)
	{
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
			// Convert ABGR -> ARGB (little-endian)
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

			return -1;
		}

		BITMAPINFOHEADER dibheader = {
			sizeof(BITMAPINFOHEADER), // biSize
				w,	// biWidth
				-h,	// biHeight (positive means bottom-up)
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

			return -1;
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

			return -1;
		}

		if (imgdata_bgra != imgdata) delete[] imgdata_bgra;
		tex_free(tex);

		c->wincursor = cursor;
	}
	else
#endif // _WIN32
	{

		int err = tex_upload(tex);
		CHECK_ERR(err);

		c->cursor = new ogl_cursor;

		c->cursor->tex = tex;
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
