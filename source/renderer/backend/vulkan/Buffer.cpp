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

#include "Buffer.h"

#include "renderer/backend/vulkan/Device.h"
#include "renderer/backend/vulkan/Utilities.h"

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

// static
std::unique_ptr<CBuffer> CBuffer::Create(
	CDevice* device, const char* name, const Type type, const uint32_t size,
	const bool dynamic)
{
	std::unique_ptr<CBuffer> buffer(new CBuffer());
	buffer->m_Device = device;
	buffer->m_Type = type;
	buffer->m_Size = size;
	buffer->m_Dynamic = dynamic;

	VkMemoryPropertyFlags properties = 0;
	VkBufferUsageFlags usage = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
	VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO;
	switch (type)
	{
	case Type::VERTEX:
		usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		break;
	case Type::INDEX:
		usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		break;
	case Type::UPLOAD:
		usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		break;
	case Type::UNIFORM:
		usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		break;
	}

	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocationCreateInfo{};
	if (type == Type::UPLOAD)
		allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
#ifndef NDEBUG
	allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
	allocationCreateInfo.pUserData = const_cast<char*>(name);
#endif
	allocationCreateInfo.requiredFlags = properties;
	allocationCreateInfo.usage = memoryUsage;
	const VkResult createBufferResult = vmaCreateBuffer(
		device->GetVMAAllocator(), &bufferCreateInfo, &allocationCreateInfo,
		&buffer->m_Buffer, &buffer->m_Allocation, &buffer->m_AllocationInfo);
	if (createBufferResult != VK_SUCCESS)
	{
		LOGERROR("Failed to create VkBuffer: %d (%s)", static_cast<int>(createBufferResult), Utilities::GetVkResultName(createBufferResult));
		return nullptr;
	}

	device->SetObjectName(VK_OBJECT_TYPE_BUFFER, buffer->m_Buffer, name);

	return buffer;
}

CBuffer::CBuffer() = default;

CBuffer::~CBuffer()
{
	if (m_Allocation != VK_NULL_HANDLE)
		m_Device->ScheduleObjectToDestroy(
			VK_OBJECT_TYPE_BUFFER, m_Buffer, m_Allocation);
}

IDevice* CBuffer::GetDevice()
{
	return m_Device;
}

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer
