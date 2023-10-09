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

#include "Texture.h"

#include "renderer/backend/vulkan/Device.h"
#include "renderer/backend/vulkan/Mapping.h"
#include "renderer/backend/vulkan/SamplerManager.h"
#include "renderer/backend/vulkan/Utilities.h"

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

// static
std::unique_ptr<CTexture> CTexture::Create(
	CDevice* device, const char* name, const Type type, const uint32_t usage,
	const Format format, const uint32_t width, const uint32_t height,
	const Sampler::Desc& defaultSamplerDesc,
	const uint32_t MIPLevelCount, const uint32_t sampleCount)
{
	std::unique_ptr<CTexture> texture(new CTexture(device));

	texture->m_Format = format;
	texture->m_Type = type;
	texture->m_Usage = usage;
	texture->m_Width = width;
	texture->m_Height = height;
	texture->m_MIPLevelCount = MIPLevelCount;
	texture->m_SampleCount = sampleCount;
	texture->m_LayerCount = type == ITexture::Type::TEXTURE_CUBE ? 6 : 1;

	if (type == Type::TEXTURE_2D_MULTISAMPLE)
		ENSURE(sampleCount > 1);

	VkFormat imageFormat = VK_FORMAT_UNDEFINED;
	// A8 and L8 are special cases for GL2.1, because it doesn't have a proper
	// channel swizzling.
	if (format == Format::A8_UNORM || format == Format::L8_UNORM)
		imageFormat = VK_FORMAT_R8_UNORM;
	else
		imageFormat = Mapping::FromFormat(format);
	texture->m_VkFormat = imageFormat;

	VkImageType imageType = VK_IMAGE_TYPE_2D;
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;

	const VkPhysicalDevice physicalDevice =
		device->GetChoosenPhysicalDevice().device;

	VkFormatProperties formatProperties{};
	vkGetPhysicalDeviceFormatProperties(
		physicalDevice, imageFormat, &formatProperties);

	VkImageUsageFlags usageFlags = 0;
	// Vulkan 1.0 implies that TRANSFER_SRC and TRANSFER_DST are supported.
	// TODO: account Vulkan 1.1.
	if (usage & Usage::TRANSFER_SRC)
		usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	if (usage & Usage::TRANSFER_DST)
		usageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	if (usage & Usage::SAMPLED)
	{
		ENSURE(type != Type::TEXTURE_2D_MULTISAMPLE);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
		{
			LOGERROR("Format %d doesn't support sampling for optimal tiling.", static_cast<int>(imageFormat));
			return nullptr;
		}
		usageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	}
	if (usage & Usage::COLOR_ATTACHMENT)
	{
		ENSURE(device->IsFramebufferFormatSupported(format));
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT))
		{
			LOGERROR("Format %d doesn't support color attachment for optimal tiling.", static_cast<int>(imageFormat));
			return nullptr;
		}
		usageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	if (usage & Usage::DEPTH_STENCIL_ATTACHMENT)
	{
		ENSURE(IsDepthFormat(format));
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
		{
			LOGERROR("Format %d doesn't support depth stencil attachment for optimal tiling.", static_cast<int>(imageFormat));
			return nullptr;
		}
		usageFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}

	if (IsDepthFormat(format))
	{
		texture->m_AttachmentImageAspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		texture->m_SamplerImageAspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (format == Format::D24_UNORM_S8_UINT || format == Format::D32_SFLOAT_S8_UINT)
			texture->m_AttachmentImageAspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
	{
		texture->m_AttachmentImageAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		texture->m_SamplerImageAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = imageType;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = MIPLevelCount;
	imageCreateInfo.arrayLayers = type == Type::TEXTURE_CUBE ? 6 : 1;
	imageCreateInfo.format = imageFormat;
	imageCreateInfo.samples = Mapping::FromSampleCount(sampleCount);
	imageCreateInfo.tiling = tiling;
	imageCreateInfo.usage = usageFlags;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (type == Type::TEXTURE_CUBE)
		imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	VmaAllocationCreateInfo allocationCreateInfo{};
	if ((usage & Usage::COLOR_ATTACHMENT) || (usage & Usage::DEPTH_STENCIL_ATTACHMENT))
		allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
#ifndef NDEBUG
	allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
	allocationCreateInfo.pUserData = const_cast<char*>(name);
#endif
	allocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	const VkResult createImageResult = vmaCreateImage(
		device->GetVMAAllocator(), &imageCreateInfo, &allocationCreateInfo,
		&texture->m_Image, &texture->m_Allocation, nullptr);
	if (createImageResult != VK_SUCCESS)
	{
		LOGERROR("Failed to create VkImage: %d (%s)",
			static_cast<int>(createImageResult), Utilities::GetVkResultName(createImageResult));
		return nullptr;
	}

	VkImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = texture->m_Image;
	imageViewCreateInfo.viewType = type == Type::TEXTURE_CUBE ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = imageFormat;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = MIPLevelCount;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = type == Type::TEXTURE_CUBE ? 6 : 1;
	if (format == Format::A8_UNORM)
	{
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_ZERO;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_ZERO;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_ZERO;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_R;
	}
	else if (format == Format::L8_UNORM)
	{
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_R;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_R;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_ONE;
	}
	else
	{
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	}

	if ((usage & Usage::COLOR_ATTACHMENT) || (usage & Usage::DEPTH_STENCIL_ATTACHMENT))
	{
		imageViewCreateInfo.subresourceRange.aspectMask = texture->m_AttachmentImageAspectMask;
		ENSURE_VK_SUCCESS(vkCreateImageView(
			device->GetVkDevice(), &imageViewCreateInfo, nullptr, &texture->m_AttachmentImageView));
	}

	if (usage & Usage::SAMPLED)
	{
		imageViewCreateInfo.subresourceRange.aspectMask = texture->m_SamplerImageAspectMask;
		ENSURE_VK_SUCCESS(vkCreateImageView(
			device->GetVkDevice(), &imageViewCreateInfo, nullptr, &texture->m_SamplerImageView));

		texture->m_Sampler = device->GetSamplerManager().GetOrCreateSampler(
			defaultSamplerDesc);
		texture->m_IsCompareEnabled = defaultSamplerDesc.compareEnabled;
	}

	device->SetObjectName(VK_OBJECT_TYPE_IMAGE, texture->m_Image, name);
	if (texture->m_AttachmentImageView != VK_NULL_HANDLE)
		device->SetObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, texture->m_AttachmentImageView, name);
	if (texture->m_SamplerImageView != VK_NULL_HANDLE)
		device->SetObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, texture->m_SamplerImageView, name);

	return texture;
}

