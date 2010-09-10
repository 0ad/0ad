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
	void RenderBase(bool losColor);
	void RenderBlends();
	void RenderOutline();
	void RenderStreams(int streamflags, bool losColor);

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
		// color modulation from LOS
		SColor4ub m_LOSColor;
	};

	struct SBlendVertex {
		// vertex position
		CVector3D m_Position;
		// color modulation from LOS
		SColor4ub m_LOSColor;
		// vertex uvs for base texture
		float m_UVs[2];
		// vertex uvs for alpha texture
		float m_AlphaUVs[2];
	};

	struct STex {
		bool operator==(const STex& rhs) const { return m_Texture==rhs.m_Texture; }
		bool operator<(const STex& rhs) const { return m_Priority<rhs.m_Priority; }
		CTerrainTextureEntry* m_Texture;
		int m_Priority;
	};

	// build this renderdata object
	void Build();

	void BuildBlends();
	void BuildIndices();
	void BuildVertices();

	// owner patch
	CPatch* m_Patch;

	// vertex buffer handle for base vertices
	CVertexBuffer::VBChunk* m_VBBase;

	// vertex buffer handle for blend vertices
	CVertexBuffer::VBChunk* m_VBBlends;

	// patch render vertices
	SBaseVertex* m_Vertices;

	// indices into base vertices for the base splats
	std::vector<unsigned short> m_Indices;

	// indices into base vertices for the shadow map pass
	std::vector<unsigned short> m_ShadowMapIndices;

	// list of base splats to apply to this patch
	std::vector<SSplat> m_Splats;

	// vertices to use for blending transition texture passes
	std::vector<SBlendVertex> m_BlendVertices;

	// remembers the index in the m_Vertices array of each blend vertex, so that we can
	// properly update its color for fog of war and shroud of darkness
	std::vector<size_t> m_BlendVertexIndices;

	// indices into blend vertices for the blend splats
	std::vector<unsigned short> m_BlendIndices;

	// splats used in blend pass
	std::vector<SSplat> m_BlendSplats;
};


#endif
