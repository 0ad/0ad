///////////////////////////////////////////////////////////////////////////////
//
// Name:		VertexBufferManager.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _VERTEXBUFFERMANAGER_H
#define _VERTEXBUFFERMANAGER_H

#include "VertexBuffer.h"

///////////////////////////////////////////////////////////////////////////////
// CVertexBufferManager: owner object for CVertexBuffer objects; acts as
// 'front end' for their allocation and destruction 
class CVertexBufferManager
{
public:
	CVertexBufferManager() {}
	~CVertexBufferManager();

	// try to allocate a buffer of given number of vertices (each of given size), 
	// and with the given type - return null if no free chunks available
	CVertexBuffer::VBChunk* Allocate(size_t vertexSize,size_t numVertices,bool dynamic);

	// return given chunk to it's owner
	void CVertexBufferManager::Release(CVertexBuffer::VBChunk* chunk);

	// empty out the batch lists of all vertex buffers
	void ClearBatchIndices();

	// return list of all buffers
	const std::list<CVertexBuffer*>& GetBufferList() const { return m_Buffers; }

private:
	// list of all known vertex buffers
	std::list<CVertexBuffer*> m_Buffers;
};

extern CVertexBufferManager g_VBMan;


#endif
