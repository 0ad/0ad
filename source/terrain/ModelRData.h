#ifndef _MODELRDATA_H
#define _MODELRDATA_H

#include <vector>
#include "res/res.h"
#include "Vector3D.h"
#include "RenderableObject.h"

class CModel;

class CModelRData : public CRenderData
{
public:
	CModelRData(CModel* model);
	~CModelRData();

	void Update();
	void RenderStreams(u32 streamflags,const CMatrix3D& transform,bool transparentPass=false);

	// sort indices of this object from back to front according to given
	// object to camera space transform; return sqrd distance to centre of nearest triangle
	float BackToFrontIndexSort(CMatrix3D& objToCam);

private:
	// build this renderdata object
	void Build();

	void BuildVertices();
	void BuildIndices();


	struct SVertex {
		// vertex position
		CVector3D m_Position;
		// vertex uvs for base texture
		float m_UVs[2];
		// vertex color
		SColor4ub m_Color;
	};	

	// owner model
	CModel* m_Model;
	// handle to models vertex buffer
	u32 m_VB;
	// model render vertices
	SVertex* m_Vertices;
	// transformed vertex normals - required for recalculating lighting on skinned models
	CVector3D* m_Normals;
	// model render indices
	u16* m_Indices;
};


#endif
