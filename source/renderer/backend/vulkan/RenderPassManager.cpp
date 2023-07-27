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

#include "precompiled.h"

#include "RenderPassManager.h"

#include "lib/hash.h"
#include "ps/containers/StaticVector.h"
#include "renderer/backend/vulkan/Device.h"
#include "renderer/backend/vulkan/Mapping.h"
#include "renderer/backend/vulkan/Texture.h"
#include "renderer/backend/vulkan/Utilities.h"

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

size_t CRenderPassManager::DescHash::operator()(const Desc& desc) const
{
	size_t seed = 0;
	hash_combine(seed, desc.sampleCount);

	if (desc.colorAttachment.has_value())
	{
		hash_combine(seed, (*desc.colorAttachment).format);
		hash_combine(seed, (*desc.colorAttachment).loadOp);
		hash_combine(seed, (*desc.colorAttachment).storeOp);
	}
	else
		hash_combine(seed, VK_FORMAT_UNDEFINED);

	if (desc.depthStencilAttachment.has_value())
	{
		hash_combine(seed, (*desc.depthStencilAttachment).format);
		hash_combine(seed, (*desc.depthStencilAttachment).loadOp);
		hash_combine(seed, (*desc.depthStencilAttachment).storeOp);
	}
	else
		hash_combine(seed, VK_FORMAT_UNDEFINED);

	return seed;
}

bool CRenderPassManager::DescEqual::operator()(const Desc& lhs, const Desc& rhs) const
{
	auto compareAttachments = [](const std::optional<Attachment>& lhs, const std::optional<Attachment>& rhs) -> bool
	{
		if (lhs.has_value() != rhs.has_value())
			return false;
		if (!lhs.has_value())
			return true;
		return
			(*lhs).format == (*rhs).format &&
			(*lhs).loadOp == (*rhs).loadOp &&
			(*lhs).storeOp == (*rhs).storeOp;
	};
	if (!compareAttachments(lhs.colorAttachment, rhs.colorAttachment))
		return false;
	if (!compareAttachments(lhs.depthStencilAttachment, rhs.depthStencilAttachment))
		return false;
	return lhs.sampleCount == rhs.sampleCount;
}

CRenderPassManager::CRenderPassManager(CDevice* device)
	: m_Device(device)
{
}

CRenderPassManager::~CRenderPassManager()
{
	for (const auto& it : m_RenderPassMap)
		if (it.second != VK_NULL_HANDLE)
		{
			m_Device->ScheduleObjectToDestroy(
				VK_OBJECT_TYPE_RENDER_PASS, it.second, VK_NULL_HANDLE);
		}
	m_RenderPassMap.clear();
}

VkRenderPass CRenderPassManager::GetOrCreateRenderPass(
	SColorAttachment* colorAttachment,
	SDepthStencilAttachment* depthStencilAttachment)
{
	Desc desc{};
	if (colorAttachment)
	{
		CTexture* colorAttachmentTexture = colorAttachment->texture->As<CTexture>();
		desc.sampleCount = colorAttachmentTexture->GetSampleCount();
		desc.colorAttachment.emplace(Attachment{
			colorAttachmentTexture->GetVkFormat(),
			colorAttachment->loadOp,
			colorAttachment->storeOp});
	}
	if (depthStencilAttachment)
	{
		CTexture* depthStencilAttachmentTexture = depthStencilAttachment->texture->As<CTexture>();
		desc.sampleCount = depthStencilAttachmentTexture->GetSampleCount();
		desc.depthStencilAttachment.emplace(Attachment{
			depthStencilAttachmentTexture->GetVkFormat(),
			depthStencilAttachment->loadOp,
			depthStencilAttachment->storeOp});
	}
	auto it = m_RenderPassMap.find(desc);
	if (it != m_RenderPassMap.end())
		return it->second;

	uint32_t attachmentCount = 0;
	PS::StaticVector<VkAttachmentDescription, 4> attachmentDescs;
	std::optional<VkAttachmentReference> colorAttachmentRef;
	std::optional<VkAttachmentReference> depthStencilAttachmentRef;

	if (colorAttachment)
	{
		CTexture* colorAttachmentTexture = colorAttachment->texture->As<CTexture>();

		VkAttachmentDescription colorAttachmentDesc{};
		colorAttachmentDesc.format = colorAttachmentTexture->GetVkFormat();
		colorAttachmentDesc.samples = Mapping::FromSampleCount(colorAttachmentTexture->GetSampleCount());
		colorAttachmentDesc.loadOp = Mapping::FromAttachmentLoadOp(colorAttachment->loadOp);
		colorAttachmentDesc.storeOp = Mapping::FromAttachmentStoreOp(colorAttachment->storeOp);
		colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		attachmentDescs.emplace_back(std::move(colorAttachmentDesc));

		colorAttachmentRef.emplace(VkAttachmentReference{
			attachmentCount++, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
	}

	if (depthStencilAttachment)
	{
		CTexture* depthStencilAttachmentTexture = depthStencilAttachment->texture->As<CTexture>();

		VkAttachmentDescription depthStencilAttachmentDesc{};
		depthStencilAttachmentDesc.format = depthStencilAttachmentTexture->GetVkFormat();
		depthStencilAttachmentDesc.samples = Mapping::FromSampleCount(depthStencilAttachmentTexture->GetSampleCount());
		depthStencilAttachmentDesc.loadOp = Mapping::FromAttachmentLoadOp(depthStencilAttachment->loadOp);
		depthStencilAttachmentDesc.storeOp = Mapping::FromAttachmentStoreOp(depthStencilAttachment->storeOp);
		depthStencilAttachmentDesc.stencilLoadOp = depthStencilAttachmentDesc.loadOp;
		depthStencilAttachmentDesc.stencilStoreOp = depthStencilAttachmentDesc.storeOp;
		depthStencilAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthStencilAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		attachmentDescs.emplace_back(std::move(depthStencilAttachmentDesc));

		depthStencilAttachmentRef.emplace(VkAttachmentReference{
			attachmentCount++, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});
	}

	VkSubpassDescription subpassDesc{};
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	if (colorAttachment)
	{
		subpassDesc.colorAttachmentCount = 1;
		subpassDesc.pColorAttachments = &(*colorAttachmentRef);
	}
	if (depthStencilAttachment)
		subpassDesc.pDepthStencilAttachment = &(*depthStencilAttachmentRef);

	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = attachmentDescs.size();
	renderPassCreateInfo.pAttachments = attachmentDescs.data();
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDesc;

	VkRenderPass renderPass = VK_NULL_HANDLE;
	ENSURE_VK_SUCCESS(
		vkCreateRenderPass(
			m_Device->GetVkDevice(), &renderPassCreateInfo, nullptr, &renderPass));
	it = m_RenderPassMap.emplace(desc, renderPass).first;

	return renderPass;
}

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer
