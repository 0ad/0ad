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

#include "Device.h"

#include "lib/external_libraries/libsdl.h"
#include "lib/hash.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/Profile.h"
#include "renderer/backend/vulkan/Buffer.h"
#include "renderer/backend/vulkan/DescriptorManager.h"
#include "renderer/backend/vulkan/DeviceCommandContext.h"
#include "renderer/backend/vulkan/DeviceSelection.h"
#include "renderer/backend/vulkan/Framebuffer.h"
#include "renderer/backend/vulkan/Mapping.h"
#include "renderer/backend/vulkan/PipelineState.h"
#include "renderer/backend/vulkan/RenderPassManager.h"
#include "renderer/backend/vulkan/RingCommandContext.h"
#include "renderer/backend/vulkan/SamplerManager.h"
#include "renderer/backend/vulkan/ShaderProgram.h"
#include "renderer/backend/vulkan/SubmitScheduler.h"
#include "renderer/backend/vulkan/SwapChain.h"
#include "renderer/backend/vulkan/Texture.h"
#include "renderer/backend/vulkan/Utilities.h"
#include "scriptinterface/JSON.h"
#include "scriptinterface/Object.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/ScriptRequest.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// According to https://wiki.libsdl.org/SDL_Vulkan_LoadLibrary the following
// functionality is supported since SDL 2.0.6.
#if SDL_VERSION_ATLEAST(2, 0, 6)
#include <SDL_vulkan.h>
#endif

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

namespace
{

std::vector<const char*> GetRequiredSDLExtensions(SDL_Window* window)
{
	if (!window)
		return {};

	const size_t MAX_EXTENSION_COUNT = 16;
	unsigned int SDLExtensionCount = MAX_EXTENSION_COUNT;
	const char* SDLExtensions[MAX_EXTENSION_COUNT];
	ENSURE(SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionCount, SDLExtensions));
	std::vector<const char*> requiredExtensions;
	requiredExtensions.reserve(SDLExtensionCount);
	std::copy_n(SDLExtensions, SDLExtensionCount, std::back_inserter(requiredExtensions));
	return requiredExtensions;
}

std::vector<std::string> GetAvailableValidationLayers()
{
	uint32_t layerCount = 0;
	ENSURE_VK_SUCCESS(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

	std::vector<VkLayerProperties> availableLayers(layerCount);
	ENSURE_VK_SUCCESS(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

	for (const VkLayerProperties& layer : availableLayers)
	{
		LOGMESSAGE("Vulkan validation layer: '%s' (%s) v%u.%u.%u.%u",
			layer.layerName, layer.description,
			VK_API_VERSION_VARIANT(layer.specVersion),
			VK_API_VERSION_MAJOR(layer.specVersion),
			VK_API_VERSION_MINOR(layer.specVersion),
			VK_API_VERSION_PATCH(layer.specVersion));
	}

	std::vector<std::string> availableValidationLayers;
	availableValidationLayers.reserve(layerCount);
	for (const VkLayerProperties& layer : availableLayers)
		availableValidationLayers.emplace_back(layer.layerName);
	return availableValidationLayers;
}

std::vector<std::string> GetAvailableInstanceExtensions(const char* layerName = nullptr)
{
	uint32_t extensionCount = 0;
	ENSURE_VK_SUCCESS(vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr));
	std::vector<VkExtensionProperties> extensions(extensionCount);
	ENSURE_VK_SUCCESS(vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, extensions.data()));

	std::vector<std::string> availableExtensions;
	for (const VkExtensionProperties& extension : extensions)
		availableExtensions.emplace_back(extension.extensionName);
	return availableExtensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
	void* UNUSED(userData))
{
	if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) || (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT))
		LOGMESSAGE("Vulkan: %s", callbackData->pMessage);
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		struct HideRule
		{
			VkDebugUtilsMessageTypeFlagsEXT flags;
			std::string_view pattern;
			bool skip;
		};
		constexpr HideRule hideRules[] =
		{
			// Not consumed shader output is a known problem which produces too
			// many warning.
			{VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT, "OutputNotConsumed", false},
			// TODO: check vkGetImageMemoryRequirements2 for prefersDedicatedAllocation.
			{VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT, "vkBindMemory-small-dedicated-allocation", false},
			{VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT, "vkAllocateMemory-small-allocation", false},
			// We have some unnecessary clears which were needed for GL.
			// Ignore message for now, because they're spawned each frame.
			{VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT, "ClearCmdBeforeDraw", true},
			{VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT, "vkCmdClearAttachments-clear-after-load", true},
			// TODO: investigate probably false-positive report.
			{VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT, "vkCmdBeginRenderPass-StoreOpDontCareThenLoadOpLoad", true},
		};
		const auto it = std::find_if(std::begin(hideRules), std::end(hideRules),
			[messageType, message = std::string_view{callbackData->pMessage}](const HideRule& hideRule) -> bool
			{
				return (hideRule.flags & messageType) && message.find(hideRule.pattern) != std::string_view::npos;
			});
		if (it == std::end(hideRules))
			LOGWARNING("Vulkan: %s", callbackData->pMessage);
		else if (!it->skip)
			LOGMESSAGE("Vulkan: %s", callbackData->pMessage);
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		LOGERROR("Vulkan: %s", callbackData->pMessage);

	return VK_FALSE;
}

// A workaround function to meet calling conventions of Vulkan, SDL and GLAD.
GLADapiproc GetInstanceProcAddr(VkInstance instance, const char* name)
{
#if SDL_VERSION_ATLEAST(2, 0, 6)
	PFN_vkGetInstanceProcAddr function = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
	return reinterpret_cast<GLADapiproc>(function(instance, name));
#else
	return nullptr;
#endif
}

} // anonymous namespace

// static
std::unique_ptr<CDevice> CDevice::Create(SDL_Window* window)
{
	if (!window)
	{
		LOGERROR("Can't create Vulkan device without window.");
		return nullptr;
	}

	GLADuserptrloadfunc gladLoadFunction = reinterpret_cast<GLADuserptrloadfunc>(GetInstanceProcAddr);

	std::unique_ptr<CDevice> device(new CDevice());
	device->m_Window = window;

#ifdef NDEBUG
	bool enableDebugMessages = false;
	CFG_GET_VAL("renderer.backend.debugmessages", enableDebugMessages);
	bool enableDebugLabels = false;
	CFG_GET_VAL("renderer.backend.debuglabels", enableDebugLabels);
	bool enableDebugScopedLabels = false;
	CFG_GET_VAL("renderer.backend.debugscopedlabels", enableDebugScopedLabels);
#else
	bool enableDebugMessages = true;
	bool enableDebugLabels = true;
	bool enableDebugScopedLabels = true;
#endif

	int gladVulkanVersion = gladLoadVulkanUserPtr(nullptr, gladLoadFunction, nullptr);
	if (!gladVulkanVersion)
	{
		LOGERROR("GLAD unable to load vulkan.");
		return nullptr;
	}

	VkApplicationInfo applicationInfo{};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pApplicationName = "0 A.D.";
	applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 27);
	applicationInfo.pEngineName = "Pyrogenesis";
	applicationInfo.engineVersion = applicationInfo.applicationVersion;
	applicationInfo.apiVersion = VK_API_VERSION_1_1;

	std::vector<const char*> requiredInstanceExtensions = GetRequiredSDLExtensions(window);

	device->m_ValidationLayers = GetAvailableValidationLayers();
	auto hasValidationLayer = [&layers = device->m_ValidationLayers](const char* name) -> bool
	{
		return std::find(layers.begin(), layers.end(), name) != layers.end();
	};
	device->m_InstanceExtensions = GetAvailableInstanceExtensions();
	auto hasInstanceExtension = [&extensions = device->m_InstanceExtensions](const char* name) -> bool
	{
		return std::find(extensions.begin(), extensions.end(), name) != extensions.end();
	};

#ifdef NDEBUG
	bool enableDebugContext = false;
	CFG_GET_VAL("renderer.backend.debugcontext", enableDebugContext);
#else
	bool enableDebugContext = true;
#endif

	if (!hasInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
		enableDebugMessages = enableDebugLabels = enableDebugScopedLabels = false;
	const bool enableDebugLayers = enableDebugContext || enableDebugMessages || enableDebugLabels || enableDebugScopedLabels;
	if (enableDebugLayers)
		requiredInstanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	std::vector<const char*> requestedValidationLayers;
	const bool enableValidationFeatures = enableDebugMessages && hasValidationLayer("VK_LAYER_KHRONOS_validation");
	if (enableValidationFeatures)
		requestedValidationLayers.emplace_back("VK_LAYER_KHRONOS_validation");

	// https://github.com/KhronosGroup/Vulkan-ValidationLayers/blob/master/docs/synchronization_usage.md
	VkValidationFeatureEnableEXT validationFeatureEnables[] =
	{
		VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
		VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
	};
	VkValidationFeaturesEXT	validationFeatures{};
	validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
	validationFeatures.enabledValidationFeatureCount = std::size(validationFeatureEnables);
	validationFeatures.pEnabledValidationFeatures = validationFeatureEnables;

	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;
	instanceCreateInfo.enabledExtensionCount = requiredInstanceExtensions.size();
	instanceCreateInfo.ppEnabledExtensionNames = requiredInstanceExtensions.data();
	if (requestedValidationLayers.empty())
	{
		instanceCreateInfo.enabledLayerCount = 0;
		instanceCreateInfo.ppEnabledLayerNames = nullptr;
	}
	else
	{
		instanceCreateInfo.enabledLayerCount = requestedValidationLayers.size();
		instanceCreateInfo.ppEnabledLayerNames = requestedValidationLayers.data();
	}

	// Enabling validation features might significantly reduce performance,
	// even more than the standard validation layer.
	if (enableValidationFeatures && enableDebugContext)
	{
		instanceCreateInfo.pNext = &validationFeatures;
	}

	const VkResult createInstanceResult = vkCreateInstance(&instanceCreateInfo, nullptr, &device->m_Instance);
	if (createInstanceResult != VK_SUCCESS)
	{
		if (createInstanceResult == VK_ERROR_INCOMPATIBLE_DRIVER)
			LOGERROR("Can't create Vulkan instance: incompatible driver.");
		else if (createInstanceResult == VK_ERROR_EXTENSION_NOT_PRESENT)
			LOGERROR("Can't create Vulkan instance: extension not present.");
		else if (createInstanceResult == VK_ERROR_LAYER_NOT_PRESENT)
			LOGERROR("Can't create Vulkan instance: layer not present.");
		else
			LOGERROR("Unknown error during Vulkan instance creation: %d", static_cast<int>(createInstanceResult));
		return nullptr;
	}

	gladVulkanVersion = gladLoadVulkanUserPtr(nullptr, gladLoadFunction, device->m_Instance);
	if (!gladVulkanVersion)
	{
		LOGERROR("GLAD unable to re-load vulkan after its instance creation.");
		return nullptr;
	}

	if (GLAD_VK_EXT_debug_utils && enableDebugMessages)
	{
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = DebugCallback;
		debugCreateInfo.pUserData = nullptr;

		ENSURE_VK_SUCCESS(vkCreateDebugUtilsMessengerEXT(
			device->m_Instance, &debugCreateInfo, nullptr, &device->m_DebugMessenger));
	}

	if (window)
		ENSURE(SDL_Vulkan_CreateSurface(window, device->m_Instance, &device->m_Surface));

	const std::vector<const char*> requiredDeviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	std::vector<SAvailablePhysicalDevice> availablePhyscialDevices =
		GetAvailablePhysicalDevices(device->m_Instance, device->m_Surface, requiredDeviceExtensions);
	for (const SAvailablePhysicalDevice& device : availablePhyscialDevices)
	{
		LOGMESSAGE("Vulkan available device: '%s' Type: %u Supported: %c",
			device.properties.deviceName, static_cast<uint32_t>(device.properties.deviceType),
			IsPhysicalDeviceUnsupported(device) ? 'N' : 'Y');
		LOGMESSAGE("  ID: %u VendorID: %u API Version: %u Driver Version: %u",
			device.properties.deviceID, device.properties.vendorID,
			device.properties.apiVersion, device.properties.driverVersion);
		for (uint32_t memoryTypeIndex = 0; memoryTypeIndex < device.memoryProperties.memoryTypeCount; ++memoryTypeIndex)
		{
			const VkMemoryType& type = device.memoryProperties.memoryTypes[memoryTypeIndex];
			LOGMESSAGE("  Memory Type Index: %u Flags: %u Heap Index: %u",
				memoryTypeIndex, static_cast<uint32_t>(type.propertyFlags), type.heapIndex);
		}
		for (uint32_t memoryHeapIndex = 0; memoryHeapIndex < device.memoryProperties.memoryHeapCount; ++memoryHeapIndex)
		{
			const VkMemoryHeap& heap = device.memoryProperties.memoryHeaps[memoryHeapIndex];
			LOGMESSAGE("  Memory Heap Index: %u Size: %zu Flags: %u",
				memoryHeapIndex, static_cast<size_t>(heap.size / 1024), static_cast<uint32_t>(heap.flags));
		}
	}
	device->m_AvailablePhysicalDevices = availablePhyscialDevices;
	// We need to remove unsupported devices first.
	availablePhyscialDevices.erase(
		std::remove_if(
			availablePhyscialDevices.begin(), availablePhyscialDevices.end(),
			IsPhysicalDeviceUnsupported),
		availablePhyscialDevices.end());
	if (availablePhyscialDevices.empty())
	{
		LOGERROR("Vulkan can not find any supported and suitable device.");
		return nullptr;
	}

	int deviceIndexOverride = -1;
	CFG_GET_VAL("renderer.backend.vulkan.deviceindexoverride", deviceIndexOverride);
	auto choosedDeviceIt = device->m_AvailablePhysicalDevices.end();
	if (deviceIndexOverride >= 0)
	{
		choosedDeviceIt = std::find_if(
			device->m_AvailablePhysicalDevices.begin(), device->m_AvailablePhysicalDevices.end(),
			[deviceIndexOverride](const SAvailablePhysicalDevice& availableDevice)
			{
				return availableDevice.index == static_cast<uint32_t>(deviceIndexOverride);
			});
		if (choosedDeviceIt == device->m_AvailablePhysicalDevices.end())
			LOGWARNING("Device with override index %d not found.", deviceIndexOverride);
	}
	if (choosedDeviceIt == device->m_AvailablePhysicalDevices.end())
	{
		// We need to choose the best available device fits our needs.
		choosedDeviceIt = min_element(
			availablePhyscialDevices.begin(), availablePhyscialDevices.end(),
			ComparePhysicalDevices);
	}
	device->m_ChoosenDevice = *choosedDeviceIt;
	const SAvailablePhysicalDevice& choosenDevice = device->m_ChoosenDevice;
	device->m_AvailablePhysicalDevices.erase(std::remove_if(
		device->m_AvailablePhysicalDevices.begin(), device->m_AvailablePhysicalDevices.end(),
		[physicalDevice = choosenDevice.device](const SAvailablePhysicalDevice& device)
		{
			return physicalDevice == device.device;
		}), device->m_AvailablePhysicalDevices.end());

	gladVulkanVersion = gladLoadVulkanUserPtr(choosenDevice.device, gladLoadFunction, device->m_Instance);
	if (!gladVulkanVersion)
	{
		LOGERROR("GLAD unable to re-load vulkan after choosing its physical device.");
		return nullptr;
	}

	auto hasDeviceExtension = [&extensions = choosenDevice.extensions](const char* name) -> bool
	{
		return std::find(extensions.begin(), extensions.end(), name) != extensions.end();
	};
	const bool hasDescriptorIndexing = hasDeviceExtension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
	const bool hasNeededDescriptorIndexingFeatures =
		hasDescriptorIndexing &&
		choosenDevice.descriptorIndexingProperties.maxUpdateAfterBindDescriptorsInAllPools >= 65536 &&
		choosenDevice.descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing &&
		choosenDevice.descriptorIndexingFeatures.runtimeDescriptorArray &&
		choosenDevice.descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount &&
		choosenDevice.descriptorIndexingFeatures.descriptorBindingPartiallyBound &&
		choosenDevice.descriptorIndexingFeatures.descriptorBindingUpdateUnusedWhilePending &&
		choosenDevice.descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind;

	std::vector<const char*> deviceExtensions = requiredDeviceExtensions;
	if (hasDescriptorIndexing)
		deviceExtensions.emplace_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

	device->m_GraphicsQueueFamilyIndex = choosenDevice.graphicsQueueFamilyIndex;
	const std::array<size_t, 1> queueFamilyIndices{{
		choosenDevice.graphicsQueueFamilyIndex
	}};

	PS::StaticVector<VkDeviceQueueCreateInfo, 1> queueCreateInfos;
	const float queuePriority = 1.0f;
	std::transform(queueFamilyIndices.begin(), queueFamilyIndices.end(),
		std::back_inserter(queueCreateInfos),
		[&queuePriority](const size_t queueFamilyIndex)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
			return queueCreateInfo;
		});

	// https://github.com/KhronosGroup/Vulkan-Guide/blob/master/chapters/enabling_features.adoc
	VkPhysicalDeviceFeatures deviceFeatures{};
	VkPhysicalDeviceFeatures2 deviceFeatures2{};
	VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingFeatures{};

	deviceFeatures.textureCompressionBC = choosenDevice.features.textureCompressionBC;
	deviceFeatures.samplerAnisotropy = choosenDevice.features.samplerAnisotropy;

	descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
	descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing =
		choosenDevice.descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing;
	descriptorIndexingFeatures.runtimeDescriptorArray =
		choosenDevice.descriptorIndexingFeatures.runtimeDescriptorArray;
	descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount =
		choosenDevice.descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount;
	descriptorIndexingFeatures.descriptorBindingPartiallyBound =
		choosenDevice.descriptorIndexingFeatures.descriptorBindingPartiallyBound;
	descriptorIndexingFeatures.descriptorBindingUpdateUnusedWhilePending =
		choosenDevice.descriptorIndexingFeatures.descriptorBindingUpdateUnusedWhilePending;
	descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind =
		choosenDevice.descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind;

	deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures2.features = deviceFeatures;
	if (hasNeededDescriptorIndexingFeatures)
		deviceFeatures2.pNext = &descriptorIndexingFeatures;

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
	deviceCreateInfo.pEnabledFeatures = nullptr;
	deviceCreateInfo.pNext = &deviceFeatures2;
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = nullptr;

	const VkResult createDeviceResult = vkCreateDevice(
		choosenDevice.device, &deviceCreateInfo, nullptr, &device->m_Device);
	if (createDeviceResult != VK_SUCCESS)
	{
		if (createDeviceResult == VK_ERROR_FEATURE_NOT_PRESENT)
			LOGERROR("Can't create Vulkan device: feature not present.");
		else if (createDeviceResult == VK_ERROR_EXTENSION_NOT_PRESENT)
			LOGERROR("Can't create Vulkan device: extension not present.");
		else
			LOGERROR("Unknown error during Vulkan device creation: %d",
				static_cast<int>(createDeviceResult));
		return nullptr;
	}

	VmaVulkanFunctions vulkanFunctions{};
	vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
	vulkanFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
	vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
	vulkanFunctions.vkAllocateMemory = vkAllocateMemory;
	vulkanFunctions.vkFreeMemory = vkFreeMemory;
	vulkanFunctions.vkMapMemory = vkMapMemory;
	vulkanFunctions.vkUnmapMemory = vkUnmapMemory;
	vulkanFunctions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
	vulkanFunctions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
	vulkanFunctions.vkBindBufferMemory = vkBindBufferMemory;
	vulkanFunctions.vkBindImageMemory = vkBindImageMemory;
	vulkanFunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
	vulkanFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
	vulkanFunctions.vkCreateBuffer = vkCreateBuffer;
	vulkanFunctions.vkDestroyBuffer = vkDestroyBuffer;
	vulkanFunctions.vkCreateImage = vkCreateImage;
	vulkanFunctions.vkDestroyImage = vkDestroyImage;
	vulkanFunctions.vkCmdCopyBuffer = vkCmdCopyBuffer;

	VmaAllocatorCreateInfo allocatorCreateInfo{};
	allocatorCreateInfo.instance = device->m_Instance;
	allocatorCreateInfo.physicalDevice = choosenDevice.device;
	allocatorCreateInfo.device = device->m_Device;
	allocatorCreateInfo.vulkanApiVersion = applicationInfo.apiVersion;
	allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
	const VkResult createVMAAllocatorResult =
		vmaCreateAllocator(&allocatorCreateInfo, &device->m_VMAAllocator);
	if (createVMAAllocatorResult != VK_SUCCESS)
	{
		LOGERROR("Failed to create VMA allocator: %d",
			static_cast<int>(createDeviceResult));
		return nullptr;
	}

	// We need to use VK_SHARING_MODE_CONCURRENT if we have graphics and present
	// in different queues.
	vkGetDeviceQueue(device->m_Device, choosenDevice.graphicsQueueFamilyIndex,
		0, &device->m_GraphicsQueue);
	ENSURE(device->m_GraphicsQueue != VK_NULL_HANDLE);

	Capabilities& capabilities = device->m_Capabilities;

	capabilities.debugLabels = enableDebugLabels;
	capabilities.debugScopedLabels = enableDebugScopedLabels;
	capabilities.S3TC = choosenDevice.features.textureCompressionBC;
	capabilities.ARBShaders = false;
	capabilities.ARBShadersShadow = false;
	capabilities.computeShaders = true;
	capabilities.instancing = true;
	capabilities.maxSampleCount = 1;
	const VkSampleCountFlags sampleCountFlags =
		choosenDevice.properties.limits.framebufferColorSampleCounts
		& choosenDevice.properties.limits.framebufferDepthSampleCounts
		& choosenDevice.properties.limits.framebufferStencilSampleCounts;
	const std::array<VkSampleCountFlagBits, 5> allowedSampleCountBits =
	{
		VK_SAMPLE_COUNT_1_BIT,
		VK_SAMPLE_COUNT_2_BIT,
		VK_SAMPLE_COUNT_4_BIT,
		VK_SAMPLE_COUNT_8_BIT,
		VK_SAMPLE_COUNT_16_BIT,
	};
	for (size_t index = 0; index < allowedSampleCountBits.size(); ++index)
		if (sampleCountFlags & allowedSampleCountBits[index])
			device->m_Capabilities.maxSampleCount = 1u << index;
	capabilities.multisampling = device->m_Capabilities.maxSampleCount > 1;
	capabilities.anisotropicFiltering = choosenDevice.features.samplerAnisotropy;
	capabilities.maxAnisotropy = choosenDevice.properties.limits.maxSamplerAnisotropy;
	capabilities.maxTextureSize =
		choosenDevice.properties.limits.maxImageDimension2D;

	device->m_RenderPassManager =
		std::make_unique<CRenderPassManager>(device.get());
	device->m_SamplerManager = std::make_unique<CSamplerManager>(device.get());
	device->m_SubmitScheduler =
		std::make_unique<CSubmitScheduler>(
			device.get(), device->m_GraphicsQueueFamilyIndex, device->m_GraphicsQueue);

	bool disableDescriptorIndexing = false;
	CFG_GET_VAL("renderer.backend.vulkan.disabledescriptorindexing", disableDescriptorIndexing);
	const bool useDescriptorIndexing = hasNeededDescriptorIndexingFeatures && !disableDescriptorIndexing;
	device->m_DescriptorManager =
		std::make_unique<CDescriptorManager>(device.get(), useDescriptorIndexing);

	device->RecreateSwapChain();

	device->m_Name = choosenDevice.properties.deviceName;
	device->m_Version =
		std::to_string(VK_API_VERSION_VARIANT(choosenDevice.properties.apiVersion)) +
		"." + std::to_string(VK_API_VERSION_MAJOR(choosenDevice.properties.apiVersion)) +
		"." + std::to_string(VK_API_VERSION_MINOR(choosenDevice.properties.apiVersion)) +
		"." + std::to_string(VK_API_VERSION_PATCH(choosenDevice.properties.apiVersion));

	device->m_DriverInformation = std::to_string(choosenDevice.properties.driverVersion);

	// Refs:
	// * https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceProperties.html
	// * https://pcisig.com/membership/member-companies
	device->m_VendorID = std::to_string(choosenDevice.properties.vendorID);

	device->m_Extensions = choosenDevice.extensions;

	return device;
}

CDevice::CDevice() = default;

CDevice::~CDevice()
{
	if (m_Device)
		vkDeviceWaitIdle(m_Device);

	// The order of destroying does matter to avoid use-after-free and validation
	// layers complaints.

	m_SubmitScheduler.reset();

	ProcessTextureToDestroyQueue(true);

	m_RenderPassManager.reset();
	m_SamplerManager.reset();
	m_DescriptorManager.reset();
	m_SwapChain.reset();

	ProcessObjectToDestroyQueue(true);

	if (m_VMAAllocator != VK_NULL_HANDLE)
		vmaDestroyAllocator(m_VMAAllocator);

	if (m_Device != VK_NULL_HANDLE)
		vkDestroyDevice(m_Device, nullptr);

	if (m_Surface != VK_NULL_HANDLE)
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

	if (GLAD_VK_EXT_debug_utils && m_DebugMessenger)
		vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);

	if (m_Instance != VK_NULL_HANDLE)
		vkDestroyInstance(m_Instance, nullptr);
}

void CDevice::Report(const ScriptRequest& rq, JS::HandleValue settings)
{
	Script::SetProperty(rq, settings, "name", "vulkan");

	Script::SetProperty(rq, settings, "extensions", m_Extensions);

	JS::RootedValue device(rq.cx);
	Script::CreateObject(rq, &device);
	ReportAvailablePhysicalDevice(m_ChoosenDevice, rq, device);
	Script::SetProperty(rq, settings, "choosen_device", device);

	JS::RootedValue availableDevices(rq.cx);
	Script::CreateArray(rq, &availableDevices, m_AvailablePhysicalDevices.size());
	for (size_t index = 0; index < m_AvailablePhysicalDevices.size(); ++index)
	{
		JS::RootedValue device(rq.cx);
		Script::CreateObject(rq, &device);
		ReportAvailablePhysicalDevice(m_AvailablePhysicalDevices[index], rq, device);
		Script::SetPropertyInt(rq, availableDevices, index, device);
	}
	Script::SetProperty(rq, settings, "available_device", availableDevices);

	Script::SetProperty(rq, settings, "instance_extensions", m_InstanceExtensions);
	Script::SetProperty(rq, settings, "validation_layers", m_ValidationLayers);
}

