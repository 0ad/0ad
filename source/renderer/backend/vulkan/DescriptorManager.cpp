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

#include "DescriptorManager.h"

#include "lib/hash.h"
#include "ps/containers/StaticVector.h"
#include "renderer/backend/vulkan/Device.h"
#include "renderer/backend/vulkan/Mapping.h"
#include "renderer/backend/vulkan/Texture.h"
#include "renderer/backend/vulkan/Utilities.h"

#include <array>
#include <numeric>

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

CDescriptorManager::CDescriptorManager(CDevice* device, const bool useDescriptorIndexing)
	: m_Device(device), m_UseDescriptorIndexing(useDescriptorIndexing),
	m_ErrorTexture(device->CreateTexture(
		"DescriptorManagerErrorTexture", ITexture::Type::TEXTURE_2D,
		ITexture::Usage::TRANSFER_DST | ITexture::Usage::SAMPLED,
		Format::R8G8B8A8_UNORM, 1, 1,
		Sampler::MakeDefaultSampler(Sampler::Filter::NEAREST, Sampler::AddressMode::REPEAT),
		1, 1))
{
	if (useDescriptorIndexing)
	{
		VkDescriptorPoolSize descriptorPoolSize{};
		descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorPoolSize.descriptorCount = DESCRIPTOR_INDEXING_BINDING_SIZE * NUMBER_OF_BINDINGS_PER_DESCRIPTOR_INDEXING_SET;

		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
		descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCreateInfo.poolSizeCount = 1;
		descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;
		descriptorPoolCreateInfo.maxSets = 1;
		descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
		ENSURE_VK_SUCCESS(vkCreateDescriptorPool(
			device->GetVkDevice(), &descriptorPoolCreateInfo, nullptr, &m_DescriptorIndexingPool));

		const VkShaderStageFlags stageFlags =
			VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		const std::array<VkDescriptorSetLayoutBinding, NUMBER_OF_BINDINGS_PER_DESCRIPTOR_INDEXING_SET> bindings{{
			{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, DESCRIPTOR_INDEXING_BINDING_SIZE, stageFlags},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, DESCRIPTOR_INDEXING_BINDING_SIZE, stageFlags},
			{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, DESCRIPTOR_INDEXING_BINDING_SIZE, stageFlags}
		}};

		const VkDescriptorBindingFlagsEXT baseBindingFlags =
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT
				| VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT
				| VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;
		const std::array<VkDescriptorBindingFlagsEXT, NUMBER_OF_BINDINGS_PER_DESCRIPTOR_INDEXING_SET> bindingFlags{{
			baseBindingFlags, baseBindingFlags, baseBindingFlags
		}};

		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlagsCreateInfo{};
		bindingFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		bindingFlagsCreateInfo.bindingCount = bindingFlags.size();
		bindingFlagsCreateInfo.pBindingFlags = bindingFlags.data();

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.bindingCount = bindings.size();
		descriptorSetLayoutCreateInfo.pBindings = bindings.data();
		descriptorSetLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
		descriptorSetLayoutCreateInfo.pNext = &bindingFlagsCreateInfo;

		ENSURE_VK_SUCCESS(vkCreateDescriptorSetLayout(
			device->GetVkDevice(), &descriptorSetLayoutCreateInfo,
			nullptr, &m_DescriptorIndexingSetLayout));

		m_DescriptorSetLayouts.emplace_back(m_DescriptorIndexingSetLayout);

		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.descriptorPool = m_DescriptorIndexingPool;
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.pSetLayouts = &m_DescriptorIndexingSetLayout;

		ENSURE_VK_SUCCESS(vkAllocateDescriptorSets(
			device->GetVkDevice(), &descriptorSetAllocateInfo, &m_DescriptorIndexingSet));

		for (DescriptorIndexingBindingMap& bindingMap : m_DescriptorIndexingBindings)
		{
			bindingMap.firstFreeIndex = 0;
			bindingMap.elements.resize(DESCRIPTOR_INDEXING_BINDING_SIZE);
			std::iota(bindingMap.elements.begin(), std::prev(bindingMap.elements.end()), 1);
			bindingMap.elements.back() = -1;
		}
	}

	// Currently we hard-code the layout for uniforms.
	const VkDescriptorSetLayoutBinding bindings[] =
	{
		{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT}
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.bindingCount = std::size(bindings);
	descriptorSetLayoutCreateInfo.pBindings = bindings;

	ENSURE_VK_SUCCESS(vkCreateDescriptorSetLayout(
		device->GetVkDevice(), &descriptorSetLayoutCreateInfo,
		nullptr, &m_UniformDescriptorSetLayout));
	m_DescriptorSetLayouts.emplace_back(m_UniformDescriptorSetLayout);
}

