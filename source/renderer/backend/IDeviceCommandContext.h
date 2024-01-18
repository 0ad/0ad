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

#ifndef INCLUDED_RENDERER_BACKEND_IDEVICECOMMANDCONTEXT
#define INCLUDED_RENDERER_BACKEND_IDEVICECOMMANDCONTEXT

#include "ps/containers/Span.h"
#include "renderer/backend/Format.h"
#include "renderer/backend/IDeviceObject.h"
#include "renderer/backend/PipelineState.h"
#include "renderer/backend/Sampler.h"

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
	/**
	 * Binds the graphics pipeline state. It should be called only inside a
	 * framebuffer pass and as rarely as possible.
	 */
	virtual void SetGraphicsPipelineState(IGraphicsPipelineState* pipelineState) = 0;

	/**
	 * Binds the graphics pipeline state. It should be called only inside a
	 * framebuffer pass and as rarely as possible.
	 */
	virtual void SetComputePipelineState(IComputePipelineState* pipelineState) = 0;

	// TODO: maybe we should add a more common type, like CRectI.
	struct Rect
	{
		int32_t x, y;
		int32_t width, height;
	};
	/**
	 * Copies source region into destination region automatically applying
	 * compatible format conversion and scaling using a provided filter.
	 * A backbuffer can't be a source.
	 */
	virtual void BlitFramebuffer(
		IFramebuffer* sourceFramebuffer, IFramebuffer* destinationFramebuffer,
		const Rect& sourceRegion, const Rect& destinationRegion,
		const Sampler::Filter filter) = 0;

	/**
	 * Resolves multisample source framebuffer attachments to destination
	 * attachments. Source attachments should have a sample count > 1 and
	 * destination attachments should have a sample count = 1.
	 * A backbuffer can't be a source.
	 */
	virtual void ResolveFramebuffer(
		IFramebuffer* sourceFramebuffer, IFramebuffer* destinationFramebuffer) = 0;

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

	/**
	 * Readbacks the current backbuffer to data in R8G8B8_UNORM format somewhen
	 * between the function call and Flush (inclusively). Because of that the
	 * data pointer should be valid in that time period and have enough space
	 * to fit the readback result.
	 * @note this operation is very slow and should not be used regularly.
	 * TODO: ideally we should do readback on Present or even asynchronously
	 * but a client doesn't support that yet.
	 */
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

	virtual void SetScissors(const uint32_t scissorCount, const Rect* scissors) = 0;
	virtual void SetViewports(const uint32_t viewportCount, const Rect* viewports) = 0;

	/**
	 * Binds the vertex input layout. It should be compatible with the shader
	 * program's one. It should be called only inside a framebuffer pass and as
	 * rarely as possible.
	 */
	virtual void SetVertexInputLayout(
		IVertexInputLayout* vertexInputLayout) = 0;

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

	/**
	 * Starts a compute pass, can't be called inside a framebuffer pass.
	 * It should be called as rarely as possible.
	 */
	virtual void BeginComputePass() = 0;

	/**
	 * Finishes a compute pass.
	 */
	virtual void EndComputePass() = 0;

	/**
	 * Dispatches groupCountX * groupCountY * groupCountZ compute groups.
	 */
	virtual void Dispatch(
		const uint32_t groupCountX,
		const uint32_t groupCountY,
		const uint32_t groupCountZ) = 0;

	/**
	 * Sets a read-only texture to the binding slot.
	 */
	virtual void SetTexture(const int32_t bindingSlot, ITexture* texture) = 0;

	/**
	 * Sets a read & write resource to the binding slot.
	 */
	virtual void SetStorageTexture(const int32_t bindingSlot, ITexture* texture) = 0;

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
