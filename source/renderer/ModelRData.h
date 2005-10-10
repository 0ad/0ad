#ifndef _MODELRDATA_H
#define _MODELRDATA_H

#include <vector>
#include "Vector3D.h"
#include "Color.h"
#include "RenderableObject.h"
#include "renderer/VertexArray.h"

#define MODELRDATA_FLAG_TRANSPARENT		(1<<0)
#define MODELRDATA_FLAG_PLAYERCOLOR		(1<<1)

class CModel;
class CModelDefRData;

class CModelRData : public CRenderData
{
	friend class CModelDefRData;
	
public:
	CModelRData(CModel* model);
	~CModelRData();

	void Update();
	void RenderStreams(u32 streamflags, int tmus = 1);

	// return render flags for this model
	u32 GetFlags() const { return m_Flags; }
	// accessor: model we're based on
	CModel* GetModel() const { return m_Model; }
	
	// sort indices of this object from back to front according to given
	// object to camera space transform; return sqrd distance to centre of nearest triangle
	float BackToFrontIndexSort(CMatrix3D& objToCam);

	// submit a model to render this frame
	static void Submit(CModel* model);
	// clear per frame model list
	static void ClearSubmissions();

	// prepare for rendering of models
	static void SetupRender(u32 streamflags);
	// reset state prepared by SetupRender
	static void FinishRender(u32 streamflags);
	
	// render all submitted models
	static void RenderModels(u32 streamflags,u32 flags=0);

private:
	// build this renderdata object
	void Build();

	void BuildStaticVertices();
	void BuildVertices();
	void BuildIndices();

	// owner model
	CModel* m_Model;
	// transformed vertex normals - required for recalculating lighting on skinned models
	// only used in render path RP_FIXED
	CVector3D* m_TempNormals;
	// vertex array
	VertexArray m_DynamicArray;
	VertexArray::Attribute m_Position;
	VertexArray::Attribute m_UV; // only used in RP_VERTEXSHADER (shared otherwise)
	VertexArray::Attribute m_Color; // only used in RP_FIXED
	VertexArray::Attribute m_Normal; // only used in RP_VERTEXSHADER
	// model render indices
	u16* m_Indices;
	// model render flags
	u32 m_Flags;
	// linked list of submitted models per CModelDefRData
	CModelRData* m_SubmissionNext;
};


#endif
