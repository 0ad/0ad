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

#include "VertexBufferManager.h"

#include "lib/ogl.h"
#include "ps/CLogger.h"

#define DUMP_VB_STATS 0 // for debugging

CVertexBufferManager g_VBMan;

CVertexBufferManager::Handle::Handle(Handle&& other)
	: m_Chunk(other.m_Chunk)
{
	other.m_Chunk = nullptr;
}

CVertexBufferManager::Handle& CVertexBufferManager::Handle::operator=(Handle&& other)
{
	if (&other == this)
		return *this;

	if (IsValid())
		Reset();

	Handle tmp(std::move(other));
	swap(*this, tmp);

	return *this;
}

CVertexBufferManager::Handle::Handle(CVertexBuffer::VBChunk* chunk)
	: m_Chunk(chunk)
{
}

void CVertexBufferManager::Handle::Reset()
{
	if (!IsValid())
		return;

	g_VBMan.Release(m_Chunk);
	m_Chunk = nullptr;
}

// Explicit shutdown of the vertex buffer subsystem.
// This avoids the ordering issues that arise when using destructors of
// global instances.
void CVertexBufferManager::Shutdown()
{
	for (int group = static_cast<int>(Group::DEFAULT); group < static_cast<int>(Group::COUNT); ++group)
		m_Buffers[group].clear();
}

/**
 * AllocateChunk: try to allocate a buffer of given number of vertices (each of
 * given size), with the given type, and using the given texture - return null
 * if no free chunks available
 */
CVertexBufferManager::Handle CVertexBufferManager::AllocateChunk(size_t vertexSize, size_t numVertices, GLenum usage, GLenum target, void* backingStore, Group group)
{
	CVertexBuffer::VBChunk* result = nullptr;

	ENSURE(usage == GL_STREAM_DRAW || usage == GL_STATIC_DRAW || usage == GL_DYNAMIC_DRAW);

	ENSURE(target == GL_ARRAY_BUFFER || target == GL_ELEMENT_ARRAY_BUFFER);

	if (CVertexBuffer::UseStreaming(usage))
		ENSURE(backingStore != NULL);

	// TODO, RC - run some sanity checks on allocation request

	std::vector<std::unique_ptr<CVertexBuffer>>& buffers = m_Buffers[static_cast<int>(group)];

#if DUMP_VB_STATS
	debug_printf("\n============================\n# allocate vsize=%zu nverts=%zu\n\n", vertexSize, numVertices);
	for (const std::unique_ptr<CVertexBuffer>& buffer : buffers)
	{
		if (buffer->CompatibleVertexType(vertexSize, usage, target))
		{
			debug_printf("%p\n", buffer.get());
			buffer->DumpStatus();
		}
	}
#endif

	// iterate through all existing buffers testing for one that'll
	// satisfy the allocation
	for (const std::unique_ptr<CVertexBuffer>& buffer : buffers)
	{
		result = buffer->Allocate(vertexSize, numVertices, usage, target, backingStore);
		if (result)
			return Handle(result);
	}

	// got this far; need to allocate a new buffer
	buffers.emplace_back(
		group == Group::DEFAULT
			? std::make_unique<CVertexBuffer>(vertexSize, usage, target)
			// Reduces the buffer size for not so frequent buffers.
			: std::make_unique<CVertexBuffer>(vertexSize, usage, target, 1024 * 1024));
	result = buffers.back()->Allocate(vertexSize, numVertices, usage, target, backingStore);

	if (!result)
	{
		LOGERROR("Failed to create VBOs (%zu*%zu)", vertexSize, numVertices);
		return Handle();
	}

	return Handle(result);
}

void CVertexBufferManager::Release(CVertexBuffer::VBChunk* chunk)
{
	ENSURE(chunk);
#if DUMP_VB_STATS
	debug_printf("\n============================\n# release %p nverts=%zu\n\n", chunk, chunk->m_Count);
#endif
	chunk->m_Owner->Release(chunk);
}

size_t CVertexBufferManager::GetBytesReserved() const
{
	size_t total = 0;
	for (int group = static_cast<int>(Group::DEFAULT); group < static_cast<int>(Group::COUNT); ++group)
		for (const std::unique_ptr<CVertexBuffer>& buffer : m_Buffers[static_cast<int>(group)])
			total += buffer->GetBytesReserved();
	return total;
}

size_t CVertexBufferManager::GetBytesAllocated() const
{
	size_t total = 0;
	for (int group = static_cast<int>(Group::DEFAULT); group < static_cast<int>(Group::COUNT); ++group)
		for (const std::unique_ptr<CVertexBuffer>& buffer : m_Buffers[static_cast<int>(group)])
			total += buffer->GetBytesAllocated();
	return total;
}
