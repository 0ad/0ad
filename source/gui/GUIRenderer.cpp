#include "precompiled.h"

#include "GUIRenderer.h"

#include "lib/ogl.h"
#include "lib/res/h_mgr.h"

#include "ps/CLogger.h"
#define LOG_CATEGORY "gui"

using namespace GUIRenderer;

// Copyable texture Handle, for use in STL containers where the Handle should
// be freed when it's finished with.

Handle_rfcnt_tex::Handle_rfcnt_tex()
: h(0)
{
}

Handle_rfcnt_tex::Handle_rfcnt_tex(Handle h_)
: h(h_)
{
}

Handle_rfcnt_tex::Handle_rfcnt_tex(const Handle_rfcnt_tex& that)
{
	h = that.h;
	if (h) h_add_ref(h);
}

Handle_rfcnt_tex::~Handle_rfcnt_tex()
{
	if (h) tex_free(h);
}

Handle_rfcnt_tex& Handle_rfcnt_tex::operator=(Handle h_)
{
	h = h_;
	return *this;
}

// Functions to perform drawing-related actions:

void GUIRenderer::UpdateDrawCallCache(DrawCalls &Calls, CStr &SpriteName, CRect &Size, std::map<CStr, CGUISprite> &Sprites)
{
	Calls.clear();

	std::map<CStr, CGUISprite>::iterator it (Sprites.find(SpriteName));
	if (it == Sprites.end())
	{
		// Sprite not found. Check whether this a special sprite:
		//     stretched:filename.ext
		//     filename.ext
		// and if so, try to create it as a new sprite.

		// TODO: Implement this.

		// Otherwise, just complain and give up:
		LOG(ERROR, LOG_CATEGORY, "Trying to use a sprite that doesn't exist (\"%s\").", (const char*)SpriteName);
		return;
	}

    Calls.reserve(it->second.m_Images.size());

	// Iterate through all the sprite's images
	std::vector<SGUIImage>::const_iterator cit;
	for (cit = it->second.m_Images.begin(); cit != it->second.m_Images.end(); ++cit)
	{
		SDrawCall Call;
		if (cit->m_TextureName.Length())
		{
			Handle h = tex_load(cit->m_TextureName);
			if (h <= 0)
			{
				LOG(ERROR, LOG_CATEGORY, "Error reading texture '%s': %lld", (const char*)cit->m_TextureName, h);
				return;
			}

			int err = tex_upload(h);
			if (err < 0)
			{
				LOG(ERROR, LOG_CATEGORY, "Error uploading texture '%s': %d", (const char*)cit->m_TextureName, err);
				return;
			}

			Call.m_TexHandle = h;

			int fmt, t_w, t_h;
			tex_info(h, &t_w, &t_h, &fmt, NULL, NULL);
			Call.m_EnableBlending = (fmt == GL_RGBA || fmt == GL_BGRA);

			CRect real = cit->m_Size.GetClientArea(Size);

			// Get the screen position/size of a single tiling of the texture
			CRect TexSize = cit->m_TextureSize.GetClientArea(real);

			CRect TexCoords;

			TexCoords.left = (TexSize.left - real.left) / TexSize.GetWidth();
			TexCoords.right = TexCoords.left + real.GetWidth() / TexSize.GetWidth();

			// 'Bottom' is actually the top in screen-space (I think),
			// because the GUI puts (0,0) at the top-left
			TexCoords.bottom = (TexSize.bottom - real.bottom) / TexSize.GetHeight();
			TexCoords.top = TexCoords.bottom + real.GetHeight() / TexSize.GetHeight();

			if (cit->m_TexturePlacementInFile != CRect())
			{
				// Save the width/height, because we'll change the values one at a time and need
				//  to be able to use the unchanged width/height
				float width = TexCoords.GetWidth(),
					  height = TexCoords.GetHeight();

				float fTW=(float)t_w, fTH=(float)t_h;

				// notice left done after right, so that left is still unchanged, that is important.
				TexCoords.right = TexCoords.left + width * cit->m_TexturePlacementInFile.right/fTW;
				TexCoords.left += width * cit->m_TexturePlacementInFile.left/fTW;

				TexCoords.bottom = TexCoords.top + height * cit->m_TexturePlacementInFile.bottom/fTH;
				TexCoords.top += height * cit->m_TexturePlacementInFile.top/fTH;
			}

			Call.m_TexCoords = TexCoords;
			Call.m_Vertices = real;
		}
		else
		{
			Call.m_TexHandle = 0;
		}

		Call.m_BackColor = cit->m_BackColor;
		Call.m_BorderColor = cit->m_Border ? cit->m_BorderColor : CColor();
		Call.m_DeltaZ = cit->m_DeltaZ;

		Calls.push_back(Call);
	}
}

