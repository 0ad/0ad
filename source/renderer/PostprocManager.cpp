/* Copyright (C) 2022 Wildfire Games.
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
#include "ps/ConfigDB.h"
#include "ps/CLogger.h"
#include "ps/CStrInternStatic.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "ps/VideoMode.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "renderer/RenderingOptions.h"
#include "tools/atlas/GameInterface/GameLoop.h"

#if !CONFIG2_GLES

CPostprocManager::CPostprocManager()
	: m_IsInitialized(false), m_PingFbo(0), m_PongFbo(0), m_PostProcEffect(L"default"),
	  m_BloomFbo(0), m_WhichBuffer(true), m_Sharpness(0.3f), m_UsingMultisampleBuffer(false),
	  m_MultisampleFBO(0), m_MultisampleCount(0)
{
}

CPostprocManager::~CPostprocManager()
{
	Cleanup();
}

bool CPostprocManager::IsEnabled() const
{
	return g_RenderingOptions.GetPostProc() &&
		g_VideoMode.GetBackend() != CVideoMode::Backend::GL_ARB;
}

void CPostprocManager::Cleanup()
{
	if (!m_IsInitialized) // Only cleanup if previously used
		return;

	if (m_PingFbo) glDeleteFramebuffersEXT(1, &m_PingFbo);
	if (m_PongFbo) glDeleteFramebuffersEXT(1, &m_PongFbo);
	if (m_BloomFbo) glDeleteFramebuffersEXT(1, &m_BloomFbo);
	m_PingFbo = m_PongFbo = m_BloomFbo = 0;

	m_ColorTex1.reset();
	m_ColorTex2.reset();
	m_DepthTex.reset();

	m_BlurTex2a.reset();
	m_BlurTex2b.reset();
	m_BlurTex4a.reset();
	m_BlurTex4b.reset();
	m_BlurTex8a.reset();
	m_BlurTex8b.reset();
}

void CPostprocManager::Initialize()
{
	if (m_IsInitialized)
		return;

	GLint maxSamples = 0;
	glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
	const GLsizei possibleSampleCounts[] = {2, 4, 8, 16};
	std::copy_if(
		std::begin(possibleSampleCounts), std::end(possibleSampleCounts),
		std::back_inserter(m_AllowedSampleCounts),
		[maxSamples](const GLsizei sampleCount) { return sampleCount <= maxSamples; } );

	// The screen size starts out correct and then must be updated with Resize()
	m_Width = g_Renderer.GetWidth();
	m_Height = g_Renderer.GetHeight();

	RecreateBuffers();
	m_IsInitialized = true;

	// Once we have initialised the buffers, we can update the techniques.
	UpdateAntiAliasingTechnique();
	UpdateSharpeningTechnique();
	UpdateSharpnessFactor();

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
		name = Renderer::Backend::GL::CTexture::Create2D( \
			Renderer::Backend::Format::R8G8B8A8, w, h, \
			Renderer::Backend::Sampler::MakeDefaultSampler( \
				Renderer::Backend::Sampler::Filter::LINEAR, \
				Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE));

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
	m_DepthTex = Renderer::Backend::GL::CTexture::Create2D(
		Renderer::Backend::Format::D24_S8, m_Width, m_Height,
		Renderer::Backend::Sampler::MakeDefaultSampler(
			Renderer::Backend::Sampler::Filter::LINEAR,
			Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE));

	glBindTexture(GL_TEXTURE_2D, m_DepthTex->GetHandle());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Set up the framebuffers with some initial textures.

	glGenFramebuffersEXT(1, &m_PingFbo);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);

	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
							   GL_TEXTURE_2D, m_ColorTex1->GetHandle(), 0);

	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT,
							   GL_TEXTURE_2D, m_DepthTex->GetHandle(), 0);

	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		LOGWARNING("Framebuffer object incomplete (A): 0x%04X", status);
	}

	glGenFramebuffersEXT(1, &m_PongFbo);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PongFbo);

	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
							   GL_TEXTURE_2D, m_ColorTex2->GetHandle(), 0);

	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT,
							   GL_TEXTURE_2D, m_DepthTex->GetHandle(), 0);

	status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		LOGWARNING("Framebuffer object incomplete (B): 0x%04X", status);
	}

	glGenFramebuffersEXT(1, &m_BloomFbo);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_BloomFbo);

	/*
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
							   GL_TEXTURE_2D, m_BloomTex1, 0);

	status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		LOGWARNING("Framebuffer object incomplete (B): 0x%04X", status);
	}
	*/

	if (m_UsingMultisampleBuffer)
	{
		DestroyMultisampleBuffer();
		CreateMultisampleBuffer();
	}

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}


