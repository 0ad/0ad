/* Copyright (c) 2017 Wildfire Games
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

#include <cstdio>
#include <cstring>
#include <sstream>

#include "lib/external_libraries/libsdl.h"
#include "lib/ogl.h"
#include "lib/res/h_mgr.h"
#include "lib/sysdep/cursor.h"
#include "ogl_tex.h"

// On Windows, allow runtime choice between system cursors and OpenGL
// cursors (Windows = more responsive, OpenGL = more consistent with what
// the game sees)
#if OS_WIN || OS_UNIX
# define ALLOW_SYS_CURSOR 1
#else
# define ALLOW_SYS_CURSOR 0
#endif

class SDLCursor
{
	SDL_Surface* surface;
	SDL_Cursor* cursor;

public:
	Status create(const PIVFS& vfs, const VfsPath& pathname, int hotspotx_, int hotspoty_, double scale)
	{
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

		surface = SDL_CreateRGBSurfaceFrom(bgra_img, (int)t.m_Width, (int)t.m_Height, 32, (int)t.m_Width*4, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
		if (!surface)
			return ERR::FAIL;
		if (scale != 1.0)
		{
			SDL_Surface* scaled_surface = SDL_CreateRGBSurface(0, surface->w * scale, surface->h * scale, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
			if (!scaled_surface)
				return ERR::FAIL;
			if (SDL_BlitScaled(surface, NULL, scaled_surface, NULL))
				return ERR::FAIL;
			SDL_FreeSurface(surface);
			surface = scaled_surface;
		}
		cursor = SDL_CreateColorCursor(surface, hotspotx_, hotspoty_);
		if (!cursor)
			return ERR::FAIL;

		return INFO::OK;
	}

	void set()
	{
		SDL_SetCursor(cursor);
	}

	void destroy()
	{
		SDL_FreeCursor(cursor);
		SDL_FreeSurface(surface);
	}
};

// no init is necessary because this is stored in struct Cursor, which
// is 0-initialized by h_mgr.
class GLCursor
{
	Handle ht;

	GLint w, h;
	int hotspotx, hotspoty;

public:
	Status create(const PIVFS& vfs, const VfsPath& pathname, int hotspotx_, int hotspoty_, double scale)
	{
		ht = ogl_tex_load(vfs, pathname);
		RETURN_STATUS_IF_ERR(ht);

		size_t width, height;
		(void)ogl_tex_get_size(ht, &width, &height, 0);
		w = (GLint)(width * scale);
		h = (GLint)(height * scale);

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
	CK_SDL,
	CK_OpenGL
};

struct Cursor
{
	double scale;

	// require kind == CK_OpenGL after reload
	bool forceGL;

	CursorKind kind;

	// valid iff kind == CK_SDL
	SDLCursor sdl_cursor;

	// valid iff kind == CK_OpenGL
	GLCursor gl_cursor;
};

H_TYPE_DEFINE(Cursor);

static void Cursor_init(Cursor* c, va_list args)
{
	c->scale = va_arg(args, double);
	c->forceGL = (va_arg(args, int) != 0);
}

static void Cursor_dtor(Cursor* c)
{
	switch(c->kind)
	{
	case CK_Default:
		break;	// nothing to do

	case CK_SDL:
		c->sdl_cursor.destroy();
		break;

	case CK_OpenGL:
		c->gl_cursor.destroy();
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

	// try loading as SDL2 cursor
	if (!c->forceGL && c->sdl_cursor.create(vfs, pathnameImage, hotspotx, hotspoty, c->scale) == INFO::OK)
		c->kind = CK_SDL;
	// fall back to GLCursor (system cursor code is disabled or failed)
	else if (c->gl_cursor.create(vfs, pathnameImage, hotspotx, hotspoty, c->scale) == INFO::OK)
		c->kind = CK_OpenGL;
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

	case CK_SDL:
		break;	// nothing to do

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

	case CK_SDL:
		type = L"sdl";
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

static Handle cursor_load(const PIVFS& vfs, const VfsPath& name, double scale, bool forceGL)
{
	return h_alloc(H_Cursor, vfs, name, 0, scale, (int)forceGL);
}

void cursor_shutdown()
{
	h_mgr_free_type(H_Cursor);
}

static Status cursor_free(Handle& h)
{
	return h_free(h, H_Cursor);
}


Status cursor_draw(const PIVFS& vfs, const wchar_t* name, int x, int y, double scale, bool forceGL)
{
	// hide the cursor
	if(!name)
	{
		SDL_ShowCursor(SDL_DISABLE);
		return INFO::OK;
	}

	Handle hc = cursor_load(vfs, name, scale, forceGL);
	// TODO: if forceGL changes at runtime after a cursor is first created,
	// we might reuse a cached version of the cursor with the old forceGL flag

	RETURN_STATUS_IF_ERR(hc); // silently ignore failures

	H_DEREF(hc, Cursor, c);

	switch(c->kind)
	{
	case CK_Default:
		break;

	case CK_SDL:
		c->sdl_cursor.set();
		SDL_ShowCursor(SDL_ENABLE);
		break;

	case CK_OpenGL:
		c->gl_cursor.draw(x, y);
		SDL_ShowCursor(SDL_DISABLE);
		break;

	default:
		DEBUG_WARN_ERR(ERR::LOGIC);
		break;
	}

	(void)cursor_free(hc);
	return INFO::OK;
}
