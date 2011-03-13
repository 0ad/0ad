/* Copyright (C) 2011 Wildfire Games.
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

/*
 * encapsulation of VBOs with sharing
 */

#include "precompiled.h"
#include "ps/Errors.h"
#include "lib/ogl.h"
#include "lib/sysdep/cpu.h"
#include "Renderer.h"
#include "VertexBuffer.h"
#include "VertexBufferManager.h"
#include "ps/CLogger.h"

///////////////////////////////////////////////////////////////////////////////
// CVertexBuffer constructor 
CVertexBuffer::CVertexBuffer(size_t vertexSize, GLenum usage, GLenum target)
	: m_VertexSize(vertexSize), m_Handle(0), m_SysMem(0), m_Usage(usage), m_Target(target)
{
	size_t size = MAX_VB_SIZE_BYTES;

	// allocate raw buffer
	if (g_Renderer.m_Caps.m_VBO)
	{
		pglGenBuffersARB(1, &m_Handle);
		pglBindBufferARB(m_Target, m_Handle);
		pglBufferDataARB(m_Target, size, 0, m_Usage);
		pglBindBufferARB(m_Target, 0);
	}
	else
	{
		m_SysMem = new u8[size];
	}

	// store max/free vertex counts
	m_MaxVertices=m_FreeVertices=size/vertexSize;
	
	// create sole free chunk
	VBChunk* chunk=new VBChunk;
	chunk->m_Owner=this;
	chunk->m_Count=m_FreeVertices;
	chunk->m_Index=0;
	m_FreeList.push_front(chunk);
}

///////////////////////////////////////////////////////////////////////////////
// CVertexBuffer destructor
CVertexBuffer::~CVertexBuffer()
{
	if (m_Handle)
	{
		pglDeleteBuffersARB(1, &m_Handle);
	}
	else if (m_SysMem)
	{
		delete[] m_SysMem;
	}

	// janwas 2004-06-14: release freelist
	typedef std::list<VBChunk*>::iterator Iter;
	for (Iter iter = m_FreeList.begin(); iter != m_FreeList.end(); ++iter)
		delete *iter;
}

///////////////////////////////////////////////////////////////////////////////
// Allocate: try to allocate a buffer of given number of vertices (each of 
// given size), with the given type, and using the given texture - return null 
// if no free chunks available
CVertexBuffer::VBChunk* CVertexBuffer::Allocate(size_t vertexSize, size_t numVertices, GLenum usage, GLenum target)
{
	// check this is the right kind of buffer
	if (usage != m_Usage || target != m_Target || vertexSize != m_VertexSize)
		return 0;

	// quick check there's enough vertices spare to allocate
	if (numVertices > m_FreeVertices)
		return 0;

	// trawl free list looking for first free chunk with enough space
	VBChunk* chunk=0;
	typedef std::list<VBChunk*>::iterator Iter;
	for (Iter iter=m_FreeList.begin();iter!=m_FreeList.end();++iter) {
		if (numVertices<=(*iter)->m_Count) {
			chunk=*iter;
			// remove this chunk from the free list
			m_FreeList.erase(iter);
			m_FreeVertices -= chunk->m_Count;
			// no need to search further ..
			break;
		}
	}

	if (!chunk) {
		// no big enough spare chunk available
		return 0;
	}

	// split chunk into two; - allocate a new chunk using all unused vertices in the 
	// found chunk, and add it to the free list
	if (chunk->m_Count > numVertices)
	{
		VBChunk* newchunk = new VBChunk;
		newchunk->m_Owner = this;
		newchunk->m_Count = chunk->m_Count - numVertices;
		newchunk->m_Index = chunk->m_Index + numVertices;
		m_FreeList.push_front(newchunk);
		m_FreeVertices += newchunk->m_Count;

		// resize given chunk
		chunk->m_Count = numVertices;
	}
	
	// return found chunk
	return chunk;
}

///////////////////////////////////////////////////////////////////////////////
// Release: return given chunk to this buffer
void CVertexBuffer::Release(VBChunk* chunk)
{
	// add to free list
	// TODO, RC - need to merge available chunks where possible to avoid 
	// excessive fragmentation of vertex buffer space
	m_FreeList.push_front(chunk);
	m_FreeVertices += chunk->m_Count;
}

///////////////////////////////////////////////////////////////////////////////
// UpdateChunkVertices: update vertex data for given chunk
void CVertexBuffer::UpdateChunkVertices(VBChunk* chunk,void* data)
{
	if (g_Renderer.m_Caps.m_VBO)
	{
		debug_assert(m_Handle);
		pglBindBufferARB(m_Target, m_Handle);
		pglBufferSubDataARB(m_Target, chunk->m_Index * m_VertexSize, chunk->m_Count * m_VertexSize, data);
		pglBindBufferARB(m_Target, 0);
	}
	else
	{
		debug_assert(m_SysMem);
		memcpy(m_SysMem + chunk->m_Index * m_VertexSize, data, chunk->m_Count * m_VertexSize);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Bind: bind to this buffer; return pointer to address required as parameter
// to glVertexPointer ( + etc) calls
u8* CVertexBuffer::Bind()
{
	if (g_Renderer.m_Caps.m_VBO)
	{
		pglBindBufferARB(m_Target, m_Handle);
		return (u8*)0;
	}
	else
	{
		return m_SysMem;
	}
}

void CVertexBuffer::Unbind()
{
	if (g_Renderer.m_Caps.m_VBO)
	{
		pglBindBufferARB(GL_ARRAY_BUFFER, 0);
		pglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}

size_t CVertexBuffer::GetBytesReserved() const
{
	return MAX_VB_SIZE_BYTES;
}

size_t CVertexBuffer::GetBytesAllocated() const
{
	return (m_MaxVertices - m_FreeVertices) * m_VertexSize;
}