void CPostprocManager::ApplyBlurDownscale2x(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	Renderer::Backend::GL::CTexture* inTex, Renderer::Backend::GL::CTexture* outTex, int inWidth, int inHeight)
{
	// Bind inTex to framebuffer for rendering.
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_BloomFbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, outTex->GetHandle(), 0);

	// Get bloom shader with instructions to simply copy texels.
	CShaderDefines defines;
	defines.Add(str_BLOOM_NOP, str_1);
	CShaderTechniquePtr tech = g_Renderer.GetShaderManager().LoadEffect(str_bloom, defines);

	tech->BeginPass();
	deviceCommandContext->SetGraphicsPipelineState(
		tech->GetGraphicsPipelineStateDesc());
	const CShaderProgramPtr& shader = tech->GetShader();

	shader->BindTexture(str_renderedTex, inTex);

	const SViewPort oldVp = g_Renderer.GetViewport();
	const SViewPort vp = { 0, 0, inWidth / 2, inHeight / 2 };
	g_Renderer.SetViewport(vp);

	float quadVerts[] =
	{
		1.0f, 1.0f,
		-1.0f, 1.0f,
		-1.0f, -1.0f,

		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f
	};
	float quadTex[] =
	{
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

void CPostprocManager::ApplyBlurGauss(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	Renderer::Backend::GL::CTexture* inOutTex, Renderer::Backend::GL::CTexture* tempTex, int inWidth, int inHeight)
{
	// Set tempTex as our rendering target.
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_BloomFbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, tempTex->GetHandle(), 0);

	// Get bloom shader, for a horizontal Gaussian blur pass.
	CShaderDefines defines2;
	defines2.Add(str_BLOOM_PASS_H, str_1);
	CShaderTechniquePtr tech = g_Renderer.GetShaderManager().LoadEffect(str_bloom, defines2);

	tech->BeginPass();
	deviceCommandContext->SetGraphicsPipelineState(
		tech->GetGraphicsPipelineStateDesc());
	CShaderProgramPtr shader = tech->GetShader();
	shader->BindTexture(str_renderedTex, inOutTex);
	shader->Uniform(str_texSize, inWidth, inHeight, 0.0f, 0.0f);

	const SViewPort oldVp = g_Renderer.GetViewport();
	const SViewPort vp = { 0, 0, inWidth, inHeight };
	g_Renderer.SetViewport(vp);

	float quadVerts[] =
	{
		1.0f, 1.0f,
		-1.0f, 1.0f,
		-1.0f, -1.0f,

		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f
	};
	float quadTex[] =
	{
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
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_BloomFbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, inOutTex->GetHandle(), 0);

	// Get bloom shader, for a vertical Gaussian blur pass.
	CShaderDefines defines3;
	defines3.Add(str_BLOOM_PASS_V, str_1);
	tech = g_Renderer.GetShaderManager().LoadEffect(str_bloom, defines3);

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

void CPostprocManager::ApplyBlur(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext)
{
	int width = m_Width, height = m_Height;

	#define SCALE_AND_BLUR(tex1, tex2, temptex) \
		ApplyBlurDownscale2x(deviceCommandContext, (tex1).get(), (tex2).get(), width, height); \
		width /= 2; \
		height /= 2; \
		ApplyBlurGauss(deviceCommandContext, (tex2).get(), (temptex).get(), width, height);

	// We do the same thing for each scale, incrementally adding more and more blur.
	SCALE_AND_BLUR(m_WhichBuffer ? m_ColorTex1 : m_ColorTex2, m_BlurTex2a, m_BlurTex2b);
	SCALE_AND_BLUR(m_BlurTex2a, m_BlurTex4a, m_BlurTex4b);
	SCALE_AND_BLUR(m_BlurTex4a, m_BlurTex8a, m_BlurTex8b);

	#undef SCALE_AND_BLUR
}


void CPostprocManager::CaptureRenderOutput()
{
	ENSURE(m_IsInitialized);

	// Leaves m_PingFbo selected for rendering; m_WhichBuffer stays true at this point.
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PongFbo);

	GLenum buffers[] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT };
	glDrawBuffers(1, buffers);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);
	glDrawBuffers(1, buffers);

	m_WhichBuffer = true;

	if (m_UsingMultisampleBuffer)
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_MultisampleFBO);
		glDrawBuffers(1, buffers);
	}
}


void CPostprocManager::ReleaseRenderOutput(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext)
{
	ENSURE(m_IsInitialized);

	const Renderer::Backend::GraphicsPipelineStateDesc pipelineStateDesc =
		Renderer::Backend::MakeDefaultGraphicsPipelineStateDesc();
	deviceCommandContext->SetGraphicsPipelineState(pipelineStateDesc);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// we blit to screen from the previous active buffer
	if (m_WhichBuffer)
		glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, m_PingFbo);
	else
		glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, m_PongFbo);

	glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
	glBlitFramebufferEXT(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height,
			      GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
	glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void CPostprocManager::ApplyEffect(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	const CShaderTechniquePtr& shaderTech1, int pass)
{
	// select the other FBO for rendering
	if (!m_WhichBuffer)
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);
	else
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PongFbo);

	glDisable(GL_DEPTH_TEST);

	shaderTech1->BeginPass(pass);
	deviceCommandContext->SetGraphicsPipelineState(
		shaderTech1->GetGraphicsPipelineStateDesc(pass));
	const CShaderProgramPtr& shader = shaderTech1->GetShader(pass);

	// Use the textures from the current FBO as input to the shader.
	// We also bind a bunch of other textures and parameters, but since
	// this only happens once per frame the overhead is negligible.
	if (m_WhichBuffer)
		shader->BindTexture(str_renderedTex, m_ColorTex1.get());
	else
		shader->BindTexture(str_renderedTex, m_ColorTex2.get());

	shader->BindTexture(str_depthTex, m_DepthTex.get());

	shader->BindTexture(str_blurTex2, m_BlurTex2a.get());
	shader->BindTexture(str_blurTex4, m_BlurTex4a.get());
	shader->BindTexture(str_blurTex8, m_BlurTex8a.get());

	shader->Uniform(str_width, m_Width);
	shader->Uniform(str_height, m_Height);
	shader->Uniform(str_zNear, m_NearPlane);
	shader->Uniform(str_zFar, m_FarPlane);

	shader->Uniform(str_sharpness, m_Sharpness);

	shader->Uniform(str_brightness, g_LightEnv.m_Brightness);
	shader->Uniform(str_hdr, g_LightEnv.m_Contrast);
	shader->Uniform(str_saturation, g_LightEnv.m_Saturation);
	shader->Uniform(str_bloom, g_LightEnv.m_Bloom);

	float quadVerts[] =
	{
		1.0f, 1.0f,
		-1.0f, 1.0f,
		-1.0f, -1.0f,

		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f
	};
	float quadTex[] =
	{
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

	shaderTech1->EndPass(pass);

	glEnable(GL_DEPTH_TEST);

	m_WhichBuffer = !m_WhichBuffer;
}

void CPostprocManager::ApplyPostproc(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext)
{
	ENSURE(m_IsInitialized);

	// Don't do anything if we are using the default effect and no AA.
	const bool hasEffects = m_PostProcEffect != L"default";
	const bool hasARB = g_VideoMode.GetBackend() == CVideoMode::Backend::GL_ARB;
	const bool hasAA = m_AATech && !hasARB;
	const bool hasSharp = m_SharpTech && !hasARB;
	if (!hasEffects && !hasAA && !hasSharp)
		return;

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PongFbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_TEXTURE_2D, 0, 0);

	GLenum buffers[] = { GL_COLOR_ATTACHMENT0_EXT };
	glDrawBuffers(1, buffers);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_TEXTURE_2D, 0, 0);
	glDrawBuffers(1, buffers);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PongFbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);

	if (hasEffects)
	{
		// First render blur textures. Note that this only happens ONLY ONCE, before any effects are applied!
		// (This may need to change depending on future usage, however that will have a fps hit)
		ApplyBlur(deviceCommandContext);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);
		for (int pass = 0; pass < m_PostProcTech->GetNumPasses(); ++pass)
			ApplyEffect(deviceCommandContext, m_PostProcTech, pass);
	}

	if (hasAA)
	{
		for (int pass = 0; pass < m_AATech->GetNumPasses(); ++pass)
			ApplyEffect(deviceCommandContext, m_AATech, pass);
	}

	if (hasSharp)
	{
		for (int pass = 0; pass < m_SharpTech->GetNumPasses(); ++pass)
			ApplyEffect(deviceCommandContext, m_SharpTech, pass);
	}

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PongFbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthTex->GetHandle(), 0);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthTex->GetHandle(), 0);
}


