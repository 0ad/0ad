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

#include "PipelineState.h"

#include "lib/hash.h"
#include "ps/CLogger.h"
#include "ps/containers/StaticVector.h"
#include "renderer/backend/vulkan/Device.h"
#include "renderer/backend/vulkan/Framebuffer.h"
#include "renderer/backend/vulkan/Mapping.h"
#include "renderer/backend/vulkan/ShaderProgram.h"
#include "renderer/backend/vulkan/Utilities.h"

#include <algorithm>

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

size_t CGraphicsPipelineState::CacheKeyHash::operator()(const CacheKey& cacheKey) const
{
	size_t seed = 0;
	hash_combine(seed, cacheKey.vertexInputLayoutUID);
	hash_combine(seed, cacheKey.framebufferUID);
	return seed;
}

bool CGraphicsPipelineState::CacheKeyEqual::operator()(const CacheKey& lhs, const CacheKey& rhs) const
{
	return
		lhs.vertexInputLayoutUID == rhs.vertexInputLayoutUID &&
		lhs.framebufferUID == rhs.framebufferUID;
}

// static
std::unique_ptr<CGraphicsPipelineState> CGraphicsPipelineState::Create(
	CDevice* device, const SGraphicsPipelineStateDesc& desc)
{
	ENSURE(desc.shaderProgram);
	std::unique_ptr<CGraphicsPipelineState> pipelineState{new CGraphicsPipelineState()};
	pipelineState->m_Device = device;
	pipelineState->m_Desc = desc;
	return pipelineState;
}

CGraphicsPipelineState::~CGraphicsPipelineState()
{
	for (const auto& it : m_PipelineMap)
	{
		if (it.second != VK_NULL_HANDLE)
			m_Device->ScheduleObjectToDestroy(
				VK_OBJECT_TYPE_PIPELINE, it.second, VK_NULL_HANDLE);
	}
}

