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

void GUIRenderer::UpdateDrawCallCache(DrawCalls &Calls, CStr &SpriteName, CRect &Size, int IconID, std::map<CStr, CGUISprite> &Sprites)
{
	// This is called only when something has changed (like the size of the
	// sprite), so it doesn't need to be particularly efficient.

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

		CRect ObjectSize = cit->m_Size.GetClientArea(Size);
		Call.m_Vertices = ObjectSize;

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

			int TexFormat, t_w, t_h;
			tex_info(h, &t_w, &t_h, &TexFormat, NULL, NULL);
			float TexWidth = (float)t_w, TexHeight = (float)t_h;
			
			// TODO: Detect the presence of an alpha channel in a nicer way
			Call.m_EnableBlending = (TexFormat == GL_RGBA || TexFormat == GL_BGRA);

			// Textures are positioned by defining a rectangular block of the
			// texture (usually the whole texture), and a rectangular block on
			// the screen. The texture is positioned to make those blocks line up.

			// Get the screen's position/size for the block
			CRect BlockScreen = cit->m_TextureSize.GetClientArea(ObjectSize);

			// Get the texture's position/size for the block:
			CRect BlockTex;

			// "real-texture-placement" overrides everything
			if (cit->m_TexturePlacementInFile != CRect())
				BlockTex = cit->m_TexturePlacementInFile;

			// Check whether this sprite has "icon-size" set
			else if (cit->m_IconSize != CSize())
			{
				int cols = t_w / (int)cit->m_IconSize.cx;
				int col = IconID % cols;
				int row = IconID / cols;
				BlockTex = CRect(cit->m_IconSize.cx*col, cit->m_IconSize.cy*row,
								 cit->m_IconSize.cx*(col+1), cit->m_IconSize.cy*(row+1));
			}

			// Use the whole texture
			else
				BlockTex = CRect(0, 0, TexWidth, TexHeight);


			// When rendering, BlockTex will be transformed onto BlockScreen.
			// Also, TexCoords will be transformed onto ObjectSize (giving the
			// UV coords at each vertex of the object). We know everything
			// except for TexCoords, so calculate it:

			CPos translation (BlockTex.TopLeft()-BlockScreen.TopLeft());
			float ScaleW = BlockTex.GetWidth()/BlockScreen.GetWidth();
			float ScaleH = BlockTex.GetHeight()/BlockScreen.GetHeight();
			
			CRect TexCoords (
						// Resize (translating to/from the origin, so the
						// topleft corner stays in the same place)
						(ObjectSize-ObjectSize.TopLeft())
						.Scale(ScaleW, ScaleH)
						+ ObjectSize.TopLeft()
						// Translate from BlockTex to BlockScreen
						+ translation
			);

			// The tex coords need to be scaled so that (texwidth,texheight) is
			// mapped onto (1,1)
			TexCoords.left   /= TexWidth;
			TexCoords.right  /= TexWidth;
			// and flip it vertically, because of some confusion between coordinate systems
			TexCoords.top    /= -TexHeight;
			TexCoords.bottom /= -TexHeight;

			Call.m_TexCoords = TexCoords;
		}
		else
		{
			Call.m_TexHandle = 0;
			Call.m_EnableBlending = !(fabs(cit->m_BackColor.a - 1.0f) < 0.0000001f);
		}

		Call.m_BackColor = cit->m_BackColor;
		Call.m_BorderColor = cit->m_Border ? cit->m_BorderColor : CColor();
		Call.m_DeltaZ = cit->m_DeltaZ;

		Calls.push_back(Call);
	}
}

void GUIRenderer::Draw(DrawCalls &Calls)
{
	// Called every frame, to draw the object (based on cached calculations)


	// Iterate through each DrawCall, and execute whatever drawing code is being called
	for (DrawCalls::const_iterator cit = Calls.begin(); cit != Calls.end(); ++cit)
	{
		glColor4f(cit->m_BackColor.r, cit->m_BackColor.g, cit->m_BackColor.b, cit->m_BackColor.a);

		if (cit->m_EnableBlending)
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
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

		}
		else
		{
			glDisable(GL_TEXTURE_2D);

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

		if (cit->m_EnableBlending)
		{
			glDisable(GL_BLEND);
		}

	}
}
