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
 * encapsulation of VBOs with batching and sharing
 */

#ifndef INCLUDED_VERTEXBUFFER
#define INCLUDED_VERTEXBUFFER

#include "lib/res/graphics/ogl_tex.h"

#include <list>
#include <vector>

// absolute maximum (bytewise) size of each GL vertex buffer object
#define MAX_VB_SIZE_BYTES		(512*1024)

///////////////////////////////////////////////////////////////////////////////
// CVertexBuffer: encapsulation of ARB_vertex_buffer_object, also supplying 
// some additional functionality for sharing buffers between multiple objects
class CVertexBuffer
{
public:
	// VBChunk: describes a portion of this vertex buffer
	struct VBChunk
	{
		// owning buffer
		CVertexBuffer* m_Owner;
		// start index of this chunk in owner
		size_t m_Index;
		// number of vertices used by chunk
		size_t m_Count;
	};

public:
	// constructor, destructor
	CVertexBuffer(size_t vertexSize, GLenum usage, GLenum target);
	~CVertexBuffer();

	// bind to this buffer; return pointer to address required as parameter
	// to glVertexPointer ( + etc) calls
	u8* Bind();

	// unbind any currently-bound buffer, so glVertexPointer etc calls will not attempt to use it
	static void Unbind();

	// update vertex data for given chunk
	void UpdateChunkVertices(VBChunk* chunk, void* data);

	size_t GetVertexSize() const { return m_VertexSize; }

	size_t GetBytesReserved() const;
	size_t GetBytesAllocated() const;

protected:
	friend class CVertexBufferManager;		// allow allocate only via CVertexBufferManager
	
	// try to allocate a buffer of given number of vertices (each of given size), 
	// and with the given type - return null if no free chunks available
	VBChunk* Allocate(size_t vertexSize, size_t numVertices, GLenum usage, GLenum target);
	// return given chunk to this buffer
	void Release(VBChunk* chunk);
	
	
private:	
	// vertex size of this vertex buffer
	size_t m_VertexSize;
	// number of vertices of above size in this buffer
	size_t m_MaxVertices;
	// list of free chunks in this buffer
	std::list<VBChunk*> m_FreeList;
	// available free vertices - total of all free vertices in the free list
	size_t m_FreeVertices;
	// handle to the actual GL vertex buffer object
	GLuint m_Handle;
	// raw system memory for systems not supporting VBOs
	u8* m_SysMem;
	// usage type of the buffer (GL_STATIC_DRAW etc)
	GLenum m_Usage;
	// buffer target (GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER)
	GLenum m_Target;
};

#endif
