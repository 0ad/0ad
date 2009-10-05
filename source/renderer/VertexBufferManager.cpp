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

/*
 * Allocate and destroy CVertexBuffers
 */

#include "precompiled.h"

#include "lib/ogl.h"
#include "VertexBufferManager.h"
#include "ps/CLogger.h"

#define LOG_CATEGORY "graphics"

// janwas 2004-06-14: added dtor

CVertexBufferManager::~CVertexBufferManager()
{
}

///////////////////////////////////////////////////////////////////////////////
// Explicit shutdown of the vertex buffer subsystem.
// This avoids the ordering issues that arise when using destructors of
// global instances.
void CVertexBufferManager::Shutdown()
{
	typedef std::list<CVertexBuffer*>::iterator Iter;
	for (Iter iter=m_Buffers.begin();iter!=m_Buffers.end();++iter)
		delete *iter;
	
	CVertexBuffer::Shutdown();
}


///////////////////////////////////////////////////////////////////////////////
// Allocate: try to allocate a buffer of given number of vertices (each of 
// given size), with the given type, and using the given texture - return null 
// if no free chunks available
CVertexBuffer::VBChunk* CVertexBufferManager::Allocate(size_t vertexSize, size_t numVertices, bool dynamic)
{
	CVertexBuffer::VBChunk* result=0;

	// TODO, RC - run some sanity checks on allocation request

	// iterate through all existing buffers testing for one that'll 
	// satisfy the allocation
	typedef std::list<CVertexBuffer*>::iterator Iter;
	for (Iter iter = m_Buffers.begin(); iter != m_Buffers.end(); ++iter) {
		CVertexBuffer* buffer = *iter;
		result = buffer->Allocate(vertexSize, numVertices, dynamic);
		if (result)
			return result;
	}

	// got this far; need to allocate a new buffer
	CVertexBuffer* buffer = new CVertexBuffer(vertexSize, dynamic);
	m_Buffers.push_front(buffer);
	result = buffer->Allocate(vertexSize, numVertices, dynamic);
	
	if (!result)
	{
		LOG(CLogger::Error, LOG_CATEGORY, "Failed to create VBOs (%lu*%lu)", (unsigned long)vertexSize, (unsigned long)numVertices);
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////
// Release: return given chunk to its owner
void CVertexBufferManager::Release(CVertexBuffer::VBChunk* chunk)
{
	debug_assert(chunk);
	chunk->m_Owner->Release(chunk);
}

///////////////////////////////////////////////////////////////////////////////
// ClearBatchIndices: empty out the batch lists of all vertex buffers
void CVertexBufferManager::ClearBatchIndices()
{
	typedef std::list<CVertexBuffer*>::iterator Iter;
	for (Iter iter=m_Buffers.begin();iter!=m_Buffers.end();++iter) {
		CVertexBuffer* buffer=*iter;
		buffer->ClearBatchIndices();
	}
}
