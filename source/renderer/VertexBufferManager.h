/* Copyright (C) 2021 Wildfire Games.
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

#include "lib/types.h"
#include "renderer/VertexBuffer.h"

#include <memory>
#include <utility>

// CVertexBufferManager: owner object for CVertexBuffer objects; acts as
// 'front end' for their allocation and destruction
class CVertexBufferManager
{
public:
	enum class Group : u32
	{
		DEFAULT,
		TERRAIN,
		WATER,
		COUNT
	};

	// Helper for automatic VBChunk lifetime management.
	class Handle
	{
	public:
		Handle() = default;
		Handle(const Handle&) = delete;
		Handle& operator=(const Handle&) = delete;

		explicit Handle(CVertexBuffer::VBChunk* chunk);
		Handle(Handle&& other);
		Handle& operator=(Handle&& other);

		~Handle() { Reset(); }

		bool IsValid() const { return m_Chunk != nullptr; }
		explicit operator bool() const { return IsValid(); }
		bool operator!() const { return !static_cast<bool>(*this); }
		void Reset();

		friend void swap(Handle& lhs, Handle& rhs)
		{
			using std::swap;

			swap(lhs.m_Chunk, rhs.m_Chunk);
		}

		CVertexBuffer::VBChunk& operator*() const { return *m_Chunk; }
		CVertexBuffer::VBChunk* operator->() const { return m_Chunk; }
		CVertexBuffer::VBChunk* Get() const { return m_Chunk; }

	private:
		CVertexBuffer::VBChunk* m_Chunk = nullptr;
	};

	/**
	 * Try to allocate a vertex buffer of the given size and type.
	 *
	 * @param vertexSize size of each vertex in the buffer
	 * @param numVertices number of vertices in the buffer
	 * @param usage GL_STATIC_DRAW, GL_DYNAMIC_DRAW, GL_STREAM_DRAW
	 * @param target typically GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER
	 * @param backingStore if usage is STATIC, this is NULL; else for DYNAMIC/STREAM,
	 *            this must be a copy of the vertex data that remains valid for the
	 *            lifetime of the VBChunk
	 * @return chunk, or NULL if no free chunks available
	 */
	CVertexBuffer::VBChunk* Allocate(size_t vertexSize, size_t numVertices, GLenum usage, GLenum target, void* backingStore = nullptr);

	/// Returns the given @p chunk to its owning buffer
	void Release(CVertexBuffer::VBChunk* chunk);

	// Same as the Allocate function but returns Handle. Should be used instead
	// of the Allocate.
	Handle AllocateChunk(size_t vertexSize, size_t numVertices, GLenum usage, GLenum target, void* backingStore = nullptr, Group group = Group::DEFAULT);

	size_t GetBytesReserved() const;
	size_t GetBytesAllocated() const;

	/// Explicit shutdown of the vertex buffer subsystem; releases all currently-allocated buffers.
	void Shutdown();

private:
	CVertexBuffer::VBChunk* AllocateImpl(size_t vertexSize, size_t numVertices, GLenum usage, GLenum target, void* backingStore = nullptr, Group group = Group::DEFAULT);

	/// List of all known vertex buffers
	std::list<std::unique_ptr<CVertexBuffer>> m_Buffers[static_cast<std::size_t>(Group::COUNT)];
};

extern CVertexBufferManager g_VBMan;

#endif
