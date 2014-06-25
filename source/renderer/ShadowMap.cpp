/* Copyright (C) 2013 Wildfire Games.
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

#include "gui/GUIutil.h"
#include "lib/bits.h"
#include "lib/ogl.h"
#include "ps/CLogger.h"
#include "ps/Profile.h"

#include "graphics/LightEnv.h"
#include "graphics/ShaderManager.h"

#include "maths/BoundingBoxAligned.h"
#include "maths/Brush.h"
#include "maths/MathUtil.h"
#include "maths/Matrix3D.h"

#include "renderer/Renderer.h"
#include "renderer/ShadowMap.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
// ShadowMap implementation

/**
 * Struct ShadowMapInternals: Internal data for the ShadowMap implementation
 */
struct ShadowMapInternals
{
	// bit depth for the depth texture
	int DepthTextureBits;
	// the EXT_framebuffer_object framebuffer
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
	CBoundingBoxAligned ShadowCasterBound;
	CBoundingBoxAligned ShadowReceiverBound;

	CBoundingBoxAligned ShadowRenderBound;

	// Camera transformed into light space
	CCamera LightspaceCamera;

	// Some drivers (at least some Intel Mesa ones) appear to handle alpha testing
	// incorrectly when the FBO has only a depth attachment.
	// When m_ShadowAlphaFix is true, we use DummyTexture to store a useless
	// alpha texture which is attached to the FBO as a workaround.
	GLuint DummyTexture;

	// Copy of renderer's standard view camera, saved between
	// BeginRender and EndRender while we replace it with the shadow camera
	CCamera SavedViewCamera;
	
	// Save the caller's FBO so it can be restored
	GLint SavedViewFBO;

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
	m->DummyTexture = 0;
	m->Width = 0;
	m->Height = 0;
	m->EffectiveWidth = 0;
	m->EffectiveHeight = 0;
	m->DepthTextureBits = 0;
	// DepthTextureBits: 24/32 are very much faster than 16, on GeForce 4 and FX;
	// but they're very much slower on Radeon 9800.
	// In both cases, the default (no specified depth) is fast, so we just use
	// that by default and hope it's alright. (Otherwise, we'd probably need to
	// do some kind of hardware detection to work out what to use.)

	// Avoid using uninitialised values in AddShadowedBound if SetupFrame wasn't called first
	m->LightTransform.SetIdentity();
}


ShadowMap::~ShadowMap()
{
	if (m->Texture)
		glDeleteTextures(1, &m->Texture);
	if (m->DummyTexture)
		glDeleteTextures(1, &m->DummyTexture);
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
	if (m->DummyTexture)
		glDeleteTextures(1, &m->DummyTexture);
	if (m->Framebuffer)
		pglDeleteFramebuffersEXT(1, &m->Framebuffer);

	m->Texture = 0;
	m->DummyTexture = 0;
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
	m->ShadowCasterBound.SetEmpty();
	m->ShadowReceiverBound.SetEmpty();

	//
	m->LightspaceCamera = camera;
	m->LightspaceCamera.m_Orientation = m->LightTransform * camera.m_Orientation;
	m->LightspaceCamera.UpdateFrustum();
}


//////////////////////////////////////////////////////////////////////////////
// AddShadowedBound: add a world-space bounding box to the bounds of shadowed
// objects
void ShadowMap::AddShadowCasterBound(const CBoundingBoxAligned& bounds)
{
	CBoundingBoxAligned lightspacebounds;

	bounds.Transform(m->LightTransform, lightspacebounds);
	m->ShadowCasterBound += lightspacebounds;
}

void ShadowMap::AddShadowReceiverBound(const CBoundingBoxAligned& bounds)
{
	CBoundingBoxAligned lightspacebounds;

	bounds.Transform(m->LightTransform, lightspacebounds);
	m->ShadowReceiverBound += lightspacebounds;
}

