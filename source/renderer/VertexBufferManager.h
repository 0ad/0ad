/* Copyright (C) 2012 Wildfire Games.
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

#ifndef INCLUDED_VERTEXBUFFERMANAGER
#define INCLUDED_VERTEXBUFFERMANAGER

#include "VertexBuffer.h"

///////////////////////////////////////////////////////////////////////////////
// CVertexBufferManager: owner object for CVertexBuffer objects; acts as
// 'front end' for their allocation and destruction 
class CVertexBufferManager
{
public:
	
	/**
	 * Try to allocate a vertex buffer of the given size and type.
	 *
	 * @param vertexSize size of each vertex in the buffer
	 * @param numVertices number of vertices in the buffer
	 * @param usage typically GL_STATIC_DRAW or GL_DYNAMIC_DRAW
	 * @param target typically GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER
	 * @return chunk, or NULL if no free chunks available
	 */
	CVertexBuffer::VBChunk* Allocate(size_t vertexSize, size_t numVertices, GLenum usage, GLenum target);

	/// Returns the given @p chunk to its owning buffer
	void Release(CVertexBuffer::VBChunk* chunk);

	/// Returns a list of all buffers
	const std::list<CVertexBuffer*>& GetBufferList() const { return m_Buffers; }

	size_t GetBytesReserved();
	size_t GetBytesAllocated();

	/// Returns the maximum possible size of a single vertex buffer
	size_t GetMaxBufferSize() const { return MAX_VB_SIZE_BYTES; }

	/// Explicit shutdown of the vertex buffer subsystem; releases all currently-allocated buffers.
	void Shutdown();

private:
	/// List of all known vertex buffers
	std::list<CVertexBuffer*> m_Buffers;
};

extern CVertexBufferManager g_VBMan;

#endif
