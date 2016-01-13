/* Copyright (C) 2016 Wildfire Games.
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

// Absolute maximum (bytewise) size of each GL vertex buffer object.
// Make it large enough for the maximum feasible mesh size (64K vertexes,
// 64 bytes per vertex in InstancingModelRenderer).
// TODO: measure what influence this has on performance
#define MAX_VB_SIZE_BYTES		(4*1024*1024)

CVertexBuffer::CVertexBuffer(size_t vertexSize, GLenum usage, GLenum target)
	: m_VertexSize(vertexSize), m_Handle(0), m_SysMem(0), m_Usage(usage), m_Target(target)
{
	size_t size = MAX_VB_SIZE_BYTES;

	if (target == GL_ARRAY_BUFFER) // vertex data buffer
	{
		// We want to store 16-bit indices to any vertex in a buffer, so the
		// buffer must never be bigger than vertexSize*64K bytes since we can 
		// address at most 64K of them with 16-bit indices
		size = std::min(size, vertexSize*65536);
	}

	// store max/free vertex counts
	m_MaxVertices = m_FreeVertices = size / vertexSize;

	// allocate raw buffer
	if (g_Renderer.m_Caps.m_VBO)
	{
		pglGenBuffersARB(1, &m_Handle);
		pglBindBufferARB(m_Target, m_Handle);
		pglBufferDataARB(m_Target, m_MaxVertices * m_VertexSize, 0, m_Usage);
		pglBindBufferARB(m_Target, 0);
	}
	else
	{
		m_SysMem = new u8[m_MaxVertices * m_VertexSize];
	}

	// create sole free chunk
	VBChunk* chunk = new VBChunk;
	chunk->m_Owner = this;
	chunk->m_Count = m_FreeVertices;
	chunk->m_Index = 0;
	m_FreeList.push_front(chunk);
}

CVertexBuffer::~CVertexBuffer()
{
	// Must have released all chunks before destroying the buffer
	ENSURE(m_AllocList.empty());

	if (m_Handle)
		pglDeleteBuffersARB(1, &m_Handle);

	delete[] m_SysMem;

	typedef std::list<VBChunk*>::iterator Iter;
	for (Iter iter = m_FreeList.begin(); iter != m_FreeList.end(); ++iter)
		delete *iter;
}


bool CVertexBuffer::CompatibleVertexType(size_t vertexSize, GLenum usage, GLenum target)
{
	if (usage != m_Usage || target != m_Target || vertexSize != m_VertexSize)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Allocate: try to allocate a buffer of given number of vertices (each of 
// given size), with the given type, and using the given texture - return null 
// if no free chunks available
CVertexBuffer::VBChunk* CVertexBuffer::Allocate(size_t vertexSize, size_t numVertices, GLenum usage, GLenum target, void* backingStore)
{
	// check this is the right kind of buffer
	if (!CompatibleVertexType(vertexSize, usage, target))
		return 0;

	if (UseStreaming(usage))
		ENSURE(backingStore != NULL);

	// quick check there's enough vertices spare to allocate
	if (numVertices > m_FreeVertices)
		return 0;

	// trawl free list looking for first free chunk with enough space
	VBChunk* chunk = 0;
	typedef std::list<VBChunk*>::iterator Iter;
	for (Iter iter = m_FreeList.begin(); iter != m_FreeList.end(); ++iter) {
		if (numVertices <= (*iter)->m_Count) {
			chunk = *iter;
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

	chunk->m_BackingStore = backingStore;
	chunk->m_Dirty = false;
	chunk->m_Needed = false;

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
	m_AllocList.push_back(chunk);
	return chunk;
}

///////////////////////////////////////////////////////////////////////////////
// Release: return given chunk to this buffer
void CVertexBuffer::Release(VBChunk* chunk)
{
	// Update total free count before potentially modifying this chunk's count
	m_FreeVertices += chunk->m_Count;

	m_AllocList.remove(chunk);

	typedef std::list<VBChunk*>::iterator Iter;

	// Coalesce with any free-list items that are adjacent to this chunk;
	// merge the found chunk with the new one, and remove the old one
	// from the list, and repeat until no more are found
	bool coalesced;
	do
	{
		coalesced = false;
		for (Iter iter = m_FreeList.begin(); iter != m_FreeList.end(); ++iter)
		{
			if ((*iter)->m_Index == chunk->m_Index + chunk->m_Count
			 || (*iter)->m_Index + (*iter)->m_Count == chunk->m_Index)
			{
				chunk->m_Index = std::min(chunk->m_Index, (*iter)->m_Index);
				chunk->m_Count += (*iter)->m_Count;
				delete *iter;
				m_FreeList.erase(iter);
				coalesced = true;
				break;
			}
		}
	}
	while (coalesced);

	m_FreeList.push_front(chunk);
}

///////////////////////////////////////////////////////////////////////////////
// UpdateChunkVertices: update vertex data for given chunk
void CVertexBuffer::UpdateChunkVertices(VBChunk* chunk, void* data)
{
	if (g_Renderer.m_Caps.m_VBO)
	{
		ENSURE(m_Handle);
		if (UseStreaming(m_Usage))
		{
			// The VBO is now out of sync with the backing store
			chunk->m_Dirty = true;

			// Sanity check: Make sure the caller hasn't tried to reallocate
			// their backing store
			ENSURE(data == chunk->m_BackingStore);
		}
		else
		{
			pglBindBufferARB(m_Target, m_Handle);
			pglBufferSubDataARB(m_Target, chunk->m_Index * m_VertexSize, chunk->m_Count * m_VertexSize, data);
			pglBindBufferARB(m_Target, 0);
		}
	}
	else
	{
		ENSURE(m_SysMem);
		memcpy(m_SysMem + chunk->m_Index * m_VertexSize, data, chunk->m_Count * m_VertexSize);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Bind: bind to this buffer; return pointer to address required as parameter
// to glVertexPointer ( + etc) calls
u8* CVertexBuffer::Bind()
{
	if (!g_Renderer.m_Caps.m_VBO)
		return m_SysMem;

	pglBindBufferARB(m_Target, m_Handle);

	if (UseStreaming(m_Usage))
	{
		// If any chunks are out of sync with the current VBO, and are
		// needed for rendering this frame, we'll need to re-upload the VBO
		bool needUpload = false;
		for (VBChunk* const& chunk : m_AllocList)
		{
			if (chunk->m_Dirty && chunk->m_Needed)
			{
				needUpload = true;
				break;
			}
		}

		if (needUpload)
		{
			// Tell the driver that it can reallocate the whole VBO
			pglBufferDataARB(m_Target, m_MaxVertices * m_VertexSize, NULL, m_Usage);

			// (In theory, glMapBufferRange with GL_MAP_INVALIDATE_BUFFER_BIT could be used
			// here instead of glBufferData(..., NULL, ...) plus glMapBuffer(), but with
			// current Intel Windows GPU drivers (as of 2015-01) it's much faster if you do
			// the explicit glBufferData.)

			while (true)
			{
				void* p = pglMapBufferARB(m_Target, GL_WRITE_ONLY);
				if (p == NULL)
				{
					// This shouldn't happen unless we run out of virtual address space
					LOGERROR("glMapBuffer failed");
					break;
				}

#ifndef NDEBUG
				// To help detect bugs where PrepareForRendering() was not called,
				// force all not-needed data to 0, so things won't get rendered
				// with undefined (but possibly still correct-looking) data.
				memset(p, 0, m_MaxVertices * m_VertexSize);
#endif

				// Copy only the chunks we need. (This condition is helpful when
				// the VBO contains data for every unit in the world, but only a
				// handful are visible on screen and we don't need to bother copying
				// the rest.)
				for (VBChunk* const& chunk : m_AllocList)
					if (chunk->m_Needed)
						memcpy((u8 *)p + chunk->m_Index * m_VertexSize, chunk->m_BackingStore, chunk->m_Count * m_VertexSize);

				if (pglUnmapBufferARB(m_Target) == GL_TRUE)
					break;

				// Unmap might fail on e.g. resolution switches, so just try again
				// and hope it will eventually succeed
				debug_printf("glUnmapBuffer failed, trying again...\n");
			}

			// Anything we just uploaded is clean; anything else is dirty
			// since the rest of the VBO content is now undefined
			for (VBChunk* const& chunk : m_AllocList)
			{
				if (chunk->m_Needed)
					chunk->m_Dirty = false;
				else
					chunk->m_Dirty = true;
			}
		}

		// Reset the flags for the next phase
		for (VBChunk* const& chunk : m_AllocList)
			chunk->m_Needed = false;
	}

	return (u8*)0;
}

u8* CVertexBuffer::GetBindAddress()
{
	if (g_Renderer.m_Caps.m_VBO)
		return (u8*)0;
	else
		return m_SysMem;
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

void CVertexBuffer::DumpStatus()
{
	debug_printf("freeverts = %d\n", (int)m_FreeVertices);

	size_t maxSize = 0;
	typedef std::list<VBChunk*>::iterator Iter;
	for (Iter iter = m_FreeList.begin(); iter != m_FreeList.end(); ++iter)
	{
		debug_printf("free chunk %p: size=%d\n", (void *)*iter, (int)((*iter)->m_Count));
		maxSize = std::max((*iter)->m_Count, maxSize);
	}
	debug_printf("max size = %d\n", (int)maxSize);
}

bool CVertexBuffer::UseStreaming(GLenum usage)
{
	return (usage == GL_DYNAMIC_DRAW || usage == GL_STREAM_DRAW);
}
