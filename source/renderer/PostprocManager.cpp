/* Copyright (C) 2024 Wildfire Games.
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
#include "maths/MathUtil.h"
#include "ps/ConfigDB.h"
#include "ps/CLogger.h"
#include "ps/CStrInternStatic.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "renderer/backend/IDevice.h"
#include "renderer/Renderer.h"
#include "renderer/RenderingOptions.h"
#include "tools/atlas/GameInterface/GameLoop.h"

#include <string_view>

namespace
{

void DrawFullscreenQuad(
	Renderer::Backend::IVertexInputLayout* vertexInputLayout,
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext)
{
	float quadVerts[] =
	{
		1.0f, 1.0f,
		-1.0f, 1.0f,
		-1.0f, -1.0f,

		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f
	};
	const bool flip =
		deviceCommandContext->GetDevice()->GetBackend() == Renderer::Backend::Backend::VULKAN;
	const float bottomV = flip ? 1.0 : 0.0f;
	const float topV = flip ? 0.0f : 1.0f;
	float quadTex[] =
	{
		1.0f, topV,
		0.0f, topV,
		0.0f, bottomV,

		0.0f, bottomV,
		1.0f, bottomV,
		1.0f, topV
	};

	deviceCommandContext->SetVertexInputLayout(vertexInputLayout);

	deviceCommandContext->SetVertexBufferData(
		0, quadVerts, std::size(quadVerts) * sizeof(quadVerts[0]));
	deviceCommandContext->SetVertexBufferData(
		1, quadTex, std::size(quadTex) * sizeof(quadTex[0]));

	deviceCommandContext->Draw(0, 6);
}

} // anonymous namespace

CPostprocManager::CPostprocManager(Renderer::Backend::IDevice* device)
	: m_Device(device), m_IsInitialized(false), m_PostProcEffect(L"default"),
	m_WhichBuffer(true), m_Sharpness(0.3f), m_UsingMultisampleBuffer(false),
	m_MultisampleCount(0)
{
}

CPostprocManager::~CPostprocManager()
{
	Cleanup();
}

bool CPostprocManager::IsEnabled() const
{
	const bool isDepthStencilFormatPresent =
		m_Device->GetPreferredDepthStencilFormat(
			Renderer::Backend::ITexture::Usage::DEPTH_STENCIL_ATTACHMENT, true, true)
				!= Renderer::Backend::Format::UNDEFINED;
	return
		g_RenderingOptions.GetPostProc() &&
		m_Device->GetBackend() != Renderer::Backend::Backend::GL_ARB &&
		isDepthStencilFormatPresent;
}

void CPostprocManager::Cleanup()
{
	if (!m_IsInitialized) // Only cleanup if previously used
		return;

	m_CaptureFramebuffer.reset();

	m_PingFramebuffer.reset();
	m_PongFramebuffer.reset();

	m_ColorTex1.reset();
	m_ColorTex2.reset();
	m_DepthTex.reset();

	for (BlurScale& scale : m_BlurScales)
	{
		for (BlurScale::Step& step : scale.steps)
		{
			step.framebuffer.reset();
			step.texture.reset();
		}
	}
}

void CPostprocManager::Initialize()
{
	if (m_IsInitialized)
		return;

	const std::array<Renderer::Backend::SVertexAttributeFormat, 2> attributes{{
		{Renderer::Backend::VertexAttributeStream::POSITION,
			Renderer::Backend::Format::R32G32_SFLOAT, 0, sizeof(float) * 2,
			Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0},
		{Renderer::Backend::VertexAttributeStream::UV0,
			Renderer::Backend::Format::R32G32_SFLOAT, 0, sizeof(float) * 2,
			Renderer::Backend::VertexAttributeRate::PER_VERTEX, 1},
	}};
	m_VertexInputLayout = g_Renderer.GetVertexInputLayout(attributes);

	const uint32_t maxSamples = m_Device->GetCapabilities().maxSampleCount;
	const uint32_t possibleSampleCounts[] = {2, 4, 8, 16};
	std::copy_if(
		std::begin(possibleSampleCounts), std::end(possibleSampleCounts),
		std::back_inserter(m_AllowedSampleCounts),
		[maxSamples](const uint32_t sampleCount) { return sampleCount <= maxSamples; } );

	// The screen size starts out correct and then must be updated with Resize()
	RecalculateSize(g_Renderer.GetWidth(), g_Renderer.GetHeight());

	RecreateBuffers();
	m_IsInitialized = true;

	// Once we have initialised the buffers, we can update the techniques.
	UpdateAntiAliasingTechnique();
	UpdateSharpeningTechnique();
	UpdateSharpnessFactor();
	CStr upscaleName;
	CFG_GET_VAL("renderer.upscale.technique", upscaleName);
	SetUpscaleTechnique(upscaleName);

	// This might happen after the map is loaded and the effect chosen
	SetPostEffect(m_PostProcEffect);

	if (m_Device->GetCapabilities().computeShaders)
		m_DownscaleComputeTech = g_Renderer.GetShaderManager().LoadEffect(CStrIntern("compute_downscale"));
}

void CPostprocManager::Resize()
{
	RecalculateSize(g_Renderer.GetWidth(), g_Renderer.GetHeight());

	// If the buffers were intialized, recreate them to the new size.
	if (m_IsInitialized)
		RecreateBuffers();
}

void CPostprocManager::RecreateBuffers()
{
	Cleanup();

	#define GEN_BUFFER_RGBA(name, w, h) \
		name = m_Device->CreateTexture2D( \
			"PostProc" #name, \
			Renderer::Backend::ITexture::Usage::SAMPLED | \
				Renderer::Backend::ITexture::Usage::COLOR_ATTACHMENT | \
				Renderer::Backend::ITexture::Usage::TRANSFER_SRC | \
				Renderer::Backend::ITexture::Usage::TRANSFER_DST, \
			Renderer::Backend::Format::R8G8B8A8_UNORM, w, h, \
			Renderer::Backend::Sampler::MakeDefaultSampler( \
				Renderer::Backend::Sampler::Filter::LINEAR, \
				Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE));

	// Two fullscreen ping-pong textures.
	GEN_BUFFER_RGBA(m_ColorTex1, m_Width, m_Height);
	GEN_BUFFER_RGBA(m_ColorTex2, m_Width, m_Height);

	if (m_UnscaledWidth != m_Width && m_Device->GetCapabilities().computeShaders)
	{
		const uint32_t usage =
			Renderer::Backend::ITexture::Usage::TRANSFER_SRC |
				Renderer::Backend::ITexture::Usage::COLOR_ATTACHMENT |
				Renderer::Backend::ITexture::Usage::SAMPLED |
				Renderer::Backend::ITexture::Usage::STORAGE;
		m_UnscaledTexture1 = m_Device->CreateTexture2D(
			"PostProcUnscaledTexture1", usage,
			Renderer::Backend::Format::R8G8B8A8_UNORM,
			m_UnscaledWidth, m_UnscaledHeight,
			Renderer::Backend::Sampler::MakeDefaultSampler(
				Renderer::Backend::Sampler::Filter::LINEAR,
				Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE));

		m_UnscaledTexture2 = m_Device->CreateTexture2D(
			"PostProcUnscaledTexture2", usage,
			Renderer::Backend::Format::R8G8B8A8_UNORM, m_UnscaledWidth, m_UnscaledHeight,
			Renderer::Backend::Sampler::MakeDefaultSampler(
				Renderer::Backend::Sampler::Filter::LINEAR,
				Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE));

		Renderer::Backend::SColorAttachment colorAttachment{};
		colorAttachment.clearColor = CColor{0.0f, 0.0f, 0.0f, 0.0f};
		colorAttachment.loadOp = Renderer::Backend::AttachmentLoadOp::LOAD;
		colorAttachment.storeOp = Renderer::Backend::AttachmentStoreOp::STORE;

		colorAttachment.texture = m_UnscaledTexture1.get();
		m_UnscaledFramebuffer1 = m_Device->CreateFramebuffer("PostprocUnscaledFramebuffer1",
			&colorAttachment, nullptr);

		colorAttachment.texture = m_UnscaledTexture2.get();
		m_UnscaledFramebuffer2 = m_Device->CreateFramebuffer("PostprocUnscaledFramebuffer2",
			&colorAttachment, nullptr);
	}

	// Textures for several blur sizes. It would be possible to reuse
	// m_BlurTex2b, thus avoiding the need for m_BlurTex4b and m_BlurTex8b, though given
	// that these are fairly small it's probably not worth complicating the coordinates passed
	// to the blur helper functions.
	uint32_t width = m_Width / 2, height = m_Height / 2;
	for (BlurScale& scale : m_BlurScales)
	{
		for (BlurScale::Step& step : scale.steps)
		{
			GEN_BUFFER_RGBA(step.texture, width, height);
			Renderer::Backend::SColorAttachment colorAttachment{};
			colorAttachment.texture = step.texture.get();
			colorAttachment.loadOp = Renderer::Backend::AttachmentLoadOp::LOAD;
			colorAttachment.storeOp = Renderer::Backend::AttachmentStoreOp::STORE;
			colorAttachment.clearColor = CColor{0.0f, 0.0f, 0.0f, 0.0f};
			step.framebuffer = m_Device->CreateFramebuffer(
				"BlurScaleStepFramebuffer", &colorAttachment, nullptr);
		}
		width = std::max(1u, width / 2);
		height = std::max(1u, height / 2);
	}

	#undef GEN_BUFFER_RGBA

	// Allocate the Depth/Stencil texture.
	m_DepthTex = m_Device->CreateTexture2D("PostProcDepthTexture",
		Renderer::Backend::ITexture::Usage::SAMPLED |
			Renderer::Backend::ITexture::Usage::DEPTH_STENCIL_ATTACHMENT,
		m_Device->GetPreferredDepthStencilFormat(
			Renderer::Backend::ITexture::Usage::SAMPLED |
				Renderer::Backend::ITexture::Usage::DEPTH_STENCIL_ATTACHMENT,
			true, true),
		m_Width, m_Height,
		Renderer::Backend::Sampler::MakeDefaultSampler(
			Renderer::Backend::Sampler::Filter::LINEAR,
			Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE));

	// Set up the framebuffers with some initial textures.
	Renderer::Backend::SColorAttachment colorAttachment{};
	colorAttachment.texture = m_ColorTex1.get();
	colorAttachment.loadOp = Renderer::Backend::AttachmentLoadOp::DONT_CARE;
	colorAttachment.storeOp = Renderer::Backend::AttachmentStoreOp::STORE;
	colorAttachment.clearColor = CColor{0.0f, 0.0f, 0.0f, 0.0f};

	Renderer::Backend::SDepthStencilAttachment depthStencilAttachment{};
	depthStencilAttachment.texture = m_DepthTex.get();
	depthStencilAttachment.loadOp = Renderer::Backend::AttachmentLoadOp::CLEAR;
	depthStencilAttachment.storeOp = Renderer::Backend::AttachmentStoreOp::STORE;

	m_CaptureFramebuffer = m_Device->CreateFramebuffer("PostprocCaptureFramebuffer",
		&colorAttachment, &depthStencilAttachment);

	colorAttachment.texture = m_ColorTex1.get();
	colorAttachment.loadOp = Renderer::Backend::AttachmentLoadOp::LOAD;
	colorAttachment.storeOp = Renderer::Backend::AttachmentStoreOp::STORE;
	m_PingFramebuffer = m_Device->CreateFramebuffer("PostprocPingFramebuffer",
		&colorAttachment, nullptr);

	colorAttachment.texture = m_ColorTex2.get();
	m_PongFramebuffer = m_Device->CreateFramebuffer("PostprocPongFramebuffer",
		&colorAttachment, nullptr);

	if (!m_CaptureFramebuffer || !m_PingFramebuffer || !m_PongFramebuffer)
	{
		LOGWARNING("Failed to create postproc framebuffers");
		g_RenderingOptions.SetPostProc(false);
	}

	if (m_UsingMultisampleBuffer)
	{
		DestroyMultisampleBuffer();
		CreateMultisampleBuffer();
	}
}


void CPostprocManager::ApplyBlurDownscale2x(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	Renderer::Backend::IFramebuffer* framebuffer,
	Renderer::Backend::ITexture* inTex, int inWidth, int inHeight)
{
	deviceCommandContext->BeginFramebufferPass(framebuffer);

	Renderer::Backend::IDeviceCommandContext::Rect viewportRect{};
	viewportRect.width = inWidth / 2;
	viewportRect.height = inHeight / 2;
	deviceCommandContext->SetViewports(1, &viewportRect);

	// Get bloom shader with instructions to simply copy texels.
	CShaderDefines defines;
	defines.Add(str_BLOOM_NOP, str_1);
	CShaderTechniquePtr tech = g_Renderer.GetShaderManager().LoadEffect(str_bloom, defines);

	deviceCommandContext->SetGraphicsPipelineState(
		tech->GetGraphicsPipelineState());
	deviceCommandContext->BeginPass();
	Renderer::Backend::IShaderProgram* shader = tech->GetShader();

	deviceCommandContext->SetTexture(
		shader->GetBindingSlot(str_renderedTex), inTex);

	DrawFullscreenQuad(m_VertexInputLayout, deviceCommandContext);

	deviceCommandContext->EndPass();
	deviceCommandContext->EndFramebufferPass();
}

void CPostprocManager::ApplyBlurGauss(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	Renderer::Backend::ITexture* inTex,
	Renderer::Backend::ITexture* tempTex,
	Renderer::Backend::IFramebuffer* tempFramebuffer,
	Renderer::Backend::IFramebuffer* outFramebuffer,
	int inWidth, int inHeight)
{
	deviceCommandContext->BeginFramebufferPass(tempFramebuffer);

	Renderer::Backend::IDeviceCommandContext::Rect viewportRect{};
	viewportRect.width = inWidth;
	viewportRect.height = inHeight;
	deviceCommandContext->SetViewports(1, &viewportRect);

	// Get bloom shader, for a horizontal Gaussian blur pass.
	CShaderDefines defines2;
	defines2.Add(str_BLOOM_PASS_H, str_1);
	CShaderTechniquePtr tech = g_Renderer.GetShaderManager().LoadEffect(str_bloom, defines2);

	deviceCommandContext->SetGraphicsPipelineState(
		tech->GetGraphicsPipelineState());
	deviceCommandContext->BeginPass();
	Renderer::Backend::IShaderProgram* shader = tech->GetShader();
	deviceCommandContext->SetTexture(
		shader->GetBindingSlot(str_renderedTex), inTex);
	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_texSize), inWidth, inHeight);

	DrawFullscreenQuad(m_VertexInputLayout, deviceCommandContext);

	deviceCommandContext->EndPass();
	deviceCommandContext->EndFramebufferPass();

	deviceCommandContext->BeginFramebufferPass(outFramebuffer);

	deviceCommandContext->SetViewports(1, &viewportRect);

	// Get bloom shader, for a vertical Gaussian blur pass.
	CShaderDefines defines3;
	defines3.Add(str_BLOOM_PASS_V, str_1);
	tech = g_Renderer.GetShaderManager().LoadEffect(str_bloom, defines3);
	deviceCommandContext->SetGraphicsPipelineState(
		tech->GetGraphicsPipelineState());

	deviceCommandContext->BeginPass();
	shader = tech->GetShader();

	// Our input texture to the shader is the output of the horizontal pass.
	deviceCommandContext->SetTexture(
		shader->GetBindingSlot(str_renderedTex), tempTex);
	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_texSize), inWidth, inHeight);

	DrawFullscreenQuad(m_VertexInputLayout, deviceCommandContext);

	deviceCommandContext->EndPass();
	deviceCommandContext->EndFramebufferPass();
}

void CPostprocManager::ApplyBlur(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext)
{
	uint32_t width = m_Width, height = m_Height;
	Renderer::Backend::ITexture* previousTexture =
		(m_WhichBuffer ? m_ColorTex1 : m_ColorTex2).get();

	for (BlurScale& scale : m_BlurScales)
	{
		ApplyBlurDownscale2x(deviceCommandContext, scale.steps[0].framebuffer.get(), previousTexture, width, height);
		width /= 2;
		height /= 2;
		ApplyBlurGauss(deviceCommandContext, scale.steps[0].texture.get(),
			scale.steps[1].texture.get(), scale.steps[1].framebuffer.get(),
			scale.steps[0].framebuffer.get(), width, height);
	}
}

Renderer::Backend::IFramebuffer* CPostprocManager::PrepareAndGetOutputFramebuffer()
{
	ENSURE(m_IsInitialized);

	// Leaves m_PingFramebuffer selected for rendering; m_WhichBuffer stays true at this point.

	m_WhichBuffer = true;

	return m_UsingMultisampleBuffer ? m_MultisampleFramebuffer.get() : m_CaptureFramebuffer.get();
}

void CPostprocManager::UpscaleTextureByCompute(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	CShaderTechnique* shaderTechnique,
	Renderer::Backend::ITexture* source,
	Renderer::Backend::ITexture* destination)
{
	Renderer::Backend::IShaderProgram* shaderProgram = shaderTechnique->GetShader();

	const std::array<float, 4> screenSize{{
		static_cast<float>(m_Width), static_cast<float>(m_Height),
		static_cast<float>(m_UnscaledWidth), static_cast<float>(m_UnscaledHeight)}};

	constexpr uint32_t threadGroupWorkRegionDim = 16;
	const uint32_t dispatchGroupCountX = DivideRoundUp(m_UnscaledWidth, threadGroupWorkRegionDim);
	const uint32_t dispatchGroupCountY = DivideRoundUp(m_UnscaledHeight, threadGroupWorkRegionDim);

	deviceCommandContext->BeginComputePass();
	deviceCommandContext->SetComputePipelineState(
		shaderTechnique->GetComputePipelineState());
	deviceCommandContext->SetUniform(shaderProgram->GetBindingSlot(str_screenSize), screenSize);
	deviceCommandContext->SetTexture(shaderProgram->GetBindingSlot(str_inTex), source);
	deviceCommandContext->SetStorageTexture(shaderProgram->GetBindingSlot(str_outTex), destination);
	deviceCommandContext->Dispatch(dispatchGroupCountX, dispatchGroupCountY, 1);
	deviceCommandContext->EndComputePass();
}

void CPostprocManager::UpscaleTextureByFullscreenQuad(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	CShaderTechnique* shaderTechnique,
	Renderer::Backend::ITexture* source,
	Renderer::Backend::IFramebuffer* destination)
{
	Renderer::Backend::IShaderProgram* shaderProgram = shaderTechnique->GetShader();

	const std::array<float, 4> screenSize{{
		static_cast<float>(m_Width), static_cast<float>(m_Height),
		static_cast<float>(m_UnscaledWidth), static_cast<float>(m_UnscaledHeight)}};

	deviceCommandContext->BeginFramebufferPass(destination);

	Renderer::Backend::IDeviceCommandContext::Rect viewportRect{};
	viewportRect.width = destination->GetWidth();
	viewportRect.height = destination->GetHeight();
	deviceCommandContext->SetViewports(1, &viewportRect);

	deviceCommandContext->SetGraphicsPipelineState(
		shaderTechnique->GetGraphicsPipelineState());
	deviceCommandContext->BeginPass();

	deviceCommandContext->SetTexture(
		shaderProgram->GetBindingSlot(str_inTex), source);
	deviceCommandContext->SetUniform(shaderProgram->GetBindingSlot(str_screenSize), screenSize);

	DrawFullscreenQuad(m_VertexInputLayout, deviceCommandContext);

	deviceCommandContext->EndPass();
	deviceCommandContext->EndFramebufferPass();
}

void CPostprocManager::ApplySharpnessAfterScale(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	CShaderTechnique* shaderTechnique,
	Renderer::Backend::ITexture* source,
	Renderer::Backend::ITexture* destination)
{
	Renderer::Backend::IShaderProgram* shaderProgram = shaderTechnique->GetShader();

	// Recommended sharpness for RCAS.
	constexpr float sharpness = 0.2f;

	const std::array<float, 4> screenSize{ {
		static_cast<float>(m_Width), static_cast<float>(m_Height),
		static_cast<float>(m_UnscaledWidth), static_cast<float>(m_UnscaledHeight)} };

	constexpr uint32_t threadGroupWorkRegionDim = 16;
	const uint32_t dispatchGroupCountX = DivideRoundUp(m_UnscaledWidth, threadGroupWorkRegionDim);
	const uint32_t dispatchGroupCountY = DivideRoundUp(m_UnscaledHeight, threadGroupWorkRegionDim);

	deviceCommandContext->BeginComputePass();
	deviceCommandContext->SetComputePipelineState(
		shaderTechnique->GetComputePipelineState());
	deviceCommandContext->SetUniform(shaderProgram->GetBindingSlot(str_sharpness), sharpness);
	deviceCommandContext->SetUniform(shaderProgram->GetBindingSlot(str_screenSize), screenSize);
	deviceCommandContext->SetTexture(shaderProgram->GetBindingSlot(str_inTex), source);
	deviceCommandContext->SetStorageTexture(
		shaderProgram->GetBindingSlot(str_outTex), destination);
	deviceCommandContext->Dispatch(dispatchGroupCountX, dispatchGroupCountY, 1);
	deviceCommandContext->EndComputePass();
}

void CPostprocManager::DownscaleTextureByCompute(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	CShaderTechnique* shaderTechnique,
	Renderer::Backend::ITexture* source,
	Renderer::Backend::ITexture* destination)
{
	Renderer::Backend::IShaderProgram* shaderProgram = shaderTechnique->GetShader();

	const std::array<float, 4> screenSize{{
		static_cast<float>(m_Width), static_cast<float>(m_Height),
		static_cast<float>(m_UnscaledWidth), static_cast<float>(m_UnscaledHeight)}};

	constexpr uint32_t threadGroupWorkRegionDim = 8;
	const uint32_t dispatchGroupCountX = DivideRoundUp(m_UnscaledWidth, threadGroupWorkRegionDim);
	const uint32_t dispatchGroupCountY = DivideRoundUp(m_UnscaledHeight, threadGroupWorkRegionDim);

	deviceCommandContext->BeginComputePass();
	deviceCommandContext->SetComputePipelineState(
		shaderTechnique->GetComputePipelineState());
	deviceCommandContext->SetUniform(shaderProgram->GetBindingSlot(str_screenSize), screenSize);
	deviceCommandContext->SetTexture(shaderProgram->GetBindingSlot(str_inTex), source);
	deviceCommandContext->SetStorageTexture(shaderProgram->GetBindingSlot(str_outTex), destination);
	deviceCommandContext->Dispatch(dispatchGroupCountX, dispatchGroupCountY, 1);
	deviceCommandContext->EndComputePass();
}

void CPostprocManager::BlitOutputFramebuffer(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	Renderer::Backend::IFramebuffer* destination)
{
	ENSURE(m_IsInitialized);

	GPU_SCOPED_LABEL(deviceCommandContext, "Copy postproc to backbuffer");

	Renderer::Backend::ITexture* previousTexture =
		(m_WhichBuffer ? m_ColorTex1 : m_ColorTex2).get();

	if (ShouldUpscale())
	{
		if (m_UpscaleComputeTech)
		{
			Renderer::Backend::ITexture* unscaledTexture = m_RCASComputeTech ? m_UnscaledTexture1.get() : m_UnscaledTexture2.get();
			UpscaleTextureByCompute(deviceCommandContext, m_UpscaleComputeTech.get(), previousTexture, unscaledTexture);
			if (m_RCASComputeTech)
				ApplySharpnessAfterScale(deviceCommandContext, m_RCASComputeTech.get(), m_UnscaledTexture1.get(), m_UnscaledTexture2.get());

			Renderer::Backend::IDeviceCommandContext::Rect sourceRegion{}, destinationRegion{};
			sourceRegion.width = m_UnscaledTexture2->GetWidth();
			sourceRegion.height = m_UnscaledTexture2->GetHeight();
			destinationRegion.width = destination->GetWidth();
			destinationRegion.height = destination->GetHeight();
			deviceCommandContext->BlitFramebuffer(
				m_UnscaledFramebuffer2.get(), destination, sourceRegion, destinationRegion,
				Renderer::Backend::Sampler::Filter::NEAREST);
		}
		else
		{
			UpscaleTextureByFullscreenQuad(deviceCommandContext, m_UpscaleTech.get(), previousTexture, destination);
		}
	}
	else if (ShouldDownscale())
	{
		Renderer::Backend::IDeviceCommandContext::Rect sourceRegion{};
		Renderer::Backend::Sampler::Filter samplerFilter{
			Renderer::Backend::Sampler::Filter::NEAREST};
		Renderer::Backend::IFramebuffer* source{nullptr};

		if (m_DownscaleComputeTech)
		{
			DownscaleTextureByCompute(deviceCommandContext, m_DownscaleComputeTech.get(), previousTexture, m_UnscaledTexture1.get());

			source = m_UnscaledFramebuffer1.get();
			sourceRegion.width = m_UnscaledTexture1->GetWidth();
			sourceRegion.height = m_UnscaledTexture1->GetHeight();
		}
		else
		{
			source = (m_WhichBuffer ? m_PingFramebuffer : m_PongFramebuffer).get();
			sourceRegion.width = source->GetWidth();
			sourceRegion.height = source->GetHeight();
			samplerFilter = Renderer::Backend::Sampler::Filter::LINEAR;
		}

		Renderer::Backend::IDeviceCommandContext::Rect destinationRegion{};
		destinationRegion.width = destination->GetWidth();
		destinationRegion.height = destination->GetHeight();
		deviceCommandContext->BlitFramebuffer(
			source, destination, sourceRegion, destinationRegion, samplerFilter);
	}
	else
	{
		Renderer::Backend::IFramebuffer* source =
			(m_WhichBuffer ? m_PingFramebuffer : m_PongFramebuffer).get();

		// We blit to the backbuffer from the previous active buffer.
		Renderer::Backend::IDeviceCommandContext::Rect region{};
		region.width = std::min(source->GetWidth(), destination->GetWidth());
		region.height = std::min(source->GetHeight(), destination->GetHeight());
		deviceCommandContext->BlitFramebuffer(
			source, destination, region, region,
			Renderer::Backend::Sampler::Filter::NEAREST);
	}
}

void CPostprocManager::ApplyEffect(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	const CShaderTechniquePtr& shaderTech, int pass)
{
	// Select the other framebuffer for rendering.
	Renderer::Backend::IFramebuffer* framebuffer =
		(m_WhichBuffer ? m_PongFramebuffer : m_PingFramebuffer).get();
	deviceCommandContext->BeginFramebufferPass(framebuffer);

	Renderer::Backend::IDeviceCommandContext::Rect viewportRect{};
	viewportRect.width = framebuffer->GetWidth();
	viewportRect.height = framebuffer->GetHeight();
	deviceCommandContext->SetViewports(1, &viewportRect);

	deviceCommandContext->SetGraphicsPipelineState(
		shaderTech->GetGraphicsPipelineState(pass));
	deviceCommandContext->BeginPass();
	Renderer::Backend::IShaderProgram* shader = shaderTech->GetShader(pass);

	// Use the textures from the current framebuffer as input to the shader.
	// We also bind a bunch of other textures and parameters, but since
	// this only happens once per frame the overhead is negligible.
	deviceCommandContext->SetTexture(
		shader->GetBindingSlot(str_renderedTex),
		m_WhichBuffer ? m_ColorTex1.get() : m_ColorTex2.get());
	deviceCommandContext->SetTexture(
		shader->GetBindingSlot(str_depthTex), m_DepthTex.get());

	deviceCommandContext->SetTexture(
		shader->GetBindingSlot(str_blurTex2), m_BlurScales[0].steps[0].texture.get());
	deviceCommandContext->SetTexture(
		shader->GetBindingSlot(str_blurTex4), m_BlurScales[1].steps[0].texture.get());
	deviceCommandContext->SetTexture(
		shader->GetBindingSlot(str_blurTex8), m_BlurScales[2].steps[0].texture.get());

	deviceCommandContext->SetUniform(shader->GetBindingSlot(str_width), m_Width);
	deviceCommandContext->SetUniform(shader->GetBindingSlot(str_height), m_Height);
	deviceCommandContext->SetUniform(shader->GetBindingSlot(str_zNear), m_NearPlane);
	deviceCommandContext->SetUniform(shader->GetBindingSlot(str_zFar), m_FarPlane);

	deviceCommandContext->SetUniform(shader->GetBindingSlot(str_sharpness), m_Sharpness);

	deviceCommandContext->SetUniform(shader->GetBindingSlot(str_brightness), g_LightEnv.m_Brightness);
	deviceCommandContext->SetUniform(shader->GetBindingSlot(str_hdr), g_LightEnv.m_Contrast);
	deviceCommandContext->SetUniform(shader->GetBindingSlot(str_saturation), g_LightEnv.m_Saturation);
	deviceCommandContext->SetUniform(shader->GetBindingSlot(str_bloom), g_LightEnv.m_Bloom);

	DrawFullscreenQuad(m_VertexInputLayout, deviceCommandContext);

	deviceCommandContext->EndPass();
	deviceCommandContext->EndFramebufferPass();

	m_WhichBuffer = !m_WhichBuffer;
}

void CPostprocManager::ApplyPostproc(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext)
{
	ENSURE(m_IsInitialized);

	// Don't do anything if we are using the default effect and no AA.
	const bool hasEffects = m_PostProcEffect != L"default";
	const bool hasARB = m_Device->GetBackend() == Renderer::Backend::Backend::GL_ARB;
	const bool hasAA = m_AATech && !hasARB;
	const bool hasSharp = m_SharpTech && !hasARB;
	if (!hasEffects && !hasAA && !hasSharp)
		return;

	GPU_SCOPED_LABEL(deviceCommandContext, "Render postproc");

	if (hasEffects)
	{
		// First render blur textures. Note that this only happens ONLY ONCE, before any effects are applied!
		// (This may need to change depending on future usage, however that will have a fps hit)
		ApplyBlur(deviceCommandContext);
		for (int pass = 0; pass < m_PostProcTech->GetNumPasses(); ++pass)
			ApplyEffect(deviceCommandContext, m_PostProcTech, pass);
	}

	if (hasAA)
	{
		for (int pass = 0; pass < m_AATech->GetNumPasses(); ++pass)
			ApplyEffect(deviceCommandContext, m_AATech, pass);
	}

	if (hasSharp && !ShouldUpscale())
	{
		for (int pass = 0; pass < m_SharpTech->GetNumPasses(); ++pass)
			ApplyEffect(deviceCommandContext, m_SharpTech, pass);
	}
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
	if (m_Device->GetBackend() == Renderer::Backend::Backend::GL_ARB || !m_IsInitialized)
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
	constexpr std::string_view msaaPrefix{"msaa"};
	if (m_AAName == str_fxaa.string())
	{
		m_AATech = g_Renderer.GetShaderManager().LoadEffect(str_fxaa);
	}
	else if (m_AAName.size() > msaaPrefix.size() &&
		std::string_view{m_AAName}.substr(0, msaaPrefix.size()) == msaaPrefix)
	{
		// We don't want to enable MSAA in Atlas, because it uses wxWidgets and its canvas.
		if (g_AtlasGameLoop && g_AtlasGameLoop->running)
			return;
		if (!m_Device->GetCapabilities().multisampling || m_AllowedSampleCounts.empty())
		{
			LOGWARNING("MSAA is unsupported.");
			return;
		}
		std::stringstream ss(m_AAName.substr(msaaPrefix.size()));
		ss >> m_MultisampleCount;
		if (std::find(std::begin(m_AllowedSampleCounts), std::end(m_AllowedSampleCounts), m_MultisampleCount) ==
		        std::end(m_AllowedSampleCounts))
		{
			m_MultisampleCount = std::min(4u, m_Device->GetCapabilities().maxSampleCount);
			LOGWARNING("Wrong MSAA sample count: %s.", m_AAName.EscapeToPrintableASCII().c_str());
		}
		m_UsingMultisampleBuffer = true;
		CreateMultisampleBuffer();
	}
}

void CPostprocManager::UpdateSharpeningTechnique()
{
	if (m_Device->GetBackend() == Renderer::Backend::Backend::GL_ARB || !m_IsInitialized)
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

void CPostprocManager::SetUpscaleTechnique(const CStr& upscaleName)
{
	m_UpscaleTech.reset();
	m_UpscaleComputeTech.reset();
	m_RCASComputeTech.reset();
	if (m_Device->GetCapabilities().computeShaders && upscaleName == "fsr")
	{
		m_UpscaleComputeTech = g_Renderer.GetShaderManager().LoadEffect(str_compute_upscale_fsr);
		m_RCASComputeTech = g_Renderer.GetShaderManager().LoadEffect(str_compute_rcas);
	}
	else if (upscaleName == "pixelated")
	{
		m_UpscaleTech = g_Renderer.GetShaderManager().LoadEffect(str_upscale_nearest);
	}
	else
	{
		m_UpscaleTech = g_Renderer.GetShaderManager().LoadEffect(str_upscale_bilinear);
	}
}

void CPostprocManager::SetDepthBufferClipPlanes(float nearPlane, float farPlane)
{
	m_NearPlane = nearPlane;
	m_FarPlane = farPlane;
}

void CPostprocManager::CreateMultisampleBuffer()
{
	m_MultisampleColorTex = m_Device->CreateTexture("PostProcColorMS",
		Renderer::Backend::ITexture::Type::TEXTURE_2D_MULTISAMPLE,
		Renderer::Backend::ITexture::Usage::COLOR_ATTACHMENT |
			Renderer::Backend::ITexture::Usage::TRANSFER_SRC,
		Renderer::Backend::Format::R8G8B8A8_UNORM, m_Width, m_Height,
		Renderer::Backend::Sampler::MakeDefaultSampler(
			Renderer::Backend::Sampler::Filter::LINEAR,
			Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE), 1, m_MultisampleCount);

	// Allocate the Depth/Stencil texture.
	m_MultisampleDepthTex = m_Device->CreateTexture("PostProcDepthMS",
		Renderer::Backend::ITexture::Type::TEXTURE_2D_MULTISAMPLE,
		Renderer::Backend::ITexture::Usage::DEPTH_STENCIL_ATTACHMENT |
			Renderer::Backend::ITexture::Usage::TRANSFER_SRC,
		m_Device->GetPreferredDepthStencilFormat(
			Renderer::Backend::ITexture::Usage::DEPTH_STENCIL_ATTACHMENT |
				Renderer::Backend::ITexture::Usage::TRANSFER_SRC,
			true, true),
		m_Width, m_Height,
		Renderer::Backend::Sampler::MakeDefaultSampler(
			Renderer::Backend::Sampler::Filter::LINEAR,
			Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE), 1, m_MultisampleCount);

	// Set up the framebuffers with some initial textures.
	Renderer::Backend::SColorAttachment colorAttachment{};
	colorAttachment.texture = m_MultisampleColorTex.get();
	colorAttachment.loadOp = Renderer::Backend::AttachmentLoadOp::DONT_CARE;
	colorAttachment.storeOp = Renderer::Backend::AttachmentStoreOp::STORE;
	colorAttachment.clearColor = CColor{0.0f, 0.0f, 0.0f, 0.0f};

	Renderer::Backend::SDepthStencilAttachment depthStencilAttachment{};
	depthStencilAttachment.texture = m_MultisampleDepthTex.get();
	depthStencilAttachment.loadOp = Renderer::Backend::AttachmentLoadOp::CLEAR;
	depthStencilAttachment.storeOp = Renderer::Backend::AttachmentStoreOp::STORE;

	m_MultisampleFramebuffer = m_Device->CreateFramebuffer(
		"PostprocMultisampleFramebuffer", &colorAttachment, &depthStencilAttachment);

	if (!m_MultisampleFramebuffer)
	{
		LOGERROR("Failed to create postproc multisample framebuffer");
		m_UsingMultisampleBuffer = false;
		DestroyMultisampleBuffer();
	}
}

void CPostprocManager::DestroyMultisampleBuffer()
{
	if (m_UsingMultisampleBuffer)
		return;
	m_MultisampleFramebuffer.reset();
	m_MultisampleColorTex.reset();
	m_MultisampleDepthTex.reset();
}

bool CPostprocManager::IsMultisampleEnabled() const
{
	return m_UsingMultisampleBuffer;
}

void CPostprocManager::ResolveMultisampleFramebuffer(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext)
{
	if (!m_UsingMultisampleBuffer)
		return;

	GPU_SCOPED_LABEL(deviceCommandContext, "Resolve postproc multisample");
	deviceCommandContext->ResolveFramebuffer(
		m_MultisampleFramebuffer.get(), m_PingFramebuffer.get());
}

void CPostprocManager::RecalculateSize(const uint32_t width, const uint32_t height)
{
	if (m_Device->GetBackend() == Renderer::Backend::Backend::GL_ARB)
	{
		m_Scale = 1.0f;
		return;
	}
	CFG_GET_VAL("renderer.scale", m_Scale);
	if (m_Scale < 0.25f || m_Scale > 2.0f)
	{
		LOGWARNING("Invalid renderer scale: %0.2f", m_Scale);
		m_Scale = 1.0f;
	}
	m_UnscaledWidth = width;
	m_UnscaledHeight = height;
	m_Width = m_UnscaledWidth * m_Scale;
	m_Height = m_UnscaledHeight * m_Scale;
}

bool CPostprocManager::ShouldUpscale() const
{
	return m_Width < m_UnscaledWidth;
}

bool CPostprocManager::ShouldDownscale() const
{
	return m_Width > m_UnscaledWidth;
}
