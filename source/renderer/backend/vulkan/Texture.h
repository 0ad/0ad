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

#ifndef INCLUDED_RENDERER_BACKEND_VULKAN_TEXTURE
#define INCLUDED_RENDERER_BACKEND_VULKAN_TEXTURE

#include "renderer/backend/ITexture.h"
#include "renderer/backend/Sampler.h"
#include "renderer/backend/vulkan/VMA.h"

#include <glad/vulkan.h>
#include <memory>

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

class CDevice;

class CTexture final : public ITexture
{
public:
	~CTexture() override;

	IDevice* GetDevice() override;

	Type GetType() const override { return m_Type; }
	uint32_t GetUsage() const override { return m_Usage; }
	Format GetFormat() const override { return m_Format; }

	uint32_t GetWidth() const override { return m_Width; }
	uint32_t GetHeight() const override { return m_Height; }
	uint32_t GetMIPLevelCount() const override { return m_MIPLevelCount; }
	uint32_t GetSampleCount() const { return m_SampleCount; }
	uint32_t GetLayerCount() const { return m_LayerCount; }

	VkImage GetImage() { return m_Image; }
	VkImageView GetAttachmentImageView() { return m_AttachmentImageView; }
	VkImageView GetSamplerImageView() { return m_SamplerImageView; }
	VkSampler GetSampler() { return m_Sampler; }
	bool IsCompareEnabled() { return m_IsCompareEnabled; }
	VkFormat GetVkFormat() const { return m_VkFormat; }

	VkImageAspectFlags GetAttachmentImageAspectMask() { return m_AttachmentImageAspectMask; }
	VkImageAspectFlags GetSamplerImageAspectMask() { return m_SamplerImageAspectMask; }

	/**
	 * @return mapped data for readback textures else returns nullptr.
	 */
	void* GetMappedData() { return m_AllocationInfo.pMappedData; }

	VkDeviceMemory GetDeviceMemory() { return m_AllocationInfo.deviceMemory; }

	bool IsInitialized() const { return m_Initialized; }
	void SetInitialized() { m_Initialized = true; }

	/**
	 * @return UID of the texture. It's unique along all textures during a whole
	 * application run. We assume that 32bits should be enough, else we'd have
	 * a too big texture flow.
	 */
	using UID = uint32_t;
	static constexpr UID INVALID_UID = 0;
	UID GetUID() const { return m_UID; }

private:
	friend class CDevice;
	friend class CSwapChain;

	CTexture();

	static std::unique_ptr<CTexture> Create(
		CDevice* device, const char* name, const Type type, const uint32_t usage,
		const Format format, const uint32_t width, const uint32_t height,
		const Sampler::Desc& defaultSamplerDesc,
		const uint32_t MIPLevelCount, const uint32_t sampleCount);

	static std::unique_ptr<CTexture> WrapBackbufferImage(
		CDevice* device, const char* name, const VkImage image, const VkFormat format,
		const VkImageUsageFlags usage, const uint32_t width, const uint32_t height);

	static std::unique_ptr<CTexture> CreateReadback(
		CDevice* device, const char* name, const Format format,
		const uint32_t width, const uint32_t height);

	Type m_Type = Type::TEXTURE_2D;
	uint32_t m_Usage = 0;
	Format m_Format = Format::UNDEFINED;
	VkFormat m_VkFormat = VK_FORMAT_UNDEFINED;
	uint32_t m_Width = 0;
	uint32_t m_Height = 0;
	uint32_t m_MIPLevelCount = 0;
	uint32_t m_SampleCount = 0;
	uint32_t m_LayerCount = 0;

	CDevice* m_Device = nullptr;

	VkImage m_Image = VK_NULL_HANDLE;
	VkImageView m_AttachmentImageView = VK_NULL_HANDLE;
	VkImageView m_SamplerImageView = VK_NULL_HANDLE;
	VkSampler m_Sampler = VK_NULL_HANDLE;
	bool m_IsCompareEnabled = false;
	VmaAllocation m_Allocation{};
	VmaAllocationInfo m_AllocationInfo{};

	UID m_UID = 0;

	// Sampler image aspect mask is submask of the attachment one. As we can't
	// have both VK_IMAGE_ASPECT_DEPTH_BIT and VK_IMAGE_ASPECT_STENCIL_BIT for
	// VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
	VkImageAspectFlags m_AttachmentImageAspectMask = 0;
	VkImageAspectFlags m_SamplerImageAspectMask = 0;

	// We store a flag of all subresources, we don't have to handle them separately.
	// It's safe to store the current state while we use a single device command
	// context.
	bool m_Initialized = false;
};

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_VULKAN_TEXTURE
