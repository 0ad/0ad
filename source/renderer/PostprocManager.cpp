/* Copyright (C) 2019 Wildfire Games.
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

#include "precompiled.h"

#include "renderer/PostprocManager.h"

#include "graphics/GameView.h"
#include "graphics/LightEnv.h"
#include "graphics/ShaderManager.h"
#include "lib/bits.h"
#include "lib/ogl.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "renderer/Renderer.h"

#if !CONFIG2_GLES

CPostprocManager::CPostprocManager()
	: m_IsInitialized(false), m_PingFbo(0), m_PongFbo(0), m_PostProcEffect(L"default"), m_ColorTex1(0), m_ColorTex2(0),
	  m_DepthTex(0), m_BloomFbo(0), m_BlurTex2a(0), m_BlurTex2b(0), m_BlurTex4a(0), m_BlurTex4b(0),
	  m_BlurTex8a(0), m_BlurTex8b(0), m_WhichBuffer(true)
{
}

CPostprocManager::~CPostprocManager()
{
	Cleanup();
}

void CPostprocManager::Cleanup()
{
	if (!m_IsInitialized) // Only cleanup if previously used
		return;

	if (m_PingFbo) pglDeleteFramebuffersEXT(1, &m_PingFbo);
	if (m_PongFbo) pglDeleteFramebuffersEXT(1, &m_PongFbo);
	if (m_BloomFbo) pglDeleteFramebuffersEXT(1, &m_BloomFbo);
	m_PingFbo = m_PongFbo = m_BloomFbo = 0;

	if (m_ColorTex1) glDeleteTextures(1, &m_ColorTex1);
	if (m_ColorTex2) glDeleteTextures(1, &m_ColorTex2);
	if (m_DepthTex) glDeleteTextures(1, &m_DepthTex);
	m_ColorTex1 = m_ColorTex2 = m_DepthTex = 0;

	if (m_BlurTex2a) glDeleteTextures(1, &m_BlurTex2a);
	if (m_BlurTex2b) glDeleteTextures(1, &m_BlurTex2b);
	if (m_BlurTex4a) glDeleteTextures(1, &m_BlurTex4a);
	if (m_BlurTex4b) glDeleteTextures(1, &m_BlurTex4b);
	if (m_BlurTex8a) glDeleteTextures(1, &m_BlurTex8a);
	if (m_BlurTex8b) glDeleteTextures(1, &m_BlurTex8b);
	m_BlurTex2a = m_BlurTex2b = m_BlurTex4a = m_BlurTex4b = m_BlurTex8a = m_BlurTex8b = 0;
}

void CPostprocManager::Initialize()
{
	if (m_IsInitialized)
		return;

	// The screen size starts out correct and then must be updated with Resize()
	m_Width = g_Renderer.GetWidth();
	m_Height = g_Renderer.GetHeight();

	RecreateBuffers();
	m_IsInitialized = true;

	// This might happen after the map is loaded and the effect chosen
	SetPostEffect(m_PostProcEffect);
}

void CPostprocManager::Resize()
{
	m_Width = g_Renderer.GetWidth();
	m_Height = g_Renderer.GetHeight();

	// If the buffers were intialized, recreate them to the new size.
	if (m_IsInitialized)
		RecreateBuffers();
}

void CPostprocManager::RecreateBuffers()
{
	Cleanup();

	#define GEN_BUFFER_RGBA(name, w, h) \
		glGenTextures(1, (GLuint*)&name); \
		glBindTexture(GL_TEXTURE_2D, name); \
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0); \
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); \
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); \
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); \
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Two fullscreen ping-pong textures.
	GEN_BUFFER_RGBA(m_ColorTex1, m_Width, m_Height);
	GEN_BUFFER_RGBA(m_ColorTex2, m_Width, m_Height);

	// Textures for several blur sizes. It would be possible to reuse
	// m_BlurTex2b, thus avoiding the need for m_BlurTex4b and m_BlurTex8b, though given
	// that these are fairly small it's probably not worth complicating the coordinates passed
	// to the blur helper functions.
	GEN_BUFFER_RGBA(m_BlurTex2a, m_Width / 2, m_Height / 2);
	GEN_BUFFER_RGBA(m_BlurTex2b, m_Width / 2, m_Height / 2);

	GEN_BUFFER_RGBA(m_BlurTex4a, m_Width / 4, m_Height / 4);
	GEN_BUFFER_RGBA(m_BlurTex4b, m_Width / 4, m_Height / 4);

	GEN_BUFFER_RGBA(m_BlurTex8a, m_Width / 8, m_Height / 8);
	GEN_BUFFER_RGBA(m_BlurTex8b, m_Width / 8, m_Height / 8);

	#undef GEN_BUFFER_RGBA

	// Allocate the Depth/Stencil texture.
	glGenTextures(1, (GLuint*)&m_DepthTex);
	glBindTexture(GL_TEXTURE_2D, m_DepthTex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8_EXT, m_Width, m_Height,
				 0, GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8_EXT, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, 0);

	// Set up the framebuffers with some initial textures.

	pglGenFramebuffersEXT(1, &m_PingFbo);
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);

	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
							   GL_TEXTURE_2D, m_ColorTex1, 0);

	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT,
							   GL_TEXTURE_2D, m_DepthTex, 0);

	GLenum status = pglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		LOGWARNING("Framebuffer object incomplete (A): 0x%04X", status);
	}

	pglGenFramebuffersEXT(1, &m_PongFbo);
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PongFbo);

	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
							   GL_TEXTURE_2D, m_ColorTex2, 0);

	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT,
							   GL_TEXTURE_2D, m_DepthTex, 0);

	status = pglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		LOGWARNING("Framebuffer object incomplete (B): 0x%04X", status);
	}

	pglGenFramebuffersEXT(1, &m_BloomFbo);
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_BloomFbo);

	/*
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
							   GL_TEXTURE_2D, m_BloomTex1, 0);

	status = pglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		LOGWARNING("Framebuffer object incomplete (B): 0x%04X", status);
	}
	*/

	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}


