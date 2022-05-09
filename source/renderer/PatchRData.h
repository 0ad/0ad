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

#ifndef INCLUDED_PATCHRDATA
#define INCLUDED_PATCHRDATA

#include "graphics/Patch.h"
#include "graphics/RenderableObject.h"
#include "maths/Vector2D.h"
#include "maths/Vector3D.h"
#include "renderer/backend/IDeviceCommandContext.h"
#include "renderer/backend/IShaderProgram.h"
#include "renderer/VertexBufferManager.h"

#include <vector>

class CPatch;
class CShaderDefines;
class CSimulation2;
class CTerrainTextureEntry;
class CTextRenderer;
class ShadowMap;

//////////////////////////////////////////////////////////////////////////////////////////////////
// CPatchRData: class encapsulating logic for rendering terrain patches; holds per
// patch data, plus some supporting static functions for batching, etc
class CPatchRData : public CRenderData
{
public:
	CPatchRData(CPatch* patch, CSimulation2* simulation);
	~CPatchRData();

	void Update(CSimulation2* simulation);
	void RenderOutline();
	void RenderPriorities(CTextRenderer& textRenderer);

	void RenderWaterSurface(
		Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
		const bool bindWaterData);
	void RenderWaterShore(
		Renderer::Backend::IDeviceCommandContext* deviceCommandContext);

	CPatch* GetPatch() { return m_Patch; }

	const CBoundingBoxAligned& GetWaterBounds() const { return m_WaterBounds; }

	static void RenderBases(
		Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
		const std::vector<CPatchRData*>& patches, const CShaderDefines& context, ShadowMap* shadow);
	static void RenderBlends(
		Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
		const std::vector<CPatchRData*>& patches, const CShaderDefines& context, ShadowMap* shadow);
	static void RenderStreams(
		Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
		const std::vector<CPatchRData*>& patches, const bool bindPositionAsTexCoord);
	static void RenderSides(
		Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
		const std::vector<CPatchRData*>& patches);

	static void PrepareShader(ShadowMap* shadow);

private:
	friend struct SBlendStackItem;

	struct SSplat
	{
		SSplat() : m_Texture(0), m_IndexCount(0) {}

		// texture to apply during splat
		CTerrainTextureEntry* m_Texture;
		// offset into the index array for this patch where splat starts
		size_t m_IndexStart;
		// number of indices used by splat
		size_t m_IndexCount;
	};

	struct SBaseVertex
	{
		// vertex position
		CVector3D m_Position;
		CVector3D m_Normal;
		// pad to a power of two
		u8 m_Padding[8];
	};
	cassert(sizeof(SBaseVertex) == 32);

	struct SSideVertex
	{
		// vertex position
		CVector3D m_Position;
		// pad to a power of two
		u8 m_Padding[4];
	};
	cassert(sizeof(SSideVertex) == 16);

	struct SBlendVertex
	{
		// vertex position
		CVector3D m_Position;
		// vertex uvs for alpha texture
		float m_AlphaUVs[2];
		CVector3D m_Normal;
	};
	cassert(sizeof(SBlendVertex) == 32);

	// Mixed Fancy/Simple water vertex description data structure
	struct SWaterVertex
	{
		// vertex position
		CVector3D m_Position;
		CVector2D m_WaterData;
		// pad to a power of two
		u8 m_Padding[12];
	};
	cassert(sizeof(SWaterVertex) == 32);

	// build this renderdata object
	void Build();

	void AddBlend(std::vector<SBlendVertex>& blendVertices, std::vector<u16>& blendIndices,
			   u16 i, u16 j, u8 shape, CTerrainTextureEntry* texture);

	void BuildBlends();
	void BuildIndices();
	void BuildVertices();
	void BuildSides();

	void BuildSide(std::vector<SSideVertex>& vertices, CPatchSideFlags side);

	// owner patch
	CPatch* m_Patch;

	// vertex buffer handle for side vertices
	CVertexBufferManager::Handle m_VBSides;

	// vertex buffer handle for base vertices
	CVertexBufferManager::Handle m_VBBase;

	// vertex buffer handle for base vertex indices
	CVertexBufferManager::Handle m_VBBaseIndices;

	// vertex buffer handle for blend vertices
	CVertexBufferManager::Handle m_VBBlends;

	// vertex buffer handle for blend vertex indices
	CVertexBufferManager::Handle m_VBBlendIndices;

	// list of base splats to apply to this patch
	std::vector<SSplat> m_Splats;

	// splats used in blend pass
	std::vector<SSplat> m_BlendSplats;

	// boundary of water in this patch
	CBoundingBoxAligned m_WaterBounds;

	// Water vertex buffer
	CVertexBufferManager::Handle m_VBWater;
	CVertexBufferManager::Handle m_VBWaterShore;

	// Water indices buffer
	CVertexBufferManager::Handle m_VBWaterIndices;
	CVertexBufferManager::Handle m_VBWaterIndicesShore;

	CSimulation2* m_Simulation;

	// Build water vertices and indices (vertex buffer and data vector)
	void BuildWater();

	// parameter allowing a varying number of triangles per patch for LOD
	// MUST be an exact divisor of PATCH_SIZE
	// compiled const for the moment until/if dynamic water LOD is offered
	// savings would be mostly beneficial for GPU or simple water
	static const ssize_t water_cell_size = 1;
};

#endif // INCLUDED_PATCHRDATA
