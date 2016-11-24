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
 * Allocate and destroy CVertexBuffers
 */

#include "precompiled.h"

#include "VertexBufferManager.h"

#include "lib/ogl.h"
#include "ps/CLogger.h"

#define DUMP_VB_STATS 0 // for debugging

CVertexBufferManager g_VBMan;

///////////////////////////////////////////////////////////////////////////////
// Explicit shutdown of the vertex buffer subsystem.
// This avoids the ordering issues that arise when using destructors of
// global instances.
void CVertexBufferManager::Shutdown()
{
	typedef std::list<CVertexBuffer*>::iterator Iter;
	for (Iter iter = m_Buffers.begin(); iter != m_Buffers.end(); ++iter)
		delete *iter;
	m_Buffers.clear();
}


///////////////////////////////////////////////////////////////////////////////
// Allocate: try to allocate a buffer of given number of vertices (each of
// given size), with the given type, and using the given texture - return null
// if no free chunks available
CVertexBuffer::VBChunk* CVertexBufferManager::Allocate(size_t vertexSize, size_t numVertices, GLenum usage, GLenum target, void* backingStore)
{
	CVertexBuffer::VBChunk* result=0;

	ENSURE(usage == GL_STREAM_DRAW || usage == GL_STATIC_DRAW || usage == GL_DYNAMIC_DRAW);

	ENSURE(target == GL_ARRAY_BUFFER || target == GL_ELEMENT_ARRAY_BUFFER);

	if (CVertexBuffer::UseStreaming(usage))
		ENSURE(backingStore != NULL);

	// TODO, RC - run some sanity checks on allocation request

	typedef std::list<CVertexBuffer*>::iterator Iter;

#if DUMP_VB_STATS
	debug_printf("\n============================\n# allocate vsize=%d nverts=%d\n\n", vertexSize, numVertices);
	for (Iter iter = m_Buffers.begin(); iter != m_Buffers.end(); ++iter) {
		CVertexBuffer* buffer = *iter;
		if (buffer->CompatibleVertexType(vertexSize, usage, target))
		{
			debug_printf("%p\n", buffer);
			buffer->DumpStatus();
		}
	}
#endif

	// iterate through all existing buffers testing for one that'll
	// satisfy the allocation
	for (Iter iter = m_Buffers.begin(); iter != m_Buffers.end(); ++iter) {
		CVertexBuffer* buffer = *iter;
		result = buffer->Allocate(vertexSize, numVertices, usage, target, backingStore);
		if (result)
			return result;
	}

	// got this far; need to allocate a new buffer
	CVertexBuffer* buffer = new CVertexBuffer(vertexSize, usage, target);
	m_Buffers.push_front(buffer);
	result = buffer->Allocate(vertexSize, numVertices, usage, target, backingStore);

	if (!result)
	{
		LOGERROR("Failed to create VBOs (%lu*%lu)", (unsigned long)vertexSize, (unsigned long)numVertices);
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////
// Release: return given chunk to its owner
void CVertexBufferManager::Release(CVertexBuffer::VBChunk* chunk)
{
	ENSURE(chunk);
#if DUMP_VB_STATS
	debug_printf("\n============================\n# release %p nverts=%d\n\n", chunk, chunk->m_Count);
#endif
	chunk->m_Owner->Release(chunk);
}


size_t CVertexBufferManager::GetBytesReserved()
{
	size_t total = 0;

	typedef std::list<CVertexBuffer*>::iterator Iter;
	for (Iter iter = m_Buffers.begin(); iter != m_Buffers.end(); ++iter)
		total += (*iter)->GetBytesReserved();

	return total;
}

size_t CVertexBufferManager::GetBytesAllocated()
{
	size_t total = 0;

	typedef std::list<CVertexBuffer*>::iterator Iter;
	for (Iter iter = m_Buffers.begin(); iter != m_Buffers.end(); ++iter)
		total += (*iter)->GetBytesAllocated();

	return total;
}
