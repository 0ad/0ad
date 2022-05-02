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
#include "renderer/backend/gl/Framebuffer.h"
#include "renderer/backend/gl/ShaderProgram.h"
#include "renderer/backend/gl/Texture.h"
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

class CDevice
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
	};

	~CDevice();

	/**
	 * Creates the GL device and the GL context for the window if it presents.
	 */
	static std::unique_ptr<CDevice> Create(SDL_Window* window, const bool arb);

	const std::string& GetName() const { return m_Name; }
	const std::string& GetVersion() const { return m_Version; }
	const std::string& GetDriverInformation() const { return m_DriverInformation; }
	const std::vector<std::string>& GetExtensions() const { return m_Extensions; }

	void Report(const ScriptRequest& rq, JS::HandleValue settings);

	CFramebuffer* GetCurrentBackbuffer() { return m_Backbuffer.get(); }

	std::unique_ptr<CDeviceCommandContext> CreateCommandContext();

	CDeviceCommandContext* GetActiveCommandContext() { return m_ActiveCommandContext; }

	std::unique_ptr<CTexture> CreateTexture(const char* name, const CTexture::Type type,
		const Format format, const uint32_t width, const uint32_t height,
		const Sampler::Desc& defaultSamplerDesc, const uint32_t MIPLevelCount, const uint32_t sampleCount);

	std::unique_ptr<CTexture> CreateTexture2D(const char* name,
		const Format format, const uint32_t width, const uint32_t height,
		const Sampler::Desc& defaultSamplerDesc, const uint32_t MIPLevelCount = 1, const uint32_t sampleCount = 1);

	std::unique_ptr<CFramebuffer> CreateFramebuffer(
		const char* name, CTexture* colorAttachment,
		CTexture* depthStencilAttachment);

	std::unique_ptr<CFramebuffer> CreateFramebuffer(
		const char* name, CTexture* colorAttachment,
		CTexture* depthStencilAttachment, const CColor& clearColor);

	std::unique_ptr<CBuffer> CreateBuffer(
		const char* name, const CBuffer::Type type, const uint32_t size, const bool dynamic);

	std::unique_ptr<IShaderProgram> CreateShaderProgram(
		const CStr& name, const CShaderDefines& defines);

	void Present();

	bool IsTextureFormatSupported(const Format format) const;

	const Capabilities& GetCapabilities() const { return m_Capabilities; }

private:
	CDevice();

	SDL_Window* m_Window = nullptr;
	SDL_GLContext m_Context = nullptr;

	std::string m_Name;
	std::string m_Version;
	std::string m_DriverInformation;
	std::vector<std::string> m_Extensions;

	// GL can have the only one command context at once.
	// TODO: remove as soon as we have no GL code outside backend, currently
	// it's used only as a helper for transition.
	CDeviceCommandContext* m_ActiveCommandContext = nullptr;

	std::unique_ptr<CFramebuffer> m_Backbuffer;

	Capabilities m_Capabilities{};
};

} // namespace GL

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_GL_DEVICE
