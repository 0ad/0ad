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

#include "Device.h"

#include "renderer/backend/dummy/Buffer.h"
#include "renderer/backend/dummy/DeviceCommandContext.h"
#include "renderer/backend/dummy/Framebuffer.h"
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

std::unique_ptr<ITexture> CDevice::CreateTexture(const char* UNUSED(name), const CTexture::Type type,
	const Format format, const uint32_t width, const uint32_t height,
	const Sampler::Desc& UNUSED(defaultSamplerDesc), const uint32_t MIPLevelCount, const uint32_t UNUSED(sampleCount))
{
	return CTexture::Create(this, type, format, width, height, MIPLevelCount);
}

std::unique_ptr<ITexture> CDevice::CreateTexture2D(const char* name,
	const Format format, const uint32_t width, const uint32_t height,
	const Sampler::Desc& defaultSamplerDesc, const uint32_t MIPLevelCount, const uint32_t sampleCount)
{
	return CreateTexture(name, ITexture::Type::TEXTURE_2D,
		format, width, height, defaultSamplerDesc, MIPLevelCount, sampleCount);
}

std::unique_ptr<IFramebuffer> CDevice::CreateFramebuffer(
	const char*, ITexture*, ITexture*)
{
	return CFramebuffer::Create(this);
}

std::unique_ptr<IFramebuffer> CDevice::CreateFramebuffer(
	const char*, ITexture*, ITexture*, const CColor&)
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

void CDevice::Present()
{
	// We have nothing to present.
}

bool CDevice::IsTextureFormatSupported(const Format UNUSED(format)) const
{
	return true;
}

} // namespace Dummy

} // namespace Backend

} // namespace Renderer