std::unique_ptr<IGraphicsPipelineState> CDevice::CreateGraphicsPipelineState(
	const SGraphicsPipelineStateDesc& pipelineStateDesc)
{
	UNUSED2(pipelineStateDesc);
	return CGraphicsPipelineState::Create(this, pipelineStateDesc);
}

std::unique_ptr<IVertexInputLayout> CDevice::CreateVertexInputLayout(
	const PS::span<const SVertexAttributeFormat> attributes)
{
	return std::make_unique<CVertexInputLayout>(this, attributes);
}

std::unique_ptr<ITexture> CDevice::CreateTexture(
	const char* name, const ITexture::Type type, const uint32_t usage,
	const Format format, const uint32_t width, const uint32_t height,
	const Sampler::Desc& defaultSamplerDesc, const uint32_t MIPLevelCount, const uint32_t sampleCount)
{
	return CTexture::Create(
		this, name, type, usage, format, width, height,
		defaultSamplerDesc, MIPLevelCount, sampleCount);
}

std::unique_ptr<ITexture> CDevice::CreateTexture2D(
	const char* name, const uint32_t usage,
	const Format format, const uint32_t width, const uint32_t height,
	const Sampler::Desc& defaultSamplerDesc, const uint32_t MIPLevelCount, const uint32_t sampleCount)
{
	return CreateTexture(
		name, ITexture::Type::TEXTURE_2D, usage, format,
		width, height, defaultSamplerDesc, MIPLevelCount, sampleCount);
}

