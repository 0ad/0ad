#ifndef _PATCHRDATA_H
#define _PATCHRDATA_H

#include <vector>
#include "res/res.h"
#include "Color.h"
#include "Vector3D.h"
#include "RenderableObject.h"
#include "VertexBufferManager.h"

class CPatch;

//////////////////////////////////////////////////////////////////////////////////////////////////
// CPatchRData: class encapsulating logic for rendering terrain patches; holds per
// patch data, plus some supporting static functions for batching, etc
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

	// submit a patch to render this frame
	static void Submit(CPatch* patch);
	// clear per frame patch list
	static void ClearSubmissions() { m_Patches.clear(); }

	// render the base pass of all patches
	static void RenderBaseSplats();
	// render the blend pass of all patches
	static void RenderBlendSplats();
	// render the outlines of all patches
	static void RenderOutlines();
	// render given streams of all patches; don't fiddle with renderstate
	static void RenderStreamsAll(u32 streamflags);
	// apply given shadow map to all terrain patches
	static void ApplyShadowMap(GLuint handle);

	// submit base batches for this patch to the vertex buffer
	void SubmitBaseBatches();
	// submit next set of blend batches for this patch to the vertex buffer;
	// return true if all blends on this patch have been submitted, else false
	bool SubmitBlendBatches();

	// perform necessary initialisation prior to rendering blend splats
	void SetupBlendBatches() { m_NextBlendSplat=0; }

private:
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
	// indices into blend vertices for the blend splats
	std::vector<unsigned short> m_BlendIndices;
	// splats used in blend pass
	std::vector<SSplat> m_BlendSplats;
	// index of the next blend splat to render 
	u32 m_NextBlendSplat;
	// list of all submitted patches
	static std::vector<CPatch*> m_Patches;
};


#endif
