/* Copyright (C) 2023 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_RENDERER_VULKAN_DEVICECOMMANDCONTEXT
#define INCLUDED_RENDERER_VULKAN_DEVICECOMMANDCONTEXT

#include "ps/containers/StaticVector.h"
#include "renderer/backend/IBuffer.h"
#include "renderer/backend/IDeviceCommandContext.h"

#include <glad/vulkan.h>
#include <memory>
#include <vector>

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

class CBuffer;
class CDevice;
class CFramebuffer;
class CGraphicsPipelineState;
class CRingCommandContext;
class CShaderProgram;
class CVertexInputLayout;

class CDeviceCommandContext final : public IDeviceCommandContext
{
public:
	~CDeviceCommandContext() override;

	IDevice* GetDevice() override;

	void SetGraphicsPipelineState(IGraphicsPipelineState* pipelineState) override;

	void BlitFramebuffer(
		IFramebuffer* destinationFramebuffer, IFramebuffer* sourceFramebuffer,
		const Rect& destinationRegion, const Rect& sourceRegion,
		const Sampler::Filter filter) override;
	void ResolveFramebuffer(
		IFramebuffer* destinationFramebuffer, IFramebuffer* sourceFramebuffer) override;

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

private:
	friend class CDevice;

	static std::unique_ptr<IDeviceCommandContext> Create(CDevice* device);

	CDeviceCommandContext();

	void PreDraw();
	void ApplyPipelineStateIfDirty();
	void BindVertexBuffer(const uint32_t bindingSlot, CBuffer* buffer, uint32_t offset);
	void BindIndexBuffer(CBuffer* buffer, uint32_t offset);

	CDevice* m_Device = nullptr;

	bool m_DebugScopedLabels = false;

	std::unique_ptr<CRingCommandContext> m_PrependCommandContext;
	std::unique_ptr<CRingCommandContext> m_CommandContext;

	CGraphicsPipelineState* m_GraphicsPipelineState = nullptr;
	CVertexInputLayout* m_VertexInputLayout = nullptr;
	CFramebuffer* m_Framebuffer = nullptr;
	CShaderProgram* m_ShaderProgram = nullptr;
	bool m_IsPipelineStateDirty = true;
	VkPipeline m_LastBoundPipeline = VK_NULL_HANDLE;

	bool m_InsideFramebufferPass = false;
	bool m_InsidePass = false;

	// Currently bound buffers to skip the same buffer bind.
	CBuffer* m_BoundIndexBuffer = nullptr;
	uint32_t m_BoundIndexBufferOffset = 0;

	class CUploadRing;
	std::unique_ptr<CUploadRing> m_VertexUploadRing, m_IndexUploadRing, m_UniformUploadRing;

	VkDescriptorPool m_UniformDescriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet m_UniformDescriptorSet = VK_NULL_HANDLE;

	// Currently we support readbacks only from backbuffer.
	struct QueuedReadback
	{
		uint32_t x = 0, y = 0;
		uint32_t width = 0, height = 0;
		// It's a responsibility of the caller to guarantee that data is valid.
		void* data = nullptr;
	};
	PS::StaticVector<QueuedReadback, 2> m_QueuedReadbacks;

	bool m_DebugBarrierAfterFramebufferPass = false;
};

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_VULKAN_DEVICECOMMANDCONTEXT