// Generate list of available effect-sets
std::vector<CStrW> CPostprocManager::GetPostEffects()
{
	std::vector<CStrW> effects;

	const VfsPath folder(L"shaders/effects/postproc/");

	VfsPaths pathnames;
	if (vfs::GetPathnames(g_VFS, folder, 0, pathnames) < 0)
		LOGERROR("Error finding Post effects in '%s'", folder.string8());

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

void CPostprocManager::UpdateAntiAliasingTechnique()
{
	if (g_VideoMode.GetBackend() == CVideoMode::Backend::GL_ARB || !m_IsInitialized)
		return;

	CStr newAAName;
	CFG_GET_VAL("antialiasing", newAAName);
	if (m_AAName == newAAName)
		return;
	m_AAName = newAAName;
	m_AATech.reset();

	if (m_UsingMultisampleBuffer)
	{
		m_UsingMultisampleBuffer = false;
		DestroyMultisampleBuffer();
	}

	// We have to hardcode names in the engine, because anti-aliasing
	// techinques strongly depend on the graphics pipeline.
	// We might use enums in future though.
	const CStr msaaPrefix = "msaa";
	if (m_AAName == "fxaa")
	{
		m_AATech = g_Renderer.GetShaderManager().LoadEffect(CStrIntern("fxaa"));
	}
	else if (m_AAName.size() > msaaPrefix.size() && m_AAName.substr(0, msaaPrefix.size()) == msaaPrefix)
	{
#if !CONFIG2_GLES
		// We don't want to enable MSAA in Atlas, because it uses wxWidgets and its canvas.
		if (g_AtlasGameLoop && g_AtlasGameLoop->running)
			return;
		const bool is_msaa_supported =
			ogl_HaveVersion(3, 3) &&
			ogl_HaveExtension("GL_ARB_multisample") &&
			ogl_HaveExtension("GL_ARB_texture_multisample") &&
			!m_AllowedSampleCounts.empty();
		if (!is_msaa_supported)
		{
			LOGWARNING("MSAA is unsupported.");
			return;
		}
		std::stringstream ss(m_AAName.substr(msaaPrefix.size()));
		ss >> m_MultisampleCount;
		if (std::find(std::begin(m_AllowedSampleCounts), std::end(m_AllowedSampleCounts), m_MultisampleCount) ==
		        std::end(m_AllowedSampleCounts))
		{
			m_MultisampleCount = 4;
			LOGWARNING("Wrong MSAA sample count: %s.", m_AAName.EscapeToPrintableASCII().c_str());
		}
		m_UsingMultisampleBuffer = true;
		CreateMultisampleBuffer();
#else
		#warning TODO: implement and test MSAA for GLES
		LOGWARNING("MSAA is unsupported.");
#endif
	}
}

void CPostprocManager::UpdateSharpeningTechnique()
{
	if (g_VideoMode.GetBackend() == CVideoMode::Backend::GL_ARB || !m_IsInitialized)
		return;

	CStr newSharpName;
	CFG_GET_VAL("sharpening", newSharpName);
	if (m_SharpName == newSharpName)
		return;
	m_SharpName = newSharpName;
	m_SharpTech.reset();

	if (m_SharpName == "cas")
	{
		m_SharpTech = g_Renderer.GetShaderManager().LoadEffect(CStrIntern(m_SharpName));
	}
}

void CPostprocManager::UpdateSharpnessFactor()
{
	CFG_GET_VAL("sharpness", m_Sharpness);
}

void CPostprocManager::SetDepthBufferClipPlanes(float nearPlane, float farPlane)
{
	m_NearPlane = nearPlane;
	m_FarPlane = farPlane;
}

void CPostprocManager::CreateMultisampleBuffer()
{
	glEnable(GL_MULTISAMPLE);

	m_MultisampleColorTex = Renderer::Backend::GL::CTexture::Create(
		Renderer::Backend::GL::CTexture::Type::TEXTURE_2D_MULTISAMPLE,
		Renderer::Backend::Format::R8G8B8A8, m_Width, m_Height,
		Renderer::Backend::Sampler::MakeDefaultSampler(
			Renderer::Backend::Sampler::Filter::LINEAR,
			Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE), 1, m_MultisampleCount);

	// Allocate the Depth/Stencil texture.
	m_MultisampleDepthTex = Renderer::Backend::GL::CTexture::Create(
		Renderer::Backend::GL::CTexture::Type::TEXTURE_2D_MULTISAMPLE,
		Renderer::Backend::Format::D24_S8, m_Width, m_Height,
		Renderer::Backend::Sampler::MakeDefaultSampler(
			Renderer::Backend::Sampler::Filter::LINEAR,
			Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE), 1, m_MultisampleCount);

	// Set up the framebuffers with some initial textures.
	glGenFramebuffersEXT(1, &m_MultisampleFBO);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_MultisampleFBO);

	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
		GL_TEXTURE_2D_MULTISAMPLE, m_MultisampleColorTex->GetHandle(), 0);

	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT,
		GL_TEXTURE_2D_MULTISAMPLE, m_MultisampleDepthTex->GetHandle(), 0);

	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		LOGWARNING("Multisample framebuffer object incomplete (A): 0x%04X", status);
		m_UsingMultisampleBuffer = false;
		DestroyMultisampleBuffer();
	}

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void CPostprocManager::DestroyMultisampleBuffer()
{
	if (m_UsingMultisampleBuffer)
		return;
	if (m_MultisampleFBO)
		glDeleteFramebuffersEXT(1, &m_MultisampleFBO);
	m_MultisampleColorTex.reset();
	m_MultisampleDepthTex.reset();
	glDisable(GL_MULTISAMPLE);
}

