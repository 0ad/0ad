/* Copyright (C) 2023 Wildfire Games.
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

#include "Device.h"

#include "renderer/backend/dummy/Buffer.h"
#include "renderer/backend/dummy/DeviceCommandContext.h"
#include "renderer/backend/dummy/Framebuffer.h"
#include "renderer/backend/dummy/PipelineState.h"
#include "renderer/backend/dummy/ShaderProgram.h"
#include "renderer/backend/dummy/Texture.h"
#include "scriptinterface/JSON.h"
#include "scriptinterface/Object.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/ScriptRequest.h"

namespace Renderer
{

namespace Backend
{

namespace Dummy
{

CDevice::CDevice()
{
	m_Name = "Dummy";
	m_Version = "Unknown";
	m_DriverInformation = "Unknown";
	m_Extensions = {};

	m_Backbuffer = CFramebuffer::Create(this);

	m_Capabilities.S3TC = true;
	m_Capabilities.ARBShaders = false;
	m_Capabilities.ARBShadersShadow = false;
	m_Capabilities.computeShaders = true;
	m_Capabilities.debugLabels = true;
	m_Capabilities.debugScopedLabels = true;
	m_Capabilities.multisampling = true;
	m_Capabilities.anisotropicFiltering = true;
	m_Capabilities.maxSampleCount = 4u;
	m_Capabilities.maxAnisotropy = 16.0f;
	m_Capabilities.maxTextureSize = 8192u;
	m_Capabilities.instancing = true;
}

CDevice::~CDevice() = default;

void CDevice::Report(const ScriptRequest& rq, JS::HandleValue settings)
{
	Script::SetProperty(rq, settings, "name", "dummy");
}

std::unique_ptr<IDeviceCommandContext> CDevice::CreateCommandContext()
{
	return CDeviceCommandContext::Create(this);
}

std::unique_ptr<IGraphicsPipelineState> CDevice::CreateGraphicsPipelineState(
	const SGraphicsPipelineStateDesc& pipelineStateDesc)
{
	return CGraphicsPipelineState::Create(this, pipelineStateDesc);
}

std::unique_ptr<IVertexInputLayout> CDevice::CreateVertexInputLayout(
	const PS::span<const SVertexAttributeFormat> UNUSED(attributes))
{
	return nullptr;
}

std::unique_ptr<ITexture> CDevice::CreateTexture(
	const char* UNUSED(name), const CTexture::Type type, const uint32_t usage,
	const Format format, const uint32_t width, const uint32_t height,
	const Sampler::Desc& UNUSED(defaultSamplerDesc), const uint32_t MIPLevelCount, const uint32_t UNUSED(sampleCount))
{
	return CTexture::Create(this, type, usage, format, width, height, MIPLevelCount);
}

std::unique_ptr<ITexture> CDevice::CreateTexture2D(
	const char* name, const uint32_t usage,
	const Format format, const uint32_t width, const uint32_t height,
	const Sampler::Desc& defaultSamplerDesc, const uint32_t MIPLevelCount, const uint32_t sampleCount)
{
	return CreateTexture(name, ITexture::Type::TEXTURE_2D, usage,
		format, width, height, defaultSamplerDesc, MIPLevelCount, sampleCount);
}

std::unique_ptr<IFramebuffer> CDevice::CreateFramebuffer(
	const char*, SColorAttachment*, SDepthStencilAttachment*)
{
	return CFramebuffer::Create(this);
}

std::unique_ptr<IBuffer> CDevice::CreateBuffer(
	const char*, const CBuffer::Type type, const uint32_t size, const bool dynamic)
{
	return CBuffer::Create(this, type, size, dynamic);
}

std::unique_ptr<IShaderProgram> CDevice::CreateShaderProgram(
	const CStr&, const CShaderDefines&)
{
	return CShaderProgram::Create(this);
}

bool CDevice::AcquireNextBackbuffer()
{
	// We have nothing to acquire.
	return true;
}

IFramebuffer* CDevice::GetCurrentBackbuffer(
	const AttachmentLoadOp, const AttachmentStoreOp,
	const AttachmentLoadOp, const AttachmentStoreOp)
{
	return m_Backbuffer.get();
}

void CDevice::Present()
{
	// We have nothing to present.
}

void CDevice::OnWindowResize(const uint32_t UNUSED(width), const uint32_t UNUSED(height))
{
}

bool CDevice::IsTextureFormatSupported(const Format UNUSED(format)) const
{
	return true;
}

bool CDevice::IsFramebufferFormatSupported(const Format UNUSED(format)) const
{
	return true;
}

Format CDevice::GetPreferredDepthStencilFormat(
	const uint32_t, const bool, const bool) const
{
	return Format::D24_UNORM_S8_UINT;
}

std::unique_ptr<IDevice> CreateDevice(SDL_Window* UNUSED(window))
{
	return std::make_unique<Dummy::CDevice>();
}

} // namespace Dummy

} // namespace Backend

} // namespace Renderer
