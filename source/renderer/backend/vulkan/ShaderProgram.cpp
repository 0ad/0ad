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

#include "precompiled.h"

#include "ShaderProgram.h"

#include "graphics/ShaderDefines.h"
#include "ps/CLogger.h"
#include "ps/containers/StaticVector.h"
#include "ps/CStr.h"
#include "ps/CStrInternStatic.h"
#include "ps/Filesystem.h"
#include "ps/Profile.h"
#include "ps/XML/Xeromyces.h"
#include "renderer/backend/vulkan/DescriptorManager.h"
#include "renderer/backend/vulkan/Device.h"
#include "renderer/backend/vulkan/RingCommandContext.h"
#include "renderer/backend/vulkan/Texture.h"
#include "renderer/backend/vulkan/Utilities.h"

#include <algorithm>
#include <limits>

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

namespace
{

VkShaderModule CreateShaderModule(CDevice* device, const VfsPath& path)
{
	CVFSFile file;
	if (file.Load(g_VFS, path) != PSRETURN_OK)
	{
		LOGERROR("Failed to load shader file: '%s'", path.string8());
		return VK_NULL_HANDLE;
	}

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	// Casting to uint32_t requires to fit alignment and size.
	ENSURE(file.GetBufferSize() % 4 == 0);
	ENSURE(reinterpret_cast<uintptr_t>(file.GetBuffer()) % alignof(uint32_t) == 0u);
	createInfo.codeSize = file.GetBufferSize();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(file.GetBuffer());

	VkShaderModule shaderModule;
	const VkResult result = vkCreateShaderModule(device->GetVkDevice(), &createInfo, nullptr, &shaderModule);
	if (result != VK_SUCCESS)
	{
		LOGERROR("Failed to create shader module from file: '%s' %d (%s)",
			path.string8(), static_cast<int>(result), Utilities::GetVkResultName(result));
		return VK_NULL_HANDLE;
	}
	device->SetObjectName(VK_OBJECT_TYPE_SHADER_MODULE, shaderModule, path.string8().c_str());
	return shaderModule;
}

VfsPath FindProgramMatchingDefines(const VfsPath& xmlFilename, const CShaderDefines& defines)
{
	CXeromyces xeroFile;
	PSRETURN ret = xeroFile.Load(g_VFS, xmlFilename);
	if (ret != PSRETURN_OK)
		return {};

	// TODO: add XML validation.

#define EL(x) const int el_##x = xeroFile.GetElementID(#x)
#define AT(x) const int at_##x = xeroFile.GetAttributeID(#x)
	EL(define);
	EL(defines);
	EL(program);
	AT(file);
	AT(name);
	AT(value);
#undef AT
#undef EL

	const CStrIntern strUndefined("UNDEFINED");
	VfsPath programFilename;
	XMBElement root = xeroFile.GetRoot();
	XERO_ITER_EL(root, rootChild)
	{
		if (rootChild.GetNodeName() == el_program)
		{
			CShaderDefines programDefines;
			XERO_ITER_EL(rootChild, programChild)
			{
				if (programChild.GetNodeName() == el_defines)
				{
					XERO_ITER_EL(programChild, definesChild)
					{
						XMBAttributeList attributes = definesChild.GetAttributes();
						if (definesChild.GetNodeName() == el_define)
						{
							const CStrIntern value(attributes.GetNamedItem(at_value));
							if (value == strUndefined)
								continue;
							programDefines.Add(
								CStrIntern(attributes.GetNamedItem(at_name)), value);
						}
					}
				}
			}

			if (programDefines == defines)
				return L"shaders/" + rootChild.GetAttributes().GetNamedItem(at_file).FromUTF8();
		}
	}

	return {};
}

} // anonymous namespace

IDevice* CVertexInputLayout::GetDevice()
{
	return m_Device;
}

