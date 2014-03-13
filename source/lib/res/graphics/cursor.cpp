/* Copyright (c) 2012 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * mouse cursors (either via OpenGL texture or hardware)
 */

#include "precompiled.h"
#include "cursor.h"

#include <cstring>
#include <cstdio>
#include <sstream>

#include "lib/ogl.h"
#include "lib/sysdep/cursor.h"
#include "ogl_tex.h"
#include "lib/res/h_mgr.h"

// On Windows, allow runtime choice between system cursors and OpenGL
// cursors (Windows = more responsive, OpenGL = more consistent with what
// the game sees)
#if OS_WIN || OS_UNIX
# define ALLOW_SYS_CURSOR 1
#else
# define ALLOW_SYS_CURSOR 0
#endif


static Status load_sys_cursor(const PIVFS& vfs, const VfsPath& pathname, int hx, int hy, sys_cursor* cursor)
{
#if !ALLOW_SYS_CURSOR
	UNUSED2(vfs);
	UNUSED2(pathname);
	UNUSED2(hx);
	UNUSED2(hy);
	UNUSED2(cursor);

	return ERR::FAIL;
#else
	shared_ptr<u8> file; size_t fileSize;
	RETURN_STATUS_IF_ERR(vfs->LoadFile(pathname, file, fileSize));

	Tex t;
	RETURN_STATUS_IF_ERR(t.decode(file, fileSize));

	// convert to required BGRA format.
	const size_t flags = (t.m_Flags | TEX_BGR) & ~TEX_DXT;
	RETURN_STATUS_IF_ERR(t.transform_to(flags));
	void* bgra_img = t.get_data();
	if(!bgra_img)
		WARN_RETURN(ERR::FAIL);

	RETURN_STATUS_IF_ERR(sys_cursor_create((int)t.m_Width, (int)t.m_Height, bgra_img, hx, hy, cursor));
	return INFO::OK;
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
	Status create(const PIVFS& vfs, const VfsPath& pathname, int hotspotx_, int hotspoty_)
	{
		ht = ogl_tex_load(vfs, pathname);
		RETURN_STATUS_IF_ERR(ht);

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
#if CONFIG2_GLES
		UNUSED2(x); UNUSED2(y);
#warning TODO: implement cursors for GLES
#else
		(void)ogl_tex_bind(ht);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		glBegin(GL_QUADS);
		glTexCoord2i(1, 0); glVertex2i( x-hotspotx+w, y+hotspoty   );
		glTexCoord2i(0, 0); glVertex2i( x-hotspotx,   y+hotspoty   );
		glTexCoord2i(0, 1); glVertex2i( x-hotspotx,   y+hotspoty-h );
		glTexCoord2i(1, 1); glVertex2i( x-hotspotx+w, y+hotspoty-h );
		glEnd();

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
#endif
	}

	Status validate() const
	{
		const GLint A = 128;	// no cursor is expected to get this big
		if(w > A || h > A || hotspotx > A || hotspoty > A)
			WARN_RETURN(ERR::_1);
		if(ht < 0)
			WARN_RETURN(ERR::_2);
		return INFO::OK;
	}
};

enum CursorKind
{
	CK_Default,
	CK_System,
	CK_OpenGL
};

struct Cursor
{
	// require kind == CK_OpenGL after reload
	bool forceGL;

	CursorKind kind;

	// valid iff kind == CK_System
	sys_cursor system_cursor;

	// valid iff kind == CK_OpenGL
	GLCursor gl_cursor;
	sys_cursor gl_empty_system_cursor;
};

H_TYPE_DEFINE(Cursor);

static void Cursor_init(Cursor* c, va_list args)
{
	c->forceGL = (va_arg(args, int) != 0);
}

static void Cursor_dtor(Cursor* c)
{
	switch(c->kind)
	{
	case CK_Default:
		break;	// nothing to do

	case CK_System:
		sys_cursor_free(c->system_cursor);
		break;

	case CK_OpenGL:
		c->gl_cursor.destroy();
		sys_cursor_free(c->gl_empty_system_cursor);
		break;

	default:
		DEBUG_WARN_ERR(ERR::LOGIC);
		break;
	}
}

