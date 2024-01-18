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

#ifndef INCLUDED_RENDERER_BACKEND_DUMMY_DEVICECOMMANDCONTEXT
#define INCLUDED_RENDERER_BACKEND_DUMMY_DEVICECOMMANDCONTEXT

#include "renderer/backend/Format.h"
#include "renderer/backend/IDeviceCommandContext.h"
#include "renderer/backend/PipelineState.h"

#include <memory>

namespace Renderer
{

namespace Backend
{

namespace Dummy
{

class CDevice;
class CBuffer;
class CFramebuffer;
class CShaderProgram;
class CTexture;

class CDeviceCommandContext : public IDeviceCommandContext
{
public:
	~CDeviceCommandContext();

	IDevice* GetDevice() override;

	void SetGraphicsPipelineState(IGraphicsPipelineState* pipelineState) override;
	void SetComputePipelineState(IComputePipelineState* pipelineState) override;

	void BlitFramebuffer(
		IFramebuffer* sourceFramebuffer, IFramebuffer* destinationFramebuffer,
		const Rect& sourceRegion, const Rect& destinationRegion,
		const Sampler::Filter filter) override;
	void ResolveFramebuffer(
		IFramebuffer* sourceFramebuffer, IFramebuffer* destinationFramebuffer) override;

	void ClearFramebuffer(const bool color, const bool depth, const bool stencil) override;
	void BeginFramebufferPass(IFramebuffer* framebuffer) override;
	void EndFramebufferPass() override;
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

	void SetVertexInputLayout(
		IVertexInputLayout* vertexInputLayout) override;

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

	void BeginComputePass() override;
	void EndComputePass() override;

	void Dispatch(
		const uint32_t groupCountX,
		const uint32_t groupCountY,
		const uint32_t groupCountZ) override;

	void SetTexture(const int32_t bindingSlot, ITexture* texture) override;

	void SetStorageTexture(const int32_t bindingSlot, ITexture* texture) override;

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

private:
	friend class CDevice;

	static std::unique_ptr<CDeviceCommandContext> Create(CDevice* device);

	CDeviceCommandContext();

	CDevice* m_Device = nullptr;
};

} // namespace Dummy

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_DUMMY_DEVICECOMMANDCONTEXT