VkPipeline CGraphicsPipelineState::GetOrCreatePipeline(
	const CVertexInputLayout* vertexInputLayout, CFramebuffer* framebuffer)
{
	CShaderProgram* shaderProgram = m_Desc.shaderProgram->As<CShaderProgram>();

	const CacheKey cacheKey =
	{
		vertexInputLayout->GetUID(), framebuffer->GetUID()
	};
	auto it = m_PipelineMap.find(cacheKey);
	if (it != m_PipelineMap.end())
		return it->second;

	PS::StaticVector<VkVertexInputBindingDescription, 16> attributeBindings;
	PS::StaticVector<VkVertexInputAttributeDescription, 16> attributes;

	const VkPhysicalDeviceLimits& limits = m_Device->GetChoosenPhysicalDevice().properties.limits;
	const uint32_t maxVertexInputAttributes = limits.maxVertexInputAttributes;
	const uint32_t maxVertexInputAttributeOffset = limits.maxVertexInputAttributeOffset;
	for (const SVertexAttributeFormat& vertexAttributeFormat : vertexInputLayout->GetAttributes())
	{
		ENSURE(vertexAttributeFormat.bindingSlot < maxVertexInputAttributes);
		ENSURE(vertexAttributeFormat.offset < maxVertexInputAttributeOffset);
		const uint32_t streamLocation = shaderProgram->GetStreamLocation(vertexAttributeFormat.stream);
		if (streamLocation == std::numeric_limits<uint32_t>::max())
			continue;
		auto it = std::find_if(attributeBindings.begin(), attributeBindings.end(),
			[slot = vertexAttributeFormat.bindingSlot](const VkVertexInputBindingDescription& desc) -> bool
		{
			return desc.binding == slot;
		});
		const VkVertexInputBindingDescription desc{
			vertexAttributeFormat.bindingSlot,
			vertexAttributeFormat.stride,
			vertexAttributeFormat.rate == VertexAttributeRate::PER_INSTANCE
				? VK_VERTEX_INPUT_RATE_INSTANCE
				: VK_VERTEX_INPUT_RATE_VERTEX };
		if (it == attributeBindings.end())
			attributeBindings.emplace_back(desc);
		else
		{
			// All attribute sharing the same binding slot should have the same description.
			ENSURE(desc.inputRate == it->inputRate && desc.stride == it->stride);
		}
		attributes.push_back({
			streamLocation,
			vertexAttributeFormat.bindingSlot,
			Mapping::FromFormat(vertexAttributeFormat.format),
			vertexAttributeFormat.offset
			});
	}

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = std::size(attributeBindings);
	vertexInputCreateInfo.pVertexBindingDescriptions = attributeBindings.data();
	vertexInputCreateInfo.vertexAttributeDescriptionCount = std::size(attributes);
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributes.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	// We don't need to specify sizes for viewports and scissors as they're in
	// dynamic state.
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = 0.0f;
	viewport.height = 0.0f;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
	depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCreateInfo.depthTestEnable =
		m_Desc.depthStencilState.depthTestEnabled ? VK_TRUE : VK_FALSE;
	depthStencilStateCreateInfo.depthWriteEnable =
		m_Desc.depthStencilState.depthWriteEnabled ? VK_TRUE : VK_FALSE;
	depthStencilStateCreateInfo.depthCompareOp =
		Mapping::FromCompareOp(m_Desc.depthStencilState.depthCompareOp);
	depthStencilStateCreateInfo.stencilTestEnable =
		m_Desc.depthStencilState.stencilTestEnabled ? VK_TRUE : VK_FALSE;
	// TODO: VkStencilOpState front, back.

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;

	rasterizationStateCreateInfo.polygonMode =
		Mapping::FromPolygonMode(m_Desc.rasterizationState.polygonMode);
	rasterizationStateCreateInfo.cullMode =
		Mapping::FromCullMode(m_Desc.rasterizationState.cullMode);
	rasterizationStateCreateInfo.frontFace =
		m_Desc.rasterizationState.frontFace == FrontFace::CLOCKWISE
		? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;

	rasterizationStateCreateInfo.depthBiasEnable =
		m_Desc.rasterizationState.depthBiasEnabled ? VK_TRUE : VK_FALSE;
	rasterizationStateCreateInfo.depthBiasConstantFactor =
		m_Desc.rasterizationState.depthBiasConstantFactor;
	rasterizationStateCreateInfo.depthBiasSlopeFactor =
		m_Desc.rasterizationState.depthBiasSlopeFactor;
	rasterizationStateCreateInfo.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.rasterizationSamples =
		Mapping::FromSampleCount(framebuffer->GetSampleCount());
	multisampleStateCreateInfo.minSampleShading = 1.0f;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
	colorBlendAttachmentState.blendEnable = m_Desc.blendState.enabled ? VK_TRUE : VK_FALSE;
	colorBlendAttachmentState.colorBlendOp =
		Mapping::FromBlendOp(m_Desc.blendState.colorBlendOp);
	colorBlendAttachmentState.srcColorBlendFactor =
		Mapping::FromBlendFactor(m_Desc.blendState.srcColorBlendFactor);
	colorBlendAttachmentState.dstColorBlendFactor =
		Mapping::FromBlendFactor(m_Desc.blendState.dstColorBlendFactor);
	colorBlendAttachmentState.alphaBlendOp =
		Mapping::FromBlendOp(m_Desc.blendState.alphaBlendOp);
	colorBlendAttachmentState.srcAlphaBlendFactor =
		Mapping::FromBlendFactor(m_Desc.blendState.srcAlphaBlendFactor);
	colorBlendAttachmentState.dstAlphaBlendFactor =
		Mapping::FromBlendFactor(m_Desc.blendState.dstAlphaBlendFactor);
	colorBlendAttachmentState.colorWriteMask =
		Mapping::FromColorWriteMask(m_Desc.blendState.colorWriteMask);

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_CLEAR;
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
	colorBlendStateCreateInfo.blendConstants[0] = m_Desc.blendState.constant.r;
	colorBlendStateCreateInfo.blendConstants[1] = m_Desc.blendState.constant.g;
	colorBlendStateCreateInfo.blendConstants[2] = m_Desc.blendState.constant.b;
	colorBlendStateCreateInfo.blendConstants[3] = m_Desc.blendState.constant.a;

	const VkDynamicState dynamicStates[] =
	{
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_VIEWPORT
	};

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(std::size(dynamicStates));
	dynamicStateCreateInfo.pDynamicStates = dynamicStates;

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	pipelineCreateInfo.stageCount = shaderProgram->GetStages().size();
	pipelineCreateInfo.pStages = shaderProgram->GetStages().data();

	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	// If renderPass is not VK_NULL_HANDLE, the pipeline is being created with
	// fragment shader state, and subpass uses a depth/stencil attachment,
	// pDepthStencilState must be a not null pointer.
	if (framebuffer->GetDepthStencilAttachment())
		pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

	pipelineCreateInfo.layout = shaderProgram->GetPipelineLayout();
	pipelineCreateInfo.renderPass = framebuffer->GetRenderPass();
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = -1;

	VkPipeline pipeline = VK_NULL_HANDLE;
	ENSURE_VK_SUCCESS(vkCreateGraphicsPipelines(
		m_Device->GetVkDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline));

	m_PipelineMap[cacheKey] = pipeline;

	return pipeline;
}

IDevice* CGraphicsPipelineState::GetDevice()
{
	return m_Device;
}

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer
