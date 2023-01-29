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

#include "precompiled.h"

#include "DeviceSelection.h"

#include "lib/code_annotation.h"
#include "lib/config2.h"
#include "renderer/backend/vulkan/Device.h"
#include "renderer/backend/vulkan/Utilities.h"
#include "scriptinterface/JSON.h"
#include "scriptinterface/Object.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/ScriptRequest.h"

#include <algorithm>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

namespace
{

std::vector<std::string> GetPhysicalDeviceExtensions(VkPhysicalDevice device)
{
	uint32_t extensionCount = 0;
	ENSURE_VK_SUCCESS(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr));
	std::vector<VkExtensionProperties> extensions(extensionCount);
	ENSURE_VK_SUCCESS(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data()));

	std::vector<std::string> availableExtensions;
	availableExtensions.reserve(extensions.size());
	for (const VkExtensionProperties& extension : extensions)
		availableExtensions.emplace_back(extension.extensionName);
	std::sort(availableExtensions.begin(), availableExtensions.end());
	return availableExtensions;
}

uint32_t GetDeviceTypeScore(const VkPhysicalDeviceType deviceType)
{
	uint32_t score = 0;
	// We prefer discrete GPU over integrated, and integrated over others.
	switch (deviceType)
	{
	case VK_PHYSICAL_DEVICE_TYPE_OTHER:
		score = 1;
		break;
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		score = 4;
		break;
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		score = 5;
		break;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		score = 3;
		break;
	case VK_PHYSICAL_DEVICE_TYPE_CPU:
		score = 2;
		break;
	default:
		break;
	}
	return score;
}

