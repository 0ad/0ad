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

#ifndef INCLUDED_RENDERER_BACKEND_IDEVICECOMMANDCONTEXT
#define INCLUDED_RENDERER_BACKEND_IDEVICECOMMANDCONTEXT

#include "ps/containers/Span.h"
#include "renderer/backend/Format.h"
#include "renderer/backend/IDeviceObject.h"
#include "renderer/backend/PipelineState.h"

#include <cstdint>
#include <functional>

namespace Renderer
{

namespace Backend
{

class IBuffer;
class IDevice;
class IFramebuffer;
class ITexture;

class IDeviceCommandContext : public IDeviceObject<IDeviceCommandContext>
{
public:
	virtual void SetGraphicsPipelineState(const GraphicsPipelineStateDesc& pipelineStateDesc) = 0;

	virtual void BlitFramebuffer(
		IFramebuffer* destinationFramebuffer, IFramebuffer* sourceFramebuffer) = 0;

	/**
	 * Starts a framebuffer pass, performs attachment load operations.
	 * It should be called as rarely as possible.
	 *
	 * @see IFramebuffer
	 */
	virtual void BeginFramebufferPass(IFramebuffer* framebuffer) = 0;

	/**
	 * Finishes a framebuffer pass, performs attachment store operations.
	 */
	virtual void EndFramebufferPass() = 0;

	/**
	 * Clears all mentioned attachments. Prefer to use attachment load operations over
	 * this function. It should be called only inside a framebuffer pass.
	 */
	virtual void ClearFramebuffer(const bool color, const bool depth, const bool stencil) = 0;

	virtual void ReadbackFramebufferSync(
		const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height,
		void* data) = 0;

	virtual void UploadTexture(ITexture* texture, const Format dataFormat,
		const void* data, const size_t dataSize,
		const uint32_t level = 0, const uint32_t layer = 0) = 0;
	virtual void UploadTextureRegion(ITexture* texture, const Format dataFormat,
		const void* data, const size_t dataSize,
		const uint32_t xOffset, const uint32_t yOffset,
		const uint32_t width, const uint32_t height,
		const uint32_t level = 0, const uint32_t layer = 0) = 0;

	using UploadBufferFunction = std::function<void(u8*)>;
	virtual void UploadBuffer(IBuffer* buffer, const void* data, const uint32_t dataSize) = 0;
	virtual void UploadBuffer(IBuffer* buffer, const UploadBufferFunction& uploadFunction) = 0;
	virtual void UploadBufferRegion(
		IBuffer* buffer, const void* data, const uint32_t dataOffset, const uint32_t dataSize) = 0;
	virtual void UploadBufferRegion(
		IBuffer* buffer, const uint32_t dataOffset, const uint32_t dataSize,
		const UploadBufferFunction& uploadFunction) = 0;

	// TODO: maybe we should add a more common type, like CRectI.
	struct Rect
	{
		int32_t x, y;
		int32_t width, height;
	};
	virtual void SetScissors(const uint32_t scissorCount, const Rect* scissors) = 0;
	virtual void SetViewports(const uint32_t viewportCount, const Rect* viewports) = 0;

	virtual void SetVertexAttributeFormat(
		const VertexAttributeStream stream,
		const Format format,
		const uint32_t offset,
		const uint32_t stride,
		const VertexAttributeRate rate,
		const uint32_t bindingSlot) = 0;
	virtual void SetVertexBuffer(
		const uint32_t bindingSlot, IBuffer* buffer, const uint32_t offset) = 0;
	virtual void SetVertexBufferData(
		const uint32_t bindingSlot, const void* data, const uint32_t dataSize) = 0;

	virtual void SetIndexBuffer(IBuffer* buffer) = 0;
	virtual void SetIndexBufferData(const void* data, const uint32_t dataSize) = 0;

	virtual void BeginPass() = 0;
	virtual void EndPass() = 0;

	virtual void Draw(const uint32_t firstVertex, const uint32_t vertexCount) = 0;
	virtual void DrawIndexed(
		const uint32_t firstIndex, const uint32_t indexCount, const int32_t vertexOffset) = 0;
	virtual void DrawInstanced(
		const uint32_t firstVertex, const uint32_t vertexCount,
		const uint32_t firstInstance, const uint32_t instanceCount) = 0;
	virtual void DrawIndexedInstanced(
		const uint32_t firstIndex, const uint32_t indexCount,
		const uint32_t firstInstance, const uint32_t instanceCount,
		const int32_t vertexOffset) = 0;
	// TODO: should be removed when performance impact is minimal on slow hardware.
	virtual void DrawIndexedInRange(
		const uint32_t firstIndex, const uint32_t indexCount,
		const uint32_t start, const uint32_t end) = 0;

	virtual void SetTexture(const int32_t bindingSlot, ITexture* texture) = 0;

	virtual void SetUniform(
		const int32_t bindingSlot,
		const float value) = 0;
	virtual void SetUniform(
		const int32_t bindingSlot,
		const float valueX, const float valueY) = 0;
	virtual void SetUniform(
		const int32_t bindingSlot,
		const float valueX, const float valueY,
		const float valueZ) = 0;
	virtual void SetUniform(
		const int32_t bindingSlot,
		const float valueX, const float valueY,
		const float valueZ, const float valueW) = 0;
	virtual void SetUniform(
		const int32_t bindingSlot, PS::span<const float> values) = 0;

	virtual void BeginScopedLabel(const char* name) = 0;
	virtual void EndScopedLabel() = 0;

	virtual void Flush() = 0;
};

} // namespace Backend

} // namespace Renderer

#define GPU_SCOPED_LABEL(deviceCommandContext, name) \
	GPUScopedLabel scopedLabel((deviceCommandContext), (name));

class GPUScopedLabel
{
public:
	GPUScopedLabel(
		Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
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
	Renderer::Backend::IDeviceCommandContext* m_DeviceCommandContext = nullptr;
};

#endif // INCLUDED_RENDERER_BACKEND_IDEVICECOMMANDCONTEXT