void CPostprocManager::ApplyBlurDownscale2x(GLuint inTex, GLuint outTex, int inWidth, int inHeight)
{
	// Bind inTex to framebuffer for rendering.
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_BloomFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, outTex, 0);

	// Get bloom shader with instructions to simply copy texels.
	CShaderDefines defines;
	defines.Add(str_BLOOM_NOP, str_1);
	CShaderTechniquePtr tech = g_Renderer.GetShaderManager().LoadEffect(str_bloom,
			g_Renderer.GetSystemShaderDefines(), defines);

	tech->BeginPass();
	CShaderProgramPtr shader = tech->GetShader();

	GLuint renderedTex = inTex;

	// Cheat by creating high quality mipmaps for inTex, so the copying operation actually
	// produces good scaling due to hardware filtering.
	glBindTexture(GL_TEXTURE_2D, renderedTex);
	pglGenerateMipmapEXT(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	shader->BindTexture(str_renderedTex, renderedTex);

	const SViewPort oldVp = g_Renderer.GetViewport();
	const SViewPort vp = { 0, 0, inWidth / 2, inHeight / 2 };
	g_Renderer.SetViewport(vp);

	float quadVerts[] = {
		1.0f, 1.0f,
		-1.0f, 1.0f,
		-1.0f, -1.0f,

		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f
	};
	float quadTex[] = {
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f
	};
	shader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 0, quadTex);
	shader->VertexPointer(2, GL_FLOAT, 0, quadVerts);
	shader->AssertPointersBound();
	glDrawArrays(GL_TRIANGLES, 0, 6);

	g_Renderer.SetViewport(oldVp);

	tech->EndPass();
}

