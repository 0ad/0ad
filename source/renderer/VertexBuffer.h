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
 * encapsulation of VBOs with batching and sharing
 */

#ifndef INCLUDED_VERTEXBUFFER
#define INCLUDED_VERTEXBUFFER

#include "lib/res/graphics/ogl_tex.h"

#include <list>
#include <vector>

// Absolute maximum (bytewise) size of each GL vertex buffer object.
// Make it large enough for the maximum feasible mesh size (64K vertexes,
// 32 bytes per vertex in ShaderModelRenderer).
// TODO: measure what influence this has on performance
#define MAX_VB_SIZE_BYTES		(4*1024*1024)

/**
 * CVertexBuffer: encapsulation of ARB_vertex_buffer_object, also supplying 
 * some additional functionality for sharing buffers between multiple objects
 */
class CVertexBuffer
{
public:

	/// VBChunk: describes a portion of this vertex buffer
	struct VBChunk
	{
		/// Owning (parent) vertex buffer
		CVertexBuffer* m_Owner;
		/// Start index of this chunk in owner
		size_t m_Index;
		/// Number of vertices used by chunk
		size_t m_Count;

	private:
		// Only CVertexBuffer can construct/delete these
		// (Other people should use g_VBMan.Allocate, g_VBMan.Release)
		friend class CVertexBuffer;
		VBChunk() {}
		~VBChunk() {}
	};

public:
	// constructor, destructor
	CVertexBuffer(size_t vertexSize, GLenum usage, GLenum target);
	~CVertexBuffer();

	/// Bind to this buffer; return pointer to address required as parameter
	/// to glVertexPointer ( + etc) calls
	u8* Bind();

	/// Get the address that Bind() will return, without actually binding
	u8* GetBindAddress();

	/// Unbind any currently-bound buffer, so glVertexPointer etc calls will not attempt to use it
	static void Unbind();

	/// Update vertex data for given chunk. Transfers the provided data to the actual OpenGL vertex buffer.
	void UpdateChunkVertices(VBChunk* chunk, void* data);

	size_t GetVertexSize() const { return m_VertexSize; }
	size_t GetBytesReserved() const;
	size_t GetBytesAllocated() const;

	/// Returns true if this vertex buffer is compatible with the specified vertex type and intended usage.
	bool CompatibleVertexType(size_t vertexSize, GLenum usage, GLenum target);

	void DumpStatus();

protected:
	friend class CVertexBufferManager;		// allow allocate only via CVertexBufferManager
	
	/// Try to allocate a buffer of given number of vertices (each of given size), 
	/// and with the given type - return null if no free chunks available
	VBChunk* Allocate(size_t vertexSize, size_t numVertices, GLenum usage, GLenum target);
	/// Return given chunk to this buffer
	void Release(VBChunk* chunk);
	
	
private:	
	/// Vertex size of this vertex buffer
	size_t m_VertexSize;
	/// Number of vertices of above size in this buffer
	size_t m_MaxVertices;
	/// List of free chunks in this buffer
	std::list<VBChunk*> m_FreeList;
	/// Available free vertices - total of all free vertices in the free list
	size_t m_FreeVertices;
	/// Handle to the actual GL vertex buffer object
	GLuint m_Handle;
	/// Raw system memory for systems not supporting VBOs
	u8* m_SysMem;
	/// Usage type of the buffer (GL_STATIC_DRAW etc)
	GLenum m_Usage;
	/// Buffer target (GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER)
	GLenum m_Target;
};

#endif
