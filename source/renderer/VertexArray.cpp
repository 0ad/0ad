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

#include "lib/alignment.h"
#include "lib/sysdep/rtl.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"
#include "ps/CLogger.h"
#include "graphics/Color.h"
#include "graphics/SColor.h"
#include "renderer/Renderer.h"
#include "renderer/VertexArray.h"
#include "renderer/VertexBuffer.h"
#include "renderer/VertexBufferManager.h"

namespace
{

uint32_t GetAttributeSize(const Renderer::Backend::Format format)
{
	switch (format)
	{
	case Renderer::Backend::Format::R8G8B8A8_UNORM: FALLTHROUGH;
	case Renderer::Backend::Format::R8G8B8A8_UINT:
		return sizeof(u8) * 4;
	case Renderer::Backend::Format::A8_UNORM:
		return sizeof(u8);
	case Renderer::Backend::Format::R16_UNORM: FALLTHROUGH;
	case Renderer::Backend::Format::R16_UINT: FALLTHROUGH;
	case Renderer::Backend::Format::R16_SINT:
		return sizeof(u16);
	case Renderer::Backend::Format::R16G16_UNORM: FALLTHROUGH;
	case Renderer::Backend::Format::R16G16_UINT: FALLTHROUGH;
	case Renderer::Backend::Format::R16G16_SINT:
		return sizeof(u16) * 2;
	case Renderer::Backend::Format::R32_SFLOAT:
		return sizeof(float);
	case Renderer::Backend::Format::R32G32_SFLOAT:
		return sizeof(float) * 2;
	case Renderer::Backend::Format::R32G32B32_SFLOAT:
		return sizeof(float) * 3;
	case Renderer::Backend::Format::R32G32B32A32_SFLOAT:
		return sizeof(float) * 4;
	default:
		break;
	};
	return 0;
}

} // anonymous namespace

VertexArray::VertexArray(
	const Renderer::Backend::IBuffer::Type type, const uint32_t usage)
	: m_Type(type), m_Usage(usage)
{
	m_NumberOfVertices = 0;

	m_BackingStore = 0;
	m_Stride = 0;
}

VertexArray::~VertexArray()
{
	Free();
}

// Free all resources on destruction or when a layout parameter changes
void VertexArray::Free()
{
	rtl_FreeAligned(m_BackingStore);
	m_BackingStore = 0;

	m_VB.Reset();
}

// Set the number of vertices stored in the array
void VertexArray::SetNumberOfVertices(const size_t numberOfVertices)
{
	if (numberOfVertices == m_NumberOfVertices)
		return;

	Free();
	m_NumberOfVertices = numberOfVertices;
}

// Add vertex attributes like Position, Normal, UV
void VertexArray::AddAttribute(Attribute* attr)
{
	// Attribute is supported is its size is greater than zero.
	ENSURE(GetAttributeSize(attr->format) > 0 && "Unsupported attribute.");

	attr->vertexArray = this;
	m_Attributes.push_back(attr);

	Free();
}

// Template specialization for GetIterator().
// We can put this into the source file because only a fixed set of types
// is supported for type safety.
template<>
VertexArrayIterator<CVector3D> VertexArray::Attribute::GetIterator<CVector3D>() const
{
	ENSURE(vertexArray);
	ENSURE(
		format == Renderer::Backend::Format::R32G32B32_SFLOAT ||
		format == Renderer::Backend::Format::R32G32B32A32_SFLOAT);

	return vertexArray->MakeIterator<CVector3D>(this);
}

template<>
VertexArrayIterator<CVector4D> VertexArray::Attribute::GetIterator<CVector4D>() const
{
	ENSURE(vertexArray);
	ENSURE(format == Renderer::Backend::Format::R32G32B32A32_SFLOAT);

	return vertexArray->MakeIterator<CVector4D>(this);
}

template<>
VertexArrayIterator<float[2]> VertexArray::Attribute::GetIterator<float[2]>() const
{
	ENSURE(vertexArray);
	ENSURE(format == Renderer::Backend::Format::R32G32_SFLOAT);

	return vertexArray->MakeIterator<float[2]>(this);
}

template<>
VertexArrayIterator<SColor4ub> VertexArray::Attribute::GetIterator<SColor4ub>() const
{
	ENSURE(vertexArray);
	ENSURE(
		format == Renderer::Backend::Format::R8G8B8A8_UNORM ||
		format == Renderer::Backend::Format::R8G8B8A8_UINT);

	return vertexArray->MakeIterator<SColor4ub>(this);
}

template<>
VertexArrayIterator<u16> VertexArray::Attribute::GetIterator<u16>() const
{
	ENSURE(vertexArray);
	ENSURE(format == Renderer::Backend::Format::R16_UINT);

	return vertexArray->MakeIterator<u16>(this);
}

template<>
VertexArrayIterator<u16[2]> VertexArray::Attribute::GetIterator<u16[2]>() const
{
	ENSURE(vertexArray);
	ENSURE(format == Renderer::Backend::Format::R16G16_UINT);

	return vertexArray->MakeIterator<u16[2]>(this);
}

template<>
VertexArrayIterator<u8> VertexArray::Attribute::GetIterator<u8>() const
{
	ENSURE(vertexArray);
	ENSURE(format == Renderer::Backend::Format::A8_UNORM);

	return vertexArray->MakeIterator<u8>(this);
}

template<>
VertexArrayIterator<u8[4]> VertexArray::Attribute::GetIterator<u8[4]>() const
{
	ENSURE(vertexArray);
	ENSURE(
		format == Renderer::Backend::Format::R8G8B8A8_UNORM ||
		format == Renderer::Backend::Format::R8G8B8A8_UINT);

	return vertexArray->MakeIterator<u8[4]>(this);
}

template<>
VertexArrayIterator<short> VertexArray::Attribute::GetIterator<short>() const
{
	ENSURE(vertexArray);
	ENSURE(format == Renderer::Backend::Format::R16_SINT);

	return vertexArray->MakeIterator<short>(this);
}

template<>
VertexArrayIterator<short[2]> VertexArray::Attribute::GetIterator<short[2]>() const
{
	ENSURE(vertexArray);
	ENSURE(format == Renderer::Backend::Format::R16G16_SINT);

	return vertexArray->MakeIterator<short[2]>(this);
}

static uint32_t RoundStride(uint32_t stride)
{
	if (stride <= 0)
		return 0;
	if (stride <= 4)
		return 4;
	if (stride <= 8)
		return 8;
	if (stride <= 16)
		return 16;

	return Align<32>(stride);
}

// Re-layout by assigning offsets on a first-come first-serve basis,
// then round up to a reasonable stride.
// Backing store is also created here, backend buffers are created on upload.
void VertexArray::Layout()
{
	Free();

	m_Stride = 0;

	for (ssize_t idx = m_Attributes.size()-1; idx >= 0; --idx)
	{
		Attribute* attr = m_Attributes[idx];
		if (attr->format == Renderer::Backend::Format::UNDEFINED)
			continue;

		const uint32_t attrSize = GetAttributeSize(attr->format);
		ENSURE(attrSize > 0);

		attr->offset = m_Stride;

		m_Stride += attrSize;

		if (m_Type == Renderer::Backend::IBuffer::Type::VERTEX)
			m_Stride = Align<4>(m_Stride);
	}

	if (m_Type == Renderer::Backend::IBuffer::Type::VERTEX)
		m_Stride = RoundStride(m_Stride);

	if (m_Stride)
		m_BackingStore = (char*)rtl_AllocateAligned(m_Stride * m_NumberOfVertices, 16);
}

void VertexArray::PrepareForRendering()
{
	m_VB->m_Owner->PrepareForRendering(m_VB.Get());
}

// (Re-)Upload the attributes.
// Create the backend buffer if necessary.
void VertexArray::Upload()
{
	ENSURE(m_BackingStore);

	if (!m_VB)
	{
		m_VB = g_Renderer.GetVertexBufferManager().AllocateChunk(
			m_Stride, m_NumberOfVertices, m_Type, m_Usage, m_BackingStore);
	}

	if (!m_VB)
	{
		LOGERROR("Failed to allocate backend buffer for vertex array");
		return;
	}

	m_VB->m_Owner->UpdateChunkVertices(m_VB.Get(), m_BackingStore);
}

void VertexArray::UploadIfNeeded(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext)
{
	m_VB->m_Owner->UploadIfNeeded(deviceCommandContext);
}

// Free the backing store to save some memory
void VertexArray::FreeBackingStore()
{
	// In streaming modes, the backing store must be retained
	ENSURE(!CVertexBuffer::UseStreaming(m_Usage));

	rtl_FreeAligned(m_BackingStore);
	m_BackingStore = 0;
}

VertexIndexArray::VertexIndexArray(const uint32_t usage) :
	VertexArray(Renderer::Backend::IBuffer::Type::INDEX, usage)
{
	m_Attr.format = Renderer::Backend::Format::R16_UINT;
	AddAttribute(&m_Attr);
}

VertexArrayIterator<u16> VertexIndexArray::GetIterator() const
{
	return m_Attr.GetIterator<u16>();
}
