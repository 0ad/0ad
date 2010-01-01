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
 * Shadow mapping related texture and matrix management
 */

#include "precompiled.h"

#include "lib/bits.h"
#include "lib/ogl.h"
#include "ps/CLogger.h"

#include "graphics/LightEnv.h"

#include "maths/Bound.h"
#include "maths/MathUtil.h"
#include "maths/Matrix3D.h"

#include "renderer/Renderer.h"
#include "renderer/ShadowMap.h"

#define LOG_CATEGORY L"graphics"


///////////////////////////////////////////////////////////////////////////////////////////////////
// ShadowMap implementation

/**
 * Struct ShadowMapInternals: Internal data for the ShadowMap implementation
 */
struct ShadowMapInternals
{
	// whether we're using depth texture or luminance map
	bool UseDepthTexture;
	// bit depth for the depth texture, if used
	int DepthTextureBits;
	// if non-zero, we're using EXT_framebuffer_object for shadow rendering,
	// and this is the framebuffer
	GLuint Framebuffer;
	// handle of shadow map
	GLuint Texture;
	// width, height of shadow map
	int Width, Height;
	// used width, height of shadow map
	int EffectiveWidth, EffectiveHeight;
	// transform light space into projected light space
	// in projected light space, the shadowbound box occupies the [-1..1] cube
	// calculated on BeginRender, after the final shadow bounds are known
	CMatrix3D LightProjection;
	// Transform world space into light space; calculated on SetupFrame
	CMatrix3D LightTransform;
	// Transform world space into texture space of the shadow map;
	// calculated on BeginRender, after the final shadow bounds are known
	CMatrix3D TextureMatrix;

	// transform light space into world space
	CMatrix3D InvLightTransform;
	// bounding box of shadowed objects in light space
	CBound ShadowBound;

	// Camera transformed into light space
	CCamera LightspaceCamera;

