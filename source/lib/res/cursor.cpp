#include "precompiled.h"

#include <string.h>
#include <sstream>

#define USE_WINDOWS_CURSOR

#if OS_WIN
#include "lib/sysdep/win/win_internal.h"
#endif

#include "res.h"
#include "ogl_tex.h"
#include "lib/ogl.h"
#include "sysdep/sysdep.h"

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

#if OS_WIN
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
#else // #if OS_WIN
struct Cursor
{
	ogl_cursor* cursor; // font texture
};
#endif // #if OS_WIN / #else

H_TYPE_DEFINE(Cursor);

static void Cursor_init(Cursor* c, va_list)
{
#if OS_WIN
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
#if OS_WIN
	if (c->type == 1)
	{
		if (c->wincursor)
			DestroyIcon(c->wincursor);
	}
	else
#endif
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
	int ret;

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

#if OS_WIN
	if (c->type == 1)
	{
		int w, h, gl_fmt, bpp;
		void* img;
		if(tex_info(ht, &w, &h, &gl_fmt, &bpp, &img) < 0 ||
		  (bpp != 32 || gl_fmt != GL_RGBA))
		{
			debug_warn("Cursor_reload: invalid texture format");
			ret = ERR_TEX_FMT_INVALID;
			goto fail;
		}

		ret = cursor_create(w, h, img, hotspotx, hotspoty, (void**)&c->wincursor);
		if(ret < 0)
		{
fail:
			tex_free(ht);
			return ret;
		}
	}
	else
#endif
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
#if OS_WIN
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

#if OS_WIN
	if (c->type == 1)
	{
		SetCursor(c->wincursor);
	}
	else
#endif
	{
		c->cursor->draw(g_mouse_x, g_yres - g_mouse_y);
	}

	cursor_free(h);
}
