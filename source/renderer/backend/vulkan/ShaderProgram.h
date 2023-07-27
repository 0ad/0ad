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

#ifndef INCLUDED_RENDERER_BACKEND_VULKAN_SHADERPROGRAM
#define INCLUDED_RENDERER_BACKEND_VULKAN_SHADERPROGRAM

#include "renderer/backend/IShaderProgram.h"
#include "renderer/backend/vulkan/Texture.h"

#include <array>
#include <cstddef>
#include <glad/vulkan.h>
#include <memory>
#include <unordered_map>
#include <vector>

class CShaderDefines;
class CStr;

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

class CDevice;

class CVertexInputLayout : public IVertexInputLayout
{
public:
	CVertexInputLayout(CDevice* device, const PS::span<const SVertexAttributeFormat> attributes)
		: m_Device(device), m_Attributes(attributes.begin(), attributes.end())
	{
		static uint32_t m_LastAvailableUID = 1;
		m_UID = m_LastAvailableUID++;
		for (const SVertexAttributeFormat& attribute : m_Attributes)
		{
			ENSURE(attribute.format != Format::UNDEFINED);
			ENSURE(attribute.stride > 0);
		}
	}

	~CVertexInputLayout() override = default;

	IDevice* GetDevice() override;

	const std::vector<SVertexAttributeFormat>& GetAttributes() const noexcept { return m_Attributes; }

	using UID = uint32_t;
	UID GetUID() const { return m_UID; }

private:
	CDevice* m_Device = nullptr;

	UID m_UID = 0;

	std::vector<SVertexAttributeFormat> m_Attributes;
};

class CShaderProgram final : public IShaderProgram
{
public:
	~CShaderProgram() override;

	IDevice* GetDevice() override;

	int32_t GetBindingSlot(const CStrIntern name) const override;

	std::vector<VfsPath> GetFileDependencies() const override;

	uint32_t GetStreamLocation(const VertexAttributeStream stream) const;

	const std::vector<VkPipelineShaderStageCreateInfo>& GetStages() const { return m_Stages; }

	void Bind();
	void Unbind();
	void PreDraw(VkCommandBuffer commandBuffer);

	VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
	VkPipelineBindPoint GetPipelineBindPoint() const { return VK_PIPELINE_BIND_POINT_GRAPHICS; }

	void SetUniform(
		const int32_t bindingSlot,
		const float value);
	void SetUniform(
		const int32_t bindingSlot,
		const float valueX, const float valueY);
	void SetUniform(
		const int32_t bindingSlot,
		const float valueX, const float valueY,
		const float valueZ);
	void SetUniform(
		const int32_t bindingSlot,
		const float valueX, const float valueY,
		const float valueZ, const float valueW);
	void SetUniform(
		const int32_t bindingSlot, PS::span<const float> values);
	void SetTexture(const int32_t bindingSlot, CTexture* texture);

	// TODO: rename to something related to buffer.
	bool IsMaterialConstantsDataOutdated() const { return m_MaterialConstantsDataOutdated; }
	void UpdateMaterialConstantsData() { m_MaterialConstantsDataOutdated = false; }
	std::byte* GetMaterialConstantsData() const { return m_MaterialConstantsData.get(); }
	uint32_t GetMaterialConstantsDataSize() const { return m_MaterialConstantsDataSize; }

private:
	friend class CDevice;

	CShaderProgram();

	std::pair<std::byte*, uint32_t> GetUniformData(
		const int32_t bindingSlot, const uint32_t dataSize);

	static std::unique_ptr<CShaderProgram> Create(
		CDevice* device, const CStr& name, const CShaderDefines& defines);

	void UpdateActiveDescriptorSet(
		VkCommandBuffer commandBuffer);

	CDevice* m_Device = nullptr;

	std::vector<VkShaderModule> m_ShaderModules;
	std::vector<VkPipelineShaderStageCreateInfo> m_Stages;
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

	std::vector<VfsPath> m_FileDependencies;

	struct PushConstant
	{
		CStrIntern name;
		uint32_t offset;
		uint32_t size;
		VkShaderStageFlags stageFlags;
	};
	struct Uniform
	{
		CStrIntern name;
		uint32_t offset;
		uint32_t size;
	};
	std::unique_ptr<std::byte[]> m_MaterialConstantsData;
	uint32_t m_MaterialConstantsDataSize = 0;
	bool m_MaterialConstantsDataOutdated = false;
	std::array<std::byte, 128> m_PushConstantData;
	uint32_t m_PushConstantDataMask = 0;
	std::array<VkShaderStageFlags, 32> m_PushConstantDataFlags;
	std::vector<PushConstant> m_PushConstants;
	std::vector<Uniform> m_Uniforms;
	std::unordered_map<CStrIntern, uint32_t> m_UniformMapping;
	std::unordered_map<CStrIntern, uint32_t> m_PushConstantMapping;

	uint32_t m_TexturesDescriptorSetSize = 0;
	bool m_BoundTexturesOutdated = false;

	VkDescriptorSetLayout m_TexturesDescriptorSetLayout = VK_NULL_HANDLE;
	std::vector<CTexture*> m_BoundTextures;
	std::vector<CTexture::UID> m_BoundTexturesUID;
	VkDescriptorSet m_ActiveTexturesDescriptorSet = VK_NULL_HANDLE;
	std::unordered_map<CStrIntern, uint32_t> m_TextureMapping;

	std::unordered_map<VertexAttributeStream, uint32_t> m_StreamLocations;
};

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_VULKAN_SHADERPROGRAM
