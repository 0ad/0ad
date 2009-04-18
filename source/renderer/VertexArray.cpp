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

#include "precompiled.h"

#include "lib/ogl.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"
#include "graphics/SColor.h"
#include "renderer/VertexArray.h"
#include "renderer/VertexBuffer.h"
#include "renderer/VertexBufferManager.h"


VertexArray::VertexArray(bool dynamic)
{
	m_Dynamic = dynamic;
	m_NumVertices = 0;
	
	m_VB = 0;
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
	delete[] m_BackingStore;
	m_BackingStore = 0;
	
	if (m_VB)
	{
		g_VBMan.Release(m_VB);
		m_VB = 0;
	}
}


// Set the number of vertices stored in the array
void VertexArray::SetNumVertices(size_t num)
{
	if (num == m_NumVertices)
		return;
	
	Free();
	m_NumVertices = num;
}


// Add vertex attributes like Position, Normal, UV
void VertexArray::AddAttribute(Attribute* attr)
{
	debug_assert((attr->type == GL_FLOAT || attr->type == GL_UNSIGNED_BYTE) && "Unsupported attribute type");
	debug_assert(attr->elems >= 1 && attr->elems <= 4);

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
	debug_assert(vertexArray);
	debug_assert(type == GL_FLOAT);
	debug_assert(elems >= 3);
	
	return vertexArray->MakeIterator<CVector3D>(this);
}

template<>
VertexArrayIterator<CVector4D> VertexArray::Attribute::GetIterator<CVector4D>() const
{
	debug_assert(vertexArray);
	debug_assert(type == GL_FLOAT);
	debug_assert(elems >= 4);
	
	return vertexArray->MakeIterator<CVector4D>(this);
}

template<>
VertexArrayIterator<float[2]> VertexArray::Attribute::GetIterator<float[2]>() const
{
	debug_assert(vertexArray);
	debug_assert(type == GL_FLOAT);
	debug_assert(elems >= 2);
	
	return vertexArray->MakeIterator<float[2]>(this);
}

template<>
VertexArrayIterator<SColor3ub> VertexArray::Attribute::GetIterator<SColor3ub>() const
{
	debug_assert(vertexArray);
	debug_assert(type == GL_UNSIGNED_BYTE);
	debug_assert(elems >= 3);
	
	return vertexArray->MakeIterator<SColor3ub>(this);
}

template<>
VertexArrayIterator<SColor4ub> VertexArray::Attribute::GetIterator<SColor4ub>() const
{
	debug_assert(vertexArray);
	debug_assert(type == GL_UNSIGNED_BYTE);
	debug_assert(elems >= 4);
	
	return vertexArray->MakeIterator<SColor4ub>(this);
}



static size_t RoundStride(size_t stride)
{
	if (stride <= 0)
		return 0;
	if (stride <= 4)
		return 4;
	if (stride <= 8)
		return 8;
	if (stride <= 16)
		return 16;
	
	return (stride + 31) & ~31;
}

// Re-layout by assigning offsets on a first-come first-serve basis,
// then round up to a reasonable stride.
// Backing store is also created here, VBOs are created on upload.
void VertexArray::Layout()
{
	Free();
	
	m_Stride = 0;
	
	//debug_printf("Layouting VertexArray\n");
	
	for(int idx = (int)m_Attributes.size()-1; idx >= 0; --idx)
	{
		Attribute* attr = m_Attributes[idx];
		
		if (!attr->type || !attr->elems)
			continue;
	
		size_t attrSize = 0;
		switch(attr->type)
		{
		case GL_UNSIGNED_BYTE:
			attrSize = sizeof(GLubyte);
			break;
		case GL_FLOAT:
			attrSize = sizeof(GLfloat);
			break;
		default:
			attrSize = 0;
			debug_warn("Bad AttributeInfo::Type"); break;
		}

		attrSize *= attr->elems;
		
		attr->offset = m_Stride;
		m_Stride = (m_Stride + attrSize + 3) & ~3;
	
		//debug_printf("%i: offset: %u\n", idx, attr->offset);
	}
	
	m_Stride = RoundStride(m_Stride);
	
	//debug_printf("Stride: %u\n", m_Stride);
	
	if (m_Stride)
		m_BackingStore = new char[m_Stride * m_NumVertices];
}


// (Re-)Upload the attributes.
// Create the VBO if necessary.
void VertexArray::Upload()
{
	debug_assert(m_BackingStore);
	
	if (!m_VB)
		m_VB = g_VBMan.Allocate(m_Stride, m_NumVertices, m_Dynamic);

	if (!m_VB) // failed to allocate VBO
		return;

	m_VB->m_Owner->UpdateChunkVertices(m_VB, m_BackingStore);
}


// Bind this array, returns the base address for calls to glVertexPointer etc.
u8* VertexArray::Bind()
{
	if (!m_VB)
		return NULL;

	u8* base = m_VB->m_Owner->Bind();
	base += m_VB->m_Index*m_Stride;
	return base;
}


// Free the backing store to save some memory
void VertexArray::FreeBackingStore()
{
	delete[] m_BackingStore;
	m_BackingStore = 0;
}

