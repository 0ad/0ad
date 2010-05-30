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
 * encapsulation of VBOs with batching and sharing
 */

#ifndef INCLUDED_VERTEXBUFFER
#define INCLUDED_VERTEXBUFFER

#include "lib/res/graphics/ogl_tex.h"

#include <list>
#include <vector>

// absolute maximum (bytewise) size of each GL vertex buffer object
#define MAX_VB_SIZE_BYTES		(512*1024)

template <typename T>
struct ctor_dtor_logger;

///////////////////////////////////////////////////////////////////////////////
// CVertexBuffer: encapsulation of ARB_vertex_buffer_object, also supplying 
// some additional functionality for batching and sharing buffers between
// multiple objects
class CVertexBuffer
{
public:
	// Batch: batch definition - defines indices into the VB to use when rendering,
	// and the texture used when doing so
	struct Batch {
		// list of indices into the vertex buffer of primitives within the batch
		std::vector<std::pair<size_t,u16*> > m_IndexData;
		// texture to apply when rendering batch
		Handle m_Texture;
	};

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
	CVertexBuffer(size_t vertexSize, bool dynamic);
	~CVertexBuffer();

	// bind to this buffer; return pointer to address required as parameter
	// to glVertexPointer ( + etc) calls
	u8* Bind();

	// unbind any currently-bound buffer, so glVertexPointer etc calls will not attempt to use it
	static void Unbind();

	// clear lists of all batches 
	void ClearBatchIndices();

	// add a batch to the render list for this buffer
	void AppendBatch(VBChunk* chunk,Handle texture,size_t numIndices,u16* indices);

	// update vertex data for given chunk
	void UpdateChunkVertices(VBChunk* chunk,void* data);

	// return this VBs batch list
	const std::vector<Batch*>& GetBatches() const { return m_Batches; }

	size_t GetVertexSize() const { return m_VertexSize; }

	// free memory
	static void Shutdown();

protected:
	friend class CVertexBufferManager;		// allow allocate only via CVertexBufferManager
	
	// try to allocate a buffer of given number of vertices (each of given size), 
	// and with the given type - return null if no free chunks available
	VBChunk* Allocate(size_t vertexSize,size_t numVertices,bool dynamic);
	// return given chunk to this buffer
	void Release(VBChunk* chunk);
	
	
private:	
	// set of all possible batches that can be used by this VB
	std::vector<Batch*> m_Batches;
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
	// type of the buffer - dynamic?
	bool m_Dynamic;
	
	// list of all spare batches, shared between all vbs
	static std::vector<Batch *> m_FreeBatches;
};

#endif