CFrustum ShadowMap::GetShadowCasterCullFrustum()
{
	// Get the bounds of all objects that can receive shadows
	CBoundingBoxAligned bound = m->ShadowReceiverBound;

	// Intersect with the camera frustum, so the shadow map doesn't have to get
	// stretched to cover the off-screen parts of large models
	bound.IntersectFrustumConservative(m->LightspaceCamera.GetFrustum());

	// ShadowBound might have been empty to begin with, producing an empty result
	if (bound.IsEmpty())
	{
		// CFrustum can't easily represent nothingness, so approximate it with
		// a single point which won't match many objects
		bound += CVector3D(0.0f, 0.0f, 0.0f);
		return bound.ToFrustum();
	}

	// Extend the bounds a long way towards the light source, to encompass
	// all objects that might cast visible shadows.
	// (The exact constant was picked entirely arbitrarily.)
	bound[0].Z -= 1000.f;

	CFrustum frustum = bound.ToFrustum();
	frustum.Transform(m->InvLightTransform);
	return frustum;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// CalcShadowMatrices: calculate required matrices for shadow map generation - the light's
// projection and transformation matrices
void ShadowMapInternals::CalcShadowMatrices()
{
	// Start building the shadow map to cover all objects that will receive shadows
	CBoundingBoxAligned receiverBound = ShadowReceiverBound;

	// Intersect with the camera frustum, so the shadow map doesn't have to get
	// stretched to cover the off-screen parts of large models
	receiverBound.IntersectFrustumConservative(LightspaceCamera.GetFrustum());

	// Intersect with the shadow caster bounds, because there's no point
	// wasting space around the edges of the shadow map that we're not going
	// to draw into
	ShadowRenderBound[0].X = std::max(receiverBound[0].X, ShadowCasterBound[0].X);
	ShadowRenderBound[0].Y = std::max(receiverBound[0].Y, ShadowCasterBound[0].Y);
	ShadowRenderBound[1].X = std::min(receiverBound[1].X, ShadowCasterBound[1].X);
	ShadowRenderBound[1].Y = std::min(receiverBound[1].Y, ShadowCasterBound[1].Y);

	// Set the near and far planes to include just the shadow casters,
	// so we make full use of the depth texture's range. Add a bit of a
	// delta so we don't accidentally clip objects that are directly on
	// the planes.
	ShadowRenderBound[0].Z = ShadowCasterBound[0].Z - 2.f;
	ShadowRenderBound[1].Z = ShadowCasterBound[1].Z + 2.f;

	// ShadowBound might have been empty to begin with, producing an empty result
	if (ShadowRenderBound.IsEmpty())
	{
		// no-op
		LightProjection.SetIdentity();
		TextureMatrix = LightTransform;
		return;
	}

	// round off the shadow boundaries to sane increments to help reduce swim effect
	float boundInc = 16.0f;
	ShadowRenderBound[0].X = floor(ShadowRenderBound[0].X / boundInc) * boundInc;
	ShadowRenderBound[0].Y = floor(ShadowRenderBound[0].Y / boundInc) * boundInc;
	ShadowRenderBound[1].X = ceil(ShadowRenderBound[1].X / boundInc) * boundInc;
	ShadowRenderBound[1].Y = ceil(ShadowRenderBound[1].Y / boundInc) * boundInc;

	// Setup orthogonal projection (lightspace -> clip space) for shadowmap rendering
	CVector3D scale = ShadowRenderBound[1] - ShadowRenderBound[0];
	CVector3D shift = (ShadowRenderBound[1] + ShadowRenderBound[0]) * -0.5;

	if (scale.X < 1.0)
		scale.X = 1.0;
	if (scale.Y < 1.0)
		scale.Y = 1.0;
	if (scale.Z < 1.0)
		scale.Z = 1.0;

	scale.X = 2.0 / scale.X;
	scale.Y = 2.0 / scale.Y;
	scale.Z = 2.0 / scale.Z;

	// make sure a given world position falls on a consistent shadowmap texel fractional offset
	float offsetX = fmod(ShadowRenderBound[0].X - LightTransform._14, 2.0f/(scale.X*EffectiveWidth));
	float offsetY = fmod(ShadowRenderBound[0].Y - LightTransform._24, 2.0f/(scale.Y*EffectiveHeight));

	LightProjection.SetZero();
	LightProjection._11 = scale.X;
	LightProjection._14 = (shift.X + offsetX) * scale.X;
	LightProjection._22 = scale.Y;
	LightProjection._24 = (shift.Y + offsetY) * scale.Y;
	LightProjection._33 = scale.Z;
	LightProjection._34 = shift.Z * scale.Z;
	LightProjection._44 = 1.0;

	// Calculate texture matrix by creating the clip space to texture coordinate matrix
	// and then concatenating all matrices that have been calculated so far

	float texscalex = scale.X * 0.5f * (float)EffectiveWidth / (float)Width;
	float texscaley = scale.Y * 0.5f * (float)EffectiveHeight / (float)Height;
	float texscalez = scale.Z * 0.5f;

	CMatrix3D lightToTex;
	lightToTex.SetZero();
	lightToTex._11 = texscalex;
	lightToTex._14 = (offsetX - ShadowRenderBound[0].X) * texscalex;
	lightToTex._22 = texscaley;
	lightToTex._24 = (offsetY - ShadowRenderBound[0].Y) * texscaley;
	lightToTex._33 = texscalez;
	lightToTex._34 = -ShadowRenderBound[0].Z * texscalez;
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
	if (DummyTexture)
	{
		glDeleteTextures(1, &DummyTexture);
		DummyTexture = 0;
	}
	if (Framebuffer)
	{
		pglDeleteFramebuffersEXT(1, &Framebuffer);
		Framebuffer = 0;
	}
	
	// save the caller's FBO	
	glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &SavedViewFBO);

	pglGenFramebuffersEXT(1, &Framebuffer);

	if (g_Renderer.m_ShadowMapSize != 0)
	{
		// non-default option to override the size
		Width = Height = g_Renderer.m_ShadowMapSize;
	}
	else
	{
		// get shadow map size as next power of two up from view width/height
		Width = Height = (int)round_up_to_pow2((unsigned)std::max(g_Renderer.GetWidth(), g_Renderer.GetHeight()));
	}
	// Clamp to the maximum texture size
	Width = std::min(Width, (int)ogl_max_tex_size);
	Height = std::min(Height, (int)ogl_max_tex_size);

	// Since we're using a framebuffer object, the whole texture is available
	EffectiveWidth = Width;
	EffectiveHeight = Height;

	const char* formatname;

	switch(DepthTextureBits)
	{
	case 16: formatname = "DEPTH_COMPONENT16"; break;
	case 24: formatname = "DEPTH_COMPONENT24"; break;
	case 32: formatname = "DEPTH_COMPONENT32"; break;
	default: formatname = "DEPTH_COMPONENT"; break;
	}

	LOGMESSAGE(L"Creating shadow texture (size %dx%d) (format = %hs)",
		Width, Height, formatname);


	if (g_Renderer.m_Options.m_ShadowAlphaFix)
	{
		glGenTextures(1, &DummyTexture);
		g_Renderer.BindTexture(0, DummyTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	glGenTextures(1, &Texture);
	g_Renderer.BindTexture(0, Texture);

	GLenum format;

#if CONFIG2_GLES
	format = GL_DEPTH_COMPONENT;
#else
	switch (DepthTextureBits)
	{
	case 16: format = GL_DEPTH_COMPONENT16; break;
	case 24: format = GL_DEPTH_COMPONENT24; break;
	case 32: format = GL_DEPTH_COMPONENT32; break;
	default: format = GL_DEPTH_COMPONENT; break;
	}
#endif

	glTexImage2D(GL_TEXTURE_2D, 0, format, Width, Height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);
	// GLES requires type == UNSIGNED_SHORT or UNSIGNED_INT

	// set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#if CONFIG2_GLES
	// GLES doesn't do depth comparisons, so treat it as a
	// basic unfiltered depth texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#else
	// Enable automatic depth comparisons
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	// Use GL_LINEAR to trigger automatic PCF on some devices
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif

	ogl_WarnIfError();

	// bind to framebuffer object
	glBindTexture(GL_TEXTURE_2D, 0);
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, Framebuffer);

	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, Texture, 0);

	if (g_Renderer.m_Options.m_ShadowAlphaFix)
	{
		pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, DummyTexture, 0);
	}
	else
	{
#if CONFIG2_GLES
#warning TODO: figure out whether the glDrawBuffer/glReadBuffer stuff is needed, since it is not supported by GLES
#else
		glDrawBuffer(GL_NONE);
#endif
	}

