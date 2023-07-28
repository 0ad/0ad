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

#ifndef INCLUDED_RENDERER_BACKEND_VULKAN_DEVICESELECTION
#define INCLUDED_RENDERER_BACKEND_VULKAN_DEVICESELECTION

#include "scriptinterface/ScriptForward.h"

#include <glad/vulkan.h>
#include <limits>
#include <vector>

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

/**
 * Structure to store all information that might be useful on device selection.
 */
struct SAvailablePhysicalDevice
{
	uint32_t index = std::numeric_limits<uint32_t>::max();
	VkPhysicalDevice device = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties properties{};
	VkPhysicalDeviceDescriptorIndexingPropertiesEXT descriptorIndexingProperties{};
	VkPhysicalDeviceMemoryProperties memoryProperties{};
	VkPhysicalDeviceFeatures features{};
	VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingFeatures{};
	std::vector<VkQueueFamilyProperties> queueFamilies;
	bool hasRequiredExtensions = false;
	bool hasOutputToSurfaceSupport = false;
	size_t graphicsQueueFamilyIndex = 0;
	size_t presentQueueFamilyIndex = 0;
	VkDeviceSize deviceTotalMemory = 0;
	VkDeviceSize hostTotalMemory = 0;
	std::vector<std::string> extensions;
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;
};

/**
 * @return all available physical devices for the Vulkan instance with
 * additional flags of surface and required extensions support.
 * We could have a single function that returns a selected device. But we use
 * multiple functions to be able to save some information about available
 * devices before filtering and give a choice to a user.
 */
std::vector<SAvailablePhysicalDevice> GetAvailablePhysicalDevices(
	VkInstance instance, VkSurfaceKHR surface,
	const std::vector<const char*>& requiredDeviceExtensions);

/**
 * @return true if we can't use the device for our needs. For example, it
 * doesn't graphics or present queues. Because we can't render the game without
 * them.
 */
bool IsPhysicalDeviceUnsupported(const SAvailablePhysicalDevice& device);

/**
 * @return true if the first device is better for our needs than the second
 * one. Useful in functions like std::sort. The first and the second devices
 * should be supported (in other words IsPhysicalDeviceSupported should
 * return true for both of them).
 */
bool ComparePhysicalDevices(
	const SAvailablePhysicalDevice& device1,
	const SAvailablePhysicalDevice& device2);

bool IsSurfaceFormatSupported(
	const VkSurfaceFormatKHR& surfaceFormat);

/**
 * Report all desired information about the available physical device.
 */
void ReportAvailablePhysicalDevice(const SAvailablePhysicalDevice& device,
	const ScriptRequest& rq, JS::HandleValue settings);

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_VULKAN_DEVICESELECTION