// static
std::unique_ptr<CShaderProgram> CShaderProgram::Create(
	CDevice* device, const CStr& name, const CShaderDefines& baseDefines)
{
	const VfsPath xmlFilename = L"shaders/" + wstring_from_utf8(name) + L".xml";

	std::unique_ptr<CShaderProgram> shaderProgram(new CShaderProgram());
	shaderProgram->m_Device = device;
	shaderProgram->m_FileDependencies = {xmlFilename};

	CShaderDefines defines = baseDefines;
	if (device->GetDescriptorManager().UseDescriptorIndexing())
		defines.Add(str_USE_DESCRIPTOR_INDEXING, str_1);

	const VfsPath programFilename = FindProgramMatchingDefines(xmlFilename, defines);
	if (programFilename.empty())
	{
		LOGERROR("Program '%s' with required defines not found.", name);
		for (const auto& pair : defines.GetMap())
			LOGERROR("  \"%s\": \"%s\"", pair.first.c_str(), pair.second.c_str());
		return nullptr;
	}
	shaderProgram->m_FileDependencies.emplace_back(programFilename);

	CXeromyces programXeroFile;
	if (programXeroFile.Load(g_VFS, programFilename) != PSRETURN_OK)
		return nullptr;
	XMBElement programRoot = programXeroFile.GetRoot();

#define EL(x) const int el_##x = programXeroFile.GetElementID(#x)
#define AT(x) const int at_##x = programXeroFile.GetAttributeID(#x)
	EL(binding);
	EL(compute);
	EL(descriptor_set);
	EL(descriptor_sets);
	EL(fragment);
	EL(member);
	EL(push_constant);
	EL(stream);
	EL(vertex);
	AT(binding);
	AT(file);
	AT(location);
	AT(name);
	AT(offset);
	AT(set);
	AT(size);
	AT(type);
#undef AT
#undef EL

	auto addPushConstant =
		[&pushConstants=shaderProgram->m_PushConstants, &pushConstantDataFlags=shaderProgram->m_PushConstantDataFlags, &at_name, &at_offset, &at_size](
			const XMBElement& element, VkShaderStageFlags stageFlags) -> bool
	{
		const XMBAttributeList attributes = element.GetAttributes();
		const CStrIntern name = CStrIntern(attributes.GetNamedItem(at_name));
		const uint32_t size = attributes.GetNamedItem(at_size).ToUInt();
		const uint32_t offset = attributes.GetNamedItem(at_offset).ToUInt();
		if (offset % 4 != 0 || size % 4 != 0)
		{
			LOGERROR("Push constant should have offset and size be multiple of 4.");
			return false;
		}
		for (PushConstant& pushConstant : pushConstants)
		{
			if (pushConstant.name == name)
			{
				if (size != pushConstant.size || offset != pushConstant.offset)
				{
					LOGERROR("All shared push constants must have the same size and offset.");
					return false;
				}
				// We found the same constant so we don't need to add it again.
				pushConstant.stageFlags |= stageFlags;
				for (uint32_t index = 0; index < (size >> 2); ++index)
					pushConstantDataFlags[(offset >> 2) + index] |= stageFlags;
				return true;
			}
			if (offset + size < pushConstant.offset || offset >= pushConstant.offset + pushConstant.size)
				continue;
			LOGERROR("All push constant must not intersect each other in memory.");
			return false;
		}
		pushConstants.push_back({name, offset, size, stageFlags});
		for (uint32_t index = 0; index < (size >> 2); ++index)
			pushConstantDataFlags[(offset >> 2) + index] = stageFlags;
		return true;
	};

	uint32_t texturesDescriptorSetSize = 0;
	std::unordered_map<CStrIntern, uint32_t> textureMapping;

	VkDescriptorType storageImageDescriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
	uint32_t storageImageDescriptorSetSize = 0;
	std::unordered_map<CStrIntern, uint32_t> storageImageMapping;

	auto addDescriptorSets = [&](const XMBElement& element) -> bool
	{
		const bool useDescriptorIndexing =
			device->GetDescriptorManager().UseDescriptorIndexing();
		// TODO: reduce the indentation.
		XERO_ITER_EL(element, descriporSetsChild)
		{
			if (descriporSetsChild.GetNodeName() == el_descriptor_set)
			{
				const uint32_t set = descriporSetsChild.GetAttributes().GetNamedItem(at_set).ToUInt();
				if (useDescriptorIndexing && set == 0 && !descriporSetsChild.GetChildNodes().empty())
				{
					LOGERROR("Descritor set for descriptor indexing shouldn't contain bindings.");
					return false;
				}
				XERO_ITER_EL(descriporSetsChild, descriporSetChild)
				{
					if (descriporSetChild.GetNodeName() == el_binding)
					{
						const XMBAttributeList attributes = descriporSetChild.GetAttributes();
						const uint32_t binding = attributes.GetNamedItem(at_binding).ToUInt();
						const uint32_t size = attributes.GetNamedItem(at_size).ToUInt();
						const CStr type = attributes.GetNamedItem(at_type);
						if (type == "uniform")
						{
							const uint32_t expectedSet =
								device->GetDescriptorManager().GetUniformSet();
							if (set != expectedSet || binding != 0)
							{
								LOGERROR("We support only a single uniform block per shader program.");
								return false;
							}
							shaderProgram->m_MaterialConstantsDataSize = size;
							XERO_ITER_EL(descriporSetChild, bindingChild)
							{
								if (bindingChild.GetNodeName() == el_member)
								{
									const XMBAttributeList memberAttributes = bindingChild.GetAttributes();
									const uint32_t offset = memberAttributes.GetNamedItem(at_offset).ToUInt();
									const uint32_t size = memberAttributes.GetNamedItem(at_size).ToUInt();
									const CStrIntern name{memberAttributes.GetNamedItem(at_name)};
									bool found = false;
									for (const Uniform& uniform : shaderProgram->m_Uniforms)
									{
										if (uniform.name == name)
										{
											if (offset != uniform.offset || size != uniform.size)
											{
												LOGERROR("All uniforms across all stage should match.");
												return false;
											}
											found = true;
										}
										else
										{
											if (offset + size <= uniform.offset || uniform.offset + uniform.size <= offset)
												continue;
											LOGERROR("Uniforms must not overlap each other.");
											return false;
										}
									}
									if (!found)
										shaderProgram->m_Uniforms.push_back({name, offset, size});
								}
							}
						}
						else if (type == "sampler1D" || type == "sampler2D" || type == "sampler2DShadow" || type == "sampler3D" || type == "samplerCube")
						{
							if (useDescriptorIndexing)
							{
								LOGERROR("We support only uniform descriptor sets with enabled descriptor indexing.");
								return false;
							}
							const CStrIntern name{attributes.GetNamedItem(at_name)};
							textureMapping[name] = binding;
							texturesDescriptorSetSize =
								std::max(texturesDescriptorSetSize, binding + 1);
						}
						else if (type == "storageImage" || type == "storageBuffer")
						{
							const CStrIntern name{attributes.GetNamedItem(at_name)};
							storageImageMapping[name] = binding;
							storageImageDescriptorSetSize =
								std::max(storageImageDescriptorSetSize, binding + 1);
							const VkDescriptorType descriptorType = type == "storageBuffer"
								? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
								: VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
							if (storageImageDescriptorType == VK_DESCRIPTOR_TYPE_MAX_ENUM)
								storageImageDescriptorType = descriptorType;
							else if (storageImageDescriptorType != descriptorType)
							{
								LOGERROR("Shader should have storages of the same type.");
								return false;
							}
						}
						else
						{
							LOGERROR("Unsupported binding: '%s'", type.c_str());
							return false;
						}
					}
				}
			}
		}
		return true;
	};

	XERO_ITER_EL(programRoot, programChild)
	{
		if (programChild.GetNodeName() == el_vertex)
		{
			if (shaderProgram->m_PipelineBindPoint != VK_PIPELINE_BIND_POINT_MAX_ENUM &&
				shaderProgram->m_PipelineBindPoint != VK_PIPELINE_BIND_POINT_GRAPHICS)
			{
				LOGERROR("Shader program can't mix different pipelines: '%s'.", name.c_str());
				return nullptr;
			}
			shaderProgram->m_PipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			const VfsPath shaderModulePath =
				L"shaders/" + programChild.GetAttributes().GetNamedItem(at_file).FromUTF8();
			shaderProgram->m_FileDependencies.emplace_back(shaderModulePath);
			shaderProgram->m_ShaderModules.emplace_back(
				CreateShaderModule(device, shaderModulePath));
			if (shaderProgram->m_ShaderModules.back() == VK_NULL_HANDLE)
				return nullptr;
			VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
			vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertexShaderStageInfo.module = shaderProgram->m_ShaderModules.back();
			vertexShaderStageInfo.pName = "main";
			shaderProgram->m_Stages.emplace_back(std::move(vertexShaderStageInfo));
			XERO_ITER_EL(programChild, stageChild)
			{
				if (stageChild.GetNodeName() == el_stream)
				{
					XMBAttributeList attributes = stageChild.GetAttributes();
					const uint32_t location = attributes.GetNamedItem(at_location).ToUInt();
					const CStr streamName = attributes.GetNamedItem(at_name);
					VertexAttributeStream stream = VertexAttributeStream::UV7;
					if (streamName == "pos")
						stream = VertexAttributeStream::POSITION;
					else if (streamName == "normal")
						stream = VertexAttributeStream::NORMAL;
					else if (streamName == "color")
						stream = VertexAttributeStream::COLOR;
					else if (streamName == "uv0")
						stream = VertexAttributeStream::UV0;
					else if (streamName == "uv1")
						stream = VertexAttributeStream::UV1;
					else if (streamName == "uv2")
						stream = VertexAttributeStream::UV2;
					else if (streamName == "uv3")
						stream = VertexAttributeStream::UV3;
					else if (streamName == "uv4")
						stream = VertexAttributeStream::UV4;
					else if (streamName == "uv5")
						stream = VertexAttributeStream::UV5;
					else if (streamName == "uv6")
						stream = VertexAttributeStream::UV6;
					else if (streamName == "uv7")
						stream = VertexAttributeStream::UV7;
					else
						debug_warn("Unknown stream");
					shaderProgram->m_StreamLocations[stream] = location;
				}
				else if (stageChild.GetNodeName() == el_push_constant)
				{
					if (!addPushConstant(stageChild, VK_SHADER_STAGE_VERTEX_BIT))
						return nullptr;
				}
				else if (stageChild.GetNodeName() == el_descriptor_sets)
				{
					if (!addDescriptorSets(stageChild))
						return nullptr;
				}
			}
		}
		else if (programChild.GetNodeName() == el_fragment)
		{
			if (shaderProgram->m_PipelineBindPoint != VK_PIPELINE_BIND_POINT_MAX_ENUM &&
				shaderProgram->m_PipelineBindPoint != VK_PIPELINE_BIND_POINT_GRAPHICS)
			{
				LOGERROR("Shader program can't mix different pipelines: '%s'.", name.c_str());
				return nullptr;
			}
			shaderProgram->m_PipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			const VfsPath shaderModulePath =
				L"shaders/" + programChild.GetAttributes().GetNamedItem(at_file).FromUTF8();
			shaderProgram->m_FileDependencies.emplace_back(shaderModulePath);
			shaderProgram->m_ShaderModules.emplace_back(
				CreateShaderModule(device, shaderModulePath));
			if (shaderProgram->m_ShaderModules.back() == VK_NULL_HANDLE)
				return nullptr;
			VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
			fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragmentShaderStageInfo.module = shaderProgram->m_ShaderModules.back();
			fragmentShaderStageInfo.pName = "main";
			shaderProgram->m_Stages.emplace_back(std::move(fragmentShaderStageInfo));
			XERO_ITER_EL(programChild, stageChild)
			{
				if (stageChild.GetNodeName() == el_push_constant)
				{
					if (!addPushConstant(stageChild, VK_SHADER_STAGE_FRAGMENT_BIT))
						return nullptr;
				}
				else if (stageChild.GetNodeName() == el_descriptor_sets)
				{
					if (!addDescriptorSets(stageChild))
						return nullptr;
				}
			}
		}
		else if (programChild.GetNodeName() == el_compute)
		{
			if (shaderProgram->m_PipelineBindPoint != VK_PIPELINE_BIND_POINT_MAX_ENUM &&
				shaderProgram->m_PipelineBindPoint != VK_PIPELINE_BIND_POINT_COMPUTE)
			{
				LOGERROR("Shader program can't mix different pipelines: '%s'.", name.c_str());
				return nullptr;
			}
			shaderProgram->m_PipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
			const VfsPath shaderModulePath =
				L"shaders/" + programChild.GetAttributes().GetNamedItem(at_file).FromUTF8();
			shaderProgram->m_FileDependencies.emplace_back(shaderModulePath);
			shaderProgram->m_ShaderModules.emplace_back(
				CreateShaderModule(device, shaderModulePath));
			if (shaderProgram->m_ShaderModules.back() == VK_NULL_HANDLE)
				return nullptr;
			VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
			computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			computeShaderStageInfo.module = shaderProgram->m_ShaderModules.back();
			computeShaderStageInfo.pName = "main";
			shaderProgram->m_Stages.emplace_back(std::move(computeShaderStageInfo));
			XERO_ITER_EL(programChild, stageChild)
			{
				if (stageChild.GetNodeName() == el_push_constant)
				{
					if (!addPushConstant(stageChild, VK_SHADER_STAGE_COMPUTE_BIT))
						return nullptr;
				}
				else if (stageChild.GetNodeName() == el_descriptor_sets)
				{
					if (!addDescriptorSets(stageChild))
						return nullptr;
				}
			}
		}
	}

	if (shaderProgram->m_Stages.empty())
	{
		LOGERROR("Program should contain at least one stage.");
		return nullptr;
	}

	ENSURE(shaderProgram->m_PipelineBindPoint != VK_PIPELINE_BIND_POINT_MAX_ENUM);

	for (size_t index = 0; index < shaderProgram->m_PushConstants.size(); ++index)
		shaderProgram->m_PushConstantMapping[shaderProgram->m_PushConstants[index].name] = index;
	std::vector<VkPushConstantRange> pushConstantRanges;
	pushConstantRanges.reserve(shaderProgram->m_PushConstants.size());
	std::transform(
		shaderProgram->m_PushConstants.begin(), shaderProgram->m_PushConstants.end(),
		std::back_insert_iterator(pushConstantRanges), [](const PushConstant& pushConstant)
		{
			return VkPushConstantRange{pushConstant.stageFlags, pushConstant.offset, pushConstant.size};
		});
	if (!pushConstantRanges.empty())
	{
		std::sort(pushConstantRanges.begin(), pushConstantRanges.end(),
			[](const VkPushConstantRange& lhs, const VkPushConstantRange& rhs)
			{
				return lhs.offset < rhs.offset;
			});
		// Merge subsequent constants.
		auto it = pushConstantRanges.begin();
		while (std::next(it) != pushConstantRanges.end())
		{
			auto next = std::next(it);
			if (it->stageFlags == next->stageFlags)
			{
				it->size = next->offset - it->offset + next->size;
				pushConstantRanges.erase(next);
			}
			else
				it = next;
		}
		for (const VkPushConstantRange& range : pushConstantRanges)
			if (std::count_if(pushConstantRanges.begin(), pushConstantRanges.end(),
				[stageFlags=range.stageFlags](const VkPushConstantRange& range) { return range.stageFlags & stageFlags; }) != 1)
			{
				LOGERROR("Any two range must not include the same stage in stageFlags.");
				return nullptr;
			}
	}

	for (size_t index = 0; index < shaderProgram->m_Uniforms.size(); ++index)
		shaderProgram->m_UniformMapping[shaderProgram->m_Uniforms[index].name] = index;
	if (!shaderProgram->m_Uniforms.empty())
	{
		if (shaderProgram->m_MaterialConstantsDataSize > device->GetChoosenPhysicalDevice().properties.limits.maxUniformBufferRange)
		{
			LOGERROR("Uniform buffer size is too big for the device.");
			return nullptr;
		}
		shaderProgram->m_MaterialConstantsData =
			std::make_unique<std::byte[]>(shaderProgram->m_MaterialConstantsDataSize);
	}

	std::vector<VkDescriptorSetLayout> layouts =
		device->GetDescriptorManager().GetDescriptorSetLayouts();
	if (texturesDescriptorSetSize > 0)
	{
		ENSURE(!device->GetDescriptorManager().UseDescriptorIndexing());
		shaderProgram->m_TextureBinding.emplace(
			device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texturesDescriptorSetSize, std::move(textureMapping));
		layouts.emplace_back(shaderProgram->m_TextureBinding->GetDescriptorSetLayout());
	}
	if (storageImageDescriptorSetSize > 0)
	{
		shaderProgram->m_StorageImageBinding.emplace(
			device, storageImageDescriptorType, storageImageDescriptorSetSize, std::move(storageImageMapping));
		layouts.emplace_back(shaderProgram->m_StorageImageBinding->GetDescriptorSetLayout());
	}

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
	pipelineLayoutCreateInfo.pSetLayouts = layouts.data();
	pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRanges.size();
	pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();

	const VkResult result = vkCreatePipelineLayout(
		device->GetVkDevice(), &pipelineLayoutCreateInfo, nullptr,
		&shaderProgram->m_PipelineLayout);
	if (result != VK_SUCCESS)
	{
		LOGERROR("Failed to create a pipeline layout: %d (%s)",
			static_cast<int>(result), Utilities::GetVkResultName(result));
		return nullptr;
	}

	return shaderProgram;
}