// static
std::unique_ptr<CTexture> CTexture::WrapBackbufferImage(
	CDevice* device, const char* name, const VkImage image, const VkFormat format,
	const VkImageUsageFlags usage, const uint32_t width, const uint32_t height)
{
	std::unique_ptr<CTexture> texture(new CTexture(device));

	if (format == VK_FORMAT_R8G8B8A8_UNORM)
		texture->m_Format = Format::R8G8B8A8_UNORM;
	else if (format == VK_FORMAT_B8G8R8A8_UNORM)
		texture->m_Format = Format::B8G8R8A8_UNORM;
	else
		texture->m_Format = Format::UNDEFINED;
	texture->m_Type = Type::TEXTURE_2D;
	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		texture->m_Usage |= Usage::COLOR_ATTACHMENT;
	if (usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
		texture->m_Usage |= Usage::TRANSFER_SRC;
	if (usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		texture->m_Usage |= Usage::TRANSFER_DST;
	texture->m_Width = width;
	texture->m_Height = height;
	texture->m_MIPLevelCount = 1;
	texture->m_SampleCount = 1;
	texture->m_LayerCount = 1;
	texture->m_VkFormat = format;
	// The image is owned by its swapchain, but we don't set a special flag
	// because the ownership is detected by m_Allocation presence.
	texture->m_Image = image;
	texture->m_AttachmentImageAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	texture->m_SamplerImageAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	VkImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = format;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;
	ENSURE_VK_SUCCESS(vkCreateImageView(
		device->GetVkDevice(), &imageViewCreateInfo, nullptr, &texture->m_AttachmentImageView));
	device->SetObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, texture->m_AttachmentImageView, name);

	return texture;
}

// static
std::unique_ptr<CTexture> CTexture::CreateReadback(
	CDevice* device, const char* name, const Format format,
	const uint32_t width, const uint32_t height)
{
	std::unique_ptr<CTexture> texture(new CTexture(device));

	texture->m_Format = format;
	texture->m_Type = Type::TEXTURE_2D;
	texture->m_Usage = Usage::TRANSFER_DST;
	texture->m_Width = width;
	texture->m_Height = height;
	texture->m_MIPLevelCount = 1;
	texture->m_SampleCount = 1;
	texture->m_LayerCount = 1;
	texture->m_VkFormat = Mapping::FromFormat(texture->m_Format);
	texture->m_AttachmentImageAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	texture->m_SamplerImageAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = texture->m_VkFormat;
	imageCreateInfo.samples = Mapping::FromSampleCount(1);
	imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo allocationCreateInfo{};
	allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
#ifndef NDEBUG
	allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
	allocationCreateInfo.pUserData = const_cast<char*>(name);
#endif
	allocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
	const VkResult createImageResult = vmaCreateImage(
		device->GetVMAAllocator(), &imageCreateInfo, &allocationCreateInfo,
		&texture->m_Image, &texture->m_Allocation, &texture->m_AllocationInfo);
	if (createImageResult != VK_SUCCESS)
	{
		LOGERROR("Failed to create VkImage: %d (%s)",
			static_cast<int>(createImageResult), Utilities::GetVkResultName(createImageResult));
		return nullptr;
	}

	if (!texture->m_AllocationInfo.pMappedData)
	{
		LOGERROR("Failed to map readback image.");
		return nullptr;
	}

	device->SetObjectName(VK_OBJECT_TYPE_IMAGE, texture->m_Image, name);

	return texture;
}

CTexture::CTexture(CDevice* device)
	: m_Device(device), m_UID(device->GenerateNextDeviceObjectUID())
{
}

CTexture::~CTexture()
{
	if (m_AttachmentImageView != VK_NULL_HANDLE)
		m_Device->ScheduleObjectToDestroy(
			VK_OBJECT_TYPE_IMAGE_VIEW, m_AttachmentImageView, VK_NULL_HANDLE);

	if (m_SamplerImageView != VK_NULL_HANDLE)
		m_Device->ScheduleObjectToDestroy(
			VK_OBJECT_TYPE_IMAGE_VIEW, m_SamplerImageView, VK_NULL_HANDLE);

	if (m_Allocation != VK_NULL_HANDLE)
		m_Device->ScheduleObjectToDestroy(
			VK_OBJECT_TYPE_IMAGE, m_Image, m_Allocation);

	m_Device->ScheduleTextureToDestroy(m_UID);
}

IDevice* CTexture::GetDevice()
{
	return m_Device;
}

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer
