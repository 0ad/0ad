///////////////////////////////////////////////////////////////////////////////
//
// Name:		VertexBufferManager.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////
#include "precompiled.h"
#include <assert.h>
#include "ogl.h"
#include "VertexBufferManager.h"

CVertexBufferManager g_VBMan;


///////////////////////////////////////////////////////////////////////////////
// Allocate: try to allocate a buffer of given number of vertices (each of 
// given size), with the given type, and using the given texture - return null 
// if no free chunks available
CVertexBuffer::VBChunk* CVertexBufferManager::Allocate(size_t vertexSize,size_t numVertices,bool dynamic)
{
	CVertexBuffer::VBChunk* result=0;

	// TODO, RC - run some sanity checks on allocation request

	// iterate through all existing buffers testing for one that'll 
	// satisfy the allocation
	typedef std::list<CVertexBuffer*>::iterator Iter;
	for (Iter iter=m_Buffers.begin();iter!=m_Buffers.end();++iter) {
		CVertexBuffer* buffer=*iter;
		result=buffer->Allocate(vertexSize,numVertices,dynamic);
		if (result) return result;
	}

	// got this far; need to allocate a new buffer
	CVertexBuffer* buffer=new CVertexBuffer(vertexSize,dynamic);
	m_Buffers.push_front(buffer);
	result=buffer->Allocate(vertexSize,numVertices,dynamic);
	
	// TODO, RC - assert not really suitable?  probably need to handle "failed to create 
	// VBO case" better
	assert(result);

	return result;
}

///////////////////////////////////////////////////////////////////////////////
// Release: return given chunk to it's owner
void CVertexBufferManager::Release(CVertexBuffer::VBChunk* chunk)
{
	assert(chunk);
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