std::unique_ptr<IFramebuffer> CDevice::CreateFramebuffer(
	const char* name, SColorAttachment* colorAttachment,
	SDepthStencilAttachment* depthStencilAttachment)
{
	return CFramebuffer::Create(
		this, name, colorAttachment, depthStencilAttachment);
}

std::unique_ptr<IBuffer> CDevice::CreateBuffer(
	const char* name, const IBuffer::Type type, const uint32_t size, const bool dynamic)
{
	return CreateCBuffer(name, type, size, dynamic);
}

std::unique_ptr<CBuffer> CDevice::CreateCBuffer(
	const char* name, const IBuffer::Type type, const uint32_t size, const bool dynamic)
{
	return CBuffer::Create(this, name, type, size, dynamic);
}

std::unique_ptr<IShaderProgram> CDevice::CreateShaderProgram(
	const CStr& name, const CShaderDefines& defines)
{
	return CShaderProgram::Create(this, name, defines);
}

std::unique_ptr<IDeviceCommandContext> CDevice::CreateCommandContext()
{
	return CDeviceCommandContext::Create(this);
}

bool CDevice::AcquireNextBackbuffer()
{
	if (!IsSwapChainValid())
	{
		vkDeviceWaitIdle(m_Device);

		RecreateSwapChain();
		if (!IsSwapChainValid())
			return false;
	}

	PROFILE3("AcquireNextBackbuffer");
	return m_SubmitScheduler->AcquireNextImage(*m_SwapChain);
}

IFramebuffer* CDevice::GetCurrentBackbuffer(
	const AttachmentLoadOp colorAttachmentLoadOp,
	const AttachmentStoreOp colorAttachmentStoreOp,
	const AttachmentLoadOp depthStencilAttachmentLoadOp,
	const AttachmentStoreOp depthStencilAttachmentStoreOp)
{
	return IsSwapChainValid() ? m_SwapChain->GetCurrentBackbuffer(
		colorAttachmentLoadOp, colorAttachmentStoreOp,
		depthStencilAttachmentLoadOp, depthStencilAttachmentStoreOp) : nullptr;
}

void CDevice::Present()
{
	if (!IsSwapChainValid())
		return;

	PROFILE3("Present");

	m_SubmitScheduler->Present(*m_SwapChain);

	ProcessObjectToDestroyQueue();
	ProcessTextureToDestroyQueue();

	++m_FrameID;
}

void CDevice::OnWindowResize(const uint32_t width, const uint32_t height)
{
	UNUSED2(width);
	UNUSED2(height);
}

bool CDevice::IsTextureFormatSupported(const Format format) const
{
	bool supported = false;
	switch (format)
	{
	case Format::UNDEFINED:
		break;

	case Format::R8G8B8_UNORM: FALLTHROUGH;
	case Format::R8G8B8A8_UNORM: FALLTHROUGH;
	case Format::A8_UNORM: FALLTHROUGH;
	case Format::L8_UNORM: FALLTHROUGH;
	case Format::R32_SFLOAT: FALLTHROUGH;
	case Format::R32G32_SFLOAT: FALLTHROUGH;
	case Format::R32G32B32_SFLOAT: FALLTHROUGH;
	case Format::R32G32B32A32_SFLOAT: FALLTHROUGH;
	case Format::D16: FALLTHROUGH;
	case Format::D24: FALLTHROUGH;
	case Format::D24_S8: FALLTHROUGH;
	case Format::D32:
		supported = true;
		break;

	case Format::BC1_RGB_UNORM: FALLTHROUGH;
	case Format::BC1_RGBA_UNORM: FALLTHROUGH;
	case Format::BC2_UNORM: FALLTHROUGH;
	case Format::BC3_UNORM:
		supported = m_Capabilities.S3TC;
		break;

	default:
		break;
	}
	return supported;
}

bool CDevice::IsFramebufferFormatSupported(const Format format) const
{
	VkFormatProperties formatProperties{};
	vkGetPhysicalDeviceFormatProperties(
		m_ChoosenDevice.device, Mapping::FromFormat(format), &formatProperties);
	if (IsDepthFormat(format))
		return formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	return formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
}

Format CDevice::GetPreferredDepthStencilFormat(
	const uint32_t UNUSED(usage), const bool depth, const bool stencil) const
{
	// TODO: account usage.
	ENSURE(depth || stencil);
	Format format = Format::UNDEFINED;
	if (stencil)
	{
		format = Format::D24_S8;
	}
	else
	{
		// TODO: add most known vendors to enum.
		// https://developer.nvidia.com/blog/vulkan-dos-donts/
		if (m_ChoosenDevice.properties.vendorID == 0x10DE)
			format = Format::D24;
		else
			format = Format::D24;
	}
	ENSURE(IsFramebufferFormatSupported(format));
	return format;
}