	// Helper functions
	void CalcShadowMatrices();
	void CreateTexture();
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// Construction/Destruction
ShadowMap::ShadowMap()
{
	m = new ShadowMapInternals;
	m->Framebuffer = 0;
	m->Texture = 0;
	m->Width = 0;
	m->Height = 0;
	m->EffectiveWidth = 0;
	m->EffectiveHeight = 0;
	m->UseDepthTexture = false;
	m->DepthTextureBits = 0;
	// DepthTextureBits: 24/32 are very much faster than 16, on GeForce 4 and FX;
	// but they're very much slower on Radeon 9800.
	// In both cases, the default (no specified depth) is fast, so we just use
	// that by default and hope it's alright. (Otherwise, we'd probably need to
	// do some kind of hardware detection to work out what to use.)
}


ShadowMap::~ShadowMap()
{
	if (m->Texture)
		glDeleteTextures(1, &m->Texture);
	if (m->Framebuffer)
		pglDeleteFramebuffersEXT(1, &m->Framebuffer);

	delete m;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Force the texture/buffer/etc to be recreated, particularly when the renderer's
// size has changed
void ShadowMap::RecreateTexture()
{
	if (m->Texture)
		glDeleteTextures(1, &m->Texture);
	if (m->Framebuffer)
		pglDeleteFramebuffersEXT(1, &m->Framebuffer);

	m->Texture = 0;
	m->Framebuffer = 0;

	// (Texture will be constructed in next SetupFrame)
}

//////////////////////////////////////////////////////////////////////////////
// SetupFrame: camera and light direction for this frame
void ShadowMap::SetupFrame(const CCamera& camera, const CVector3D& lightdir)
{
	if (!m->Texture)
		m->CreateTexture();

	CVector3D z = lightdir;
	CVector3D y;
	CVector3D x = camera.m_Orientation.GetIn();
	CVector3D eyepos = camera.m_Orientation.GetTranslation();

	z.Normalize();
	x -= z * z.Dot(x);
	if (x.Length() < 0.001)
	{
		// this is invoked if the camera and light directions almost coincide
		// assumption: light direction has a significant Z component
		x = CVector3D(1.0, 0.0, 0.0);
		x -= z * z.Dot(x);
	}
	x.Normalize();
	y = z.Cross(x);

	// X axis perpendicular to light direction, flowing along with view direction
	m->LightTransform._11 = x.X;
	m->LightTransform._12 = x.Y;
	m->LightTransform._13 = x.Z;

	// Y axis perpendicular to light and view direction
	m->LightTransform._21 = y.X;
	m->LightTransform._22 = y.Y;
	m->LightTransform._23 = y.Z;

	// Z axis is in direction of light
	m->LightTransform._31 = z.X;
	m->LightTransform._32 = z.Y;
	m->LightTransform._33 = z.Z;

	// eye is at the origin of the coordinate system
	m->LightTransform._14 = -x.Dot(eyepos);
	m->LightTransform._24 = -y.Dot(eyepos);
	m->LightTransform._34 = -z.Dot(eyepos);

	m->LightTransform._41 = 0.0;
	m->LightTransform._42 = 0.0;
	m->LightTransform._43 = 0.0;
	m->LightTransform._44 = 1.0;

	m->LightTransform.GetInverse(m->InvLightTransform);
	m->ShadowBound.SetEmpty();

	//
	m->LightspaceCamera = camera;
	m->LightspaceCamera.m_Orientation = m->LightTransform * camera.m_Orientation;
	m->LightspaceCamera.UpdateFrustum();
}


//////////////////////////////////////////////////////////////////////////////
// AddShadowedBound: add a world-space bounding box to the bounds of shadowed
// objects
void ShadowMap::AddShadowedBound(const CBound& bounds)
{
	CBound lightspacebounds;

	bounds.Transform(m->LightTransform, lightspacebounds);
	m->ShadowBound += lightspacebounds;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// CalcShadowMatrices: calculate required matrices for shadow map generation - the light's
// projection and transformation matrices
void ShadowMapInternals::CalcShadowMatrices()
{
	CRenderer& renderer = g_Renderer;

	float minZ = ShadowBound[0].Z;

	ShadowBound.IntersectFrustumConservative(LightspaceCamera.GetFrustum());

	// minimum Z bound must not be clipped too much, because objects that lie outside
	// the shadow bounds cannot cast shadows either
	// the 2.0 is rather arbitrary: it should be big enough so that we won't accidently miss
	// a shadow generator, and small enough not to affect Z precision
	ShadowBound[0].Z = minZ - 2.0;

	// Setup orthogonal projection (lightspace -> clip space) for shadowmap rendering
	CVector3D scale = ShadowBound[1] - ShadowBound[0];
	CVector3D shift = (ShadowBound[1] + ShadowBound[0]) * -0.5;

	if (scale.X < 1.0)
		scale.X = 1.0;
	if (scale.Y < 1.0)
		scale.Y = 1.0;
	if (scale.Z < 1.0)
		scale.Z = 1.0;

	scale.X = 2.0 / scale.X;
	scale.Y = 2.0 / scale.Y;
	scale.Z = 2.0 / scale.Z;

	LightProjection.SetZero();
	LightProjection._11 = scale.X;
	LightProjection._14 = shift.X * scale.X;
	LightProjection._22 = scale.Y;
	LightProjection._24 = shift.Y * scale.Y;
	LightProjection._33 = scale.Z;
	LightProjection._34 = shift.Z * scale.Z + renderer.m_ShadowZBias;
	LightProjection._44 = 1.0;


	// Calculate texture matrix by creating the clip space to texture coordinate matrix
	// and then concatenating all matrices that have been calculated so far
	CMatrix3D lightToTex;
	float texscalex = (float)EffectiveWidth / (float)Width;
	float texscaley = (float)EffectiveHeight / (float)Height;
	float texscalez = 1.0;

	texscalex = texscalex / (ShadowBound[1].X - ShadowBound[0].X);
	texscaley = texscaley / (ShadowBound[1].Y - ShadowBound[0].Y);
	texscalez = texscalez / (ShadowBound[1].Z - ShadowBound[0].Z);

	lightToTex.SetZero();
	lightToTex._11 = texscalex;
	lightToTex._14 = -ShadowBound[0].X * texscalex;
	lightToTex._22 = texscaley;
	lightToTex._24 = -ShadowBound[0].Y * texscaley;
	lightToTex._33 = texscalez;
	lightToTex._34 = -ShadowBound[0].Z * texscalez;
	lightToTex._44 = 1.0;

	TextureMatrix = lightToTex * LightTransform;
}



//////////////////////////////////////////////////////////////////////////
// Create the shadow map
void ShadowMapInternals::CreateTexture()
{
	// Cleanup
	if (Texture)
	{
		glDeleteTextures(1, &Texture);
		Texture = 0;
	}
	if (Framebuffer)
	{
		pglDeleteFramebuffersEXT(1, &Framebuffer);
		Framebuffer = 0;
	}

	// Prepare FBO if available
	// Note: luminance is not an RGB format, so a luminance texture cannot be used
	// as a color buffer
	if (UseDepthTexture && g_Renderer.GetCapabilities().m_FramebufferObject)
	{
		pglGenFramebuffersEXT(1, &Framebuffer);
	}

	if (g_Renderer.m_ShadowMapSize != 0)
	{
		// non-default option to override the size
		Width = Height = g_Renderer.m_ShadowMapSize;
	}
	else
	{
		// get shadow map size as next power of two up from view width and height
		Width = (int)round_up_to_pow2((unsigned)g_Renderer.GetWidth());
		Height = (int)round_up_to_pow2((unsigned)g_Renderer.GetHeight());
	}
	// Clamp to the maximum texture size
	Width = std::min(Width, (int)ogl_max_tex_size);
	Height = std::min(Height, (int)ogl_max_tex_size);

	// If we're using a framebuffer object, the whole texture is available; otherwise
	// we're limited to the part of the screen buffer that is actually visible
	if (Framebuffer)
	{
		EffectiveWidth = Width;
		EffectiveHeight = Height;
	}
	else
	{
		EffectiveWidth = std::min(Width, g_Renderer.GetWidth());
		EffectiveHeight = std::min(Height, g_Renderer.GetHeight());
	}

	const char* formatname = "LUMINANCE";

	if (UseDepthTexture)
	{
		switch(DepthTextureBits)
		{
		case 16: formatname = "DEPTH_COMPONENT16"; break;
		case 24: formatname = "DEPTH_COMPONENT24"; break;
		case 32: formatname = "DEPTH_COMPONENT32"; break;
		default: formatname = "DEPTH_COMPONENT"; break;
		}
	}

	LOG(CLogger::Normal,  LOG_CATEGORY, L"Creating shadow texture (size %dx%d) (format = %hs)%ls",
		Width, Height, formatname, Framebuffer ? L" (using EXT_framebuffer_object)" : L"");

	// create texture object
	glGenTextures(1, &Texture);
	g_Renderer.BindTexture(0, Texture);

	int size = Width*Height;

	if (UseDepthTexture)
	{
		GLenum format;

		switch(DepthTextureBits)
		{
		case 16: format = GL_DEPTH_COMPONENT16; break;
		case 24: format = GL_DEPTH_COMPONENT24; break;
		case 32: format = GL_DEPTH_COMPONENT32; break;
		default: format = GL_DEPTH_COMPONENT; break;
		}

		float* buf = new float[size];
		for (int i = 0; i < size; i++)
			buf[i] = 1.0;
		glTexImage2D(GL_TEXTURE_2D, 0, format, Width, Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, buf);
		delete[] buf;

		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	}
	else
	{
		u32* buf = new u32[size];
		for (int i = 0; i < size; i++)
			buf[i] = 0x00ffffff;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
		delete[] buf;
	}

	// set texture parameters
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// bind to framebuffer object
	if (Framebuffer)
	{
		debug_assert(UseDepthTexture);

		glBindTexture(GL_TEXTURE_2D, 0);
		pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, Framebuffer);

		pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
				GL_TEXTURE_2D, Texture, 0);

		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		GLenum status = pglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

		pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

		if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
		{
			LOG(CLogger::Warning, LOG_CATEGORY, L"Framebuffer object incomplete: %04d", status);

			pglDeleteFramebuffersEXT(1, &Framebuffer);
			Framebuffer = 0;
			EffectiveWidth = std::min(Width, g_Renderer.GetWidth());
			EffectiveHeight = std::min(Height, g_Renderer.GetHeight());
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Set up to render into shadow map texture
void ShadowMap::BeginRender()
{
	// HACK HACK: this depends in non-obvious ways on the behaviour of the caller

	// Calc remaining shadow matrices
	m->CalcShadowMatrices();

	if (m->Framebuffer)
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m->Framebuffer);
	}

	// clear buffers
	if (m->UseDepthTexture)
	{
		glClear(GL_DEPTH_BUFFER_BIT);
		glColorMask(0,0,0,0);
	}
	else
	{
		glClearColor(1,1,1,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	// setup viewport
	glViewport(0, 0, m->EffectiveWidth, m->EffectiveHeight);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf(&m->LightProjection._11);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf(&m->LightTransform._11);

	glEnable(GL_SCISSOR_TEST);
	glScissor(1,1, m->EffectiveWidth-2, m->EffectiveHeight-2);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Finish rendering into shadow map texture
void ShadowMap::EndRender()
{
	glDisable(GL_SCISSOR_TEST);

	// copy result into shadow map texture
	if (m->Framebuffer)
	{
		pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
	else
	{
		if (!g_Renderer.GetDisableCopyShadow())
		{
			g_Renderer.BindTexture(0, m->Texture);
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, m->EffectiveWidth, m->EffectiveHeight);
		}
	}

	glViewport(0, 0, g_Renderer.GetWidth(), g_Renderer.GetHeight());

	if (m->UseDepthTexture)
	{
		glColorMask(1,1,1,1);
	}

	// restore matrix stack
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Retrieve the texture handle and texture matrix for shadowing
GLuint ShadowMap::GetTexture() const
{
	return m->Texture;
}

const CMatrix3D& ShadowMap::GetTextureMatrix() const
{
	return m->TextureMatrix;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Using depth textures vs. a simple luminance map
bool ShadowMap::GetUseDepthTexture() const
{
	return m->UseDepthTexture;
}

void ShadowMap::SetUseDepthTexture(bool depthTexture)
{
	if (depthTexture)
	{
		if (!g_Renderer.GetCapabilities().m_DepthTextureShadows)
		{
			LOG(CLogger::Warning, LOG_CATEGORY, L"Depth textures are not supported by your graphics card/driver. Fallback to luminance map (no self-shadowing)!");
			depthTexture = false;
		}
	}

	if (depthTexture != m->UseDepthTexture)
	{
		if (m->Texture)
		{
			glDeleteTextures(1, &m->Texture);
			m->Texture = 0;
		}
		m->Width = m->Height = 0;

		m->UseDepthTexture = depthTexture;
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Depth texture bits
int ShadowMap::GetDepthTextureBits() const
{
	return m->DepthTextureBits;
}

void ShadowMap::SetDepthTextureBits(int bits)
{
	if (bits != m->DepthTextureBits)
	{
		if (m->Texture)
		{
			glDeleteTextures(1, &m->Texture);
			m->Texture = 0;
		}
		m->Width = m->Height = 0;

		m->DepthTextureBits = bits;
	}
}


//////////////////////////////////////////////////////////////////////////////
// RenderDebugDisplay: debug visualizations
//  - blue: objects in shadow
void ShadowMap::RenderDebugDisplay()
{
	glDepthMask(0);
	glDisable(GL_CULL_FACE);

	// Render shadow bound
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glMultMatrixf(&m->InvLightTransform._11);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4ub(0,0,255,64);
	m->ShadowBound.Render();
	glDisable(GL_BLEND);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glColor3ub(0,0,255);
	m->ShadowBound.Render();

	glBegin(GL_LINES);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(0.0, 0.0, 50.0);
	glEnd();
	glBegin(GL_POLYGON);
		glVertex3f(0.0, 0.0, 50.0);
		glVertex3f(50.0, 0.0, 50.0);
		glVertex3f(0.0, 50.0, 50.0);
	glEnd();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPopMatrix();

#if 0
	CMatrix3D InvTexTransform;

	m->TextureMatrix.GetInverse(InvTexTransform);

	// Render representative texture rectangle
	glPushMatrix();
	glMultMatrixf(&InvTexTransform._11);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4ub(255,0,0,64);
	glBegin(GL_QUADS);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(1.0, 0.0, 0.0);
		glVertex3f(1.0, 1.0, 0.0);
		glVertex3f(0.0, 1.0, 0.0);
	glEnd();
	glDisable(GL_BLEND);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glColor3ub(255,0,0);
	glBegin(GL_QUADS);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(1.0, 0.0, 0.0);
		glVertex3f(1.0, 1.0, 0.0);
		glVertex3f(0.0, 1.0, 0.0);
	glEnd();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPopMatrix();
#endif

	// Render the shadow map
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	g_Renderer.BindTexture(0, m->Texture);
	if (m->UseDepthTexture)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	}
	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(0.2f, 0.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(0.2f, 0.2f);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 0.2f);
	glEnd();
	if (m->UseDepthTexture)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	}

	glEnable(GL_CULL_FACE);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glEnable(GL_DEPTH_TEST);
	glDepthMask(1);
}
