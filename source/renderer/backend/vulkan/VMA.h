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

#ifndef INCLUDED_RENDERER_BACKEND_VULKAN_VMA
#define INCLUDED_RENDERER_BACKEND_VULKAN_VMA

#include "lib/debug.h"
#include "lib/sysdep/os.h"
#include "ps/CLogger.h"

#include <glad/vulkan.h>
#include <mutex>

#define VMA_VULKAN_VERSION 1001000
#define VMA_ASSERT(EXPR) ASSERT(EXPR)
#define VMA_HEAVY_ASSERT(EXPR) ENSURE(EXPR)
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_BUFFER_DEVICE_ADDRESS 0

#ifndef NDEBUG
#define VMA_DEBUG_LOG(...) do { debug_printf(__VA_ARGS__); debug_printf("\n"); } while (0)
#define VMA_STATS_STRING_ENABLED 1
#else
#define VMA_DEBUG_LOG(...)
#define VMA_STATS_STRING_ENABLED 0
#endif

#if OS_WIN
// MSVC doesn't enable std::shared_mutex for XP toolkit.
#define VMA_USE_STL_SHARED_MUTEX 0
#endif

#if GCC_VERSION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wundef"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif
#if CLANG_VERSION
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat"
#pragma clang diagnostic ignored "-Wnullability-completeness"
#pragma clang diagnostic ignored "-Wparentheses"
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-variable"
#endif
#if MSC_VERSION
#pragma warning(push, 1)
#pragma warning(disable: 4100) // Unreferenced formal parameter.
#pragma warning(disable: 4701) // Potentially uninitialized local variable used.
#pragma warning(disable: 4703) // Potentially uninitialized local pointer variable used.
#endif

// GCC before 13 doesn't account some diagnostics:
//   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53431
#if GCC_VERSION && !defined(_MSVC_LANG)
#define _MSVC_LANG 0
#endif

// VMA always uses extension variants and glad doesn't provide extension
// defines and types that were promoted to the core version. So we need to
// define them manually.
#define PFN_vkBindBufferMemory2KHR PFN_vkBindBufferMemory2
#define PFN_vkBindImageMemory2KHR PFN_vkBindImageMemory2
#define PFN_vkGetBufferMemoryRequirements2KHR PFN_vkGetBufferMemoryRequirements2
#define PFN_vkGetImageMemoryRequirements2KHR PFN_vkGetImageMemoryRequirements2
#define PFN_vkGetPhysicalDeviceMemoryProperties2KHR PFN_vkGetPhysicalDeviceMemoryProperties2

#define vkBindBufferMemory2KHR vkBindBufferMemory2
#define vkBindImageMemory2KHR vkBindImageMemory2
#define vkGetBufferMemoryRequirements2KHR vkGetBufferMemoryRequirements2
#define vkGetImageMemoryRequirements2KHR vkGetImageMemoryRequirements2
#define vkGetPhysicalDeviceMemoryProperties2KHR vkGetPhysicalDeviceMemoryProperties2

#define VkBindBufferMemoryInfoKHR VkBindBufferMemoryInfo
#define VkBindImageMemoryInfoKHR VkBindImageMemoryInfo
#define VkBufferMemoryRequirementsInfo2KHR VkBufferMemoryRequirementsInfo2
#define VkImageMemoryRequirementsInfo2KHR VkImageMemoryRequirementsInfo2
#define VkMemoryDedicatedAllocateInfoKHR VkMemoryDedicatedAllocateInfo
#define VkMemoryDedicatedRequirementsKHR VkMemoryDedicatedRequirements
#define VkMemoryRequirements2KHR VkMemoryRequirements2
#define VkPhysicalDeviceMemoryProperties2KHR VkPhysicalDeviceMemoryProperties2

#define VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO_KHR VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO
#define VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO_KHR VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO
#define VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2_KHR VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2
#define VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2_KHR VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2
#define VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO
#define VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS
#define VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2_KHR VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2

// Those extensions should be enabled automatically by picking Vulkan 1.1.
#define VK_AMD_device_coherent_memory 0
#define VK_KHR_bind_memory2 0
#define VK_KHR_get_memory_requirements2 0
#define VK_KHR_external_memory 0
#define VK_EXT_memory_budget 0
#define VK_EXT_memory_priority 0

#include "third_party/vma/vk_mem_alloc.h"

#if GCC_VERSION
#pragma GCC diagnostic pop
#endif
#if CLANG_VERSION
#pragma clang diagnostic pop
#endif
#if MSC_VERSION
#pragma warning(pop)
#endif

#endif // INCLUDED_RENDERER_BACKEND_VULKAN_VMA
