#ifndef _PATCHRDATA_H
#define _PATCHRDATA_H

#include <vector>
#include "res/res.h"
#include "Color.h"
#include "Vector3D.h"
#include "RenderableObject.h"

class CPatch;

class CPatchRData : public CRenderData
{
public:
	CPatchRData(CPatch* patch); 
	~CPatchRData(); 

	void Update();
	void RenderBase();
	void RenderBlends();
	void RenderOutline();
	void RenderStreams(u32 streamflags);

private:
	// build this renderdata object
	void Build();

	void BuildBlends();
	void BuildIndices();
	void BuildVertices();

	struct SSplat {
		SSplat() : m_Texture(0), m_IndexCount(0) {}	

		// handle of texture to apply during splat
		Handle m_Texture;
		// offset into the index array for this patch where splat starts
		u32 m_IndexStart;
		// number of indices used by splat
		u32 m_IndexCount;
	};

	struct SBaseVertex {
		// vertex position
		CVector3D m_Position;
		// vertex color
		SColor4ub m_Color;
		// vertex uvs for base texture
		float m_UVs[2];
	};	

	struct SBlendVertex {
		// vertex position
		CVector3D m_Position;
		// vertex color
		SColor4ub m_Color;
		// vertex uvs for base texture
		float m_UVs[2];
		// vertex uvs for alpha texture
		float m_AlphaUVs[2];
	};	

	struct STex {
		bool operator==(const STex& rhs) const { return m_Handle==rhs.m_Handle; }
		bool operator<(const STex& rhs) const { return m_Priority<rhs.m_Priority; }
		Handle m_Handle;
		int m_Priority;
	};

	// owner patch
	CPatch* m_Patch;
	// vertex buffer handle for base vertices
	u32 m_VBBase;
	// vertex buffer handle for blend vertices
	u32 m_VBBlends;
	// patch render vertices
	SBaseVertex* m_Vertices;
	// patch index list
	std::vector<unsigned short> m_Indices;
	// list of base splats to apply to this patch
	std::vector<SSplat> m_Splats;
	// vertices to use for blending transition texture passes
	std::vector<SBlendVertex> m_BlendVertices;
	// indices into blend vertices for the blend splats
	std::vector<unsigned short> m_BlendIndices;
	// splats used in blend pass
	std::vector<SSplat> m_BlendSplats;
};


#endif
