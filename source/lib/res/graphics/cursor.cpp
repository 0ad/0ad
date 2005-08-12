#include "precompiled.h"

#include <string.h>
#include <sstream>

// On Windows, allow runtime choice between system cursors and OpenGL
// cursors (Windows = more responsive, OpenGL = more consistent with what
// the game sees)
#define USE_WINDOWS_CURSOR 1

#include "lib/ogl.h"
#include "sysdep/sysdep.h"	// sys_cursor_*
#include "../res.h"
#include "ogl_tex.h"
#include "cursor.h"

// no init is necessary because this is stored in struct Cursor, which
// is 0-initialized by h_mgr.
class GLCursor
{
	Handle ht;
	int w, h;
	int hotspotx, hotspoty;

public:
	void create(Handle ht_, int w_, int h_, int hotspotx_, int hotspoty_)
	{
		ht = ht_;
		w = w_; h = h_;
		hotspotx = hotspotx_; hotspoty = hotspoty_;

		WARN_ERR(tex_upload(ht, GL_NEAREST));
	}

	void destroy()
	{
		WARN_ERR(tex_free(ht));
	}

	void draw(int x, int y)
	{
		WARN_ERR(tex_bind(ht));
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		// OpenGL's coordinate system is "upside-down"; correct for that.
		y = g_yres - y;

		glBegin(GL_QUADS);
		glTexCoord2i(0, 0); glVertex2i( x-hotspotx,   y+hotspoty   );
		glTexCoord2i(1, 0); glVertex2i( x-hotspotx+w, y+hotspoty   );
		glTexCoord2i(1, 1); glVertex2i( x-hotspotx+w, y+hotspoty-h );
		glTexCoord2i(0, 1); glVertex2i( x-hotspotx,   y+hotspoty-h );
		glEnd();
	}
};


struct Cursor
{
	void* sys_cursor;

	// valid iff sys_cursor == 0.
	GLCursor gl_cursor;
};

H_TYPE_DEFINE(Cursor);

static void Cursor_init(Cursor* UNUSED(c), va_list UNUSED(args))
{
}

static void Cursor_dtor(Cursor* c)
{
	if(c->sys_cursor)
		WARN_ERR(sys_cursor_free(c->sys_cursor));
	else
		c->gl_cursor.destroy();
}

static int Cursor_reload(Cursor* c, const char* name, Handle)
{
	char filename[VFS_MAX_PATH];

	// Load the .txt file containing the pixel offset of the cursor's
	// hotspot (the bit of it that's drawn at (g_mouse_x,g_mouse_y) )
	snprintf(filename, ARRAY_SIZE(filename), "art/textures/cursors/%s.txt", name);
	int hotspotx = 0, hotspoty = 0;
	{
		void* p; size_t size;
		Handle hm = vfs_load(filename, p, size);
		WARN_ERR(hm);
		if(hm > 0)
		{
			std::stringstream s(std::string((const char*)p, size));
			s >> hotspotx >> hotspoty;

			WARN_ERR(mem_free_h(hm));
		}
	}

	snprintf(filename, ARRAY_SIZE(filename), "art/textures/cursors/%s.png", name);
	Handle ht = tex_load(filename);
	RETURN_ERR(ht);

	int w = 0, h = 0;
	int gl_fmt = 0, bpp = 0;
	void* img = 0;
	WARN_ERR(tex_info(ht, &w, &h, &gl_fmt, &bpp, &img));

#if USE_WINDOWS_CURSOR
	// verify texture format (this isn't done in sys_cursor_create to
	// avoid needing to pass gl_fmt and bpp; it assumes 32-bit RGBA).
	if(bpp != 32 || gl_fmt != GL_RGBA)
		debug_warn("Cursor_reload: invalid texture format");
	else
		WARN_ERR(sys_cursor_create(w, h, img, hotspotx, hotspoty, &c->sys_cursor));
#endif

	// if the system cursor code is disabled or failed, fall back to GLCursor.
	if(!c->sys_cursor)
		c->gl_cursor.create(ht, w, h, hotspotx, hotspoty);

	return 0;
}

// note: these standard resource interface functions are not exposed to the
// caller. all we need here is storage for the sys_cursor / GLCursor and
// a name -> data lookup mechanism, both provided by h_mgr.
// in other words, we continually create/free the cursor resource in
// cursor_draw and trust h_mgr's caching to absorb it.

static Handle cursor_load(const char* name)
{
	return h_alloc(H_Cursor, name, 0);
}

static int cursor_free(Handle& h)
{
	return h_free(h, H_Cursor);
}


// draw the specified cursor at the given pixel coordinates
// (origin is top-left to match the windowing system).
// uses a hardware mouse cursor where available, otherwise a
// portable OpenGL implementation.
int cursor_draw(const char* name, int x, int y)
{
	// Use 'null' to disable the cursor
	if(!name)
	{
		WARN_ERR(sys_cursor_set(0));
		return 0;
	}

	Handle hc = cursor_load(name);
	RETURN_ERR(hc);
	H_DEREF(hc, Cursor, c);

	if(c->sys_cursor)
		WARN_ERR(sys_cursor_set(c->sys_cursor));
	else
		c->gl_cursor.draw(x, y);

	(void)cursor_free(hc);
	return 0;
}