void GUIRenderer::Draw(DrawCalls &Calls)
{

	// Iterate through each DrawCall, and execute whatever drawing code is being called
	for (DrawCalls::const_iterator cit = Calls.begin(); cit != Calls.end(); ++cit)
	{
		glColor4f(cit->m_BackColor.r, cit->m_BackColor.g, cit->m_BackColor.b, cit->m_BackColor.a);

		if (cit->m_EnableBlending)
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
		}
		else
		{
			glDisable(GL_BLEND);
		}

		if (cit->m_TexHandle.h)
		{
			// TODO: Handle the GL state in a nicer way

			glEnable(GL_TEXTURE_2D);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

			tex_bind(cit->m_TexHandle.h);

			glBegin(GL_QUADS);

				glTexCoord2f(cit->m_TexCoords.right,cit->m_TexCoords.bottom);
				glVertex3f(cit->m_Vertices.right,	cit->m_Vertices.bottom,	cit->m_DeltaZ);

				glTexCoord2f(cit->m_TexCoords.left,	cit->m_TexCoords.bottom);
				glVertex3f(cit->m_Vertices.left,	cit->m_Vertices.bottom,	cit->m_DeltaZ);

				glTexCoord2f(cit->m_TexCoords.left,	cit->m_TexCoords.top);
				glVertex3f(cit->m_Vertices.left,	cit->m_Vertices.top,	cit->m_DeltaZ);

				glTexCoord2f(cit->m_TexCoords.right,cit->m_TexCoords.top);
				glVertex3f(cit->m_Vertices.right,	cit->m_Vertices.top,	cit->m_DeltaZ);

			glEnd();

			glDisable(GL_TEXTURE_2D);

		}
		else
		{

			glBegin(GL_QUADS);
				glVertex3f(cit->m_Vertices.right,	cit->m_Vertices.bottom,	cit->m_DeltaZ);
				glVertex3f(cit->m_Vertices.left,	cit->m_Vertices.bottom,	cit->m_DeltaZ);
				glVertex3f(cit->m_Vertices.left,	cit->m_Vertices.top,	cit->m_DeltaZ);
				glVertex3f(cit->m_Vertices.right,	cit->m_Vertices.top,	cit->m_DeltaZ);
			glEnd();


			if (cit->m_BorderColor != CColor())
			{
				glColor4f(cit->m_BorderColor.r, cit->m_BorderColor.g, cit->m_BorderColor.b, cit->m_BorderColor.a);
				glBegin(GL_LINE_LOOP);
					glVertex3f(cit->m_Vertices.left,		cit->m_Vertices.top+1.f,	cit->m_DeltaZ);
					glVertex3f(cit->m_Vertices.right-1.f,	cit->m_Vertices.top+1.f,	cit->m_DeltaZ);
					glVertex3f(cit->m_Vertices.right-1.f,	cit->m_Vertices.bottom,		cit->m_DeltaZ);
					glVertex3f(cit->m_Vertices.left,		cit->m_Vertices.bottom,		cit->m_DeltaZ);
				glEnd();
			}
		}
	}
}