void CPostprocManager::ApplyBlurGauss(GLuint inOutTex, GLuint tempTex, int inWidth, int inHeight)
{
	// Set tempTex as our rendering target.
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_BloomFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, tempTex, 0);

	// Get bloom shader, for a horizontal Gaussian blur pass.
	CShaderDefines defines2;
	defines2.Add(str_BLOOM_PASS_H, str_1);
	CShaderTechniquePtr tech = g_Renderer.GetShaderManager().LoadEffect(str_bloom,
			g_Renderer.GetSystemShaderDefines(), defines2);

	tech->BeginPass();
	CShaderProgramPtr shader = tech->GetShader();
	shader->BindTexture(str_renderedTex, inOutTex);
	shader->Uniform(str_texSize, inWidth, inHeight, 0.0f, 0.0f);

	const SViewPort oldVp = g_Renderer.GetViewport();
	const SViewPort vp = { 0, 0, inWidth, inHeight };
	g_Renderer.SetViewport(vp);

	float quadVerts[] = {
		1.0f, 1.0f,
		-1.0f, 1.0f,
		-1.0f, -1.0f,

		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f
	};
	float quadTex[] = {
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f
	};
	shader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 0, quadTex);
	shader->VertexPointer(2, GL_FLOAT, 0, quadVerts);
	shader->AssertPointersBound();
	glDrawArrays(GL_TRIANGLES, 0, 6);

	g_Renderer.SetViewport(oldVp);

	tech->EndPass();

	// Set result texture as our render target.
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_BloomFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, inOutTex, 0);

	// Get bloom shader, for a vertical Gaussian blur pass.
	CShaderDefines defines3;
	defines3.Add(str_BLOOM_PASS_V, str_1);
	tech = g_Renderer.GetShaderManager().LoadEffect(str_bloom,
			g_Renderer.GetSystemShaderDefines(), defines3);

	tech->BeginPass();
	shader = tech->GetShader();

	// Our input texture to the shader is the output of the horizontal pass.
	shader->BindTexture(str_renderedTex, tempTex);
	shader->Uniform(str_texSize, inWidth, inHeight, 0.0f, 0.0f);

	g_Renderer.SetViewport(vp);

	shader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 0, quadTex);
	shader->VertexPointer(2, GL_FLOAT, 0, quadVerts);
	shader->AssertPointersBound();
	glDrawArrays(GL_TRIANGLES, 0, 6);

	g_Renderer.SetViewport(oldVp);

	tech->EndPass();
}

void CPostprocManager::ApplyBlur()
{
	glDisable(GL_BLEND);

	GLint originalFBO;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &originalFBO);

	int width = m_Width, height = m_Height;

	#define SCALE_AND_BLUR(tex1, tex2, temptex) \
		ApplyBlurDownscale2x(tex1, tex2, width, height); \
		width /= 2; \
		height /= 2; \
		ApplyBlurGauss(tex2, temptex, width, height);

	// We do the same thing for each scale, incrementally adding more and more blur.
	SCALE_AND_BLUR(m_WhichBuffer ? m_ColorTex1 : m_ColorTex2, m_BlurTex2a, m_BlurTex2b);
	SCALE_AND_BLUR(m_BlurTex2a, m_BlurTex4a, m_BlurTex4b);
	SCALE_AND_BLUR(m_BlurTex4a, m_BlurTex8a, m_BlurTex8b);

	#undef SCALE_AND_BLUR

	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, originalFBO);
}


void CPostprocManager::CaptureRenderOutput()
{
	ENSURE(m_IsInitialized);

	// clear both FBOs and leave m_PingFbo selected for rendering;
	// m_WhichBuffer stays true at this point
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PongFbo);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	GLenum buffers[] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT };
	pglDrawBuffers(1, buffers);

	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	pglDrawBuffers(1, buffers);

	m_WhichBuffer = true;
}


void CPostprocManager::ReleaseRenderOutput()
{
	ENSURE(m_IsInitialized);

	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// we blit to screen from the previous active buffer
	if (m_WhichBuffer)
		pglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, m_PingFbo);
	else
		pglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, m_PongFbo);

	pglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
	pglBlitFramebufferEXT(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height,
			      GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
	pglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);

	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void CPostprocManager::ApplyEffect(CShaderTechniquePtr &shaderTech1, int pass)
{
	// select the other FBO for rendering
	if (!m_WhichBuffer)
		pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);
	else
		pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PongFbo);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	shaderTech1->BeginPass(pass);
	CShaderProgramPtr shader = shaderTech1->GetShader(pass);

	shader->Bind();

	// Use the textures from the current FBO as input to the shader.
	// We also bind a bunch of other textures and parameters, but since
	// this only happens once per frame the overhead is negligible.
	if (m_WhichBuffer)
		shader->BindTexture(str_renderedTex, m_ColorTex1);
	else
		shader->BindTexture(str_renderedTex, m_ColorTex2);

	shader->BindTexture(str_depthTex, m_DepthTex);

	shader->BindTexture(str_blurTex2, m_BlurTex2a);
	shader->BindTexture(str_blurTex4, m_BlurTex4a);
	shader->BindTexture(str_blurTex8, m_BlurTex8a);

	shader->Uniform(str_width, m_Width);
	shader->Uniform(str_height, m_Height);
	shader->Uniform(str_zNear, m_NearPlane);
	shader->Uniform(str_zFar, m_FarPlane);

	shader->Uniform(str_brightness, g_LightEnv.m_Brightness);
	shader->Uniform(str_hdr, g_LightEnv.m_Contrast);
	shader->Uniform(str_saturation, g_LightEnv.m_Saturation);
	shader->Uniform(str_bloom, g_LightEnv.m_Bloom);

	float quadVerts[] = {
		1.0f, 1.0f,
		-1.0f, 1.0f,
		-1.0f, -1.0f,

		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f
	};
	float quadTex[] = {
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f
	};
	shader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 0, quadTex);
	shader->VertexPointer(2, GL_FLOAT, 0, quadVerts);
	shader->AssertPointersBound();
	glDrawArrays(GL_TRIANGLES, 0, 6);

	shader->Unbind();

	shaderTech1->EndPass(pass);

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	m_WhichBuffer = !m_WhichBuffer;
}

