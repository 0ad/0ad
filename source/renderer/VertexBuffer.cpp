///////////////////////////////////////////////////////////////////////////////
//
// Name:		VertexBuffer.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////
#include "precompiled.h"
#include "ogl.h"
#include "Renderer.h"
#include "VertexBuffer.h"

///////////////////////////////////////////////////////////////////////////////
// shared list of all free batch objects
std::vector<CVertexBuffer::Batch*> CVertexBuffer::m_FreeBatches;

///////////////////////////////////////////////////////////////////////////////
// CVertexBuffer constructor 
CVertexBuffer::CVertexBuffer(size_t vertexSize,bool dynamic) 
	: m_VertexSize(vertexSize), m_Dynamic(dynamic), m_SysMem(0), m_Handle(0)
{
	// store max/free vertex counts
	m_MaxVertices=m_FreeVertices=MAX_VB_SIZE_BYTES/vertexSize;

	// allocate raw buffer
	if (g_Renderer.m_Caps.m_VBO) {
		glGenBuffersARB(1,&m_Handle);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_Handle);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB,MAX_VB_SIZE_BYTES,0,m_Dynamic ? GL_STREAM_DRAW_ARB : GL_STATIC_DRAW_ARB);
	} else {
		m_SysMem=new u8[MAX_VB_SIZE_BYTES];
	}

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
	if (m_Handle) {
		glDeleteBuffersARB(1,&m_Handle);
	} else if (m_SysMem) {
		delete[] m_SysMem;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Allocate: try to allocate a buffer of given number of vertices (each of 
// given size), with the given type, and using the given texture - return null 
// if no free chunks available
CVertexBuffer::VBChunk* CVertexBuffer::Allocate(size_t vertexSize,size_t numVertices,bool dynamic)
{
	// check this is the right kind of buffer
	if (dynamic!=m_Dynamic || vertexSize!=m_VertexSize) return 0;

	// quick check there's enough vertices spare to allocate
	if (numVertices>m_FreeVertices) return 0;

	// trawl free list looking for first free chunk with enough space
	VBChunk* chunk=0;
	typedef std::list<VBChunk*>::iterator Iter;
	for (Iter iter=m_FreeList.begin();iter!=m_FreeList.end();++iter) {
		chunk=*iter;
		if (numVertices<=chunk->m_Count) {
			// remove this chunk from the free list
			size_t size1=m_FreeList.size();
			m_FreeList.erase(iter);
			size_t size2=m_FreeList.size();
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
	VBChunk* newchunk=new VBChunk;
	newchunk->m_Owner=this;
	newchunk->m_Count=chunk->m_Count-numVertices;
	newchunk->m_Index=chunk->m_Index+numVertices;
	m_FreeList.push_front(newchunk);

	// resize given chunk, resize total available free vertices
	chunk->m_Count=numVertices;
	m_FreeVertices-=numVertices;
	
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
	m_FreeVertices+=chunk->m_Count;
}

///////////////////////////////////////////////////////////////////////////////
// ClearBatchIndices: clear lists of all batches 
void CVertexBuffer::ClearBatchIndices()
{
	for (uint i=0;i<m_Batches.size();i++) {
		m_Batches[i]->m_IndexData.clear();
		m_FreeBatches.push_back(m_Batches[i]);
	}
	m_Batches.clear();
}


///////////////////////////////////////////////////////////////////////////////
// AppendBatch: add a batch to the render list for this buffer
void CVertexBuffer::AppendBatch(VBChunk* chunk,Handle texture,size_t numIndices,u16* indices)
{
	// try and find a batch using this texture
	size_t i;
	Batch* batch=0;
	for (i=0;i<m_Batches.size();++i) {
		if (m_Batches[i]->m_Texture==texture) {
			batch=m_Batches[i];
			break;
		}
	}
	if (!batch) {
		if (m_FreeBatches.size()) {
			batch=m_FreeBatches.back();
			m_FreeBatches.pop_back();
		} else {
			batch=new Batch;
		}
		m_Batches.push_back(batch);
		batch->m_Texture=texture;
	}

	// resize the chunk's batch to fit it's indices
	size_t cursize=batch->m_IndexData.size();
	batch->m_IndexData.resize(cursize+1);
	// store batch
	batch->m_IndexData[cursize]=std::pair<size_t,u16*>(numIndices,indices);
//	memcpy(&batch->m_Indices[0]+cursize,indices,sizeof(u16)*numIndices);
}


///////////////////////////////////////////////////////////////////////////////
// UpdateChunkVertices: update vertex data for given chunk
void CVertexBuffer::UpdateChunkVertices(VBChunk* chunk,void* data)
{
	if (g_Renderer.m_Caps.m_VBO) {
		assert(m_Handle);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_Handle);
		glBufferSubDataARB(GL_ARRAY_BUFFER_ARB,chunk->m_Index*m_VertexSize,chunk->m_Count*m_VertexSize,data);
	} else {
		assert(m_SysMem);
		memcpy(m_SysMem+chunk->m_Index*m_VertexSize,data,chunk->m_Count*m_VertexSize);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Bind: bind to this buffer; return pointer to address required as parameter
// to glVertexPointer ( + etc) calls
u8* CVertexBuffer::Bind()
{
	u8* base;
	if (g_Renderer.m_Caps.m_VBO) {
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_Handle);
		base=(u8*) 0;
	} else {
		base=(u8*) m_SysMem;
	}
	return base;
}