VkDeviceSize GetDeviceTotalMemory(
	const VkPhysicalDeviceMemoryProperties& memoryProperties)
{
	VkDeviceSize totalMemory = 0;
	for (uint32_t heapIndex = 0; heapIndex < memoryProperties.memoryHeapCount; ++heapIndex)
		if (memoryProperties.memoryHeaps[heapIndex].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			totalMemory += memoryProperties.memoryHeaps[heapIndex].size;
	return totalMemory;
}

VkDeviceSize GetHostTotalMemory(
	const VkPhysicalDeviceMemoryProperties& memoryProperties)
{
	VkDeviceSize totalMemory = 0;
	for (uint32_t heapIndex = 0; heapIndex < memoryProperties.memoryHeapCount; ++heapIndex)
		if ((memoryProperties.memoryHeaps[heapIndex].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) == 0)
			totalMemory += memoryProperties.memoryHeaps[heapIndex].size;
	return totalMemory;
}

// We don't support some types in JS, so wrap them to have in the report.
template<typename T, typename Tag = void>
struct ReportFormatHelper
{
	std::string operator()(const T&) const { return "unknown"; }
};

template<typename T>
struct ReportFormatHelper<T, typename std::enable_if_t<std::is_floating_point_v<T>>>
{
	float operator()(const T& value) const { return static_cast<float>(value); }
};

template<typename T>
struct ReportFormatHelper<T, typename std::enable_if_t<std::is_integral_v<T>>>
{
	static constexpr bool IsSigned = std::is_signed_v<T>;
	using ResultType = std::conditional_t<IsSigned, int32_t, uint32_t>;
	uint32_t operator()(const T& value) const
	{
		if (value > std::numeric_limits<ResultType>::max())
			return std::numeric_limits<ResultType>::max();
		if constexpr (IsSigned)
		{
			if (value < std::numeric_limits<ResultType>::min())
				return std::numeric_limits<ResultType>::min();
		}
		return static_cast<ResultType>(value);
	}
};

template<typename T>
struct ReportFormatHelper<T, typename std::enable_if_t<std::is_enum_v<T>>>
{
	using HelperType = ReportFormatHelper<std::underlying_type_t<T>>;
	using ResultType = std::invoke_result_t<HelperType, std::underlying_type_t<T>>;
	ResultType operator()(const T& value) const
	{
		HelperType helper{};
		return helper(value);
	}
};

template<typename T>
struct ReportFormatHelper<T, typename std::enable_if_t<std::is_array_v<T>>>
{
	using HelperType = ReportFormatHelper<std::remove_extent_t<T>>;
	using ElementType = std::invoke_result_t<HelperType, std::remove_extent_t<T>>;
	std::vector<ElementType> operator()(const T& value) const
	{
		std::vector<ElementType> arr;
		arr.reserve(std::size(value));
		HelperType helper{};
		for (const auto& element : value)
			arr.emplace_back(helper(element));
		return arr;
	}
};

SAvailablePhysicalDevice MakeAvailablePhysicalDevice(
	const uint32_t physicalDeviceIndex, VkPhysicalDevice physicalDevice,
	VkSurfaceKHR surface, const std::vector<const char*>& requiredDeviceExtensions)
{
	SAvailablePhysicalDevice availablePhysicalDevice{};

	availablePhysicalDevice.index = physicalDeviceIndex;
	availablePhysicalDevice.device = physicalDevice;
	availablePhysicalDevice.hasOutputToSurfaceSupport = false;
	availablePhysicalDevice.extensions = GetPhysicalDeviceExtensions(availablePhysicalDevice.device);
	auto hasExtension = [&extensions = availablePhysicalDevice.extensions](const char* name) -> bool
	{
		return std::find(extensions.begin(), extensions.end(), name) != extensions.end();
	};

	availablePhysicalDevice.hasRequiredExtensions =
		std::all_of(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end(), hasExtension);

	vkGetPhysicalDeviceMemoryProperties(
		availablePhysicalDevice.device, &availablePhysicalDevice.memoryProperties);
	availablePhysicalDevice.deviceTotalMemory =
		GetDeviceTotalMemory(availablePhysicalDevice.memoryProperties);
	availablePhysicalDevice.hostTotalMemory =
		GetHostTotalMemory(availablePhysicalDevice.memoryProperties);

	if (hasExtension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME))
	{
		VkPhysicalDeviceDescriptorIndexingPropertiesEXT descriptorIndexingProperties{};
		descriptorIndexingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES_EXT;

		VkPhysicalDeviceProperties2 devicesProperties2{};
		devicesProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		devicesProperties2.pNext = &descriptorIndexingProperties;
		vkGetPhysicalDeviceProperties2(availablePhysicalDevice.device, &devicesProperties2);
		availablePhysicalDevice.properties = devicesProperties2.properties;
		availablePhysicalDevice.descriptorIndexingProperties = descriptorIndexingProperties;

		VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingFeatures{};
		descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;

		VkPhysicalDeviceFeatures2 deviceFeatures2{};
		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures2.pNext = &descriptorIndexingFeatures;
		vkGetPhysicalDeviceFeatures2(availablePhysicalDevice.device, &deviceFeatures2);
		availablePhysicalDevice.features = deviceFeatures2.features;
		availablePhysicalDevice.descriptorIndexingFeatures = descriptorIndexingFeatures;
	}
	else
	{
		vkGetPhysicalDeviceProperties(availablePhysicalDevice.device, &availablePhysicalDevice.properties);
		vkGetPhysicalDeviceFeatures(availablePhysicalDevice.device, &availablePhysicalDevice.features);
	}

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(availablePhysicalDevice.device, &queueFamilyCount, nullptr);
	availablePhysicalDevice.queueFamilies.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(
		availablePhysicalDevice.device, &queueFamilyCount, availablePhysicalDevice.queueFamilies.data());

	availablePhysicalDevice.graphicsQueueFamilyIndex = availablePhysicalDevice.queueFamilies.size();
	availablePhysicalDevice.presentQueueFamilyIndex = availablePhysicalDevice.queueFamilies.size();
	for (size_t familyIdx = 0; familyIdx < availablePhysicalDevice.queueFamilies.size(); ++familyIdx)
	{
		const VkQueueFamilyProperties& queueFamily = availablePhysicalDevice.queueFamilies[familyIdx];
		if (surface != VK_NULL_HANDLE)
		{
			VkBool32 hasOutputToSurfaceSupport = false;
			ENSURE_VK_SUCCESS(vkGetPhysicalDeviceSurfaceSupportKHR(
				availablePhysicalDevice.device, familyIdx, surface, &hasOutputToSurfaceSupport));
			availablePhysicalDevice.hasOutputToSurfaceSupport = hasOutputToSurfaceSupport;
			if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && hasOutputToSurfaceSupport)
			{
				availablePhysicalDevice.graphicsQueueFamilyIndex = familyIdx;
				availablePhysicalDevice.presentQueueFamilyIndex = familyIdx;
			}
		}
	}

	ENSURE_VK_SUCCESS(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		availablePhysicalDevice.device, surface, &availablePhysicalDevice.surfaceCapabilities));

	uint32_t surfaceFormatCount = 0;
	ENSURE_VK_SUCCESS(vkGetPhysicalDeviceSurfaceFormatsKHR(
		availablePhysicalDevice.device, surface, &surfaceFormatCount, nullptr));
	if (surfaceFormatCount > 0)
	{
		availablePhysicalDevice.surfaceFormats.resize(surfaceFormatCount);
		ENSURE_VK_SUCCESS(vkGetPhysicalDeviceSurfaceFormatsKHR(
			availablePhysicalDevice.device, surface, &surfaceFormatCount, availablePhysicalDevice.surfaceFormats.data()));
	}

	uint32_t presentModeCount = 0;
	ENSURE_VK_SUCCESS(vkGetPhysicalDeviceSurfacePresentModesKHR(
		availablePhysicalDevice.device, surface, &presentModeCount, nullptr));
	if (presentModeCount > 0)
	{
		availablePhysicalDevice.presentModes.resize(presentModeCount);
		ENSURE_VK_SUCCESS(vkGetPhysicalDeviceSurfacePresentModesKHR(
			availablePhysicalDevice.device, surface, &presentModeCount, availablePhysicalDevice.presentModes.data()));
	}

	return availablePhysicalDevice;
}

} // anonymous namespace

