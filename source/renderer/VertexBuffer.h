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

/*
 * encapsulation of VBOs with batching and sharing
 */

#ifndef INCLUDED_VERTEXBUFFER
#define INCLUDED_VERTEXBUFFER

#include "renderer/backend/gl/Buffer.h"
#include "renderer/backend/gl/DeviceCommandContext.h"

#include <memory>
#include <vector>

/**
 * CVertexBuffer: encapsulation of ARB_vertex_buffer_object, also supplying
 * some additional functionality for sharing buffers between multiple objects.
 *
 * The class can be used in two modes, depending on the usage parameter:
 *
 * Static buffer: Call Allocate() with backingStore = nullptr. Then call
 * UpdateChunkVertices() with any pointer - the data will be immediately copied
 * to the VBO. This should be used for vertex data that rarely changes.
 *
 * Dynamic buffer: Call Allocate() with backingStore pointing
 * at some memory that will remain valid for the lifetime of the CVertexBuffer.
 * This should be used for vertex data that may change every frame.
 * Rendering is expected to occur in two phases:
 *   - "Prepare" phase:
 *       If this chunk is going to be used for rendering during the next rendering phase,
 *       you must call PrepareForRendering().
 *       If the vertex data in backingStore has been modified since the last uploading phase,
 *       you must call UpdateChunkVertices().
 *   - "Upload" phase:
 *       UploadedIfNeeded() can be called (multiple times). The vertex data will be uploaded
 *       to the GPU if necessary.
 * It is okay to have multiple prepare/upload cycles per frame (though slightly less
 * efficient), but they must occur sequentially.
 */
class CVertexBuffer
{
	NONCOPYABLE(CVertexBuffer);

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
		/// If UseStreaming() is true, points at the data for this chunk
		void* m_BackingStore;

		/// If true, the VBO is not consistent with the chunk's backing store
		/// (and will need to be re-uploaded before rendering with this chunk)
		bool m_Dirty;

		/// If true, we have been told this chunk is going to be used for
		/// rendering in the next uploading phase and will need to be uploaded
		bool m_Needed;

	private:
		// Only CVertexBuffer can construct/delete these
		// (Other people should use g_VBMan.Allocate, g_VBMan.Release)
		friend class CVertexBuffer;
		VBChunk() {}
		~VBChunk() {}
	};

public:
	// constructor, destructor
	CVertexBuffer(
		const char* name, const size_t vertexSize,
		const Renderer::Backend::GL::CBuffer::Type type, const bool dynamic);
	CVertexBuffer(
		const char* name, const size_t vertexSize,
		const Renderer::Backend::GL::CBuffer::Type type, const bool dynamic,
		const size_t maximumBufferSize);
	~CVertexBuffer();

	void UploadIfNeeded(Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext);

	/// Make the vertex data available for the next usage.
	void PrepareForRendering(VBChunk* chunk);

	/// Update vertex data for given chunk. Transfers the provided data to the actual OpenGL vertex buffer.
	void UpdateChunkVertices(VBChunk* chunk, void* data);

	size_t GetVertexSize() const { return m_VertexSize; }
	size_t GetBytesReserved() const;
	size_t GetBytesAllocated() const;

	/// Returns true if this vertex buffer is compatible with the specified vertex type and intended usage.
	bool CompatibleVertexType(
		const size_t vertexSize, const Renderer::Backend::GL::CBuffer::Type type,
		const bool dynamic) const;

	void DumpStatus() const;

	/**
	 * Given the usage flags of a buffer that has been (or will be) allocated:
	 *
	 * If true, we assume the buffer is going to be modified on every frame,
	 * so we will re-upload the entire buffer every frame using glMapBuffer.
	 * This requires the buffer's owner to hold onto its backing store.
	 *
	 * If false, we assume it will change rarely, and use direct upload to
	 * update it incrementally. The backing store can be freed to save memory.
	 */
	static bool UseStreaming(const bool dynamic);

	Renderer::Backend::GL::CBuffer* GetBuffer() { return m_Buffer.get(); }

private:
	friend class CVertexBufferManager;		// allow allocate only via CVertexBufferManager

	/// Try to allocate a buffer of given number of vertices (each of given size),
	/// and with the given type - return null if no free chunks available
	VBChunk* Allocate(
		const size_t vertexSize, const size_t numberOfVertices,
		const Renderer::Backend::GL::CBuffer::Type type, const bool dynamic,
		void* backingStore);
	/// Return given chunk to this buffer
	void Release(VBChunk* chunk);

	/// Vertex size of this vertex buffer
	size_t m_VertexSize;
	/// Number of vertices of above size in this buffer
	size_t m_MaxVertices;
	/// List of free chunks in this buffer
	std::vector<VBChunk*> m_FreeList;
	/// List of allocated chunks
	std::vector<VBChunk*> m_AllocList;
	/// Available free vertices - total of all free vertices in the free list
	size_t m_FreeVertices;

	std::unique_ptr<Renderer::Backend::GL::CBuffer> m_Buffer;

	bool m_HasNeededChunks;
};

#endif // INCLUDED_VERTEXBUFFER