CShaderProgram::CShaderProgram() = default;

CShaderProgram::~CShaderProgram()
{
	if (m_PipelineLayout != VK_NULL_HANDLE)
		m_Device->ScheduleObjectToDestroy(VK_OBJECT_TYPE_PIPELINE_LAYOUT, m_PipelineLayout, VK_NULL_HANDLE);
	for (VkShaderModule shaderModule : m_ShaderModules)
		if (shaderModule != VK_NULL_HANDLE)
			m_Device->ScheduleObjectToDestroy(VK_OBJECT_TYPE_SHADER_MODULE, shaderModule, VK_NULL_HANDLE);
}

IDevice* CShaderProgram::GetDevice()
{
	return m_Device;
}

int32_t CShaderProgram::GetBindingSlot(const CStrIntern name) const
{
	if (auto it = m_PushConstantMapping.find(name); it != m_PushConstantMapping.end())
		return it->second;
	if (auto it = m_UniformMapping.find(name); it != m_UniformMapping.end())
		return it->second + m_PushConstants.size();
	if (const int32_t bindingSlot = m_TextureBinding.has_value() ? m_TextureBinding->GetBindingSlot(name) : -1; bindingSlot != -1)
		return bindingSlot + m_PushConstants.size() + m_UniformMapping.size();
	if (const int32_t bindingSlot = m_StorageImageBinding.has_value() ? m_StorageImageBinding->GetBindingSlot(name) : -1; bindingSlot != -1)
		return bindingSlot + m_PushConstants.size() + m_UniformMapping.size() + (m_TextureBinding.has_value() ? m_TextureBinding->GetBoundDeviceObjects().size() : 0);
	return -1;
}