std::vector<SAvailablePhysicalDevice> GetAvailablePhysicalDevices(
	VkInstance instance, VkSurfaceKHR surface,
	const std::vector<const char*>& requiredDeviceExtensions)
{
	uint32_t physicalDeviceCount = 0;
	ENSURE_VK_SUCCESS(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr));
	if (physicalDeviceCount == 0)
		return {};

	std::vector<SAvailablePhysicalDevice> availablePhysicalDevices;
	availablePhysicalDevices.reserve(physicalDeviceCount);

	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	ENSURE_VK_SUCCESS(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()));
	for (uint32_t physicalDeviceIndex = 0; physicalDeviceIndex < physicalDeviceCount; ++physicalDeviceIndex)
	{
		availablePhysicalDevices.emplace_back(MakeAvailablePhysicalDevice(
			physicalDeviceIndex, physicalDevices[physicalDeviceIndex], surface, requiredDeviceExtensions));
	}

	return availablePhysicalDevices;
}

bool IsPhysicalDeviceUnsupported(const SAvailablePhysicalDevice& device)
{
	if (!device.hasRequiredExtensions)
		return true;
	// We can't draw something without graphics queue. And currently we don't
	// support separate queues for graphics and present.
	if (device.graphicsQueueFamilyIndex != device.presentQueueFamilyIndex)
		return true;
	if (device.graphicsQueueFamilyIndex == device.queueFamilies.size())
		return true;
	if (!device.hasOutputToSurfaceSupport)
		return true;
	if (device.properties.limits.maxBoundDescriptorSets < 4)
		return true;
	// It's guaranteed to have sRGB but we don't support it yet.
	return std::none_of(device.surfaceFormats.begin(), device.surfaceFormats.end(), IsSurfaceFormatSupported);
}

bool ComparePhysicalDevices(
	const SAvailablePhysicalDevice& device1,
	const SAvailablePhysicalDevice& device2)
{
	const uint32_t deviceTypeScore1 = GetDeviceTypeScore(device1.properties.deviceType);
	const uint32_t deviceTypeScore2 = GetDeviceTypeScore(device2.properties.deviceType);
	if (deviceTypeScore1 != deviceTypeScore2)
		return deviceTypeScore1 > deviceTypeScore2;
	// We use a total device memory amount to compare. We assume that more memory
	// means better performance as previous metrics are equal.
	return device1.deviceTotalMemory > device2.deviceTotalMemory;
}