void CDevice::ScheduleObjectToDestroy(
	VkObjectType type, const uint64_t handle, const VmaAllocation allocation)
{
	m_ObjectToDestroyQueue.push({m_FrameID, type, handle, allocation});
}

void CDevice::ScheduleTextureToDestroy(const CTexture::UID uid)
{
	m_TextureToDestroyQueue.push({m_FrameID, uid});
}

void CDevice::SetObjectName(VkObjectType type, const uint64_t handle, const char* name)
{
	if (!m_Capabilities.debugLabels)
		return;
	VkDebugUtilsObjectNameInfoEXT nameInfo{};
	nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	nameInfo.objectType = type;
	nameInfo.objectHandle = handle;
	nameInfo.pObjectName = name;
	vkSetDebugUtilsObjectNameEXT(m_Device, &nameInfo);
}

std::unique_ptr<CRingCommandContext> CDevice::CreateRingCommandContext(const size_t size)
{
	return std::make_unique<CRingCommandContext>(
		this, size, m_GraphicsQueueFamilyIndex, *m_SubmitScheduler);
}

void CDevice::RecreateSwapChain()
{
	int surfaceDrawableWidth = 0, surfaceDrawableHeight = 0;
	SDL_Vulkan_GetDrawableSize(m_Window, &surfaceDrawableWidth, &surfaceDrawableHeight);
	m_SwapChain = CSwapChain::Create(
		this, m_Surface, surfaceDrawableWidth, surfaceDrawableHeight, std::move(m_SwapChain));
}

bool CDevice::IsSwapChainValid()
{
	return m_SwapChain && m_SwapChain->IsValid();
}

void CDevice::ProcessObjectToDestroyQueue(const bool ignoreFrameID)
{
	while (!m_ObjectToDestroyQueue.empty() &&
		(ignoreFrameID || m_ObjectToDestroyQueue.front().frameID + NUMBER_OF_FRAMES_IN_FLIGHT < m_FrameID))
	{
		ObjectToDestroy& object = m_ObjectToDestroyQueue.front();
#if VK_USE_64_BIT_PTR_DEFINES
		void* handle = reinterpret_cast<void*>(object.handle);
#else
		const uint64_t handle = object.handle;
#endif
		switch (object.type)
		{
		case VK_OBJECT_TYPE_IMAGE:
			vmaDestroyImage(GetVMAAllocator(), static_cast<VkImage>(handle), object.allocation);
			break;
		case VK_OBJECT_TYPE_BUFFER:
			vmaDestroyBuffer(GetVMAAllocator(), static_cast<VkBuffer>(handle), object.allocation);
			break;
		case VK_OBJECT_TYPE_IMAGE_VIEW:
			vkDestroyImageView(m_Device, static_cast<VkImageView>(handle), nullptr);
			break;
		case VK_OBJECT_TYPE_BUFFER_VIEW:
			vkDestroyBufferView(m_Device, static_cast<VkBufferView>(handle), nullptr);
			break;
		case VK_OBJECT_TYPE_FRAMEBUFFER:
			vkDestroyFramebuffer(m_Device, static_cast<VkFramebuffer>(handle), nullptr);
			break;
		case VK_OBJECT_TYPE_RENDER_PASS:
			vkDestroyRenderPass(m_Device, static_cast<VkRenderPass>(handle), nullptr);
			break;
		case VK_OBJECT_TYPE_SAMPLER:
			vkDestroySampler(m_Device, static_cast<VkSampler>(handle), nullptr);
			break;
		case VK_OBJECT_TYPE_SHADER_MODULE:
			vkDestroyShaderModule(m_Device, static_cast<VkShaderModule>(handle), nullptr);
			break;
		case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
			vkDestroyPipelineLayout(m_Device, static_cast<VkPipelineLayout>(handle), nullptr);
			break;
		case VK_OBJECT_TYPE_PIPELINE:
			vkDestroyPipeline(m_Device, static_cast<VkPipeline>(handle), nullptr);
			break;
		default:
			debug_warn("Unsupported object to destroy type.");
		}
		m_ObjectToDestroyQueue.pop();
	}
}

void CDevice::ProcessTextureToDestroyQueue(const bool ignoreFrameID)
{
	while (!m_TextureToDestroyQueue.empty() &&
		(ignoreFrameID || m_TextureToDestroyQueue.front().first + NUMBER_OF_FRAMES_IN_FLIGHT < m_FrameID))
	{
		GetDescriptorManager().OnTextureDestroy(m_TextureToDestroyQueue.front().second);
		m_TextureToDestroyQueue.pop();
	}
}

std::unique_ptr<IDevice> CreateDevice(SDL_Window* window)
{
	return Vulkan::CDevice::Create(window);
}

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer
