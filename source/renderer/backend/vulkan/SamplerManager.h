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

#ifndef INCLUDED_RENDERER_BACKEND_VULKAN_SAMPLERMANAGER
#define INCLUDED_RENDERER_BACKEND_VULKAN_SAMPLERMANAGER

#include "renderer/backend/Sampler.h"

#include <glad/vulkan.h>
#include <memory>
#include <unordered_map>

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

class CDevice;

/**
 * A helper class to store unique samplers. The manager doesn't track usages of
 * its samplers but keep them alive until its end. So before destroying the
 * manager its owner should guarantee no usage.
 */
class CSamplerManager
{
public:
	CSamplerManager(CDevice* device);
	~CSamplerManager();

	/**
	 * @return a sampler matches the description. The returned sampler is owned by
	 * the manager.
	 */
	VkSampler GetOrCreateSampler(const Sampler::Desc& samplerDesc);

private:
	CDevice* m_Device = nullptr;

	struct SamplerDescHash
	{
		size_t operator()(const Sampler::Desc& samplerDesc) const;
	};
	struct SamplerDescEqual
	{
		bool operator()(const Sampler::Desc& lhs, const Sampler::Desc& rhs) const;
	};
	std::unordered_map<Sampler::Desc, VkSampler, SamplerDescHash, SamplerDescEqual> m_SamplerMap;
};

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_VULKAN_SAMPLERMANAGER
