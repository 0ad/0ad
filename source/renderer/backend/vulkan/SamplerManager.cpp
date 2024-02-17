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

#include "SamplerManager.h"

#include "lib/hash.h"
#include "renderer/backend/vulkan/Device.h"
#include "renderer/backend/vulkan/Mapping.h"
#include "renderer/backend/vulkan/Utilities.h"

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

size_t CSamplerManager::SamplerDescHash::operator()(const Sampler::Desc& samplerDesc) const
{
	size_t seed = 0;

	hash_combine(seed, samplerDesc.magFilter);
	hash_combine(seed, samplerDesc.minFilter);
	hash_combine(seed, samplerDesc.mipFilter);
	hash_combine(seed, samplerDesc.addressModeU);
	hash_combine(seed, samplerDesc.addressModeV);
	hash_combine(seed, samplerDesc.addressModeW);

	hash_combine(seed, samplerDesc.mipLODBias);
	hash_combine(seed, samplerDesc.anisotropyEnabled);
	hash_combine(seed, samplerDesc.maxAnisotropy);

	hash_combine(seed, samplerDesc.borderColor);
	hash_combine(seed, samplerDesc.compareEnabled);
	hash_combine(seed, samplerDesc.compareOp);

	return seed;
}

bool CSamplerManager::SamplerDescEqual::operator()(const Sampler::Desc& lhs, const Sampler::Desc& rhs) const
{
	return
		lhs.magFilter == rhs.magFilter &&
		lhs.minFilter == rhs.minFilter &&
		lhs.mipFilter == rhs.mipFilter &&
		lhs.addressModeU == rhs.addressModeU &&
		lhs.addressModeV == rhs.addressModeV &&
		lhs.addressModeW == rhs.addressModeW &&
		lhs.mipLODBias == rhs.mipLODBias &&
		lhs.anisotropyEnabled == rhs.anisotropyEnabled &&
		lhs.maxAnisotropy == rhs.maxAnisotropy &&
		lhs.borderColor == rhs.borderColor &&
		lhs.compareEnabled == rhs.compareEnabled &&
		lhs.compareOp == rhs.compareOp;
}

CSamplerManager::CSamplerManager(CDevice* device)
	: m_Device(device)
{
}

CSamplerManager::~CSamplerManager()
{
	for (const auto& it : m_SamplerMap)
		if (it.second != VK_NULL_HANDLE)
		{
			m_Device->ScheduleObjectToDestroy(
				VK_OBJECT_TYPE_SAMPLER, it.second, VK_NULL_HANDLE);
		}
	m_SamplerMap.clear();
}

VkSampler CSamplerManager::GetOrCreateSampler(
	const Sampler::Desc& samplerDesc)
{
	auto it = m_SamplerMap.find(samplerDesc);
	if (it != m_SamplerMap.end())
		return it->second;

	const IDevice::Capabilities& capabilities = m_Device->GetCapabilities();

	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = samplerDesc.magFilter == Sampler::Filter::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
	samplerCreateInfo.minFilter = samplerDesc.minFilter == Sampler::Filter::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
	samplerCreateInfo.mipmapMode = samplerDesc.mipFilter == Sampler::Filter::LINEAR ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerCreateInfo.addressModeU = Mapping::FromAddressMode(samplerDesc.addressModeU);
	samplerCreateInfo.addressModeV = Mapping::FromAddressMode(samplerDesc.addressModeV);
	samplerCreateInfo.addressModeW = Mapping::FromAddressMode(samplerDesc.addressModeW);
	samplerCreateInfo.mipLodBias = samplerDesc.mipLODBias;
	if (samplerDesc.anisotropyEnabled && capabilities.anisotropicFiltering)
	{
		samplerCreateInfo.anisotropyEnable = VK_TRUE;
		samplerCreateInfo.maxAnisotropy = std::min(samplerDesc.maxAnisotropy, capabilities.maxAnisotropy);
	}
	samplerCreateInfo.compareEnable = samplerDesc.compareEnabled ? VK_TRUE : VK_FALSE;
	samplerCreateInfo.compareOp = Mapping::FromCompareOp(samplerDesc.compareOp);
	samplerCreateInfo.minLod = -1000.0f;
	samplerCreateInfo.maxLod = 1000.0f;
	switch (samplerDesc.borderColor)
	{
	case Sampler::BorderColor::TRANSPARENT_BLACK:
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		break;
	case Sampler::BorderColor::OPAQUE_BLACK:
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		break;
	case Sampler::BorderColor::OPAQUE_WHITE:
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		break;
	}

	VkSampler sampler = VK_NULL_HANDLE;
	ENSURE_VK_SUCCESS(vkCreateSampler(
		m_Device->GetVkDevice(), &samplerCreateInfo, nullptr, &sampler));
	it = m_SamplerMap.emplace(samplerDesc, sampler).first;

	return sampler;
}

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer
