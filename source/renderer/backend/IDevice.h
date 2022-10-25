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

#ifndef INCLUDED_RENDERER_BACKEND_IDEVICE
#define INCLUDED_RENDERER_BACKEND_IDEVICE

#include "graphics/Color.h"
#include "renderer/backend/Backend.h"
#include "renderer/backend/Format.h"
#include "renderer/backend/IBuffer.h"
#include "renderer/backend/IDevice.h"
#include "renderer/backend/IDeviceCommandContext.h"
#include "renderer/backend/IFramebuffer.h"
#include "renderer/backend/IShaderProgram.h"
#include "renderer/backend/ITexture.h"
#include "scriptinterface/ScriptForward.h"

#include <memory>
#include <string>
#include <vector>

class CShaderDefines;
class CStr;

namespace Renderer
{

namespace Backend
{

class IDevice
{
public:
	struct Capabilities
	{
		bool S3TC;
		bool ARBShaders;
		bool ARBShadersShadow;
		bool computeShaders;
		bool debugLabels;
		bool debugScopedLabels;
		bool multisampling;
		bool anisotropicFiltering;
		uint32_t maxSampleCount;
		float maxAnisotropy;
		uint32_t maxTextureSize;
		bool instancing;
	};

	virtual ~IDevice() {}

	virtual Backend GetBackend() const = 0;

	virtual const std::string& GetName() const = 0;
	virtual const std::string& GetVersion() const = 0;
	virtual const std::string& GetDriverInformation() const = 0;
	virtual const std::vector<std::string>& GetExtensions() const = 0;

	virtual void Report(const ScriptRequest& rq, JS::HandleValue settings) = 0;

	virtual IFramebuffer* GetCurrentBackbuffer() = 0;

	virtual std::unique_ptr<IDeviceCommandContext> CreateCommandContext() = 0;

	virtual std::unique_ptr<ITexture> CreateTexture(
		const char* name, const ITexture::Type type, const uint32_t usage,
		const Format format, const uint32_t width, const uint32_t height,
		const Sampler::Desc& defaultSamplerDesc, const uint32_t MIPLevelCount, const uint32_t sampleCount) = 0;

	virtual std::unique_ptr<ITexture> CreateTexture2D(
		const char* name, const uint32_t usage,
		const Format format, const uint32_t width, const uint32_t height,
		const Sampler::Desc& defaultSamplerDesc, const uint32_t MIPLevelCount = 1, const uint32_t sampleCount = 1) = 0;

	virtual std::unique_ptr<IFramebuffer> CreateFramebuffer(
		const char* name, ITexture* colorAttachment,
		ITexture* depthStencilAttachment) = 0;

	virtual std::unique_ptr<IFramebuffer> CreateFramebuffer(
		const char* name, ITexture* colorAttachment,
		ITexture* depthStencilAttachment, const CColor& clearColor) = 0;

	virtual std::unique_ptr<IBuffer> CreateBuffer(
		const char* name, const IBuffer::Type type, const uint32_t size, const bool dynamic) = 0;

	virtual std::unique_ptr<IShaderProgram> CreateShaderProgram(
		const CStr& name, const CShaderDefines& defines) = 0;

	virtual void AcquireNextBackbuffer() = 0;
	virtual void Present() = 0;

	virtual bool IsTextureFormatSupported(const Format format) const = 0;

	virtual bool IsFramebufferFormatSupported(const Format format) const = 0;

	virtual const Capabilities& GetCapabilities() const = 0;
};

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_IDEVICE
