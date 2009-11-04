/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
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
#include "../h_mgr.h"
#include "lib/file/vfs/vfs.h"
extern PIVFS g_VFS;

// On Windows, allow runtime choice between system cursors and OpenGL
// cursors (Windows = more responsive, OpenGL = more consistent with what
// the game sees)
#if OS_WIN
# define ALLOW_SYS_CURSOR 1
#else
# define ALLOW_SYS_CURSOR 0
#endif


static LibError load_sys_cursor(const VfsPath& pathname, int hx, int hy, sys_cursor* cursor)
{
#if !ALLOW_SYS_CURSOR
	UNUSED2(pathname);
	UNUSED2(hx);
	UNUSED2(hy);
	UNUSED2(cursor);

	return ERR::FAIL;
#else
	shared_ptr<u8> file; size_t fileSize;
	RETURN_ERR(g_VFS->LoadFile(pathname, file, fileSize));

	ScopedTex t;
	RETURN_ERR(tex_decode(file, fileSize, &t));

	// convert to required BGRA format.
	const size_t flags = (t.flags | TEX_BGR) & ~TEX_DXT;
	RETURN_ERR(tex_transform_to(&t, flags));
	void* bgra_img = tex_get_data(&t);
	if(!bgra_img)
		WARN_RETURN(ERR::FAIL);

	RETURN_ERR(sys_cursor_create((int)t.w, (int)t.h, bgra_img, hx, hy, cursor));
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
	CursorKind kind;

	// valid iff kind == CK_System
	sys_cursor system_cursor;

	// valid iff kind == CK_OpenGL
	GLCursor gl_cursor;
	sys_cursor gl_empty_system_cursor;
};

H_TYPE_DEFINE(Cursor);

static void Cursor_init(Cursor* UNUSED(c), va_list UNUSED(args))
{
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
		debug_assert(0);
		break;
	}
}

static LibError Cursor_reload(Cursor* c, const VfsPath& name, Handle)
{
	const VfsPath path(L"art/textures/cursors");
	const VfsPath pathname(path/name);

	// read pixel offset of the cursor's hotspot [the bit of it that's
	// drawn at (g_mouse_x,g_mouse_y)] from file.
	int hotspotx = 0, hotspoty = 0;
	{
		const VfsPath pathnameHotspot = fs::change_extension(pathname, L".txt");
		shared_ptr<u8> buf; size_t size;
		RETURN_ERR(g_VFS->LoadFile(pathnameHotspot, buf, size));
		std::wstringstream s(std::wstring((const wchar_t*)buf.get(), size));
		s >> hotspotx >> hotspoty;
	}

	const VfsPath pathnameImage = fs::change_extension(pathname, L".dds");

	// try loading as system cursor (2d, hardware accelerated)
	if(load_sys_cursor(pathnameImage, hotspotx, hotspoty, &c->system_cursor) == INFO::OK)
		c->kind = CK_System;
	// fall back to GLCursor (system cursor code is disabled or failed)
	else if(c->gl_cursor.create(pathnameImage, hotspotx, hotspoty) == INFO::OK)
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

static LibError Cursor_validate(const Cursor* c)
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
		RETURN_ERR(c->gl_cursor.validate());
		break;

	default:
		WARN_RETURN(ERR::_2);
		break;
	}
		
	return INFO::OK;
}

static LibError Cursor_to_string(const Cursor* c, char* buf)
{
	const char* type;
	switch(c->kind)
	{
	case CK_Default:
		type = "default";
		break;

	case CK_System:
		type = "sys";
		break;

	case CK_OpenGL:
		type = "gl";
		break;

	default:
		debug_assert(0);
		type = "?";
		break;
	}

	snprintf(buf, H_STRING_LEN, "cursor (%hs)", type);
	return INFO::OK;
}


// note: these standard resource interface functions are not exposed to the
// caller. all we need here is storage for the sys_cursor / GLCursor and
// a name -> data lookup mechanism, both provided by h_mgr.
// in other words, we continually create/free the cursor resource in
// cursor_draw and trust h_mgr's caching to absorb it.

static Handle cursor_load(const VfsPath& name)
{
	return h_alloc(H_Cursor, name, 0);
}

static LibError cursor_free(Handle& h)
{
	return h_free(h, H_Cursor);
}


LibError cursor_draw(const wchar_t* name, int x, int y)
{
	// hide the cursor
	if(!name)
	{
		sys_cursor_set(0);
		return INFO::OK;
	}

	Handle hc = cursor_load(name);
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
		debug_assert(0);
		break;
	}

	(void)cursor_free(hc);
	return INFO::OK;
}
