/* Copyright (C) 2011 Wildfire Games.
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

#ifndef INCLUDED_PATCHRDATA
#define INCLUDED_PATCHRDATA

#include <vector>
#include "graphics/SColor.h"
#include "maths/Vector3D.h"
#include "graphics/RenderableObject.h"
#include "VertexBufferManager.h"

class CPatch;
class CTerrainTextureEntry;

//////////////////////////////////////////////////////////////////////////////////////////////////
// CPatchRData: class encapsulating logic for rendering terrain patches; holds per
// patch data, plus some supporting static functions for batching, etc
class CPatchRData : public CRenderData
{
public:
	CPatchRData(CPatch* patch);
	~CPatchRData();

	void Update();
	void RenderOutline();
	void RenderSides();
	void RenderPriorities();

	static void RenderBases(const std::vector<CPatchRData*>& patches);
	static void RenderBlends(const std::vector<CPatchRData*>& patches);
	static void RenderStreams(const std::vector<CPatchRData*>& patches, int streamflags);

private:
	struct SSplat {
		SSplat() : m_Texture(0), m_IndexCount(0) {}

		// texture to apply during splat
		CTerrainTextureEntry* m_Texture;
		// offset into the index array for this patch where splat starts
		size_t m_IndexStart;
		// number of indices used by splat
		size_t m_IndexCount;
	};

	struct SBaseVertex {
		// vertex position
		CVector3D m_Position;
		// diffuse color from sunlight
		SColor4ub m_DiffuseColor;
		// vertex uvs for base texture
		float m_UVs[2];
		// add some padding since VBOs prefer power-of-two sizes
		u32 m_Padding[2];
	};
	cassert(sizeof(SBaseVertex) == 32);

	struct SSideVertex {
		// vertex position
		CVector3D m_Position;
		// add some padding
		u32 m_Padding[1];
	};
	cassert(sizeof(SSideVertex) == 16);

	struct SBlendVertex {
		// vertex position
		CVector3D m_Position;
		// vertex uvs for base texture
		float m_UVs[2];
		// vertex uvs for alpha texture
		float m_AlphaUVs[2];
		// add some padding
		u32 m_Padding[1];
	};
	cassert(sizeof(SBlendVertex) == 32);

	// build this renderdata object
	void Build();

	void AddBlend(u16 i, u16 j, u8 shape);

	void BuildBlends();
	void BuildIndices();
	void BuildVertices();
	void BuildSides();

	void BuildSide(std::vector<SSideVertex>& vertices, CPatchSideFlags side);

	// owner patch
	CPatch* m_Patch;

	// vertex buffer handle for base vertices
	CVertexBuffer::VBChunk* m_VBBase;

	// vertex buffer handle for base vertex indices
	CVertexBuffer::VBChunk* m_VBBaseIndices;

	// vertex buffer handle for side vertices
	CVertexBuffer::VBChunk* m_VBSides;

	// vertex buffer handle for blend vertices
	CVertexBuffer::VBChunk* m_VBBlends;

	// patch render vertices
	SBaseVertex* m_Vertices;

	// list of base splats to apply to this patch
	std::vector<SSplat> m_Splats;

	// vertices to use for blending transition texture passes
	std::vector<SBlendVertex> m_BlendVertices;

	// splats used in blend pass
	std::vector<SSplat> m_BlendSplats;
};


#endif