static Status Cursor_reload(Cursor* c, const PIVFS& vfs, const VfsPath& name, Handle)
{
	const VfsPath pathname(VfsPath(L"art/textures/cursors") / name);

	// read pixel offset of the cursor's hotspot [the bit of it that's
	// drawn at (g_mouse_x,g_mouse_y)] from file.
	int hotspotx = 0, hotspoty = 0;
	{
		const VfsPath pathnameHotspot = pathname.ChangeExtension(L".txt");
		shared_ptr<u8> buf; size_t size;
		RETURN_STATUS_IF_ERR(vfs->LoadFile(pathnameHotspot, buf, size));
		std::wstringstream s(std::wstring((const wchar_t*)buf.get(), size));
		s >> hotspotx >> hotspoty;
	}

	const VfsPath pathnameImage = pathname.ChangeExtension(L".png");

	// try loading as system cursor (2d, hardware accelerated)
	if(!c->forceGL && load_sys_cursor(vfs, pathnameImage, hotspotx, hotspoty, &c->system_cursor) == INFO::OK)
		c->kind = CK_System;
	// fall back to GLCursor (system cursor code is disabled or failed)
	else if(c->gl_cursor.create(vfs, pathnameImage, hotspotx, hotspoty) == INFO::OK)
	{
		c->kind = CK_OpenGL;
		// (we need to hide the system cursor when using a OpenGL cursor)
		sys_cursor_create_empty(&c->gl_empty_system_cursor);
	}
	// everything failed, leave cursor unchanged
	else
		c->kind = CK_Default;

	return INFO::OK;
}

static Status Cursor_validate(const Cursor* c)
{
	switch(c->kind)
	{
	case CK_Default:
		break;	// nothing to do

	case CK_System:
		if(c->system_cursor == 0)
			WARN_RETURN(ERR::_1);
		break;

	case CK_OpenGL:
		RETURN_STATUS_IF_ERR(c->gl_cursor.validate());
		break;

	default:
		WARN_RETURN(ERR::_2);
		break;
	}

	return INFO::OK;
}

static Status Cursor_to_string(const Cursor* c, wchar_t* buf)
{
	const wchar_t* type;
	switch(c->kind)
	{
	case CK_Default:
		type = L"default";
		break;

	case CK_System:
		type = L"sys";
		break;

	case CK_OpenGL:
		type = L"gl";
		break;

	default:
		DEBUG_WARN_ERR(ERR::LOGIC);
		type = L"?";
		break;
	}

	swprintf_s(buf, H_STRING_LEN, L"cursor (%ls)", type);
	return INFO::OK;
}


// note: these standard resource interface functions are not exposed to the
// caller. all we need here is storage for the sys_cursor / GLCursor and
// a name -> data lookup mechanism, both provided by h_mgr.
// in other words, we continually create/free the cursor resource in
// cursor_draw and trust h_mgr's caching to absorb it.

static Handle cursor_load(const PIVFS& vfs, const VfsPath& name, bool forceGL)
{
	return h_alloc(H_Cursor, vfs, name, 0, (int)forceGL);
}

void cursor_shutdown()
{
	h_mgr_free_type(H_Cursor);
}

static Status cursor_free(Handle& h)
{
	return h_free(h, H_Cursor);
}


Status cursor_draw(const PIVFS& vfs, const wchar_t* name, int x, int y, bool forceGL)
{
	// hide the cursor
	if(!name)
	{
		sys_cursor_set(0);
		return INFO::OK;
	}

	Handle hc = cursor_load(vfs, name, forceGL);
	// TODO: if forceGL changes at runtime after a cursor is first created,
	// we might reuse a cached version of the cursor with the old forceGL flag

	RETURN_STATUS_IF_ERR(hc); // silently ignore failures

	H_DEREF(hc, Cursor, c);

	switch(c->kind)
	{
	case CK_Default:
		break;

	case CK_System:
		sys_cursor_set(c->system_cursor);
		break;

	case CK_OpenGL:
		c->gl_cursor.draw(x, y);
		// note: gl_empty_system_cursor can be 0 if sys_cursor_create_empty
		// failed; in that case we don't want to sys_cursor_set because that
		// would restore the default cursor (which is exactly what we're
		// trying to avoid here)
		if(c->gl_empty_system_cursor)
			sys_cursor_set(c->gl_empty_system_cursor);
		break;

	default:
		DEBUG_WARN_ERR(ERR::LOGIC);
		break;
	}

	(void)cursor_free(hc);
	return INFO::OK;
}