void CPostprocManager::ApplyPostproc()
{
	ENSURE(m_IsInitialized);

	// Don't do anything if we are using the default effect.
	if (m_PostProcEffect == L"default")
		return;

	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PongFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_TEXTURE_2D, 0, 0);

	GLenum buffers[] = { GL_COLOR_ATTACHMENT0_EXT };
	pglDrawBuffers(1, buffers);

	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_TEXTURE_2D, 0, 0);
	pglDrawBuffers(1, buffers);

	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PongFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);

	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);

	// First render blur textures. Note that this only happens ONLY ONCE, before any effects are applied!
	// (This may need to change depending on future usage, however that will have a fps hit)
	ApplyBlur();

	for (int pass = 0; pass < m_PostProcTech->GetNumPasses(); ++pass)
		ApplyEffect(m_PostProcTech, pass);

	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PongFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthTex, 0);

	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthTex, 0);
}


// Generate list of available effect-sets
std::vector<CStrW> CPostprocManager::GetPostEffects()
{
	std::vector<CStrW> effects;

	const VfsPath path(L"shaders/effects/postproc/");

	VfsPaths pathnames;
	if (vfs::GetPathnames(g_VFS, path, 0, pathnames) < 0)
		LOGERROR("Error finding Post effects in '%s'", path.string8());

	for (const VfsPath& path : pathnames)
		if (path.Extension() == L".xml")
			effects.push_back(path.Basename().string());

	// Add the default "null" effect to the list.
	effects.push_back(L"default");

	sort(effects.begin(), effects.end());

	return effects;
}

void CPostprocManager::SetPostEffect(const CStrW& name)
{
	if (m_IsInitialized)
	{
		if (name != L"default")
		{
			CStrW n = L"postproc/" + name;
			m_PostProcTech = g_Renderer.GetShaderManager().LoadEffect(CStrIntern(n.ToUTF8()));
		}
	}

	m_PostProcEffect = name;
}

void CPostprocManager::SetDepthBufferClipPlanes(float nearPlane, float farPlane)
{
	m_NearPlane = nearPlane;
	m_FarPlane = farPlane;
}

#else

#warning TODO: implement PostprocManager for GLES

void ApplyBlurDownscale2x(GLuint UNUSED(inTex), GLuint UNUSED(outTex), int UNUSED(inWidth), int UNUSED(inHeight))
{
}

void CPostprocManager::ApplyBlurGauss(GLuint UNUSED(inOutTex), GLuint UNUSED(tempTex), int UNUSED(inWidth), int UNUSED(inHeight))
{
}

void CPostprocManager::ApplyEffect(CShaderTechniquePtr &UNUSED(shaderTech1), int UNUSED(pass))
{
}

CPostprocManager::CPostprocManager()
{
}

CPostprocManager::~CPostprocManager()
{
}

void CPostprocManager::Initialize()
{
}

void CPostprocManager::Resize()
{
}

void CPostprocManager::Cleanup()
{
}

void CPostprocManager::RecreateBuffers()
{
}

std::vector<CStrW> CPostprocManager::GetPostEffects()
{
	return std::vector<CStrW>();
}

void CPostprocManager::SetPostEffect(const CStrW& UNUSED(name))
{
}

void CPostprocManager::SetDepthBufferClipPlanes(float UNUSED(nearPlane), float UNUSED(farPlane))
{
}

void CPostprocManager::CaptureRenderOutput()
{
}

void CPostprocManager::ApplyPostproc()
{
}

void CPostprocManager::ReleaseRenderOutput()
{
}

#endif