bool CPostprocManager::IsMultisampleEnabled() const
{
	return m_UsingMultisampleBuffer;
}

void CPostprocManager::ResolveMultisampleFramebuffer()
{
	if (!m_UsingMultisampleBuffer)
		return;

	glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, m_PingFbo);
	glBlitFramebufferEXT(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height,
		GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);
}

#else

#warning TODO: implement PostprocManager for GLES

void ApplyBlurDownscale2x(
	Renderer::Backend::GL::CDeviceCommandContext* UNUSED(deviceCommandContext),
	Renderer::Backend::GL::CTexture* UNUSED(inTex),
	Renderer::Backend::GL::CTexture* UNUSED(outTex),
	int UNUSED(inWidth), int UNUSED(inHeight))
{
}

void CPostprocManager::ApplyBlurGauss(
	Renderer::Backend::GL::CDeviceCommandContext* UNUSED(deviceCommandContext),
	Renderer::Backend::GL::CTexture* UNUSED(inOutTex),
	Renderer::Backend::GL::CTexture* UNUSED(tempTex),
	int UNUSED(inWidth), int UNUSED(inHeight))
{
}

void CPostprocManager::ApplyEffect(
	Renderer::Backend::GL::CDeviceCommandContext* UNUSED(deviceCommandContext),
	const CShaderTechniquePtr& UNUSED(shaderTech1), int UNUSED(pass))
{
}

CPostprocManager::CPostprocManager()
{
}

CPostprocManager::~CPostprocManager()
{
}

bool CPostprocManager::IsEnabled() const
{
	return false;
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

void CPostprocManager::UpdateAntiAliasingTechnique()
{
}

void CPostprocManager::UpdateSharpeningTechnique()
{
}

void CPostprocManager::UpdateSharpnessFactor()
{
}

void CPostprocManager::CaptureRenderOutput()
{
}

void CPostprocManager::ApplyPostproc(
	Renderer::Backend::GL::CDeviceCommandContext* UNUSED(deviceCommandContext))
{
}

void CPostprocManager::ReleaseRenderOutput(
	Renderer::Backend::GL::CDeviceCommandContext* UNUSED(deviceCommandContext))
{
}

void CPostprocManager::CreateMultisampleBuffer()
{
}

void CPostprocManager::DestroyMultisampleBuffer()
{
}

bool CPostprocManager::IsMultisampleEnabled() const
{
	return false;
}

void CPostprocManager::ResolveMultisampleFramebuffer()
{
}

#endif
