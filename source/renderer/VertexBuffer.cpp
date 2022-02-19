/* Copyright (C) 2022 Wildfire Games.
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

#include "precompiled.h"

#include "VertexBuffer.h"

#include "lib/ogl.h"
#include "lib/sysdep/cpu.h"
#include "ps/CLogger.h"
#include "ps/Errors.h"
#include "ps/VideoMode.h"
#include "renderer/backend/gl/Device.h"
#include "renderer/Renderer.h"

#include <algorithm>
#include <cstring>
#include <iterator>

// Absolute maximum (bytewise) size of each GL vertex buffer object.
// Make it large enough for the maximum feasible mesh size (64K vertexes,
// 64 bytes per vertex in InstancingModelRenderer).
// TODO: measure what influence this has on performance
constexpr std::size_t MAX_VB_SIZE_BYTES = 4 * 1024 * 1024;

CVertexBuffer::CVertexBuffer(
	const char* name, const size_t vertexSize,
	const Renderer::Backend::GL::CBuffer::Type type, const bool dynamic)
	: CVertexBuffer(name, vertexSize, type, dynamic, MAX_VB_SIZE_BYTES)
{
}

CVertexBuffer::CVertexBuffer(
	const char* name, const size_t vertexSize,
	const Renderer::Backend::GL::CBuffer::Type type, const bool dynamic,
	const size_t maximumBufferSize)
	: m_VertexSize(vertexSize), m_HasNeededChunks(false)
{
	size_t size = maximumBufferSize;

	if (type == Renderer::Backend::GL::CBuffer::Type::VERTEX)
	{
		// We want to store 16-bit indices to any vertex in a buffer, so the
		// buffer must never be bigger than vertexSize*64K bytes since we can
		// address at most 64K of them with 16-bit indices
		size = std::min(size, vertexSize * 65536);
	}

	// store max/free vertex counts
	m_MaxVertices = m_FreeVertices = size / vertexSize;

	m_Buffer = g_VideoMode.GetBackendDevice()->CreateBuffer(
		name, type, m_MaxVertices * m_VertexSize, dynamic);

	// create sole free chunk
	VBChunk* chunk = new VBChunk;
	chunk->m_Owner = this;
	chunk->m_Count = m_FreeVertices;
	chunk->m_Index = 0;
	m_FreeList.emplace_back(chunk);
}

CVertexBuffer::~CVertexBuffer()
{
	// Must have released all chunks before destroying the buffer
	ENSURE(m_AllocList.empty());

	m_Buffer.reset();

	for (VBChunk* const& chunk : m_FreeList)
		delete chunk;
}

bool CVertexBuffer::CompatibleVertexType(
	const size_t vertexSize, const Renderer::Backend::GL::CBuffer::Type type,
	const bool dynamic) const
{
	ENSURE(m_Buffer);
	return type == m_Buffer->GetType() && dynamic == m_Buffer->IsDynamic() && vertexSize == m_VertexSize;
}

///////////////////////////////////////////////////////////////////////////////
// Allocate: try to allocate a buffer of given number of vertices (each of
// given size), with the given type, and using the given texture - return null
// if no free chunks available
CVertexBuffer::VBChunk* CVertexBuffer::Allocate(
	const size_t vertexSize, const size_t numberOfVertices,
	const Renderer::Backend::GL::CBuffer::Type type, const bool dynamic,
	void* backingStore)
{
	// check this is the right kind of buffer
	if (!CompatibleVertexType(vertexSize, type, dynamic))
		return nullptr;

	if (UseStreaming(dynamic))
		ENSURE(backingStore != nullptr);

	// quick check there's enough vertices spare to allocate
	if (numberOfVertices > m_FreeVertices)
		return nullptr;

	// trawl free list looking for first free chunk with enough space
	std::vector<VBChunk*>::iterator best_iter = m_FreeList.end();
	for (std::vector<VBChunk*>::iterator iter = m_FreeList.begin(); iter != m_FreeList.end(); ++iter)
	{
		if (numberOfVertices == (*iter)->m_Count)
		{
			best_iter = iter;
			break;
		}
		else if (numberOfVertices < (*iter)->m_Count && (best_iter == m_FreeList.end() || (*best_iter)->m_Count < (*iter)->m_Count))
			best_iter = iter;
	}

	// We could not find a large enough chunk.
	if (best_iter == m_FreeList.end())
		return nullptr;

	VBChunk* chunk = *best_iter;
	m_FreeList.erase(best_iter);
	m_FreeVertices -= chunk->m_Count;

	chunk->m_BackingStore = backingStore;
	chunk->m_Dirty = false;
	chunk->m_Needed = false;

	// split chunk into two; - allocate a new chunk using all unused vertices in the
	// found chunk, and add it to the free list
	if (chunk->m_Count > numberOfVertices)
	{
		VBChunk* newchunk = new VBChunk;
		newchunk->m_Owner = this;
		newchunk->m_Count = chunk->m_Count - numberOfVertices;
		newchunk->m_Index = chunk->m_Index + numberOfVertices;
		m_FreeList.emplace_back(newchunk);
		m_FreeVertices += newchunk->m_Count;

		// resize given chunk
		chunk->m_Count = numberOfVertices;
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

	m_AllocList.erase(std::find(m_AllocList.begin(), m_AllocList.end(), chunk));

	// Sorting O(nlogn) shouldn't be too far from O(n) by performance, because
	// the container is partly sorted already.
	std::sort(
		m_FreeList.begin(), m_FreeList.end(),
		[](const VBChunk* chunk1, const VBChunk* chunk2) -> bool
		{
			return chunk1->m_Index < chunk2->m_Index;
		});

	// Coalesce with any free-list items that are adjacent to this chunk;
	// merge the found chunk with the new one, and remove the old one
	// from the list.
	for (std::vector<VBChunk*>::iterator iter = m_FreeList.begin(); iter != m_FreeList.end();)
	{
		if ((*iter)->m_Index == chunk->m_Index + chunk->m_Count
		 || (*iter)->m_Index + (*iter)->m_Count == chunk->m_Index)
		{
			chunk->m_Index = std::min(chunk->m_Index, (*iter)->m_Index);
			chunk->m_Count += (*iter)->m_Count;
			delete *iter;
			iter = m_FreeList.erase(iter);
			if (!m_FreeList.empty() && iter != m_FreeList.begin())
				iter = std::prev(iter);
		}
		else
		{
			++iter;
		}
	}

	m_FreeList.emplace_back(chunk);
}

///////////////////////////////////////////////////////////////////////////////
// UpdateChunkVertices: update vertex data for given chunk
void CVertexBuffer::UpdateChunkVertices(VBChunk* chunk, void* data)
{
	ENSURE(m_Buffer);
	if (UseStreaming(m_Buffer->IsDynamic()))
	{
		// The VBO is now out of sync with the backing store
		chunk->m_Dirty = true;

		// Sanity check: Make sure the caller hasn't tried to reallocate
		// their backing store
		ENSURE(data == chunk->m_BackingStore);
	}
	else
	{
		g_Renderer.GetDeviceCommandContext()->UploadBufferRegion(
			m_Buffer.get(), data, chunk->m_Index * m_VertexSize, chunk->m_Count * m_VertexSize);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Bind: bind to this buffer; return pointer to address required as parameter
// to glVertexPointer ( + etc) calls
u8* CVertexBuffer::Bind(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext)
{
	if (UseStreaming(m_Buffer->IsDynamic()))
	{
		if (!m_HasNeededChunks)
		{
			deviceCommandContext->BindBuffer(m_Buffer->GetType(), m_Buffer.get());
			return nullptr;
		}

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
			deviceCommandContext->UploadBuffer(m_Buffer.get(), [&](u8* mappedData)
			{
#ifndef NDEBUG
				// To help detect bugs where PrepareForRendering() was not called,
				// force all not-needed data to 0, so things won't get rendered
				// with undefined (but possibly still correct-looking) data.
				memset(mappedData, 0, m_MaxVertices * m_VertexSize);
#endif

				// Copy only the chunks we need. (This condition is helpful when
				// the VBO contains data for every unit in the world, but only a
				// handful are visible on screen and we don't need to bother copying
				// the rest.)
				for (VBChunk* const& chunk : m_AllocList)
					if (chunk->m_Needed)
						std::memcpy(mappedData + chunk->m_Index * m_VertexSize, chunk->m_BackingStore, chunk->m_Count * m_VertexSize);
			});

			// Anything we just uploaded is clean; anything else is dirty
			// since the rest of the VBO content is now undefined
			for (VBChunk* const& chunk : m_AllocList)
			{
				if (chunk->m_Needed)
				{
					chunk->m_Dirty = false;
					chunk->m_Needed = false;
				}
				else
					chunk->m_Dirty = true;
			}
		}
		else
		{
			// Reset the flags for the next phase.
			for (VBChunk* const& chunk : m_AllocList)
				chunk->m_Needed = false;
		}

		m_HasNeededChunks = false;
	}
	deviceCommandContext->BindBuffer(m_Buffer->GetType(), m_Buffer.get());

	return nullptr;
}

void CVertexBuffer::Unbind(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext)
{
	deviceCommandContext->BindBuffer(
		Renderer::Backend::GL::CBuffer::Type::VERTEX, nullptr);
	deviceCommandContext->BindBuffer(
		Renderer::Backend::GL::CBuffer::Type::INDEX, nullptr);
}

size_t CVertexBuffer::GetBytesReserved() const
{
	return MAX_VB_SIZE_BYTES;
}

size_t CVertexBuffer::GetBytesAllocated() const
{
	return (m_MaxVertices - m_FreeVertices) * m_VertexSize;
}

void CVertexBuffer::DumpStatus() const
{
	debug_printf("freeverts = %d\n", static_cast<int>(m_FreeVertices));

	size_t maxSize = 0;
	for (VBChunk* const& chunk : m_FreeList)
	{
		debug_printf("free chunk %p: size=%d\n", static_cast<void *>(chunk), static_cast<int>(chunk->m_Count));
		maxSize = std::max(chunk->m_Count, maxSize);
	}
	debug_printf("max size = %d\n", static_cast<int>(maxSize));
}

bool CVertexBuffer::UseStreaming(const bool dynamic)
{
	return dynamic;
}

void CVertexBuffer::PrepareForRendering(VBChunk* chunk)
{
	chunk->m_Needed = true;
	m_HasNeededChunks = true;
}
