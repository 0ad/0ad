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

#ifndef INCLUDED_RENDERER_BACKEND_VULKAN_DESCRIPTORMANAGER
#define INCLUDED_RENDERER_BACKEND_VULKAN_DESCRIPTORMANAGER

#include "renderer/backend/Sampler.h"
#include "renderer/backend/vulkan/Texture.h"

#include <glad/vulkan.h>
#include <limits>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

class CDevice;

class CDescriptorManager
{
public:
	CDescriptorManager(CDevice* device, const bool useDescriptorIndexing);
	~CDescriptorManager();

	bool UseDescriptorIndexing() const { return m_UseDescriptorIndexing; }

	/**
	 * @return a single type descriptor set layout with the number of bindings
	 * equals to the size. The returned layout is owned by the manager.
	 */
	VkDescriptorSetLayout GetSingleTypeDescritorSetLayout(
		VkDescriptorType type, const uint32_t size);

	VkDescriptorSet GetSingleTypeDescritorSet(
		VkDescriptorType type, VkDescriptorSetLayout layout,
		const std::vector<CTexture::UID>& texturesUID,
		const std::vector<CTexture*>& textures);

	uint32_t GetUniformSet() const;

	uint32_t GetTextureDescriptor(CTexture* texture);
	void OnTextureDestroy(const CTexture::UID uid);

	const VkDescriptorSetLayout& GetDescriptorIndexingSetLayout() const { return m_DescriptorIndexingSetLayout; }
	const VkDescriptorSetLayout& GetUniformDescriptorSetLayout() const { return m_UniformDescriptorSetLayout; }
	const VkDescriptorSet& GetDescriptorIndexingSet() { return m_DescriptorIndexingSet; }

	const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const { return m_DescriptorSetLayouts; }

private:
	struct SingleTypePool
	{
		VkDescriptorSetLayout layout;
		VkDescriptorPool pool;
		int16_t firstFreeIndex = 0;
		std::vector<std::pair<VkDescriptorSet, int16_t>> elements;
	};
	SingleTypePool& GetSingleTypePool(const VkDescriptorType type, const uint32_t size);

	CDevice* m_Device = nullptr;

	bool m_UseDescriptorIndexing = false;

	VkDescriptorPool m_DescriptorIndexingPool = VK_NULL_HANDLE;
	VkDescriptorSet m_DescriptorIndexingSet = VK_NULL_HANDLE;
	VkDescriptorSetLayout m_DescriptorIndexingSetLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout m_UniformDescriptorSetLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;

	static constexpr uint32_t DESCRIPTOR_INDEXING_BINDING_SIZE = 16384;
	static constexpr uint32_t NUMBER_OF_BINDINGS_PER_DESCRIPTOR_INDEXING_SET = 3;

	struct DescriptorIndexingBindingMap
	{
		static_assert(std::numeric_limits<int16_t>::max() >= DESCRIPTOR_INDEXING_BINDING_SIZE);
		int16_t firstFreeIndex = 0;
		std::vector<int16_t> elements;
		std::unordered_map<CTexture::UID, int16_t> map;
	};
	std::array<DescriptorIndexingBindingMap, NUMBER_OF_BINDINGS_PER_DESCRIPTOR_INDEXING_SET>
		m_DescriptorIndexingBindings;
	std::unordered_map<CTexture::UID, uint32_t> m_TextureToBindingMap;

	std::unordered_map<VkDescriptorType, std::vector<SingleTypePool>> m_SingleTypePools;
	std::unordered_map<CTexture::UID, std::vector<std::tuple<VkDescriptorType, uint8_t, int16_t>>> m_TextureSingleTypePoolMap;

	using SingleTypeCacheKey = std::pair<VkDescriptorSetLayout, std::vector<CTexture::UID>>;
	struct SingleTypeCacheKeyHash
	{
		size_t operator()(const SingleTypeCacheKey& key) const;
	};
	std::unordered_map<SingleTypeCacheKey, VkDescriptorSet, SingleTypeCacheKeyHash> m_SingleTypeSets;
};

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_VULKAN_DESCRIPTORMANAGER
