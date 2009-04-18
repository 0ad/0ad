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

#ifndef INCLUDED_VERTEXBUFFERMANAGER
#define INCLUDED_VERTEXBUFFERMANAGER

#include "VertexBuffer.h"

///////////////////////////////////////////////////////////////////////////////
// CVertexBufferManager: owner object for CVertexBuffer objects; acts as
// 'front end' for their allocation and destruction 
class CVertexBufferManager
{
public:
	CVertexBufferManager() {}
	~CVertexBufferManager();

	// Explicit shutdown of the vertex buffer subsystem
	void Shutdown();
	
	// try to allocate a buffer of given number of vertices (each of given size), 
	// and with the given type - return null if no free chunks available
	CVertexBuffer::VBChunk* Allocate(size_t vertexSize,size_t numVertices,bool dynamic);

	// return given chunk to it's owner
	void Release(CVertexBuffer::VBChunk* chunk);

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