std::vector<VfsPath> CShaderProgram::GetFileDependencies() const
{
	return m_FileDependencies;
}

uint32_t CShaderProgram::GetStreamLocation(const VertexAttributeStream stream) const
{
	auto it = m_StreamLocations.find(stream);
	return it != m_StreamLocations.end() ? it->second : std::numeric_limits<uint32_t>::max();
}

void CShaderProgram::Bind()
{
	if (m_MaterialConstantsData)
		m_MaterialConstantsDataOutdated = true;
}

void CShaderProgram::Unbind()
{
	if (m_TextureBinding.has_value())
		m_TextureBinding->Unbind();
	if (m_StorageImageBinding.has_value())
		m_StorageImageBinding->Unbind();
}

void CShaderProgram::PreDraw(CRingCommandContext& commandContext)
{
	BindOutdatedDescriptorSets(commandContext);
	if (m_PushConstantDataMask)
	{
		for (uint32_t index = 0; index < 32;)
		{
			if (!(m_PushConstantDataMask & (1 << index)))
			{
				++index;
				continue;
			}
			uint32_t indexEnd = index + 1;
			while (indexEnd < 32 && (m_PushConstantDataMask & (1 << indexEnd)) && m_PushConstantDataFlags[index] == m_PushConstantDataFlags[indexEnd])
				++indexEnd;
			vkCmdPushConstants(
				commandContext.GetCommandBuffer(), GetPipelineLayout(),
				m_PushConstantDataFlags[index],
				index * 4, (indexEnd - index) * 4, m_PushConstantData.data() + index * 4);
			index = indexEnd;
		}
		m_PushConstantDataMask = 0;
	}
}

