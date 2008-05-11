/**
 * =========================================================================
 * File        : cursor.cpp
 * Project     : 0 A.D.
 * Description : mouse cursors (either via OpenGL texture or hardware)
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "cursor.h"

#include "../h_mgr.h"
#include "lib/file/vfs/vfs.h"
extern PIVFS g_VFS;

#include <string.h>
#include <sstream>

// On Windows, allow runtime choice between system cursors and OpenGL
// cursors (Windows = more responsive, OpenGL = more consistent with what
// the game sees)
#if OS_WIN
# define ALLOW_SYS_CURSOR 1
#else
# define ALLOW_SYS_CURSOR 0
#endif

#include "lib/ogl.h"
#include "lib/sysdep/cursor.h"
#include "ogl_tex.h"

/*
	This is used to create the sys cursor to use together with the OpenGL
	cursor. I.e. to set a transparent cursor on X-windows where we don't use
	the X11 cursor, and on windows should the hardware cursor setup fail.
	
	Shouldn't be called when both hardware/software cursor fails (i.e. invalid
	cursor file given) - in that case we'd rather use the default cursor.
*/ 
static void* load_empty_sys_cursor()
{
	void* sys_cursor = 0;
	if(sys_cursor_create_empty(&sys_cursor) < 0)
	{
		debug_assert(0);
		return 0;
	}
	return sys_cursor;
}

static void* load_sys_cursor(const VfsPath& pathname, int hx, int hy)
{
#if !ALLOW_SYS_CURSOR
	UNUSED2(pathname);
	UNUSED2(hx);
	UNUSED2(hy);

	return 0;
#else
	shared_ptr<u8> file; size_t fileSize;
	if(g_VFS->LoadFile(pathname, file, fileSize) < 0)
		return 0;

	Tex t;
	if(tex_decode(file, fileSize, &t) < 0)
		return 0;

	{
	void* sys_cursor = 0;	// return value

	// convert to required BGRA format.
	const int flags = (t.flags | TEX_BGR) & ~TEX_DXT;
	if(tex_transform_to(&t, flags) < 0)
		goto fail;
	void* bgra_img = tex_get_data(&t);
	if(!bgra_img)
		goto fail;

	if(sys_cursor_create(t.w, t.h, bgra_img, hx, hy, &sys_cursor) < 0)
		goto fail;

	tex_free(&t);
	return sys_cursor;
	}

fail:
	debug_assert(0);
	tex_free(&t);
	return 0;
#endif
}


// no init is necessary because this is stored in struct Cursor, which
// is 0-initialized by h_mgr.
class GLCursor
{
	Handle ht;

	GLint w, h;
	int hotspotx, hotspoty;

public:
	LibError create(const VfsPath& pathname, int hotspotx_, int hotspoty_)
	{
		ht = ogl_tex_load(pathname);
		RETURN_ERR(ht);

		size_t width, height;
		(void)ogl_tex_get_size(ht, &width, &height, 0);
		w = (GLint)width;
		h = (GLint)height;

		hotspotx = hotspotx_; hotspoty = hotspoty_;

		(void)ogl_tex_set_filter(ht, GL_NEAREST);
		(void)ogl_tex_upload(ht);
		return INFO::OK;
	}

	void destroy()
	{
		// note: we're stored in a resource => ht is initially 0 =>
		// this is safe, no need for an is_valid flag
		(void)ogl_tex_free(ht);
	}

	void draw(int x, int y) const
	{
		(void)ogl_tex_bind(ht);
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

	LibError validate() const
	{
		const size_t A = 128;	// no cursor is expected to get this big
		if(w > A || h > A || hotspotx > A || hotspoty > A)
			WARN_RETURN(ERR::_1);
		if(ht < 0)
			WARN_RETURN(ERR::_2);
		return INFO::OK;
	}
};


struct Cursor
{
	void* sys_cursor;

	// valid iff sys_cursor == 0.
	GLCursor gl_cursor;
	// a system cursor to use together with the gl_cursor
	void *gl_sys_cursor;
};

H_TYPE_DEFINE(Cursor);

static void Cursor_init(Cursor* UNUSED(c), va_list UNUSED(args))
{
}

static void Cursor_dtor(Cursor* c)
{
	// (note: these are safe, no need for an is_valid flag)

	if(c->sys_cursor)
		WARN_ERR(sys_cursor_free(c->sys_cursor));
	else
		c->gl_cursor.destroy();
}

static LibError Cursor_reload(Cursor* c, const VfsPath& name, Handle)
{
	const VfsPath path("art/textures/cursors");
	const std::string basename = name.string();

	// read pixel offset of the cursor's hotspot [the bit of it that's
	// drawn at (g_mouse_x,g_mouse_y)] from file.
	int hotspotx = 0, hotspoty = 0;
	{
		const VfsPath pathname(path / (basename + ".txt"));
		shared_ptr<u8> buf; size_t size;
		RETURN_ERR(g_VFS->LoadFile(pathname, buf, size));
		std::stringstream s(std::string((const char*)buf.get(), size));
		s >> hotspotx >> hotspoty;
	}

	// load actual cursor
	const VfsPath pathname(path / (basename + ".dds"));
	// .. try loading as system cursor (2d, hardware accelerated)
	c->sys_cursor = load_sys_cursor(pathname, hotspotx, hotspoty);
	// .. fall back to GLCursor (system cursor code is disabled or failed)
	if(!c->sys_cursor)
	{
		LibError err=c->gl_cursor.create(pathname, hotspotx, hotspoty);
		
		if (err == INFO::OK)
			c->gl_sys_cursor = load_empty_sys_cursor();
		
		return err;			
	}

	return INFO::OK;
}

static LibError Cursor_validate(const Cursor* c)
{
	// note: system cursors have no state to speak of, so we don't need to
	// validate them.

	if(!c->sys_cursor)
		RETURN_ERR(c->gl_cursor.validate());
	return INFO::OK;
}

static LibError Cursor_to_string(const Cursor* c, char* buf)
{
	const char* type = c->sys_cursor? "sys" : "gl";
	snprintf(buf, H_STRING_LEN, "(%s)", type);
	return INFO::OK;
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

static LibError cursor_free(Handle& h)
{
	return h_free(h, H_Cursor);
}


// draw the specified cursor at the given pixel coordinates
// (origin is top-left to match the windowing system).
// uses a hardware mouse cursor where available, otherwise a
// portable OpenGL implementation.
LibError cursor_draw(const char* name, int x, int y)
{
	// Use 'null' to disable the cursor
	if(!name)
	{
		WARN_ERR(sys_cursor_set(0));
		return INFO::OK;
	}

	Handle hc = cursor_load(name);
	H_DEREF(hc, Cursor, c);

	if(c->sys_cursor)
		WARN_ERR(sys_cursor_set(c->sys_cursor));
	else
	{
		c->gl_cursor.draw(x, y);
		// Here, gl_sys_cursor is either a pointer to a valid cursor or NULL.
		// It is NULL if the gl_cursor init failed or if load_empty_sys_cursor
		// failed - in the first case, we want to use the default system cursor,
		// in the second case setting the default cursor yields no change.
		WARN_ERR(sys_cursor_set(c->gl_sys_cursor));
	}

	(void)cursor_free(hc);
	return INFO::OK;
}
