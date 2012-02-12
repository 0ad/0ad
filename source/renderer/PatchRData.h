/* Copyright (C) 2012 Wildfire Games.
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
#include "graphics/ShaderProgram.h"
#include "VertexBufferManager.h"

class CPatch;
class CTerrainTextureEntry;
class CTextRenderer;

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
	void RenderPriorities(CTextRenderer& textRenderer);

	void RenderWater();

	static void RenderBases(const std::vector<CPatchRData*>& patches, CShaderProgramPtr shader);
	static void RenderBlends(const std::vector<CPatchRData*>& patches, CShaderProgramPtr shader);
	static void RenderStreams(const std::vector<CPatchRData*>& patches, int streamflags);

	CPatch* GetPatch() { return m_Patch; }

	const CBoundingBoxAligned& GetWaterBounds() const { return m_WaterBounds; }

private:
	friend struct SBlendStackItem;

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
	};
	cassert(sizeof(SBaseVertex) == 16);

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
		// diffuse color from sunlight
		SColor4ub m_DiffuseColor;
		// vertex uvs for alpha texture
		float m_AlphaUVs[2];
		// add some padding since VBOs prefer power-of-two sizes
		u32 m_Padding[2];
	};
	cassert(sizeof(SBlendVertex) == 32);

	// Mixed Fancy/Simple water vertex description data structure
	struct SWaterVertex {
		// vertex position
		CVector3D m_Position;
		// (p,q,r, a) where
		//   p*255 + q*-255 + r = depth of water
		//   a = depth-dependent alpha
		SColor4ub m_DepthData;
	};
	cassert(sizeof(SWaterVertex) == 16);

	// build this renderdata object
	void Build();

	void AddBlend(std::vector<SBlendVertex>& blendVertices, std::vector<u16>& blendIndices, u16 i, u16 j, u8 shape);

	void BuildBlends();
	void BuildIndices();
	void BuildVertices();
	void BuildSides();

	void BuildSide(std::vector<SSideVertex>& vertices, CPatchSideFlags side);

	// owner patch
	CPatch* m_Patch;

	// vertex buffer handle for side vertices
	CVertexBuffer::VBChunk* m_VBSides;

	// vertex buffer handle for base vertices
	CVertexBuffer::VBChunk* m_VBBase;

	// vertex buffer handle for base vertex indices
	CVertexBuffer::VBChunk* m_VBBaseIndices;

	// vertex buffer handle for blend vertices
	CVertexBuffer::VBChunk* m_VBBlends;

	// vertex buffer handle for blend vertex indices
	CVertexBuffer::VBChunk* m_VBBlendIndices;

	// list of base splats to apply to this patch
	std::vector<SSplat> m_Splats;

	// splats used in blend pass
	std::vector<SSplat> m_BlendSplats;

	// boundary of water in this patch
	CBoundingBoxAligned m_WaterBounds;

	// Water vertex buffer
	CVertexBuffer::VBChunk* m_VBWater;

	// Water indices buffer
	CVertexBuffer::VBChunk* m_VBWaterIndices;

	// Build water vertices and indices (vertex buffer and data vector)
	void BuildWater();

	// parameter allowing a varying number of triangles per patch for LOD
	// MUST be an exact divisor of PATCH_SIZE
	// compiled const for the moment until/if dynamic water LOD is offered
	// savings would be mostly beneficial for GPU or simple water
	static const ssize_t water_cell_size = 1;
};

#endif
