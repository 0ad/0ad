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
#include "ps/CStr.h"

#define DUMP_VB_STATS 0 // for debugging

namespace
{

const char* GetBufferTypeName(
	const Renderer::Backend::GL::CBuffer::Type type)
{
	const char* name = "UnknownBuffer";
	switch (type)
	{
	case Renderer::Backend::GL::CBuffer::Type::VERTEX:
		name = "VertexBuffer";
		break;
	case Renderer::Backend::GL::CBuffer::Type::INDEX:
		name = "IndexBuffer";
		break;
	default:
		debug_warn("Unknown buffer type");
		break;
	}
	return name;
}

const char* GetGroupName(
	const CVertexBufferManager::Group group)
{
	const char* name = "Unknown";
	switch (group)
	{
	case CVertexBufferManager::Group::DEFAULT:
		name = "Default";
		break;
	case CVertexBufferManager::Group::TERRAIN:
		name = "Terrain";
		break;
	case CVertexBufferManager::Group::WATER:
		name = "Water";
		break;
	default:
		debug_warn("Unknown buffer group");
		break;
	}
	return name;
}

} // anonymous namespace

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
CVertexBufferManager::Handle CVertexBufferManager::AllocateChunk(
	const size_t vertexSize, const size_t numberOfVertices,
	const Renderer::Backend::GL::CBuffer::Type type,
	const bool dynamic, void* backingStore, Group group)
{
	ENSURE(vertexSize > 0);
	ENSURE(numberOfVertices > 0);

	CVertexBuffer::VBChunk* result = nullptr;

	if (CVertexBuffer::UseStreaming(dynamic))
		ENSURE(backingStore != NULL);

	// TODO, RC - run some sanity checks on allocation request

	std::vector<std::unique_ptr<CVertexBuffer>>& buffers = m_Buffers[static_cast<int>(group)];

#if DUMP_VB_STATS
	debug_printf("\n============================\n# allocate vsize=%zu nverts=%zu\n\n", vertexSize, numVertices);
	for (const std::unique_ptr<CVertexBuffer>& buffer : buffers)
	{
		if (buffer->CompatibleVertexType(vertexSize, type, dynamic))
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
		result = buffer->Allocate(vertexSize, numberOfVertices, type, dynamic, backingStore);
		if (result)
			return Handle(result);
	}

	char bufferName[64] = {0};
	snprintf(
		bufferName, std::size(bufferName), "%s (%s, %zu%s)",
		GetBufferTypeName(type), GetGroupName(group), vertexSize, (dynamic ? ", dynamic" : ""));

	// got this far; need to allocate a new buffer
	buffers.emplace_back(
		group == Group::DEFAULT
			? std::make_unique<CVertexBuffer>(bufferName, vertexSize, type, dynamic)
			// Reduces the buffer size for not so frequent buffers.
			: std::make_unique<CVertexBuffer>(bufferName, vertexSize, type, dynamic, 1024 * 1024));
	result = buffers.back()->Allocate(vertexSize, numberOfVertices, type, dynamic, backingStore);

	if (!result)
	{
		LOGERROR("Failed to create VBOs (%zu*%zu)", vertexSize, numberOfVertices);
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