bool IsSurfaceFormatSupported(
	const VkSurfaceFormatKHR& surfaceFormat)
{
	return
		surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
		(surfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM ||
		surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM);
}

void ReportAvailablePhysicalDevice(const SAvailablePhysicalDevice& device,
	const ScriptRequest& rq, JS::HandleValue settings)
{
	Script::SetProperty(rq, settings, "name", device.properties.deviceName);
	Script::SetProperty(rq, settings, "version",
		std::to_string(VK_API_VERSION_VARIANT(device.properties.apiVersion)) +
		"." + std::to_string(VK_API_VERSION_MAJOR(device.properties.apiVersion)) +
		"." + std::to_string(VK_API_VERSION_MINOR(device.properties.apiVersion)) +
		"." + std::to_string(VK_API_VERSION_PATCH(device.properties.apiVersion)));
	Script::SetProperty(rq, settings, "apiVersion", device.properties.apiVersion);
	Script::SetProperty(rq, settings, "driverVersion", device.properties.driverVersion);
	Script::SetProperty(rq, settings, "vendorID", device.properties.vendorID);
	Script::SetProperty(rq, settings, "deviceID", device.properties.deviceID);
	Script::SetProperty(rq, settings, "deviceType", static_cast<int32_t>(device.properties.deviceType));
	Script::SetProperty(rq, settings, "index", device.index);

	JS::RootedValue memory(rq.cx);
	Script::CreateObject(rq, &memory);

	JS::RootedValue memoryTypes(rq.cx);
	Script::CreateArray(rq, &memoryTypes, device.memoryProperties.memoryTypeCount);
	for (uint32_t memoryTypeIndex = 0; memoryTypeIndex < device.memoryProperties.memoryTypeCount; ++memoryTypeIndex)
	{
		const VkMemoryType& type = device.memoryProperties.memoryTypes[memoryTypeIndex];
		JS::RootedValue memoryType(rq.cx);
		Script::CreateObject(rq, &memoryType);
		Script::SetProperty(rq, memoryType, "propertyFlags", static_cast<uint32_t>(type.propertyFlags));
		Script::SetProperty(rq, memoryType, "heapIndex", type.heapIndex);
		Script::SetPropertyInt(rq, memoryTypes, memoryTypeIndex, memoryType);
	}
	JS::RootedValue memoryHeaps(rq.cx);
	Script::CreateArray(rq, &memoryHeaps, device.memoryProperties.memoryHeapCount);
	for (uint32_t memoryHeapIndex = 0; memoryHeapIndex < device.memoryProperties.memoryHeapCount; ++memoryHeapIndex)
	{
		const VkMemoryHeap& heap = device.memoryProperties.memoryHeaps[memoryHeapIndex];
		JS::RootedValue memoryHeap(rq.cx);
		Script::CreateObject(rq, &memoryHeap);
		// We can't serialize uint64_t in JS, so put data in KiB.
		Script::SetProperty(rq, memoryHeap, "size", static_cast<uint32_t>(heap.size / 1024));
		Script::SetProperty(rq, memoryHeap, "flags", static_cast<uint32_t>(heap.flags));
		Script::SetPropertyInt(rq, memoryHeaps, memoryHeapIndex, memoryHeap);
	}

	Script::SetProperty(rq, memory, "types", memoryTypes);
	Script::SetProperty(rq, memory, "heaps", memoryHeaps);
	Script::SetProperty(rq, settings, "memory", memory);

	JS::RootedValue constants(rq.cx);
	Script::CreateObject(rq, &constants);

	JS::RootedValue limitsConstants(rq.cx);
	Script::CreateObject(rq, &limitsConstants);
#define REPORT_LIMITS_CONSTANT(NAME) \
	do \
	{ \
		const ReportFormatHelper<decltype(device.properties.limits.NAME)> helper{}; \
		Script::SetProperty(rq, limitsConstants, #NAME, helper(device.properties.limits.NAME)); \
	} while (0)
	REPORT_LIMITS_CONSTANT(maxImageDimension1D);
	REPORT_LIMITS_CONSTANT(maxImageDimension2D);
	REPORT_LIMITS_CONSTANT(maxImageDimension3D);
	REPORT_LIMITS_CONSTANT(maxImageDimensionCube);
	REPORT_LIMITS_CONSTANT(maxImageArrayLayers);
	REPORT_LIMITS_CONSTANT(maxUniformBufferRange);
	REPORT_LIMITS_CONSTANT(maxStorageBufferRange);
	REPORT_LIMITS_CONSTANT(maxPushConstantsSize);
	REPORT_LIMITS_CONSTANT(maxMemoryAllocationCount);
	REPORT_LIMITS_CONSTANT(maxSamplerAllocationCount);
	REPORT_LIMITS_CONSTANT(bufferImageGranularity);
	REPORT_LIMITS_CONSTANT(maxBoundDescriptorSets);
	REPORT_LIMITS_CONSTANT(maxPerStageDescriptorSamplers);
	REPORT_LIMITS_CONSTANT(maxPerStageDescriptorUniformBuffers);
	REPORT_LIMITS_CONSTANT(maxPerStageDescriptorStorageBuffers);
	REPORT_LIMITS_CONSTANT(maxPerStageDescriptorSampledImages);
	REPORT_LIMITS_CONSTANT(maxPerStageDescriptorStorageImages);
	REPORT_LIMITS_CONSTANT(maxPerStageDescriptorInputAttachments);
	REPORT_LIMITS_CONSTANT(maxPerStageResources);
	REPORT_LIMITS_CONSTANT(maxDescriptorSetSamplers);
	REPORT_LIMITS_CONSTANT(maxDescriptorSetUniformBuffers);
	REPORT_LIMITS_CONSTANT(maxDescriptorSetUniformBuffersDynamic);
	REPORT_LIMITS_CONSTANT(maxDescriptorSetStorageBuffers);
	REPORT_LIMITS_CONSTANT(maxDescriptorSetStorageBuffersDynamic);
	REPORT_LIMITS_CONSTANT(maxDescriptorSetSampledImages);
	REPORT_LIMITS_CONSTANT(maxDescriptorSetStorageImages);
	REPORT_LIMITS_CONSTANT(maxDescriptorSetInputAttachments);
	REPORT_LIMITS_CONSTANT(maxVertexInputAttributes);
	REPORT_LIMITS_CONSTANT(maxVertexInputBindings);
	REPORT_LIMITS_CONSTANT(maxVertexInputAttributeOffset);
	REPORT_LIMITS_CONSTANT(maxVertexInputBindingStride);
	REPORT_LIMITS_CONSTANT(maxComputeSharedMemorySize);
	REPORT_LIMITS_CONSTANT(maxComputeWorkGroupCount);
	REPORT_LIMITS_CONSTANT(maxComputeWorkGroupInvocations);
	REPORT_LIMITS_CONSTANT(maxComputeWorkGroupSize);
	REPORT_LIMITS_CONSTANT(maxDrawIndexedIndexValue);
	REPORT_LIMITS_CONSTANT(maxSamplerLodBias);
	REPORT_LIMITS_CONSTANT(maxSamplerAnisotropy);
	REPORT_LIMITS_CONSTANT(minMemoryMapAlignment);
	REPORT_LIMITS_CONSTANT(minTexelBufferOffsetAlignment);
	REPORT_LIMITS_CONSTANT(minUniformBufferOffsetAlignment);
	REPORT_LIMITS_CONSTANT(minStorageBufferOffsetAlignment);
	REPORT_LIMITS_CONSTANT(maxFramebufferWidth);
	REPORT_LIMITS_CONSTANT(maxFramebufferHeight);
	REPORT_LIMITS_CONSTANT(maxFramebufferLayers);
	REPORT_LIMITS_CONSTANT(framebufferColorSampleCounts);
	REPORT_LIMITS_CONSTANT(framebufferDepthSampleCounts);
	REPORT_LIMITS_CONSTANT(framebufferStencilSampleCounts);
	REPORT_LIMITS_CONSTANT(framebufferNoAttachmentsSampleCounts);
	REPORT_LIMITS_CONSTANT(maxColorAttachments);
	REPORT_LIMITS_CONSTANT(sampledImageColorSampleCounts);
	REPORT_LIMITS_CONSTANT(sampledImageDepthSampleCounts);
	REPORT_LIMITS_CONSTANT(sampledImageStencilSampleCounts);
	REPORT_LIMITS_CONSTANT(storageImageSampleCounts);
	REPORT_LIMITS_CONSTANT(optimalBufferCopyOffsetAlignment);
	REPORT_LIMITS_CONSTANT(optimalBufferCopyRowPitchAlignment);
#undef REPORT_LIMITS_CONSTANT
	Script::SetProperty(rq, constants, "limits", limitsConstants);

	JS::RootedValue descriptorIndexingConstants(rq.cx);
	Script::CreateObject(rq, &descriptorIndexingConstants);
#define REPORT_DESCRIPTOR_INDEXING_CONSTANT(NAME) \
	do \
	{ \
		const ReportFormatHelper<decltype(device.descriptorIndexingProperties.NAME)> helper{}; \
		Script::SetProperty(rq, descriptorIndexingConstants, #NAME, helper(device.descriptorIndexingProperties.NAME)); \
	} while (0)
	REPORT_DESCRIPTOR_INDEXING_CONSTANT(maxUpdateAfterBindDescriptorsInAllPools);
	REPORT_DESCRIPTOR_INDEXING_CONSTANT(shaderSampledImageArrayNonUniformIndexingNative);
	REPORT_DESCRIPTOR_INDEXING_CONSTANT(maxPerStageDescriptorUpdateAfterBindSamplers);
	REPORT_DESCRIPTOR_INDEXING_CONSTANT(maxPerStageDescriptorUpdateAfterBindSampledImages);
	REPORT_DESCRIPTOR_INDEXING_CONSTANT(maxPerStageDescriptorUpdateAfterBindUniformBuffers);
	REPORT_DESCRIPTOR_INDEXING_CONSTANT(maxPerStageUpdateAfterBindResources);
	REPORT_DESCRIPTOR_INDEXING_CONSTANT(maxDescriptorSetUpdateAfterBindSamplers);
	REPORT_DESCRIPTOR_INDEXING_CONSTANT(maxDescriptorSetUpdateAfterBindSampledImages);
	REPORT_DESCRIPTOR_INDEXING_CONSTANT(maxDescriptorSetUpdateAfterBindUniformBuffers);
	REPORT_DESCRIPTOR_INDEXING_CONSTANT(maxDescriptorSetUpdateAfterBindUniformBuffersDynamic);
#undef REPORT_DESCRIPTOR_INDEXING_CONSTANT
	Script::SetProperty(rq, constants, "descriptor_indexing", descriptorIndexingConstants);

	Script::SetProperty(rq, settings, "constants", constants);

	JS::RootedValue features(rq.cx);
	Script::CreateObject(rq, &features);
#define REPORT_FEATURE(NAME) \
	Script::SetProperty(rq, features, #NAME, static_cast<bool>(device.features.NAME));
	REPORT_FEATURE(robustBufferAccess);
	REPORT_FEATURE(fullDrawIndexUint32);
	REPORT_FEATURE(imageCubeArray);
	REPORT_FEATURE(geometryShader);
	REPORT_FEATURE(tessellationShader);
	REPORT_FEATURE(logicOp);
	REPORT_FEATURE(multiDrawIndirect);
	REPORT_FEATURE(depthClamp);
	REPORT_FEATURE(depthBiasClamp);
	REPORT_FEATURE(fillModeNonSolid);
	REPORT_FEATURE(samplerAnisotropy);
	REPORT_FEATURE(textureCompressionETC2);
	REPORT_FEATURE(textureCompressionASTC_LDR);
	REPORT_FEATURE(textureCompressionBC);
	REPORT_FEATURE(pipelineStatisticsQuery);
	REPORT_FEATURE(shaderUniformBufferArrayDynamicIndexing);
	REPORT_FEATURE(shaderSampledImageArrayDynamicIndexing);
#undef REPORT_FEATURE

#define REPORT_DESCRIPTOR_INDEXING_FEATURE(NAME) \
	Script::SetProperty(rq, features, #NAME, static_cast<bool>(device.descriptorIndexingFeatures.NAME));
	REPORT_DESCRIPTOR_INDEXING_FEATURE(shaderSampledImageArrayNonUniformIndexing);
	REPORT_DESCRIPTOR_INDEXING_FEATURE(descriptorBindingUniformBufferUpdateAfterBind);
	REPORT_DESCRIPTOR_INDEXING_FEATURE(descriptorBindingSampledImageUpdateAfterBind);
	REPORT_DESCRIPTOR_INDEXING_FEATURE(descriptorBindingPartiallyBound);
	REPORT_DESCRIPTOR_INDEXING_FEATURE(descriptorBindingUpdateUnusedWhilePending);
	REPORT_DESCRIPTOR_INDEXING_FEATURE(descriptorBindingPartiallyBound);
	REPORT_DESCRIPTOR_INDEXING_FEATURE(descriptorBindingVariableDescriptorCount);
	REPORT_DESCRIPTOR_INDEXING_FEATURE(runtimeDescriptorArray);
#undef REPORT_DESCRIPTOR_INDEXING_FEATURE

	Script::SetProperty(rq, settings, "features", features);

	JS::RootedValue presentModes(rq.cx);
	Script::CreateArray(rq, &presentModes, device.presentModes.size());
	for (size_t index = 0; index < device.presentModes.size(); ++index)
	{
		Script::SetPropertyInt(
			rq, presentModes, index, static_cast<uint32_t>(device.presentModes[index]));
	}
	Script::SetProperty(rq, settings, "present_modes", presentModes);

	JS::RootedValue surfaceFormats(rq.cx);
	Script::CreateArray(rq, &surfaceFormats, device.surfaceFormats.size());
	for (size_t index = 0; index < device.surfaceFormats.size(); ++index)
	{
		JS::RootedValue surfaceFormat(rq.cx);
		Script::CreateObject(rq, &surfaceFormat);
		Script::SetProperty(
			rq, surfaceFormat, "format", static_cast<uint32_t>(device.surfaceFormats[index].format));
		Script::SetProperty(
			rq, surfaceFormat, "color_space", static_cast<uint32_t>(device.surfaceFormats[index].colorSpace));
		Script::SetPropertyInt(rq, surfaceFormats, index, surfaceFormat);
	}
	Script::SetProperty(rq, settings, "surface_formats", surfaceFormats);

	JS::RootedValue surfaceCapabilities(rq.cx);
	Script::CreateObject(rq, &surfaceCapabilities);
#define REPORT_SURFACE_CAPABILITIES_CONSTANT(NAME) \
	do \
	{ \
		const ReportFormatHelper<decltype(device.surfaceCapabilities.NAME)> helper{}; \
		Script::SetProperty(rq, surfaceCapabilities, #NAME, helper(device.surfaceCapabilities.NAME)); \
	} while (0)
	REPORT_SURFACE_CAPABILITIES_CONSTANT(minImageCount);
	REPORT_SURFACE_CAPABILITIES_CONSTANT(maxImageCount);
	REPORT_SURFACE_CAPABILITIES_CONSTANT(maxImageArrayLayers);
	REPORT_SURFACE_CAPABILITIES_CONSTANT(supportedTransforms);
	REPORT_SURFACE_CAPABILITIES_CONSTANT(supportedCompositeAlpha);
	REPORT_SURFACE_CAPABILITIES_CONSTANT(supportedUsageFlags);
#undef REPORT_SURFACE_CAPABILITIES_CONSTANT
	Script::SetProperty(rq, settings, "surface_capabilities", surfaceCapabilities);
}

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer
