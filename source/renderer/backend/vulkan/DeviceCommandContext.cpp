/* Copyright (C) 2023 Wildfire Games.
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

#include "precompiled.h"

#include "DeviceCommandContext.h"

#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/containers/Span.h"
#include "ps/containers/StaticVector.h"
#include "renderer/backend/vulkan/Buffer.h"
#include "renderer/backend/vulkan/DescriptorManager.h"
#include "renderer/backend/vulkan/Device.h"
#include "renderer/backend/vulkan/Framebuffer.h"
#include "renderer/backend/vulkan/PipelineState.h"
#include "renderer/backend/vulkan/RingCommandContext.h"
#include "renderer/backend/vulkan/ShaderProgram.h"
#include "renderer/backend/vulkan/Texture.h"
#include "renderer/backend/vulkan/Utilities.h"

#include <algorithm>

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

namespace
{

constexpr uint32_t UNIFORM_BUFFER_SIZE = 8 * 1024 * 1024;
constexpr uint32_t FRAME_INPLACE_BUFFER_SIZE = 1024 * 1024;

struct SBaseImageState
{
	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkAccessFlags accessMask = 0;
	VkPipelineStageFlags stageMask = 0;
};

SBaseImageState GetBaseImageState(CTexture* texture)
{
	if (texture->GetUsage() & ITexture::Usage::SAMPLED)
	{
		return {
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT};
	}
	else if (texture->GetUsage() & ITexture::Usage::COLOR_ATTACHMENT)
	{
		return {
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	}
	else if (texture->GetUsage() & ITexture::Usage::DEPTH_STENCIL_ATTACHMENT)
	{
		return {
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT};
	}
	return {};
}

class ScopedImageLayoutTransition
{
public:
	ScopedImageLayoutTransition(
		CRingCommandContext& commandContext, const PS::span<CTexture* const> textures,
		const VkImageLayout layout, const VkAccessFlags accessMask, const VkPipelineStageFlags stageMask)
		: m_CommandContext(commandContext), m_Textures(textures), m_Layout(layout),
		m_AccessMask(accessMask), m_StageMask(stageMask)
	{
		for (CTexture* const texture : m_Textures)
		{
			const auto state = GetBaseImageState(texture);

			VkImageLayout oldLayout = state.layout;
			if (!texture->IsInitialized())
				oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			Utilities::SetTextureLayout(
				m_CommandContext.GetCommandBuffer(), texture,
				oldLayout, m_Layout,
				state.accessMask, m_AccessMask, state.stageMask, m_StageMask);
		}
	}

	~ScopedImageLayoutTransition()
	{
		for (CTexture* const texture : m_Textures)
		{
			const auto state = GetBaseImageState(texture);

			Utilities::SetTextureLayout(
				m_CommandContext.GetCommandBuffer(), texture,
				m_Layout, state.layout,
				m_AccessMask, state.accessMask, m_StageMask, state.stageMask);
		}
	}

private:
	CRingCommandContext& m_CommandContext;
	const PS::span<CTexture* const> m_Textures;
	const VkImageLayout m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;
	const VkAccessFlags m_AccessMask = 0;
	const VkPipelineStageFlags m_StageMask = 0;
};

} // anonymous namespace

// static
std::unique_ptr<IDeviceCommandContext> CDeviceCommandContext::Create(CDevice* device)
{
	std::unique_ptr<CDeviceCommandContext> deviceCommandContext(new CDeviceCommandContext());
	deviceCommandContext->m_Device = device;
	deviceCommandContext->m_DebugScopedLabels = device->GetCapabilities().debugScopedLabels;
	deviceCommandContext->m_PrependCommandContext =
		device->CreateRingCommandContext(NUMBER_OF_FRAMES_IN_FLIGHT);
	deviceCommandContext->m_CommandContext =
		device->CreateRingCommandContext(NUMBER_OF_FRAMES_IN_FLIGHT);

	deviceCommandContext->m_InPlaceVertexBuffer = device->CreateCBuffer(
		"InPlaceVertexBuffer", IBuffer::Type::VERTEX, FRAME_INPLACE_BUFFER_SIZE, true);
	deviceCommandContext->m_InPlaceIndexBuffer = device->CreateCBuffer(
		"InPlaceIndexBuffer", IBuffer::Type::INDEX, FRAME_INPLACE_BUFFER_SIZE, true);

	deviceCommandContext->m_InPlaceVertexStagingBuffer = device->CreateCBuffer(
		"InPlaceVertexStagingBuffer", IBuffer::Type::UPLOAD, NUMBER_OF_FRAMES_IN_FLIGHT * FRAME_INPLACE_BUFFER_SIZE, true);
	deviceCommandContext->m_InPlaceIndexStagingBuffer = device->CreateCBuffer(
		"InPlaceIndexStagingBuffer", IBuffer::Type::UPLOAD, NUMBER_OF_FRAMES_IN_FLIGHT * FRAME_INPLACE_BUFFER_SIZE, true);

	deviceCommandContext->m_UniformBuffer = device->CreateCBuffer(
		"UniformBuffer", IBuffer::Type::UNIFORM, UNIFORM_BUFFER_SIZE, true);
	deviceCommandContext->m_UniformStagingBuffer = device->CreateCBuffer(
		"UniformStagingBuffer", IBuffer::Type::UPLOAD, NUMBER_OF_FRAMES_IN_FLIGHT * UNIFORM_BUFFER_SIZE, true);

	deviceCommandContext->m_InPlaceVertexStagingBufferMappedData =
		deviceCommandContext->m_InPlaceVertexStagingBuffer->GetMappedData();
	ENSURE(deviceCommandContext->m_InPlaceVertexStagingBufferMappedData);
	deviceCommandContext->m_InPlaceIndexStagingBufferMappedData =
		deviceCommandContext->m_InPlaceIndexStagingBuffer->GetMappedData();
	ENSURE(deviceCommandContext->m_InPlaceIndexStagingBufferMappedData);
	deviceCommandContext->m_UniformStagingBufferMappedData =
		deviceCommandContext->m_UniformStagingBuffer->GetMappedData();
	ENSURE(deviceCommandContext->m_UniformStagingBufferMappedData);

	// TODO: reduce the code duplication.
	VkDescriptorPoolSize descriptorPoolSize{};
	descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	descriptorPoolSize.descriptorCount = 1;

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = 1;
	descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;
	descriptorPoolCreateInfo.maxSets = 1;
	ENSURE_VK_SUCCESS(vkCreateDescriptorPool(
		device->GetVkDevice(), &descriptorPoolCreateInfo, nullptr, &deviceCommandContext->m_UniformDescriptorPool));

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = deviceCommandContext->m_UniformDescriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &device->GetDescriptorManager().GetUniformDescriptorSetLayout();

	ENSURE_VK_SUCCESS(vkAllocateDescriptorSets(
		device->GetVkDevice(), &descriptorSetAllocateInfo, &deviceCommandContext->m_UniformDescriptorSet));

	// TODO: fix the hard-coded size.
	const VkDescriptorBufferInfo descriptorBufferInfos[1] =
	{
		{deviceCommandContext->m_UniformBuffer->GetVkBuffer(), 0u, 512u}
	};

	VkWriteDescriptorSet writeDescriptorSet{};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = deviceCommandContext->m_UniformDescriptorSet;
	writeDescriptorSet.dstBinding = 0;
	writeDescriptorSet.dstArrayElement = 0;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.pBufferInfo = descriptorBufferInfos;

	vkUpdateDescriptorSets(
		device->GetVkDevice(), 1, &writeDescriptorSet, 0, nullptr);

	return deviceCommandContext;
}

CDeviceCommandContext::CDeviceCommandContext() = default;

CDeviceCommandContext::~CDeviceCommandContext()
{
	VkDevice device = m_Device->GetVkDevice();

	vkDeviceWaitIdle(device);

	if (m_UniformDescriptorPool != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(device, m_UniformDescriptorPool, nullptr);
}

IDevice* CDeviceCommandContext::GetDevice()
{
	return m_Device;
}

void CDeviceCommandContext::SetGraphicsPipelineState(
	IGraphicsPipelineState* pipelineState)
{
	ENSURE(pipelineState);
	m_GraphicsPipelineState = pipelineState->As<CGraphicsPipelineState>();

	CShaderProgram* shaderProgram = m_GraphicsPipelineState->GetShaderProgram()->As<CShaderProgram>();
	if (m_ShaderProgram != shaderProgram)
	{
		if (m_ShaderProgram)
			m_ShaderProgram->Unbind();
		m_ShaderProgram = shaderProgram;
	}
	m_IsPipelineStateDirty = true;
}

void CDeviceCommandContext::BlitFramebuffer(IFramebuffer* destinationFramebuffer, IFramebuffer* sourceFramebuffer)
{
	ENSURE(!m_InsideFramebufferPass);
	const auto& sourceColorAttachments =
		sourceFramebuffer->As<CFramebuffer>()->GetColorAttachments();
	const auto& destinationColorAttachments =
		destinationFramebuffer->As<CFramebuffer>()->GetColorAttachments();
	ENSURE(sourceColorAttachments.size() == destinationColorAttachments.size());
	// TODO: account depth.
	//ENSURE(
	//	static_cast<bool>(sourceFramebuffer->As<CFramebuffer>()->GetDepthStencilAttachment()) ==
	//		static_cast<bool>(destinationFramebuffer->As<CFramebuffer>()->GetDepthStencilAttachment()));

	for (CTexture* sourceColorAttachment : sourceColorAttachments)
	{
		ENSURE(sourceColorAttachment->GetUsage() & ITexture::Usage::TRANSFER_SRC);
	}
	for (CTexture* destinationColorAttachment : destinationColorAttachments)
	{
		ENSURE(destinationColorAttachment->GetUsage() & ITexture::Usage::TRANSFER_DST);
	}

	// TODO: combine barriers, reduce duplication, add depth.
	ScopedImageLayoutTransition scopedColorAttachmentsTransition{
		*m_CommandContext,
		{sourceColorAttachments.begin(), sourceColorAttachments.end()},
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT};
	ScopedImageLayoutTransition destinationColorAttachmentsTransition{
		*m_CommandContext,
		{destinationColorAttachments.begin(), destinationColorAttachments.end()},
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT};

	// TODO: split BlitFramebuffer into ResolveFramebuffer and BlitFramebuffer.
	if (sourceFramebuffer->As<CFramebuffer>()->GetSampleCount() == 1)
	{
		// TODO: we need to check for VK_FORMAT_FEATURE_BLIT_*_BIT for used formats.
		for (CFramebuffer::ColorAttachments::size_type index = 0; index < destinationColorAttachments.size(); ++index)
		{
			CTexture* sourceColorAttachment = sourceColorAttachments[index];
			CTexture* destinationColorAttachment = destinationColorAttachments[index];

			VkImageBlit region{};
			region.srcOffsets[1].x = sourceColorAttachment->GetWidth();
			region.srcOffsets[1].y = sourceColorAttachment->GetHeight();
			region.srcOffsets[1].z = 1;
			region.dstOffsets[1].x = destinationColorAttachment->GetWidth();
			region.dstOffsets[1].y = destinationColorAttachment->GetHeight();
			region.dstOffsets[1].z = 1;
			region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.srcSubresource.mipLevel = 0;
			region.srcSubresource.baseArrayLayer = 0;
			region.srcSubresource.layerCount = 1;
			region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.dstSubresource.mipLevel = 0;
			region.dstSubresource.baseArrayLayer = 0;
			region.dstSubresource.layerCount = 1;

			ENSURE(sourceColorAttachment->GetImage() != VK_NULL_HANDLE);
			ENSURE(destinationColorAttachment->GetImage() != VK_NULL_HANDLE);
			vkCmdBlitImage(
				m_CommandContext->GetCommandBuffer(),
				sourceColorAttachment->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				destinationColorAttachment->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &region, VK_FILTER_NEAREST);
		}
	}
	else
	{
		ENSURE(sourceFramebuffer->As<CFramebuffer>()->GetSampleCount() > 1);
		ENSURE(destinationFramebuffer->As<CFramebuffer>()->GetSampleCount() == 1);
		ENSURE(sourceFramebuffer->As<CFramebuffer>()->GetWidth() == destinationFramebuffer->As<CFramebuffer>()->GetWidth());
		ENSURE(sourceFramebuffer->As<CFramebuffer>()->GetHeight() == destinationFramebuffer->As<CFramebuffer>()->GetHeight());
		for (CFramebuffer::ColorAttachments::size_type index = 0; index < destinationColorAttachments.size(); ++index)
		{
			CTexture* sourceColorAttachment = sourceColorAttachments[index];
			CTexture* destinationColorAttachment = destinationColorAttachments[index];

			VkImageResolve region{};
			region.extent.width = sourceColorAttachment->GetWidth();
			region.extent.height = sourceColorAttachment->GetHeight();
			region.extent.depth = 1;
			region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.srcSubresource.mipLevel = 0;
			region.srcSubresource.baseArrayLayer = 0;
			region.srcSubresource.layerCount = 1;
			region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.dstSubresource.mipLevel = 0;
			region.dstSubresource.baseArrayLayer = 0;
			region.dstSubresource.layerCount = 1;

			vkCmdResolveImage(
				m_CommandContext->GetCommandBuffer(),
				sourceColorAttachment->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				destinationColorAttachment->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &region);
		}
	}
}

void CDeviceCommandContext::ClearFramebuffer(const bool color, const bool depth, const bool stencil)
{
	ENSURE(m_InsideFramebufferPass);
	ENSURE(m_Framebuffer);
	PS::StaticVector<VkClearAttachment, 4> clearAttachments;
	if (color)
	{
		ENSURE(!m_Framebuffer->GetColorAttachments().empty());
		for (size_t index = 0; index < m_Framebuffer->GetColorAttachments().size(); ++index)
		{
			VkClearAttachment clearAttachment{};
			clearAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			const CColor& clearColor = m_Framebuffer->GetClearColor();
			clearAttachment.clearValue.color.float32[0] = clearColor.r;
			clearAttachment.clearValue.color.float32[1] = clearColor.g;
			clearAttachment.clearValue.color.float32[2] = clearColor.b;
			clearAttachment.clearValue.color.float32[3] = clearColor.a;
			clearAttachment.colorAttachment = index;
			clearAttachments.emplace_back(std::move(clearAttachment));
		}
	}
	if (depth || stencil)
	{
		ENSURE(m_Framebuffer->GetDepthStencilAttachment());
		if (stencil)
		{
			const Format depthStencilFormat =
				m_Framebuffer->GetDepthStencilAttachment()->GetFormat();
			ENSURE(depthStencilFormat == Format::D24_UNORM_S8_UINT ||
				depthStencilFormat == Format::D32_SFLOAT_S8_UINT);
		}
		VkClearAttachment clearAttachment{};
		if (depth)
			clearAttachment.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
		if (stencil)
			clearAttachment.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		clearAttachment.clearValue.depthStencil.depth = 1.0f;
		clearAttachment.clearValue.depthStencil.stencil = 0;
		clearAttachments.emplace_back(std::move(clearAttachment));
	}
	VkClearRect clearRect{};
	clearRect.layerCount = 1;
	clearRect.rect.offset.x = 0;
	clearRect.rect.offset.y = 0;
	clearRect.rect.extent.width = m_Framebuffer->GetWidth();
	clearRect.rect.extent.height = m_Framebuffer->GetHeight();
	vkCmdClearAttachments(
		m_CommandContext->GetCommandBuffer(),
		clearAttachments.size(), clearAttachments.data(),
		1, &clearRect);
}

void CDeviceCommandContext::BeginFramebufferPass(IFramebuffer* framebuffer)
{
	ENSURE(framebuffer);
	m_IsPipelineStateDirty = true;
	m_Framebuffer = framebuffer->As<CFramebuffer>();
	m_GraphicsPipelineState = nullptr;
	m_VertexInputLayout = nullptr;

	SetScissors(0, nullptr);

	for (CTexture* colorAttachment : m_Framebuffer->GetColorAttachments())
	{
		if (!(colorAttachment->GetUsage() & ITexture::Usage::SAMPLED) && colorAttachment->IsInitialized())
			continue;
		VkImageLayout oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		if (!colorAttachment->IsInitialized())
			oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		Utilities::SetTextureLayout(
			m_CommandContext->GetCommandBuffer(), colorAttachment,
			oldLayout,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	}

	CTexture* depthStencilAttachment = m_Framebuffer->GetDepthStencilAttachment();
	if (depthStencilAttachment && ((depthStencilAttachment->GetUsage() & ITexture::Usage::SAMPLED) || !depthStencilAttachment->IsInitialized()))
	{
		VkImageLayout oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		if (!depthStencilAttachment->IsInitialized())
			oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		Utilities::SetTextureLayout(
			m_CommandContext->GetCommandBuffer(), depthStencilAttachment, oldLayout,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
	}

	m_InsideFramebufferPass = true;

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_Framebuffer->GetRenderPass();
	renderPassBeginInfo.framebuffer = m_Framebuffer->GetFramebuffer();
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = { m_Framebuffer->GetWidth(), m_Framebuffer->GetHeight() };

	PS::StaticVector<VkClearValue, 4> clearValues;
	const bool needsClearValues =
		m_Framebuffer->GetColorAttachmentLoadOp() == AttachmentLoadOp::CLEAR ||
		(m_Framebuffer->GetDepthStencilAttachment() &&
			m_Framebuffer->GetDepthStencilAttachmentLoadOp() == AttachmentLoadOp::CLEAR);
	if (needsClearValues)
	{
		for (CTexture* colorAttachment : m_Framebuffer->GetColorAttachments())
		{
			UNUSED2(colorAttachment);
			const CColor& clearColor = m_Framebuffer->GetClearColor();
			// The four array elements of the clear color map to R, G, B, and A
			// components of image formats, in order.
			clearValues.emplace_back();
			clearValues.back().color.float32[0] = clearColor.r;
			clearValues.back().color.float32[1] = clearColor.g;
			clearValues.back().color.float32[2] = clearColor.b;
			clearValues.back().color.float32[3] = clearColor.a;
		}
		if (m_Framebuffer->GetDepthStencilAttachment())
		{
			clearValues.emplace_back();
			clearValues.back().depthStencil.depth = 1.0f;
			clearValues.back().depthStencil.stencil = 0;
		}
		renderPassBeginInfo.clearValueCount = clearValues.size();
		renderPassBeginInfo.pClearValues = clearValues.data();
	}

	vkCmdBeginRenderPass(m_CommandContext->GetCommandBuffer(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void CDeviceCommandContext::EndFramebufferPass()
{
	ENSURE(m_InsideFramebufferPass);
	vkCmdEndRenderPass(m_CommandContext->GetCommandBuffer());

	m_InsideFramebufferPass = false;
	m_BoundIndexBuffer = nullptr;

	ENSURE(m_Framebuffer);
	for (CTexture* colorAttachment : m_Framebuffer->GetColorAttachments())
	{
		if (!(colorAttachment->GetUsage() & ITexture::Usage::SAMPLED))
			continue;
		Utilities::SetTextureLayout(
			m_CommandContext->GetCommandBuffer(), colorAttachment,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	}

	CTexture* depthStencilAttachment = m_Framebuffer->GetDepthStencilAttachment();
	if (depthStencilAttachment && (depthStencilAttachment->GetUsage() & ITexture::Usage::SAMPLED))
	{
		Utilities::SetTextureLayout(
			m_CommandContext->GetCommandBuffer(), depthStencilAttachment,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	}

	m_LastBoundPipeline = VK_NULL_HANDLE;
	if (m_ShaderProgram)
		m_ShaderProgram->Unbind();
	m_ShaderProgram = nullptr;
}

void CDeviceCommandContext::ReadbackFramebufferSync(
	const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height,
	void* data)
{
	UNUSED2(x);
	UNUSED2(y);
	UNUSED2(width);
	UNUSED2(height);
	UNUSED2(data);
	LOGERROR("Vulkan: framebuffer readback is not implemented yet.");
}

void CDeviceCommandContext::UploadTexture(ITexture* texture, const Format dataFormat,
	const void* data, const size_t dataSize,
	const uint32_t level, const uint32_t layer)
{
	(m_InsideFramebufferPass ? m_PrependCommandContext : m_CommandContext)->ScheduleUpload(
		texture->As<CTexture>(), dataFormat, data, dataSize, level, layer);
}

void CDeviceCommandContext::UploadTextureRegion(ITexture* texture, const Format dataFormat,
	const void* data, const size_t dataSize,
	const uint32_t xOffset, const uint32_t yOffset,
	const uint32_t width, const uint32_t height,
	const uint32_t level, const uint32_t layer)
{
	(m_InsideFramebufferPass ? m_PrependCommandContext : m_CommandContext)->ScheduleUpload(
		texture->As<CTexture>(), dataFormat, data, dataSize, xOffset, yOffset, width, height, level, layer);
}

void CDeviceCommandContext::UploadBuffer(IBuffer* buffer, const void* data, const uint32_t dataSize)
{
	ENSURE(!m_InsideFramebufferPass);
	m_CommandContext->ScheduleUpload(
		buffer->As<CBuffer>(), data, 0, dataSize);
}

void CDeviceCommandContext::UploadBuffer(IBuffer* buffer, const UploadBufferFunction& uploadFunction)
{
	ENSURE(!m_InsideFramebufferPass);
	m_CommandContext->ScheduleUpload(
		buffer->As<CBuffer>(), 0, buffer->As<CBuffer>()->GetSize(), uploadFunction);
}

void CDeviceCommandContext::UploadBufferRegion(
	IBuffer* buffer, const void* data, const uint32_t dataOffset, const uint32_t dataSize)
{
	ENSURE(!m_InsideFramebufferPass);
	m_CommandContext->ScheduleUpload(
		buffer->As<CBuffer>(), data, dataOffset, dataSize);
}

void CDeviceCommandContext::UploadBufferRegion(
	IBuffer* buffer, const uint32_t dataOffset, const uint32_t dataSize,
	const UploadBufferFunction& uploadFunction)
{
	m_CommandContext->ScheduleUpload(
		buffer->As<CBuffer>(), dataOffset, dataSize, uploadFunction);
}

void CDeviceCommandContext::SetScissors(const uint32_t scissorCount, const Rect* scissors)
{
	ENSURE(m_Framebuffer);
	ENSURE(scissorCount <= 1);
	VkRect2D scissor{};
	if (scissorCount == 1)
	{
		// the x and y members of offset member of any element of pScissors must be
		// greater than or equal to 0.
		int32_t x = scissors[0].x;
		int32_t y = m_Framebuffer->GetHeight() - scissors[0].y - scissors[0].height;
		int32_t width = scissors[0].width;
		int32_t height = scissors[0].height;
		if (x < 0)
		{
			width = std::max(0, width + x);
			x = 0;
		}
		if (y < 0)
		{
			height = std::max(0, height + y);
			y = 0;
		}
		scissor.offset.x = x;
		scissor.offset.y = y;
		scissor.extent.width = width;
		scissor.extent.height = height;
	}
	else
	{
		scissor.extent.width = m_Framebuffer->GetWidth();
		scissor.extent.height = m_Framebuffer->GetHeight();
	}
	vkCmdSetScissor(m_CommandContext->GetCommandBuffer(), 0, 1, &scissor);
}

void CDeviceCommandContext::SetViewports(const uint32_t viewportCount, const Rect* viewports)
{
	ENSURE(m_Framebuffer);
	ENSURE(viewportCount == 1);

	VkViewport viewport{};
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	viewport.x = static_cast<float>(viewports[0].x);
	viewport.y = static_cast<float>(static_cast<int32_t>(m_Framebuffer->GetHeight()) - viewports[0].y - viewports[0].height);
	viewport.width = static_cast<float>(viewports[0].width);
	viewport.height = static_cast<float>(viewports[0].height);

	vkCmdSetViewport(m_CommandContext->GetCommandBuffer(), 0, 1, &viewport);
}

void CDeviceCommandContext::SetVertexInputLayout(
	IVertexInputLayout* vertexInputLayout)
{
	ENSURE(vertexInputLayout);
	m_IsPipelineStateDirty = true;
	m_VertexInputLayout = vertexInputLayout->As<CVertexInputLayout>();
}

void CDeviceCommandContext::SetVertexBuffer(
	const uint32_t bindingSlot, IBuffer* buffer, const uint32_t offset)
{
	BindVertexBuffer(bindingSlot, buffer->As<CBuffer>(), offset);
}

void CDeviceCommandContext::SetVertexBufferData(
	const uint32_t bindingSlot, const void* data, const uint32_t dataSize)
{
	// TODO: check vertex buffer alignment.
	const uint32_t ALIGNMENT = 32;

	uint32_t destination = m_InPlaceBlockIndex * FRAME_INPLACE_BUFFER_SIZE + m_InPlaceBlockVertexOffset;
	uint32_t destination2 = m_InPlaceBlockVertexOffset;
	// TODO: add overflow checks.
	m_InPlaceBlockVertexOffset = (m_InPlaceBlockVertexOffset + dataSize + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
	std::memcpy(static_cast<uint8_t*>(m_InPlaceVertexStagingBufferMappedData) + destination, data, dataSize);

	BindVertexBuffer(bindingSlot, m_InPlaceVertexBuffer.get(), destination2);
}

void CDeviceCommandContext::SetIndexBuffer(IBuffer* buffer)
{
	BindIndexBuffer(buffer->As<CBuffer>(), 0);
}

void CDeviceCommandContext::SetIndexBufferData(
	const void* data, const uint32_t dataSize)
{
	// TODO: check index buffer alignment.
	const uint32_t ALIGNMENT = 32;

	uint32_t destination = m_InPlaceBlockIndex * FRAME_INPLACE_BUFFER_SIZE + m_InPlaceBlockIndexOffset;
	uint32_t destination2 = m_InPlaceBlockIndexOffset;
	// TODO: add overflow checks.
	m_InPlaceBlockIndexOffset = (m_InPlaceBlockIndexOffset + dataSize + ALIGNMENT - 1) & (~(ALIGNMENT - 1));
	std::memcpy(static_cast<uint8_t*>(m_InPlaceIndexStagingBufferMappedData) + destination, data, dataSize);

	BindIndexBuffer(m_InPlaceIndexBuffer.get(), destination2);
}

void CDeviceCommandContext::BeginPass()
{
	ENSURE(m_InsideFramebufferPass);
	m_InsidePass = true;
}

void CDeviceCommandContext::EndPass()
{
	ENSURE(m_InsidePass);
	m_InsidePass = false;
}

void CDeviceCommandContext::Draw(const uint32_t firstVertex, const uint32_t vertexCount)
{
	PreDraw();
	vkCmdDraw(m_CommandContext->GetCommandBuffer(), vertexCount, 1, firstVertex, 0);
}

void CDeviceCommandContext::DrawIndexed(
	const uint32_t firstIndex, const uint32_t indexCount, const int32_t vertexOffset)
{
	ENSURE(vertexOffset == 0);
	PreDraw();
	vkCmdDrawIndexed(m_CommandContext->GetCommandBuffer(), indexCount, 1, firstIndex, 0, 0);
}

void CDeviceCommandContext::DrawInstanced(
	const uint32_t firstVertex, const uint32_t vertexCount,
	const uint32_t firstInstance, const uint32_t instanceCount)
{
	PreDraw();
	vkCmdDraw(
		m_CommandContext->GetCommandBuffer(), vertexCount, instanceCount, firstVertex, firstInstance);
}

void CDeviceCommandContext::DrawIndexedInstanced(
	const uint32_t firstIndex, const uint32_t indexCount,
	const uint32_t firstInstance, const uint32_t instanceCount,
	const int32_t vertexOffset)
{
	PreDraw();
	vkCmdDrawIndexed(
		m_CommandContext->GetCommandBuffer(), indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CDeviceCommandContext::DrawIndexedInRange(
	const uint32_t firstIndex, const uint32_t indexCount,
	const uint32_t UNUSED(start), const uint32_t UNUSED(end))
{
	DrawIndexed(firstIndex, indexCount, 0);
}

void CDeviceCommandContext::SetTexture(const int32_t bindingSlot, ITexture* texture)
{
	if (bindingSlot < 0)
		return;

	ENSURE(m_InsidePass);
	ENSURE(texture);
	CTexture* textureToBind = texture->As<CTexture>();
	ENSURE(textureToBind->GetUsage() & ITexture::Usage::SAMPLED);

	if (!m_Device->GetDescriptorManager().UseDescriptorIndexing())
	{
		// We can't bind textures which are used as color attachments.
		const auto& colorAttachments = m_Framebuffer->GetColorAttachments();
		ENSURE(std::find(
			colorAttachments.begin(), colorAttachments.end(), textureToBind) == colorAttachments.end());
		ENSURE(m_Framebuffer->GetDepthStencilAttachment() != textureToBind);

		ENSURE(textureToBind->IsInitialized());
	}

	m_ShaderProgram->SetTexture(bindingSlot, textureToBind);
}

void CDeviceCommandContext::SetUniform(
	const int32_t bindingSlot,
	const float value)
{
	ENSURE(m_InsidePass);
	m_ShaderProgram->SetUniform(bindingSlot, value);
}

void CDeviceCommandContext::SetUniform(
	const int32_t bindingSlot,
	const float valueX, const float valueY)
{
	ENSURE(m_InsidePass);
	m_ShaderProgram->SetUniform(bindingSlot, valueX, valueY);
}

void CDeviceCommandContext::SetUniform(
	const int32_t bindingSlot,
	const float valueX, const float valueY,
	const float valueZ)
{
	ENSURE(m_InsidePass);
	m_ShaderProgram->SetUniform(bindingSlot, valueX, valueY, valueZ);
}

void CDeviceCommandContext::SetUniform(
	const int32_t bindingSlot,
	const float valueX, const float valueY,
	const float valueZ, const float valueW)
{
	ENSURE(m_InsidePass);
	m_ShaderProgram->SetUniform(bindingSlot, valueX, valueY, valueZ, valueW);
}

void CDeviceCommandContext::SetUniform(
	const int32_t bindingSlot, PS::span<const float> values)
{
	ENSURE(m_InsidePass);
	m_ShaderProgram->SetUniform(bindingSlot, values);
}

void CDeviceCommandContext::BeginScopedLabel(const char* name)
{
	if (!m_DebugScopedLabels)
		return;
	VkDebugUtilsLabelEXT label{};
	label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	label.pLabelName = name;
	vkCmdBeginDebugUtilsLabelEXT(m_CommandContext->GetCommandBuffer(), &label);
}

void CDeviceCommandContext::EndScopedLabel()
{
	if (!m_DebugScopedLabels)
		return;
	vkCmdEndDebugUtilsLabelEXT(m_CommandContext->GetCommandBuffer());
}

void CDeviceCommandContext::Flush()
{
	ENSURE(!m_InsideFramebufferPass);
	// TODO: remove hard-coded values and reduce duplication.
	// TODO: fix unsafe copying when overlaping flushes/frames.

	if (m_InPlaceBlockVertexOffset > 0)
	{
		VkBufferCopy region{};
		region.srcOffset = m_InPlaceBlockIndex * FRAME_INPLACE_BUFFER_SIZE;
		region.dstOffset = 0;
		region.size = m_InPlaceBlockVertexOffset;

		vkCmdCopyBuffer(
			m_PrependCommandContext->GetCommandBuffer(),
			m_InPlaceVertexStagingBuffer->GetVkBuffer(),
			m_InPlaceVertexBuffer->GetVkBuffer(), 1, &region);
	}

	if (m_InPlaceBlockIndexOffset > 0)
	{
		VkBufferCopy region{};
		region.srcOffset = m_InPlaceBlockIndex * FRAME_INPLACE_BUFFER_SIZE;
		region.dstOffset = 0;
		region.size = m_InPlaceBlockIndexOffset;
		vkCmdCopyBuffer(
			m_PrependCommandContext->GetCommandBuffer(),
			m_InPlaceIndexStagingBuffer->GetVkBuffer(),
			m_InPlaceIndexBuffer->GetVkBuffer(), 1, &region);
	}

	if (m_InPlaceBlockVertexOffset > 0 || m_InPlaceBlockIndexOffset > 0)
	{
		VkMemoryBarrier memoryBarrier{};
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

		vkCmdPipelineBarrier(
			m_PrependCommandContext->GetCommandBuffer(),
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
			1, &memoryBarrier, 0, nullptr, 0, nullptr);
	}

	if (m_UniformOffset > 0)
	{
		VkBufferCopy region{};
		// TODO: fix values
		region.srcOffset = (m_UniformStagingBuffer->GetSize() / NUMBER_OF_FRAMES_IN_FLIGHT) * m_UniformIndexOffset;
		region.dstOffset = 0;
		region.size = m_UniformOffset;
		vkCmdCopyBuffer(
			m_PrependCommandContext->GetCommandBuffer(),
			m_UniformStagingBuffer->GetVkBuffer(),
			m_UniformBuffer->GetVkBuffer(), 1, &region);
		m_UniformIndexOffset = (m_UniformIndexOffset + 1) % NUMBER_OF_FRAMES_IN_FLIGHT;
		m_UniformOffset = 0;
	}

	m_IsPipelineStateDirty = true;
	// TODO: maybe move management to CDevice.
	m_InPlaceBlockIndex = (m_InPlaceBlockIndex + 1) % NUMBER_OF_FRAMES_IN_FLIGHT;
	m_InPlaceBlockVertexOffset = 0;
	m_InPlaceBlockIndexOffset = 0;

	m_PrependCommandContext->Flush();
	m_CommandContext->Flush();
}

void CDeviceCommandContext::PreDraw()
{
	ENSURE(m_InsidePass);
	ApplyPipelineStateIfDirty();
	m_ShaderProgram->PreDraw(m_CommandContext->GetCommandBuffer());
	if (m_ShaderProgram->IsMaterialConstantsDataOutdated())
	{
		const VkDeviceSize alignment =
			std::max(static_cast<VkDeviceSize>(16), m_Device->GetChoosenPhysicalDevice().properties.limits.minUniformBufferOffsetAlignment);
		const uint32_t offset = m_UniformOffset + m_UniformIndexOffset * (m_UniformStagingBuffer->GetSize() / NUMBER_OF_FRAMES_IN_FLIGHT);
		std::memcpy(static_cast<uint8_t*>(m_UniformStagingBufferMappedData) + offset,
			m_ShaderProgram->GetMaterialConstantsData(),
			m_ShaderProgram->GetMaterialConstantsDataSize());
		m_ShaderProgram->UpdateMaterialConstantsData();

		// TODO: maybe move inside shader program to reduce the # of bind calls.
		vkCmdBindDescriptorSets(
			m_CommandContext->GetCommandBuffer(), m_ShaderProgram->GetPipelineBindPoint(),
			m_ShaderProgram->GetPipelineLayout(), m_Device->GetDescriptorManager().GetUniformSet(),
			1, &m_UniformDescriptorSet, 1, &m_UniformOffset);

		m_UniformOffset += (m_ShaderProgram->GetMaterialConstantsDataSize() + alignment - 1) & ~(alignment - 1);
	}
}

void CDeviceCommandContext::ApplyPipelineStateIfDirty()
{
	if (!m_IsPipelineStateDirty)
		return;
	m_IsPipelineStateDirty = false;

	ENSURE(m_GraphicsPipelineState);
	ENSURE(m_VertexInputLayout);
	ENSURE(m_Framebuffer);

	VkPipeline pipeline = m_GraphicsPipelineState->GetOrCreatePipeline(
		m_VertexInputLayout, m_Framebuffer);
	ENSURE(pipeline != VK_NULL_HANDLE);

	if (m_LastBoundPipeline != pipeline)
	{
		m_LastBoundPipeline = pipeline;
		vkCmdBindPipeline(m_CommandContext->GetCommandBuffer(), m_ShaderProgram->GetPipelineBindPoint(), pipeline);

		m_ShaderProgram->Bind();

		if (m_Device->GetDescriptorManager().UseDescriptorIndexing())
		{
			vkCmdBindDescriptorSets(
				m_CommandContext->GetCommandBuffer(), m_ShaderProgram->GetPipelineBindPoint(),
				m_ShaderProgram->GetPipelineLayout(), 0,
				1, &m_Device->GetDescriptorManager().GetDescriptorIndexingSet(), 0, nullptr);
		}
	}
}

void CDeviceCommandContext::BindVertexBuffer(
	const uint32_t bindingSlot, CBuffer* buffer, uint32_t offset)
{
	VkBuffer vertexBuffers[] = { buffer->GetVkBuffer() };
	VkDeviceSize offsets[] = { offset };
	vkCmdBindVertexBuffers(
		m_CommandContext->GetCommandBuffer(), bindingSlot, std::size(vertexBuffers), vertexBuffers, offsets);
}

void CDeviceCommandContext::BindIndexBuffer(CBuffer* buffer, uint32_t offset)
{
	if (buffer == m_BoundIndexBuffer && offset == m_BoundIndexBufferOffset)
		return;
	m_BoundIndexBuffer = buffer;
	m_BoundIndexBufferOffset = offset;
	vkCmdBindIndexBuffer(
		m_CommandContext->GetCommandBuffer(), buffer->GetVkBuffer(), offset, VK_INDEX_TYPE_UINT16);
}

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer
