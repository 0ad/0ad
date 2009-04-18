/* Copyright (C) 2009 Wildfire Games.
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

#include "renderer/VertexBuffer.h"

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
// Furthermore, it stores dynamic and static attributes in different
// vertex buffers, so that needless re-uploads of static data is avoided.
//
// Note that this class will not allocate any OpenGL resources until one
// of the Upload functions is called.
class VertexArray
{
public:
	struct Attribute
	{
		// Data type. Currently supported: GL_FLOAT
		GLenum type;
		// How many elements per vertex (e.g. 3 for RGB, 2 for UV)
		GLuint elems;
		
		// Offset (in bytes) into a vertex structure (filled in by Layout())
		size_t offset;
		
		VertexArray* vertexArray;

		Attribute() : type(0), elems(0), offset(0), vertexArray(0) { }
	
		// Get an iterator for the given attribute that initially points at the first vertex.
		// Supported types T: CVector3D, CVector4D, float[], SColor3ub, SColor4ub
		// This function verifies at runtime that the requested type T matches
		// the attribute definition passed to AddAttribute().
		template<typename T>
		VertexArrayIterator<T> GetIterator() const;
	};
	
public:
	VertexArray(bool dynamic);
	~VertexArray();
	
	// Set the number of vertices stored in the array
	void SetNumVertices(size_t num);
	// Add vertex attributes
	void AddAttribute(Attribute* attr);

	size_t GetNumVertices() const { return m_NumVertices; }
	size_t GetStride() const { return m_Stride; }
	
	// Layout the vertex array format and create backing buffer in RAM.
	// You must call Layout() after changing the number of vertices or
	// attributes.
	// All vertex data is lost when a vertex array is re-layouted.
	void Layout();
	// (Re-)Upload the static/dynamic attributes of the vertex array.
	void Upload();
	// Bind this array, returns the base address for calls to glVertexPointer etc.
	u8* Bind();

	// If you know for certain that you'll never have to change the data again,
	// call this to free some memory.
	void FreeBackingStore();
	
private:
	void Free();

	template<typename T>
	VertexArrayIterator<T> MakeIterator(const Attribute* attr)
	{
		debug_assert(attr->type && attr->elems);
		return VertexArrayIterator<T>(m_BackingStore + attr->offset, m_Stride);
	}

	bool m_Dynamic;
	size_t m_NumVertices;
	std::vector<Attribute*> m_Attributes;

	CVertexBuffer::VBChunk* m_VB;
	size_t m_Stride;
	char* m_BackingStore;
};


#endif // INCLUDED_VERTEXARRAY