CDescriptorManager::~CDescriptorManager()
{
	VkDevice device = m_Device->GetVkDevice();

	for (auto& pair: m_SingleTypePools)
	{
		for (SingleTypePool& pool : pair.second)
		{
			if (pool.pool != VK_NULL_HANDLE)
				vkDestroyDescriptorPool(device, pool.pool, nullptr);
			if (pool.layout != VK_NULL_HANDLE)
				vkDestroyDescriptorSetLayout(device, pool.layout, nullptr);
		}
	}
	m_SingleTypePools.clear();

	for (VkDescriptorSetLayout descriptorSetLayout : m_DescriptorSetLayouts)
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	m_DescriptorSetLayouts.clear();

	if (m_DescriptorIndexingPool != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(device, m_DescriptorIndexingPool, nullptr);
}

CDescriptorManager::SingleTypePool& CDescriptorManager::GetSingleTypePool(
	const VkDescriptorType type, const uint32_t size)
{
	ENSURE(size > 0 && size <= 16);
	std::vector<SingleTypePool>& pools = m_SingleTypePools[type];
	if (pools.size() <= size)
		pools.resize(size + 1);
	SingleTypePool& pool = pools[size];
	if (pool.pool == VK_NULL_HANDLE)
	{
		constexpr uint32_t maxSets = 16384;

		VkDescriptorPoolSize descriptorPoolSize{};
		descriptorPoolSize.type = type;
		descriptorPoolSize.descriptorCount = maxSets * size;

		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
		descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCreateInfo.poolSizeCount = 1;
		descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;
		descriptorPoolCreateInfo.maxSets = maxSets;
		ENSURE_VK_SUCCESS(vkCreateDescriptorPool(
			m_Device->GetVkDevice(), &descriptorPoolCreateInfo, nullptr, &pool.pool));

		const VkPipelineStageFlags stageFlags =
			VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		PS::StaticVector<VkDescriptorSetLayoutBinding, 16> bindings;
		for (uint32_t index = 0; index < size; ++index)
			bindings.emplace_back(VkDescriptorSetLayoutBinding{index, type, 1, stageFlags});

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.bindingCount = size;
		descriptorSetLayoutCreateInfo.pBindings = bindings.data();

		ENSURE_VK_SUCCESS(vkCreateDescriptorSetLayout(
			m_Device->GetVkDevice(), &descriptorSetLayoutCreateInfo, nullptr, &pool.layout));

		pool.firstFreeIndex = 0;
		pool.elements.reserve(maxSets);
		for (uint32_t index = 0; index < maxSets; ++index)
			pool.elements.push_back({VK_NULL_HANDLE, 1, static_cast<int16_t>(index + 1)});
		pool.elements.back().nextFreeIndex = SingleTypePool::INVALID_INDEX;
	}
	return pool;
}

VkDescriptorSetLayout CDescriptorManager::GetSingleTypeDescritorSetLayout(
	VkDescriptorType type, const uint32_t size)
{
	return GetSingleTypePool(type, size).layout;
}

size_t CDescriptorManager::SingleTypeCacheKeyHash::operator()(const SingleTypeCacheKey& key) const
{
	size_t seed = 0;
	hash_combine(seed, key.first);
	for (DeviceObjectUID uid : key.second)
		hash_combine(seed, uid);
	return seed;
}

std::pair<VkDescriptorSet, bool> CDescriptorManager::GetSingleTypeDescritorSetImpl(
	VkDescriptorType type, VkDescriptorSetLayout layout,
	const std::vector<DeviceObjectUID>& uids)
{
	ENSURE(!uids.empty());
	const SingleTypeCacheKey key{layout, uids};
	auto it = m_SingleTypeSets.find(key);
	bool created = false;
	if (it == m_SingleTypeSets.end())
	{
		SingleTypePool& pool = GetSingleTypePool(type, uids.size());
		const int16_t elementIndex = pool.firstFreeIndex;
		ENSURE(elementIndex != SingleTypePool::INVALID_INDEX);
		SingleTypePool::Element& element = pool.elements[elementIndex];
		ENSURE(pool.firstFreeIndex != element.nextFreeIndex);
		pool.firstFreeIndex = element.nextFreeIndex;
		++element.version;
		// Occupy the index.
		element.nextFreeIndex = SingleTypePool::INVALID_INDEX;

		if (element.set == VK_NULL_HANDLE)
		{
			VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
			descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAllocateInfo.descriptorPool = pool.pool;
			descriptorSetAllocateInfo.descriptorSetCount = 1;
			descriptorSetAllocateInfo.pSetLayouts = &layout;

			ENSURE_VK_SUCCESS(vkAllocateDescriptorSets(
				m_Device->GetVkDevice(), &descriptorSetAllocateInfo, &element.set));
		}

		it = m_SingleTypeSets.emplace(key, element.set).first;
		created = true;

		for (const DeviceObjectUID uid : uids)
			if (uid != INVALID_DEVICE_OBJECT_UID)
				m_UIDToSingleTypePoolMap[uid].push_back({type, element.version, elementIndex, static_cast<uint8_t>(uids.size())});
	}
	return {it->second, created};
}

VkDescriptorSet CDescriptorManager::GetSingleTypeDescritorSet(
	VkDescriptorType type, VkDescriptorSetLayout layout,
	const std::vector<DeviceObjectUID>& texturesUID,
	const std::vector<CTexture*>& textures)
{
	ENSURE(texturesUID.size() == textures.size());
	ENSURE(!texturesUID.empty());
	const auto [set, justCreated] = GetSingleTypeDescritorSetImpl(type, layout, texturesUID);
	if (!justCreated)
		return set;

	const VkImageLayout imageLayout = type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
		? VK_IMAGE_LAYOUT_GENERAL
		: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	PS::StaticVector<VkDescriptorImageInfo, 16> infos;
	for (CTexture* texture : textures)
	{
		if (!texture)
		{
			// We can use a default texture only for read-only bindings.
			ENSURE(type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
			texture = m_ErrorTexture->As<CTexture>();
		}
		ENSURE(texture->GetUsage() & ITexture::Usage::SAMPLED);

		VkDescriptorImageInfo descriptorImageInfo{};
		descriptorImageInfo.imageLayout = imageLayout;
		descriptorImageInfo.imageView = texture->GetSamplerImageView();
		descriptorImageInfo.sampler = texture->GetSampler();
		infos.emplace_back(std::move(descriptorImageInfo));
	}

	VkWriteDescriptorSet writeDescriptorSet{};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = set;
	writeDescriptorSet.dstBinding = 0;
	writeDescriptorSet.dstArrayElement = 0;
	writeDescriptorSet.descriptorType = type;
	writeDescriptorSet.descriptorCount = static_cast<uint32_t>(infos.size());
	writeDescriptorSet.pImageInfo = infos.data();

	vkUpdateDescriptorSets(
		m_Device->GetVkDevice(), 1, &writeDescriptorSet, 0, nullptr);

	return set;
}

uint32_t CDescriptorManager::GetUniformSet() const
{
	return m_UseDescriptorIndexing ? 1 : 0;
}

uint32_t CDescriptorManager::GetTextureDescriptor(CTexture* texture)
{
	ENSURE(m_UseDescriptorIndexing);
	ENSURE(texture->GetUsage() & ITexture::Usage::SAMPLED);

	uint32_t binding = 0;
	if (texture->GetType() == ITexture::Type::TEXTURE_2D &&
		IsDepthFormat(texture->GetFormat()) &&
		texture->IsCompareEnabled())
		binding = 2;
	else if (texture->GetType() == ITexture::Type::TEXTURE_CUBE)
		binding = 1;

	DescriptorIndexingBindingMap& bindingMap = m_DescriptorIndexingBindings[binding];
	auto it = bindingMap.map.find(texture->GetUID());
	if (it != bindingMap.map.end())
		return it->second;
	m_TextureToBindingMap[texture->GetUID()] = binding;

	ENSURE(bindingMap.firstFreeIndex != -1);
	uint32_t descriptorSetIndex = bindingMap.firstFreeIndex;
	bindingMap.firstFreeIndex = bindingMap.elements[bindingMap.firstFreeIndex];

	ENSURE(texture->GetType() != ITexture::Type::TEXTURE_2D_MULTISAMPLE);

	VkDescriptorImageInfo descriptorImageInfo{};
	descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	descriptorImageInfo.imageView = texture->GetSamplerImageView();
	descriptorImageInfo.sampler = texture->GetSampler();

	VkWriteDescriptorSet writeDescriptorSet{};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = m_DescriptorIndexingSet;
	writeDescriptorSet.dstBinding = binding;
	writeDescriptorSet.dstArrayElement = descriptorSetIndex;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.pImageInfo = &descriptorImageInfo;

	vkUpdateDescriptorSets(
		m_Device->GetVkDevice(), 1, &writeDescriptorSet, 0, nullptr);

	bindingMap.map[texture->GetUID()] = descriptorSetIndex;

	return descriptorSetIndex;
}

void CDescriptorManager::OnTextureDestroy(const DeviceObjectUID uid)
{
	ENSURE(uid != INVALID_DEVICE_OBJECT_UID);
	if (m_UseDescriptorIndexing)
	{
		DescriptorIndexingBindingMap& bindingMap =
			m_DescriptorIndexingBindings[m_TextureToBindingMap[uid]];
		auto it = bindingMap.map.find(uid);
		// It's possible to not have the texture in the map. Because a texture will
		// be added to it only in case of usage.
		if (it == bindingMap.map.end())
			return;
		const int16_t index = it->second;
		bindingMap.elements[index] = bindingMap.firstFreeIndex;
		bindingMap.firstFreeIndex = index;
		bindingMap.map.erase(it);
	}
	else
	{
		auto it = m_UIDToSingleTypePoolMap.find(uid);
		if (it == m_UIDToSingleTypePoolMap.end())
			return;
		for (const auto& entry : it->second)
		{
			SingleTypePool& pool = GetSingleTypePool(entry.type, entry.size);
			SingleTypePool::Element& element = pool.elements[entry.elementIndex];
			// Multiple textures might be used by the same descriptor set and
			// we don't need to reset it if it was already.
			if (element.version == entry.version && element.nextFreeIndex == SingleTypePool::INVALID_INDEX)
			{
				ENSURE(pool.firstFreeIndex != entry.elementIndex);
				element.nextFreeIndex = pool.firstFreeIndex;
				pool.firstFreeIndex = entry.elementIndex;
			}
		}
		m_UIDToSingleTypePoolMap.erase(it);
	}
}

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer
