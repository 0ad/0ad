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

#ifndef INCLUDED_RENDERER_BACKEND_GL_DEVICE
#define INCLUDED_RENDERER_BACKEND_GL_DEVICE

#include "renderer/backend/Format.h"
#include "renderer/backend/gl/Buffer.h"
#include "renderer/backend/gl/DeviceForward.h"
#include "renderer/backend/gl/Framebuffer.h"
#include "renderer/backend/gl/ShaderProgram.h"
#include "renderer/backend/gl/Texture.h"
#include "renderer/backend/IDevice.h"
#include "scriptinterface/ScriptForward.h"

#include <memory>
#include <string>
#include <vector>

typedef struct SDL_Window SDL_Window;
typedef void* SDL_GLContext;

namespace Renderer
{

namespace Backend
{

namespace GL
{

class CDeviceCommandContext;

class CDevice final : public IDevice
{
public:
	~CDevice() override;

	/**
	 * Creates the GL device and the GL context for the window if it presents.
	 */
	static std::unique_ptr<IDevice> Create(SDL_Window* window, const bool arb);

	Backend GetBackend() const override { return m_ARB ? Backend::GL_ARB : Backend::GL; }

	const std::string& GetName() const override { return m_Name; }
	const std::string& GetVersion() const override { return m_Version; }
	const std::string& GetDriverInformation() const override { return m_DriverInformation; }
	const std::vector<std::string>& GetExtensions() const override { return m_Extensions; }

	void Report(const ScriptRequest& rq, JS::HandleValue settings) override;

	IFramebuffer* GetCurrentBackbuffer() override { return m_Backbuffer.get(); }

	std::unique_ptr<IDeviceCommandContext> CreateCommandContext() override;

	CDeviceCommandContext* GetActiveCommandContext() { return m_ActiveCommandContext; }

	std::unique_ptr<ITexture> CreateTexture(
		const char* name, const ITexture::Type type, const uint32_t usage,
		const Format format, const uint32_t width, const uint32_t height,
		const Sampler::Desc& defaultSamplerDesc, const uint32_t MIPLevelCount, const uint32_t sampleCount) override;

	std::unique_ptr<ITexture> CreateTexture2D(
		const char* name, const uint32_t usage,
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

	bool AcquireNextBackbuffer() override;
	void Present() override;

	bool IsTextureFormatSupported(const Format format) const override;

	bool IsFramebufferFormatSupported(const Format format) const override;

	const Capabilities& GetCapabilities() const override { return m_Capabilities; }

private:
	CDevice();

	SDL_Window* m_Window = nullptr;
	SDL_GLContext m_Context = nullptr;

	bool m_ARB = false;

	std::string m_Name;
	std::string m_Version;
	std::string m_DriverInformation;
	std::vector<std::string> m_Extensions;

	// GL can have the only one command context at once.
	// TODO: remove as soon as we have no GL code outside backend, currently
	// it's used only as a helper for transition.
	CDeviceCommandContext* m_ActiveCommandContext = nullptr;

	std::unique_ptr<CFramebuffer> m_Backbuffer;
	bool m_BackbufferAcquired = false;

	Capabilities m_Capabilities{};
};

} // namespace GL

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_GL_DEVICE
