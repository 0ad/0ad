/* Copyright (C) 2015 Wildfire Games.
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
#include "lib/ogl.h"
#include "lib/sysdep/rtl.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"
#include "ps/CLogger.h"
#include "graphics/Color.h"
#include "graphics/SColor.h"
#include "renderer/VertexArray.h"
#include "renderer/VertexBuffer.h"
#include "renderer/VertexBufferManager.h"


VertexArray::VertexArray(GLenum usage, GLenum target)
{
	m_Usage = usage;
	m_Target = target;
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
	rtl_FreeAligned(m_BackingStore);
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
	ENSURE(
		(attr->type == GL_FLOAT || attr->type == GL_SHORT || attr->type == GL_UNSIGNED_SHORT || attr->type == GL_UNSIGNED_BYTE)
		&& "Unsupported attribute type"
	);
	ENSURE(attr->elems >= 1 && attr->elems <= 4);

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
	ENSURE(type == GL_FLOAT);
	ENSURE(elems >= 3);

	return vertexArray->MakeIterator<CVector3D>(this);
}

template<>
VertexArrayIterator<CVector4D> VertexArray::Attribute::GetIterator<CVector4D>() const
{
	ENSURE(vertexArray);
	ENSURE(type == GL_FLOAT);
	ENSURE(elems >= 4);

	return vertexArray->MakeIterator<CVector4D>(this);
}

template<>
VertexArrayIterator<float[2]> VertexArray::Attribute::GetIterator<float[2]>() const
{
	ENSURE(vertexArray);
	ENSURE(type == GL_FLOAT);
	ENSURE(elems >= 2);

	return vertexArray->MakeIterator<float[2]>(this);
}

template<>
VertexArrayIterator<SColor3ub> VertexArray::Attribute::GetIterator<SColor3ub>() const
{
	ENSURE(vertexArray);
	ENSURE(type == GL_UNSIGNED_BYTE);
	ENSURE(elems >= 3);

	return vertexArray->MakeIterator<SColor3ub>(this);
}

template<>
VertexArrayIterator<SColor4ub> VertexArray::Attribute::GetIterator<SColor4ub>() const
{
	ENSURE(vertexArray);
	ENSURE(type == GL_UNSIGNED_BYTE);
	ENSURE(elems >= 4);

	return vertexArray->MakeIterator<SColor4ub>(this);
}

template<>
VertexArrayIterator<u16> VertexArray::Attribute::GetIterator<u16>() const
{
	ENSURE(vertexArray);
	ENSURE(type == GL_UNSIGNED_SHORT);
	ENSURE(elems >= 1);

	return vertexArray->MakeIterator<u16>(this);
}

template<>
VertexArrayIterator<u16[2]> VertexArray::Attribute::GetIterator<u16[2]>() const
{
	ENSURE(vertexArray);
	ENSURE(type == GL_UNSIGNED_SHORT);
	ENSURE(elems >= 2);

	return vertexArray->MakeIterator<u16[2]>(this);
}

template<>
VertexArrayIterator<u8> VertexArray::Attribute::GetIterator<u8>() const
{
	ENSURE(vertexArray);
	ENSURE(type == GL_UNSIGNED_BYTE);
	ENSURE(elems >= 1);

	return vertexArray->MakeIterator<u8>(this);
}

template<>
VertexArrayIterator<u8[4]> VertexArray::Attribute::GetIterator<u8[4]>() const
{
	ENSURE(vertexArray);
	ENSURE(type == GL_UNSIGNED_BYTE);
	ENSURE(elems >= 4);

	return vertexArray->MakeIterator<u8[4]>(this);
}

template<>
VertexArrayIterator<short> VertexArray::Attribute::GetIterator<short>() const
{
	ENSURE(vertexArray);
	ENSURE(type == GL_SHORT);
	ENSURE(elems >= 1);

	return vertexArray->MakeIterator<short>(this);
}

template<>
VertexArrayIterator<short[2]> VertexArray::Attribute::GetIterator<short[2]>() const
{
	ENSURE(vertexArray);
	ENSURE(type == GL_SHORT);
	ENSURE(elems >= 2);

	return vertexArray->MakeIterator<short[2]>(this);
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

	return Align<32>(stride);
}

// Re-layout by assigning offsets on a first-come first-serve basis,
// then round up to a reasonable stride.
// Backing store is also created here, VBOs are created on upload.
void VertexArray::Layout()
{
	Free();

	m_Stride = 0;

	//debug_printf("Layouting VertexArray\n");

	for (ssize_t idx = m_Attributes.size()-1; idx >= 0; --idx)
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
		case GL_SHORT:
			attrSize = sizeof(GLshort);
			break;
		case GL_UNSIGNED_SHORT:
			attrSize = sizeof(GLushort);
			break;
		case GL_FLOAT:
			attrSize = sizeof(GLfloat);
			break;
		default:
			attrSize = 0;
			debug_warn(L"Bad Attribute::type"); break;
		}

		attrSize *= attr->elems;

		attr->offset = m_Stride;

		m_Stride += attrSize;

		if (m_Target == GL_ARRAY_BUFFER)
			m_Stride = Align<4>(m_Stride);

		//debug_printf("%i: offset: %u\n", idx, attr->offset);
	}

	if (m_Target == GL_ARRAY_BUFFER)
		m_Stride = RoundStride(m_Stride);

	//debug_printf("Stride: %u\n", m_Stride);

	if (m_Stride)
		m_BackingStore = (char*)rtl_AllocateAligned(m_Stride * m_NumVertices, 16);
}

void VertexArray::PrepareForRendering()
{
	m_VB->m_Owner->PrepareForRendering(m_VB);
}

// (Re-)Upload the attributes.
// Create the VBO if necessary.
void VertexArray::Upload()
{
	ENSURE(m_BackingStore);

	if (!m_VB)
		m_VB = g_VBMan.Allocate(m_Stride, m_NumVertices, m_Usage, m_Target, m_BackingStore);

	if (!m_VB)
	{
		LOGERROR("Failed to allocate VBO for vertex array");
		return;
	}

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
	// In streaming modes, the backing store must be retained
	ENSURE(!CVertexBuffer::UseStreaming(m_Usage));

	rtl_FreeAligned(m_BackingStore);
	m_BackingStore = 0;
}



VertexIndexArray::VertexIndexArray(GLenum usage) :
	VertexArray(usage, GL_ELEMENT_ARRAY_BUFFER)
{
	m_Attr.type = GL_UNSIGNED_SHORT;
	m_Attr.elems = 1;
	AddAttribute(&m_Attr);
}

VertexArrayIterator<u16> VertexIndexArray::GetIterator() const
{
	return m_Attr.GetIterator<u16>();
}
