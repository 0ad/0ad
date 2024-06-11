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

#ifndef INCLUDED_VERTEXARRAY
#define INCLUDED_VERTEXARRAY

#include "renderer/backend/Format.h"
#include "renderer/backend/IBuffer.h"
#include "renderer/backend/IDeviceCommandContext.h"
#include "renderer/VertexBufferManager.h"

#include <vector>

// Iterator
template<typename T>
class VertexArrayIterator
{
public:
	typedef T Type;

public:
	VertexArrayIterator() :
		m_Data(0), m_Stride(0)
	{
	}

	VertexArrayIterator(char* data, size_t stride) :
		m_Data(data), m_Stride(stride)
	{
	}

	VertexArrayIterator(const VertexArrayIterator& rhs) :
		m_Data(rhs.m_Data), m_Stride(rhs.m_Stride)
	{
	}

	VertexArrayIterator& operator=(const VertexArrayIterator& rhs)
	{
		m_Data = rhs.m_Data;
		m_Stride = rhs.m_Stride;
		return *this;
	}

	// Accessors
	T& operator*() const { return *(T*)m_Data; }
	T* operator->() const { return (T*)m_Data; }
	T& operator[](size_t idx) const { return *(T*)(m_Data + idx*m_Stride); }

	// Walking
	VertexArrayIterator& operator++()
	{
		m_Data += m_Stride;
		return *this;
	}
	VertexArrayIterator operator++(int)
	{
		VertexArrayIterator tmp = *this;
		m_Data += m_Stride;
		return tmp;
	}
	VertexArrayIterator& operator--()
	{
		m_Data -= m_Stride;
		return *this;
	}
	VertexArrayIterator operator--(int)
	{
		VertexArrayIterator tmp = *this;
		m_Data -= m_Stride;
		return tmp;
	}

	VertexArrayIterator& operator+=(ssize_t rhs)
	{
		m_Data += rhs*m_Stride;
		return *this;
	}
	VertexArrayIterator& operator-=(ssize_t rhs)
	{
		m_Data -= rhs*m_Stride;
		return *this;
	}

	VertexArrayIterator operator+(ssize_t rhs) const
	{
		VertexArrayIterator tmp = *this;
		tmp.m_Data += rhs*m_Stride;
		return tmp;
	}
	VertexArrayIterator operator-(ssize_t rhs) const
	{
		VertexArrayIterator tmp = *this;
		tmp.m_Data -= rhs*m_Stride;
		return tmp;
	}

	// Accessors for raw buffer data, for performance-critical code
	char* GetData() const
	{
		return m_Data;
	}
	size_t GetStride() const
	{
		return m_Stride;
	}

private:
	char* m_Data;
	size_t m_Stride;
};


// Manage a vertex array with a runtime-determined set of attributes.
//
// Purpose: Different rendering paths sometimes require different sets of
// attributes (e.g. normal vector vs. color data), which is difficult to
// support with hardcoded vertex structures.
// This class chooses the vertex layout at runtime, based on the attributes
// that are actually needed.
//
// Note that this class will not allocate any backend resources until one
// of the Upload functions is called.
class VertexArray
{
public:
	struct Attribute
	{
		Renderer::Backend::Format format = Renderer::Backend::Format::UNDEFINED;

		// Offset (in bytes) into a vertex structure (filled in by Layout())
		uint32_t offset = 0;

		VertexArray* vertexArray = nullptr;

		Attribute() {}

		// Get an iterator over the backing store for the given attribute that
		// initially points at the first vertex.
		// Supported types T: CVector3D, CVector4D, float[2], SColor4ub,
		// u16, u16[2], u8[4], short, short[2].
		// This function verifies at runtime that the requested type T matches
		// the attribute definition passed to AddAttribute().
		template<typename T>
		VertexArrayIterator<T> GetIterator() const;
	};

public:
	VertexArray(
		const Renderer::Backend::IBuffer::Type type, const uint32_t usage);
	~VertexArray();

	// Set the number of vertices stored in the array
	void SetNumberOfVertices(const size_t numberOfVertices);
	// Add vertex attributes
	void AddAttribute(Attribute* attr);

	size_t GetNumberOfVertices() const { return m_NumberOfVertices; }
	uint32_t GetStride() const { return m_Stride; }

	// Layout the vertex array format and create backing buffer in RAM.
	// You must call Layout() after changing the number of vertices or
	// attributes.
	// All vertex data is lost when a vertex array is re-layouted.
	void Layout();
	// (Re-)Upload the attributes of the vertex array from the backing store to
	// the underlying buffer.
	void Upload();
	// Make this vertex array's data available for the next series of calls to Bind
	void PrepareForRendering();

	void UploadIfNeeded(Renderer::Backend::IDeviceCommandContext* deviceCommandContext);

	// If you know for certain that you'll never have to change the data again,
	// call this to free some memory.
	void FreeBackingStore();

	Renderer::Backend::IBuffer* GetBuffer() { return m_VB ? m_VB->m_Owner->GetBuffer() : nullptr; }

	uint32_t GetOffset() const { return m_VB ? m_VB->m_Index : 0; }

private:
	void Free();

	template<typename T>
	VertexArrayIterator<T> MakeIterator(const Attribute* attr)
	{
		ENSURE(attr->format != Renderer::Backend::Format::UNDEFINED);
		return VertexArrayIterator<T>(m_BackingStore + attr->offset, m_Stride);
	}

	Renderer::Backend::IBuffer::Type m_Type;
	uint32_t m_Usage = 0;
	size_t m_NumberOfVertices;
	std::vector<Attribute*> m_Attributes;

	CVertexBufferManager::Handle m_VB;
	uint32_t m_Stride;
	char* m_BackingStore; // 16-byte aligned, to allow fast SSE access
};

/**
 * A VertexArray that is specialised to handle 16-bit array indices.
 * Call UploadIfNeeded() before use in Draw/DrawIndexed.
 */
class VertexIndexArray : public VertexArray
{
public:
	VertexIndexArray(const uint32_t usage);

	/// Gets the iterator over the (only) attribute in this array, i.e. a u16.
	VertexArrayIterator<u16> GetIterator() const;

private:
	Attribute m_Attr;
};

#endif // INCLUDED_VERTEXARRAY
