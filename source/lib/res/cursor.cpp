#include "precompiled.h"

#include <string.h>
#include <sstream>


#ifdef _WIN32

#include "lib/sysdep/win/win_internal.h"

#include "lib/res/tex.h"
#include "lib/res/ogl_tex.h"
#include "lib/res/vfs.h"
#include "lib/ogl.h"
#include "CVFSFile.h"


// Slightly hacky resource-loading code, but it doesn't
// need to do anything particularly clever.
// Pass name==NULL to unset the cursor. Pass name=="xyz"
// to load "art/textures/cursors/xyz.txt" (containing e.g. "10 10"
// for hotspot position) and "art/textures/cursors/xyz.png".

void cursor_draw(const char* name)
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
		CVFSFile file;
		if (file.Load(filename) != PSRETURN_OK)
		{
			assert(! "Error loading cursor hotspot .txt file");
			return;
		}
		std::stringstream s;
		s << file.GetAsString();
		s >> hotspotx >> hotspoty;
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

		return;
	}

	BITMAPINFOHEADER dibheader = {
		sizeof(BITMAPINFOHEADER), // biSize
			w,	// biWidth
			h,	// biHeight (positive means bottom-up)
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

/****************************************************************/
#else // #ifdef _WIN32

#include "lib/res/tex.h"
#include "lib/res/ogl_tex.h"
#include "lib/res/vfs.h"
#include "lib/ogl.h"
#include "CVFSFile.h"

extern int mouse_x, mouse_y;

struct Cursor {
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
			glTexCoord2i(0, 1); glVertex2i( x-hotspotx,   y+hotspoty   );
			glTexCoord2i(1, 1); glVertex2i( x-hotspotx+w, y+hotspoty   );
			glTexCoord2i(1, 0); glVertex2i( x-hotspotx+w, y+hotspoty-h );
			glTexCoord2i(0, 0); glVertex2i( x-hotspotx,   y+hotspoty-h );
		glEnd();
	}
};

extern int g_yres;

// TODO: Remove duplication with Windows cursor code, handle errors, rewrite.
void cursor_draw(const char* name)
{
	static struct Cursor* last_cursor = NULL;
	static char* last_name = NULL;

	if (name == NULL)
	{
		// Clean up

		if (last_cursor)
		{
			tex_free(last_cursor->tex);
			delete last_cursor;
		}
		last_cursor = NULL;

		delete[] last_name;
		last_name = NULL;

		return;
	}

	// Don't do much if it's the same as last time
	if (last_name && !strcmp(name, last_name))
	{
		last_cursor->draw(mouse_x, g_yres-mouse_y);
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
		CVFSFile file;
		if (file.Load(filename) != PSRETURN_OK)
		{
			assert(! "Error loading cursor hotspot .txt file");
			return;
		}
		std::stringstream s;
		s << file.GetAsString();
		s >> hotspotx >> hotspoty;
	}

	sprintf(filename, "art/textures/cursors/%s.png", name);

	Handle tex = tex_load(filename);
	if (tex <= 0)
	{
		// TODO: Handle errors
		assert(! "Error loading cursor texture");
		return;
	}

	int err = tex_upload(tex);
	if (err < 0)
	{
		// TODO: Handle errors
		assert(! "Error uploading cursor texture");
		return;
	}

	// Create the cursor object:

	int w, h;
	tex_info(tex, &w, &h, NULL, NULL, NULL);

	struct Cursor* cursor = new struct Cursor;
	cursor->tex = tex;
	cursor->hotspotx = hotspotx;
	cursor->hotspoty = hotspoty;
	cursor->w = w;
	cursor->h = h;

	if (last_cursor)
	{
		tex_free(last_cursor->tex);
		delete last_cursor;
	}
	cursor->draw(mouse_x, mouse_y);
	last_cursor = cursor;
}

#endif // #ifdef _WIN32 / #else
