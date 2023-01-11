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

#ifndef INCLUDED_RENDERER_BACKEND_VULKAN_BUFFER
#define INCLUDED_RENDERER_BACKEND_VULKAN_BUFFER

#include "renderer/backend/IBuffer.h"
#include "renderer/backend/vulkan/VMA.h"

#include <glad/vulkan.h>
#include <memory>

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

class CDevice;

class CBuffer final : public IBuffer
{
public:
	~CBuffer() override;

	IDevice* GetDevice() override;

	Type GetType() const override { return m_Type; }
	uint32_t GetSize() const override { return m_Size; }
	bool IsDynamic() const override { return m_Dynamic; }

	VkBuffer GetVkBuffer() { return m_Buffer; }

	/**
	 * @return mapped data for UPLOAD buffers else returns nullptr.
	 */
	void* GetMappedData() { return m_AllocationInfo.pMappedData; }

private:
	friend class CDevice;

	static std::unique_ptr<CBuffer> Create(
		CDevice* device, const char* name, const Type type, const uint32_t size,
		const bool dynamic);

	CBuffer();

	CDevice* m_Device = nullptr;

	Type m_Type = Type::VERTEX;
	uint32_t m_Size = 0;
	bool m_Dynamic = false;

	VkBuffer m_Buffer = VK_NULL_HANDLE;
	VmaAllocation m_Allocation = VK_NULL_HANDLE;
	VmaAllocationInfo m_AllocationInfo{};
};

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_VULKAN_BUFFER
