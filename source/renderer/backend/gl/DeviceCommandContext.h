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

#ifndef INCLUDED_RENDERER_GL_DEVICECOMMANDCONTEXT
#define INCLUDED_RENDERER_GL_DEVICECOMMANDCONTEXT

#include "lib/ogl.h"
#include "renderer/backend/Format.h"
#include "renderer/backend/gl/Buffer.h"
#include "renderer/backend/PipelineState.h"

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

namespace Renderer
{

namespace Backend
{

namespace GL
{

class CDevice;
class CFramebuffer;
class CTexture;

class CDeviceCommandContext
{
public:
	~CDeviceCommandContext();

	CDevice* GetDevice() { return m_Device; }

	void SetGraphicsPipelineState(const GraphicsPipelineStateDesc& pipelineStateDesc);

	void BlitFramebuffer(CFramebuffer* destinationFramebuffer, CFramebuffer* sourceFramebuffer);

	void ClearFramebuffer();
	void ClearFramebuffer(const bool color, const bool depth, const bool stencil);
	void SetFramebuffer(CFramebuffer* framebuffer);
	void ReadbackFramebufferSync(
		const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height,
		void* data);

	void UploadTexture(CTexture* texture, const Format dataFormat,
		const void* data, const size_t dataSize,
		const uint32_t level = 0, const uint32_t layer = 0);
	void UploadTextureRegion(CTexture* texture, const Format dataFormat,
		const void* data, const size_t dataSize,
		const uint32_t xOffset, const uint32_t yOffset,
		const uint32_t width, const uint32_t height,
		const uint32_t level = 0, const uint32_t layer = 0);

	using UploadBufferFunction = std::function<void(u8*)>;
	void UploadBuffer(CBuffer* buffer, const void* data, const uint32_t dataSize);
	void UploadBuffer(CBuffer* buffer, const UploadBufferFunction& uploadFunction);
	void UploadBufferRegion(
		CBuffer* buffer, const void* data, const uint32_t dataOffset, const uint32_t dataSize);
	void UploadBufferRegion(
		CBuffer* buffer, const uint32_t dataOffset, const uint32_t dataSize,
		const UploadBufferFunction& uploadFunction);

	// TODO: maybe we should add a more common type, like CRectI.
	struct Rect
	{
		int32_t x, y;
		int32_t width, height;
	};
	void SetScissors(const uint32_t scissorCount, const Rect* scissors);
	void SetViewports(const uint32_t viewportCount, const Rect* viewports);

	void SetIndexBuffer(CBuffer* buffer);
	void SetIndexBufferData(const void* data);

	void BeginPass();
	void EndPass();

	void Draw(const uint32_t firstVertex, const uint32_t vertexCount);
	void DrawIndexed(
		const uint32_t firstIndex, const uint32_t indexCount, const int32_t vertexOffset);
	// TODO: should be removed when performance impact is minimal on slow hardware.
	void DrawIndexedInRange(
		const uint32_t firstIndex, const uint32_t indexCount,
		const uint32_t start, const uint32_t end);

	void BeginScopedLabel(const char* name);
	void EndScopedLabel();

	// TODO: remove direct binding after moving shaders.
	void BindTexture(const uint32_t unit, const GLenum target, const GLuint handle);
	void BindBuffer(const CBuffer::Type type, CBuffer* buffer);
	// We need to know when to invalidate our texture bind cache.
	void OnTextureDestroy(CTexture* texture);

	void Flush();

private:
	friend class CDevice;

	static std::unique_ptr<CDeviceCommandContext> Create(CDevice* device);

	CDeviceCommandContext(CDevice* device);

	void ResetStates();

	void SetGraphicsPipelineStateImpl(
		const GraphicsPipelineStateDesc& pipelineStateDesc, const bool force);

	CDevice* m_Device = nullptr;

	GraphicsPipelineStateDesc m_GraphicsPipelineStateDesc{};
	CFramebuffer* m_Framebuffer = nullptr;
	uint32_t m_ScissorCount = 0;
	// GL2.1 doesn't support more than 1 scissor.
	std::array<Rect, 1> m_Scissors;

	uint32_t m_ScopedLabelDepth = 0;

	CBuffer* m_IndexBuffer = nullptr;
	const void* m_IndexBufferData = nullptr;

	bool m_InsidePass = false;

	uint32_t m_ActiveTextureUnit = 0;
	using BindUnit = std::pair<GLenum, GLuint>;
	std::array<BindUnit, 16> m_BoundTextures;
	class ScopedBind
	{
	public:
		ScopedBind(CDeviceCommandContext* deviceCommandContext,
			const GLenum target, const GLuint handle);

		~ScopedBind();
	private:
		CDeviceCommandContext* m_DeviceCommandContext = nullptr;
		BindUnit m_OldBindUnit;
	};
};

} // namespace GL

} // namespace Backend

} // namespace Renderer

#define GPU_SCOPED_LABEL(deviceCommandContext, name) \
	GPUScopedLabel scopedLabel((deviceCommandContext), (name));

class GPUScopedLabel
{
public:
	GPUScopedLabel(
		Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
		const char* name)
		: m_DeviceCommandContext(deviceCommandContext)
	{
		m_DeviceCommandContext->BeginScopedLabel(name);
	}

	~GPUScopedLabel()
	{
		m_DeviceCommandContext->EndScopedLabel();
	}

private:
	Renderer::Backend::GL::CDeviceCommandContext* m_DeviceCommandContext = nullptr;
};

#endif // INCLUDED_RENDERER_GL_DEVICECOMMANDCONTEXT