#if !CONFIG2_GLES
	glReadBuffer(GL_NONE);
#endif

	ogl_WarnIfError();

	GLenum status = pglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, SavedViewFBO);

	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		LOGWARNING(L"Framebuffer object incomplete: 0x%04X", status);

		// Disable shadow rendering (but let the user try again if they want)
		g_Renderer.m_Options.m_Shadows = false;
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Set up to render into shadow map texture
void ShadowMap::BeginRender()
{
	// HACK HACK: this depends in non-obvious ways on the behaviour of the caller
	
	// save caller's FBO
	glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &m->SavedViewFBO);

	// Calc remaining shadow matrices
	m->CalcShadowMatrices();

	{
		PROFILE("bind framebuffer");
		glBindTexture(GL_TEXTURE_2D, 0);
		pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m->Framebuffer);
	}

	// clear buffers
	{
		PROFILE("clear depth texture");
		// In case we used m_ShadowAlphaFix, we ought to clear the unused
		// color buffer too, else Mali 400 drivers get confused.
		// Might as well clear stencil too for completeness.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glColorMask(0,0,0,0);
	}

	// setup viewport
	const SViewPort vp = { 0, 0, m->EffectiveWidth, m->EffectiveHeight };
	g_Renderer.SetViewport(vp);

	m->SavedViewCamera = g_Renderer.GetViewCamera();

	CCamera c = m->SavedViewCamera;
	c.SetProjection(m->LightProjection);
	c.GetOrientation() = m->InvLightTransform;
	g_Renderer.SetViewCamera(c);

#if !CONFIG2_GLES
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(&m->LightProjection._11);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(&m->LightTransform._11);
#endif

	glEnable(GL_SCISSOR_TEST);
	glScissor(1,1, m->EffectiveWidth-2, m->EffectiveHeight-2);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Finish rendering into shadow map texture
void ShadowMap::EndRender()
{
	glDisable(GL_SCISSOR_TEST);

	g_Renderer.SetViewCamera(m->SavedViewCamera);

	{
		PROFILE("unbind framebuffer");
		pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m->SavedViewFBO);
	}

	const SViewPort vp = { 0, 0, g_Renderer.GetWidth(), g_Renderer.GetHeight() };
	g_Renderer.SetViewport(vp);

	glColorMask(1,1,1,1);
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

///////////////////////////////////////////////////////////////////////////////////////////////////
// Depth texture size
int ShadowMap::GetWidth() const
{
	return m->Width;
}

int ShadowMap::GetHeight() const
{
	return m->Height;
}

//////////////////////////////////////////////////////////////////////////////

void ShadowMap::RenderDebugBounds()
{
	CShaderTechniquePtr shaderTech = g_Renderer.GetShaderManager().LoadEffect(str_gui_solid);
	shaderTech->BeginPass();
	CShaderProgramPtr shader = shaderTech->GetShader();

	glDepthMask(0);
	glDisable(GL_CULL_FACE);

	// Render various shadow bounds:
	//  Yellow = bounds of objects in view frustum that receive shadows
	//  Red = culling frustum used to find potential shadow casters
	//  Green = bounds of objects in culling frustum that cast shadows
	//  Blue = frustum used for rendering the shadow map

	shader->Uniform(str_transform, g_Renderer.GetViewCamera().GetViewProjection() * m->InvLightTransform);

	shader->Uniform(str_color, 1.0f, 1.0f, 0.0f, 1.0f);
	m->ShadowReceiverBound.RenderOutline(shader);

	shader->Uniform(str_color, 0.0f, 1.0f, 0.0f, 1.0f);
	m->ShadowCasterBound.RenderOutline(shader);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	shader->Uniform(str_color, 0.0f, 0.0f, 1.0f, 0.25f);
	m->ShadowRenderBound.Render(shader);
	glDisable(GL_BLEND);

	shader->Uniform(str_color, 0.0f, 0.0f, 1.0f, 1.0f);
	m->ShadowRenderBound.RenderOutline(shader);

	// Render light frustum

	shader->Uniform(str_transform, g_Renderer.GetViewCamera().GetViewProjection());

	CFrustum frustum = GetShadowCasterCullFrustum();
	// We don't have a function to create a brush directly from a frustum, so use
	// the ugly approach of creating a large cube and then intersecting with the frustum
	CBoundingBoxAligned dummy(CVector3D(-1e4, -1e4, -1e4), CVector3D(1e4, 1e4, 1e4));
	CBrush brush(dummy);
	CBrush frustumBrush;
	brush.Intersect(frustum, frustumBrush);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	shader->Uniform(str_color, 1.0f, 0.0f, 0.0f, 0.25f);
	frustumBrush.Render(shader);
	glDisable(GL_BLEND);

	shader->Uniform(str_color, 1.0f, 0.0f, 0.0f, 1.0f);
	frustumBrush.RenderOutline(shader);


	shaderTech->EndPass();

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

	glEnable(GL_CULL_FACE);
	glDepthMask(1);
}

void ShadowMap::RenderDebugTexture()
{
	glDepthMask(0);

	glDisable(GL_DEPTH_TEST);

#if !CONFIG2_GLES
	g_Renderer.BindTexture(0, m->Texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
#endif

	CShaderTechniquePtr texTech = g_Renderer.GetShaderManager().LoadEffect(str_gui_basic);
	texTech->BeginPass();
	CShaderProgramPtr texShader = texTech->GetShader();

	texShader->Uniform(str_transform, GetDefaultGuiMatrix());
	texShader->BindTexture(str_tex, m->Texture);

	float s = 256.f;
	float boxVerts[] = {
 		0,0, 0,s, s,0,
		s,0, 0,s, s,s
	};
	float boxUV[] = {
		0,0, 0,1, 1,0,
		1,0, 0,1, 1,1
	};

	texShader->VertexPointer(2, GL_FLOAT, 0, boxVerts);
	texShader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 0, boxUV);
	texShader->AssertPointersBound();
	glDrawArrays(GL_TRIANGLES, 0, 6);

	texTech->EndPass();

#if !CONFIG2_GLES
	g_Renderer.BindTexture(0, m->Texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
#endif

	glEnable(GL_DEPTH_TEST);
	glDepthMask(1);
}
