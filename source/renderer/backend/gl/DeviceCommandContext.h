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

#ifndef INCLUDED_RENDERER_BACKEND_GL_DEVICECOMMANDCONTEXT
#define INCLUDED_RENDERER_BACKEND_GL_DEVICECOMMANDCONTEXT

#include "lib/ogl.h"
#include "ps/containers/Span.h"
#include "renderer/backend/Format.h"
#include "renderer/backend/gl/Buffer.h"
#include "renderer/backend/IDeviceCommandContext.h"
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
class CShaderProgram;
class CTexture;

class CDeviceCommandContext final : public IDeviceCommandContext
{
public:
	~CDeviceCommandContext();

	IDevice* GetDevice() override;

	void SetGraphicsPipelineState(const GraphicsPipelineStateDesc& pipelineStateDesc) override;

	void BlitFramebuffer(IFramebuffer* destinationFramebuffer, IFramebuffer* sourceFramebuffer) override;

	void BeginFramebufferPass(IFramebuffer* framebuffer) override;
	void EndFramebufferPass() override;
	void ClearFramebuffer(const bool color, const bool depth, const bool stencil) override;
	void ReadbackFramebufferSync(
		const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height,
		void* data) override;

	void UploadTexture(ITexture* texture, const Format dataFormat,
		const void* data, const size_t dataSize,
		const uint32_t level = 0, const uint32_t layer = 0) override;
	void UploadTextureRegion(ITexture* texture, const Format dataFormat,
		const void* data, const size_t dataSize,
		const uint32_t xOffset, const uint32_t yOffset,
		const uint32_t width, const uint32_t height,
		const uint32_t level = 0, const uint32_t layer = 0) override;

	using UploadBufferFunction = std::function<void(u8*)>;
	void UploadBuffer(IBuffer* buffer, const void* data, const uint32_t dataSize) override;
	void UploadBuffer(IBuffer* buffer, const UploadBufferFunction& uploadFunction) override;
	void UploadBufferRegion(
		IBuffer* buffer, const void* data, const uint32_t dataOffset, const uint32_t dataSize) override;
	void UploadBufferRegion(
		IBuffer* buffer, const uint32_t dataOffset, const uint32_t dataSize,
		const UploadBufferFunction& uploadFunction) override;

	void SetScissors(const uint32_t scissorCount, const Rect* scissors) override;
	void SetViewports(const uint32_t viewportCount, const Rect* viewports) override;

	void SetVertexAttributeFormat(
		const VertexAttributeStream stream,
		const Format format,
		const uint32_t offset,
		const uint32_t stride,
		const VertexAttributeRate rate,
		const uint32_t bindingSlot) override;
	void SetVertexBuffer(
		const uint32_t bindingSlot, IBuffer* buffer, const uint32_t offset) override;
	void SetVertexBufferData(
		const uint32_t bindingSlot, const void* data, const uint32_t dataSize) override;

	void SetIndexBuffer(IBuffer* buffer) override;
	void SetIndexBufferData(const void* data, const uint32_t dataSize) override;

	void BeginPass() override;
	void EndPass() override;

	void Draw(const uint32_t firstVertex, const uint32_t vertexCount) override;
	void DrawIndexed(
		const uint32_t firstIndex, const uint32_t indexCount, const int32_t vertexOffset) override;
	void DrawInstanced(
		const uint32_t firstVertex, const uint32_t vertexCount,
		const uint32_t firstInstance, const uint32_t instanceCount) override;
	void DrawIndexedInstanced(
		const uint32_t firstIndex, const uint32_t indexCount,
		const uint32_t firstInstance, const uint32_t instanceCount,
		const int32_t vertexOffset) override;
	void DrawIndexedInRange(
		const uint32_t firstIndex, const uint32_t indexCount,
		const uint32_t start, const uint32_t end) override;

	void SetTexture(const int32_t bindingSlot, ITexture* texture) override;

	void SetUniform(
		const int32_t bindingSlot,
		const float value) override;
	void SetUniform(
		const int32_t bindingSlot,
		const float valueX, const float valueY) override;
	void SetUniform(
		const int32_t bindingSlot,
		const float valueX, const float valueY,
		const float valueZ) override;
	void SetUniform(
		const int32_t bindingSlot,
		const float valueX, const float valueY,
		const float valueZ, const float valueW) override;
	void SetUniform(
		const int32_t bindingSlot, PS::span<const float> values) override;

	void BeginScopedLabel(const char* name) override;
	void EndScopedLabel() override;

	void Flush() override;

	// We need to know when to invalidate our texture bind cache.
	void OnTextureDestroy(CTexture* texture);

private:
	friend class CDevice;
	friend class CTexture;

	static std::unique_ptr<CDeviceCommandContext> Create(CDevice* device);

	CDeviceCommandContext(CDevice* device);

	void ResetStates();

	void SetGraphicsPipelineStateImpl(
		const GraphicsPipelineStateDesc& pipelineStateDesc, const bool force);

	void BindTexture(const uint32_t unit, const GLenum target, const GLuint handle);
	void BindBuffer(const IBuffer::Type type, CBuffer* buffer);

	CDevice* m_Device = nullptr;

	GraphicsPipelineStateDesc m_GraphicsPipelineStateDesc{};
	CFramebuffer* m_Framebuffer = nullptr;
	CShaderProgram* m_ShaderProgram = nullptr;
	uint32_t m_ScissorCount = 0;
	// GL2.1 doesn't support more than 1 scissor.
	std::array<Rect, 1> m_Scissors;

	uint32_t m_ScopedLabelDepth = 0;

	CBuffer* m_VertexBuffer = nullptr;
	CBuffer* m_IndexBuffer = nullptr;
	const void* m_IndexBufferData = nullptr;

	bool m_InsideFramebufferPass = false;
	bool m_InsidePass = false;

	uint32_t m_ActiveTextureUnit = 0;
	struct BindUnit
	{
		GLenum target;
		GLuint handle;
	};
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
		uint32_t m_ActiveTextureUnit = 0;
	};

	using BoundBuffer = std::pair<GLenum, GLuint>;
	std::array<BoundBuffer, 2> m_BoundBuffers;
	class ScopedBufferBind
	{
	public:
		ScopedBufferBind(
			CDeviceCommandContext* deviceCommandContext, CBuffer* buffer);

		~ScopedBufferBind();
	private:
		CDeviceCommandContext* m_DeviceCommandContext = nullptr;
		size_t m_CacheIndex = 0;
	};

	struct VertexAttributeFormat
	{
		Format format;
		uint32_t offset;
		uint32_t stride;
		VertexAttributeRate rate;
		uint32_t bindingSlot;

		bool active;
		bool initialized;
	};
	std::array<
		VertexAttributeFormat,
		static_cast<size_t>(VertexAttributeStream::UV7) + 1> m_VertexAttributeFormat;
};

} // namespace GL

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_GL_DEVICECOMMANDCONTEXT