void CShaderProgram::PreDispatch(
	CRingCommandContext& commandContext)
{
	PreDraw(commandContext);

	if (m_StorageImageBinding.has_value())
		for (CTexture* texture : m_StorageImageBinding->GetBoundDeviceObjects())
			if (texture)
			{
				if (!(texture->GetUsage() & ITexture::Usage::SAMPLED) && texture->IsInitialized())
					continue;
				VkImageLayout oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				if (!texture->IsInitialized())
					oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				Utilities::SetTextureLayout(
					commandContext.GetCommandBuffer(), texture,
					oldLayout,
					VK_IMAGE_LAYOUT_GENERAL,
					VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
			}
}

void CShaderProgram::PostDispatch(CRingCommandContext& commandContext)
{
	if (m_StorageImageBinding.has_value())
		for (CTexture* texture : m_StorageImageBinding->GetBoundDeviceObjects())
			if (texture)
			{
				if (!(texture->GetUsage() & ITexture::Usage::SAMPLED) && texture->IsInitialized())
					continue;
				Utilities::SetTextureLayout(
					commandContext.GetCommandBuffer(), texture,
					VK_IMAGE_LAYOUT_GENERAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
			}
}

void CShaderProgram::BindOutdatedDescriptorSets(
	CRingCommandContext& commandContext)
{
	// TODO: combine calls after more sets to bind.
	PS::StaticVector<std::tuple<uint32_t, VkDescriptorSet>, 2> descriptortSets;
	if (m_TextureBinding.has_value() && m_TextureBinding->IsOutdated())
	{
		constexpr uint32_t TEXTURE_BINDING_SET = 1u;
		descriptortSets.emplace_back(TEXTURE_BINDING_SET, m_TextureBinding->UpdateAndReturnDescriptorSet());
	}
	if (m_StorageImageBinding.has_value() && m_StorageImageBinding->IsOutdated())
	{
		constexpr uint32_t STORAGE_IMAGE_BINDING_SET = 2u;
		descriptortSets.emplace_back(STORAGE_IMAGE_BINDING_SET, m_StorageImageBinding->UpdateAndReturnDescriptorSet());
	}

	for (const auto [firstSet, descriptorSet] : descriptortSets)
	{
		vkCmdBindDescriptorSets(
			commandContext.GetCommandBuffer(), GetPipelineBindPoint(), GetPipelineLayout(),
			firstSet, 1, &descriptorSet, 0, nullptr);
	}
}

void CShaderProgram::SetUniform(
	const int32_t bindingSlot,
	const float value)
{
	const float values[1] = {value};
	SetUniform(bindingSlot, PS::span<const float>(values, values + 1));
}

void CShaderProgram::SetUniform(
	const int32_t bindingSlot,
	const float valueX, const float valueY)
{
	const float values[2] = {valueX, valueY};
	SetUniform(bindingSlot, PS::span<const float>(values, values + 2));
}

void CShaderProgram::SetUniform(
	const int32_t bindingSlot,
	const float valueX, const float valueY,
	const float valueZ)
{
	const float values[3] = {valueX, valueY, valueZ};
	SetUniform(bindingSlot, PS::span<const float>(values, values + 3));
}

void CShaderProgram::SetUniform(
	const int32_t bindingSlot,
	const float valueX, const float valueY,
	const float valueZ, const float valueW)
{
	const float values[4] = {valueX, valueY, valueZ, valueW};
	SetUniform(bindingSlot, PS::span<const float>(values, values + 4));
}

void CShaderProgram::SetUniform(const int32_t bindingSlot, PS::span<const float> values)
{
	if (bindingSlot < 0)
		return;
	const auto data = GetUniformData(bindingSlot, values.size() * sizeof(float));
	std::memcpy(data.first, values.data(), data.second);
}

std::pair<std::byte*, uint32_t> CShaderProgram::GetUniformData(
	const int32_t bindingSlot, const uint32_t dataSize)
{
	if (bindingSlot < static_cast<int32_t>(m_PushConstants.size()))
	{
		const uint32_t size = m_PushConstants[bindingSlot].size;
		const uint32_t offset = m_PushConstants[bindingSlot].offset;
		ENSURE(size <= dataSize);
		m_PushConstantDataMask |= ((1 << (size >> 2)) - 1) << (offset >> 2);
		return {m_PushConstantData.data() + offset, size};
	}
	else
	{
		ENSURE(bindingSlot - m_PushConstants.size() < m_Uniforms.size());
		const Uniform& uniform = m_Uniforms[bindingSlot - m_PushConstants.size()];
		m_MaterialConstantsDataOutdated = true;
		const uint32_t size = uniform.size;
		const uint32_t offset = uniform.offset;
		ENSURE(size <= dataSize);
		return {m_MaterialConstantsData.get() + offset, size};
	}
}

void CShaderProgram::SetTexture(const int32_t bindingSlot, CTexture* texture)
{
	if (bindingSlot < 0)
		return;
	CDescriptorManager& descriptorManager = m_Device->GetDescriptorManager();
	if (descriptorManager.UseDescriptorIndexing())
	{
		const uint32_t descriptorIndex = descriptorManager.GetTextureDescriptor(texture->As<CTexture>());
		ENSURE(bindingSlot < static_cast<int32_t>(m_PushConstants.size()));

		const uint32_t size = m_PushConstants[bindingSlot].size;
		const uint32_t offset = m_PushConstants[bindingSlot].offset;
		ENSURE(size == sizeof(descriptorIndex));
		std::memcpy(m_PushConstantData.data() + offset, &descriptorIndex, size);
		m_PushConstantDataMask |= ((1 << (size >> 2)) - 1) << (offset >> 2);
	}
	else
	{
		ENSURE(bindingSlot >= static_cast<int32_t>(m_PushConstants.size() + m_UniformMapping.size()));
		ENSURE(m_TextureBinding.has_value());
		const uint32_t index = bindingSlot - (m_PushConstants.size() + m_UniformMapping.size());
		m_TextureBinding->SetObject(index, texture);
	}
}

void CShaderProgram::SetStorageTexture(const int32_t bindingSlot, CTexture* texture)
{
	if (bindingSlot < 0)
		return;
	const int32_t offset = static_cast<int32_t>(m_PushConstants.size() + m_UniformMapping.size() + (m_TextureBinding.has_value() ? m_TextureBinding->GetBoundDeviceObjects().size() : 0));
	ENSURE(bindingSlot >= offset);
	ENSURE(m_StorageImageBinding.has_value());
	const uint32_t index = bindingSlot - offset;
	m_StorageImageBinding->SetObject(index, texture);
}

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer
