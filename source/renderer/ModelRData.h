#ifndef _MODELRDATA_H
#define _MODELRDATA_H

#include <vector>
#include "Vector3D.h"
#include "Color.h"
#include "RenderableObject.h"

#define MODELRDATA_FLAG_TRANSPARENT		(1<<0)

class CModel;

class CModelRData : public CRenderData
{
public:
	CModelRData(CModel* model);
	~CModelRData();

	void Update();
	void RenderStreams(u32 streamflags);

	// return render flags for this model
	u32 GetFlags() const { return m_Flags; }

	// sort indices of this object from back to front according to given
	// object to camera space transform; return sqrd distance to centre of nearest triangle
	float BackToFrontIndexSort(CMatrix3D& objToCam);

	// submit a model to render this frame
	static void Submit(CModel* model);
	// clear per frame patch list
	static void ClearSubmissions() { m_Models.clear(); }

	// render all submitted models
	static void RenderModels(u32 streamflags,u32 flags=0);

private:
	// build this renderdata object
	void Build();

	void BuildVertices();
	void BuildIndices();

	// submit batches for this model to the vertex buffer
	void SubmitBatches();

	struct SVertex {
		// vertex position
		CVector3D m_Position;
		// vertex uvs for base texture
		float m_UVs[2];
		// RGB vertex color
		RGBColor m_Color;
	};	

	// owner model
	CModel* m_Model;
	// vertex buffer object for this model
	CVertexBuffer::VBChunk* m_VB;
	// model render vertices
	SVertex* m_Vertices;
	// transformed vertex normals - required for recalculating lighting on skinned models
	CVector3D* m_Normals;
	// model render indices
	u16* m_Indices;
	// model render flags
	u32 m_Flags;
	// list of all submitted models
	static std::vector<CModel*> m_Models;
};


#endif
