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

#ifndef INCLUDED_RENDERER_BACKEND_VULKAN_DEVICE
#define INCLUDED_RENDERER_BACKEND_VULKAN_DEVICE

#include "renderer/backend/IDevice.h"
#include "scriptinterface/ScriptForward.h"

#include <memory>

typedef struct SDL_Window SDL_Window;

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

class CDevice : public IDevice
{
public:
	/**
	 * Creates the Vulkan device.
	 */
	static std::unique_ptr<CDevice> Create(SDL_Window* window);

	~CDevice() override;

	const std::string& GetName() const override { return m_Name; }
	const std::string& GetVersion() const override { return m_Version; }
	const std::string& GetDriverInformation() const override { return m_DriverInformation; }
	const std::vector<std::string>& GetExtensions() const override { return m_Extensions; }

	void Report(const ScriptRequest& rq, JS::HandleValue settings) override;

	IFramebuffer* GetCurrentBackbuffer() override;

	std::unique_ptr<IDeviceCommandContext> CreateCommandContext() override;

	std::unique_ptr<ITexture> CreateTexture(const char* name, const ITexture::Type type,
		const Format format, const uint32_t width, const uint32_t height,
		const Sampler::Desc& defaultSamplerDesc, const uint32_t MIPLevelCount, const uint32_t sampleCount) override;

	std::unique_ptr<ITexture> CreateTexture2D(const char* name,
		const Format format, const uint32_t width, const uint32_t height,
		const Sampler::Desc& defaultSamplerDesc, const uint32_t MIPLevelCount = 1, const uint32_t sampleCount = 1) override;

	std::unique_ptr<IFramebuffer> CreateFramebuffer(
		const char* name, ITexture* colorAttachment,
		ITexture* depthStencilAttachment) override;

	std::unique_ptr<IFramebuffer> CreateFramebuffer(
		const char* name, ITexture* colorAttachment,
		ITexture* depthStencilAttachment, const CColor& clearColor) override;

	std::unique_ptr<IBuffer> CreateBuffer(
		const char* name, const IBuffer::Type type, const uint32_t size, const bool dynamic) override;

	std::unique_ptr<IShaderProgram> CreateShaderProgram(
		const CStr& name, const CShaderDefines& defines) override;

	void Present() override;

	bool IsTextureFormatSupported(const Format format) const override;

	bool IsFramebufferFormatSupported(const Format format) const override;

	const Capabilities& GetCapabilities() const override { return m_Capabilities; }

private:
	CDevice();

	std::string m_Name;
	std::string m_Version;
	std::string m_DriverInformation;
	std::vector<std::string> m_Extensions;

	Capabilities m_Capabilities{};
};

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_VULKAN_DEVICE
