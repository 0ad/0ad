#include "precompiled.h"

#include <assert.h>
#include <algorithm>
#include "res/ogl_tex.h"
#include "Renderer.h"
#include "TransparencyRenderer.h"
#include "ModelRData.h"
#include "Model.h"
#include "MaterialManager.h"

///////////////////////////////////////////////////////////////////
// shared list of all submitted models this frame
std::vector<CModel*> CModelRData::m_Models;

///////////////////////////////////////////////////////////////////
// CModelRData constructor
CModelRData::CModelRData(CModel* model) 
	: m_Model(model), m_Vertices(0), m_Normals(0), m_Indices(0), m_VB(0), m_Flags(0)
{
	assert(model);
	// build all data now
	Build();
}

///////////////////////////////////////////////////////////////////
// CModelRData destructor
CModelRData::~CModelRData() 
{
	// clean up system copies of data
	delete[] m_Indices;
	delete[] m_Vertices;
	delete[] m_Normals;
	if (m_VB) {
		// release vertex buffer chunks
		g_VBMan.Release(m_VB);
	}
}

void CModelRData::Build()
{
	// build data
	BuildVertices();
	BuildIndices();
	// force a texture load on models texture
	g_Renderer.LoadTexture(m_Model->GetTexture(),GL_CLAMP_TO_EDGE);
	// setup model render flags
	/*if (g_Renderer.IsTextureTransparent(m_Model->GetTexture())) {
		m_Flags|=MODELRDATA_FLAG_TRANSPARENT;
	}*/
    if(m_Model->GetMaterial().UsesAlpha())
        m_Flags |= MODELRDATA_FLAG_TRANSPARENT;
}

void CModelRData::BuildIndices()
{	
	CModelDefPtr mdef=m_Model->GetModelDef();
	assert(mdef);
	
	// must have a valid vertex buffer by this point so we know where indices are supposed to start
	assert(m_VB);
	
	// allocate indices if we haven't got any already
	if (!m_Indices) {
		m_Indices=new u16[mdef->GetNumFaces()*3];
	}

	// build indices
	u32 base=(u32)m_VB->m_Index;
	u32 indices=0;
	SModelFace* faces=mdef->GetFaces();
	for (int j=0; j<mdef->GetNumFaces(); j++) {
		SModelFace& face=faces[j];
		m_Indices[indices++]=face.m_Verts[0]+base;
		m_Indices[indices++]=face.m_Verts[1]+base;
		m_Indices[indices++]=face.m_Verts[2]+base;
	}
}








static SColor4ub ConvertColor(const RGBColor& src)
{
	SColor4ub result;
	result.R=clamp(int(src.X*255),0,255);
	result.G=clamp(int(src.Y*255),0,255);
	result.B=clamp(int(src.Z*255),0,255);
	result.A=0xff;
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SkinPoint: skin the vertex position using it's blend data and given bone matrices
static void SkinPoint(const SModelVertex& vertex,const CMatrix3D* matrices,CVector3D& result)
{
	CVector3D tmp;
	const SVertexBlend& blend=vertex.m_Blend;

	// must have at least one valid bone if we're using SkinPoint
	assert(blend.m_Bone[0]!=0xff);

	const CMatrix3D& m=matrices[blend.m_Bone[0]];
	m.Transform(vertex.m_Coords,result);
	result*=blend.m_Weight[0];

	for (u32 i=1;blend.m_Bone[i]!=0xff && i<SVertexBlend::SIZE;i++) {
		const CMatrix3D& m=matrices[blend.m_Bone[i]];
		m.Transform(vertex.m_Coords,tmp);
		result+=tmp*blend.m_Weight[i];
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SkinPoint: skin the vertex normal using it's blend data and given bone matrices
static void SkinNormal(const SModelVertex& vertex,const CMatrix3D* invmatrices,CVector3D& result)
{
	CVector3D tmp;
	const SVertexBlend& blend=vertex.m_Blend;

	// must have at least one valid bone if we're using SkinNormal
	assert(blend.m_Bone[0]!=0xff);

	const CMatrix3D& m=invmatrices[blend.m_Bone[0]];
	m.RotateTransposed(vertex.m_Norm,result);
	result*=blend.m_Weight[0];

	for (u32 i=1;vertex.m_Blend.m_Bone[i]!=0xff && i<SVertexBlend::SIZE;i++) {
		const CMatrix3D& m=invmatrices[blend.m_Bone[i]];
		m.RotateTransposed(vertex.m_Norm,tmp);
		result+=tmp*blend.m_Weight[i];
	}
}

void CModelRData::BuildVertices()
{
	CModelDefPtr mdef=m_Model->GetModelDef();

	// allocate vertices if we haven't got any already
	if (!m_Vertices) {
		m_Vertices=new SVertex[mdef->GetNumVertices()];
		m_Normals=new CVector3D[mdef->GetNumVertices()];
	}

	// build vertices
	u32 numVertices=mdef->GetNumVertices();
	SModelVertex* vertices=mdef->GetVertices();
	const CMatrix3D* bonematrices=m_Model->GetBoneMatrices();
	if (bonematrices) {
		// boned model - calculate skinned vertex positions/normals
		const CMatrix3D* invbonematrices=m_Model->GetInvBoneMatrices();
		for (uint j=0; j<numVertices; j++) {
			SkinPoint(vertices[j],bonematrices,m_Vertices[j].m_Position);
			SkinNormal(vertices[j],invbonematrices,m_Normals[j]);
		}
	} else {
		// just copy regular positions, transform normals to world space
		const CMatrix3D& transform=m_Model->GetTransform();
		const CMatrix3D& invtransform=m_Model->GetInvTransform();
		for (uint j=0; j<numVertices; j++) {
			transform.Transform(vertices[j].m_Coords,m_Vertices[j].m_Position);
			invtransform.RotateTransposed(vertices[j].m_Norm,m_Normals[j]);
		}
	}
	
	// now fill in UV and vertex colour data
	for (uint j=0; j<numVertices; j++) {
		m_Vertices[j].m_UVs[0]=vertices[j].m_U;
		m_Vertices[j].m_UVs[1]=1-vertices[j].m_V;
		g_Renderer.m_SHCoeffsUnits.Evaluate(m_Normals[j],m_Vertices[j].m_Color);
	}

	// upload everything to vertex buffer - create one if necessary
	if (!m_VB) {
		m_VB=g_VBMan.Allocate(sizeof(SVertex),mdef->GetNumVertices(),mdef->GetNumBones() ? true : false);
	}
	m_VB->m_Owner->UpdateChunkVertices(m_VB,m_Vertices);
}


void CModelRData::RenderStreams(u32 streamflags)
{	
	CModelDefPtr mdldef=m_Model->GetModelDef();
	
	if (streamflags & STREAM_UV0)
    {
        m_Model->GetMaterial().Bind();
        g_Renderer.SetTexture(0,m_Model->GetTexture());
    }

	u8* base=m_VB->m_Owner->Bind();

	// set vertex pointers
	u32 stride=sizeof(SVertex);
	glVertexPointer(3,GL_FLOAT,stride,base+offsetof(SVertex,m_Position));
	if (streamflags & STREAM_COLOR) glColorPointer(3,GL_FLOAT,stride,base+offsetof(SVertex,m_Color));
	if (streamflags & STREAM_UV0) glTexCoordPointer(2,GL_FLOAT,stride,base+offsetof(SVertex,m_UVs));

	// render the lot
	u32 numFaces=mdldef->GetNumFaces();
//	glDrawRangeElements(GL_TRIANGLES,0,mdldef->GetNumVertices(),numFaces*3,GL_UNSIGNED_SHORT,m_Indices);
	glDrawElements(GL_TRIANGLES,numFaces*3,GL_UNSIGNED_SHORT,m_Indices);

    if(streamflags & STREAM_UV0)
        m_Model->GetMaterial().Unbind();
	
	// bump stats
	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_ModelTris+=numFaces;
}


void CModelRData::Update()
{	
	if (m_UpdateFlags!=0) {
		// renderdata changed : rebuild necessary portions
		if (m_UpdateFlags & RENDERDATA_UPDATE_VERTICES) {
			BuildVertices();
		}
		if (m_UpdateFlags & RENDERDATA_UPDATE_INDICES) {
			BuildIndices();
		}

		m_UpdateFlags=0;
	}
}

typedef std::pair<int,float> IntFloatPair;
static std::vector<IntFloatPair> IndexSorter;

struct SortFacesByDist {
	bool operator()(const IntFloatPair& lhs,const IntFloatPair& rhs) {
		return lhs.second>rhs.second ? true : false;
	}
};

float CModelRData::BackToFrontIndexSort(CMatrix3D& objToCam)
{
	float mindist=1.0e30f;
	CVector3D osvtx,csvtx;

	CModelDefPtr mdldef=m_Model->GetModelDef();

	SModelVertex* vtxs=mdldef->GetVertices();
	
	u32 numFaces=mdldef->GetNumFaces();
	SModelFace* faces=mdldef->GetFaces();
	
	IndexSorter.reserve(numFaces);

	SModelFace* facePtr=faces;
	u32 i;
	for (i=0;i<numFaces;i++)
	{
		osvtx=vtxs[facePtr->m_Verts[0]].m_Coords;
		osvtx+=vtxs[facePtr->m_Verts[1]].m_Coords;
		osvtx+=vtxs[facePtr->m_Verts[2]].m_Coords;
		osvtx*=1.0f/3.0f;

		csvtx=objToCam.Transform(osvtx);
		float distsqrd=SQR(csvtx.X)+SQR(csvtx.Y)+SQR(csvtx.Z);
		if (distsqrd<mindist) mindist=distsqrd;

		IndexSorter.push_back(IntFloatPair(i,distsqrd));
		facePtr++;
	}

	std::sort(IndexSorter.begin(),IndexSorter.end(),SortFacesByDist());
	
	// now build index list
	u32 indices=0;
	for (i=0;i<numFaces;i++) {
		SModelFace& face=faces[IndexSorter[i].first];
		m_Indices[indices++]=(u16)(face.m_Verts[0]+m_VB->m_Index);
		m_Indices[indices++]=(u16)(face.m_Verts[1]+m_VB->m_Index);
		m_Indices[indices++]=(u16)(face.m_Verts[2]+m_VB->m_Index);
	}

	// clear list for next call
	IndexSorter.clear();

	return mindist;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// SubmitBatches: submit batches for this model to the vertex buffer
void CModelRData::SubmitBatches()
{
	assert(m_VB);
	m_VB->m_Owner->AppendBatch(m_VB,m_Model->GetTexture()->GetHandle(),m_Model->GetModelDef()->GetNumFaces()*3,m_Indices);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// RenderModels: render all submitted models; assumes necessary client states already enabled,
// and texture environment already setup as required
void CModelRData::RenderModels(u32 streamflags,u32 flags)
{
	uint i;
#if 1
	// submit batches for each model to the vertex buffer
	for (i=0;i<m_Models.size();++i) {
		u32 mflags=m_Models[i]->GetFlags();
		if (!flags || (m_Models[i]->GetFlags()&flags)) {
			CModelRData* modeldata=(CModelRData*) m_Models[i]->GetRenderData();
			modeldata->SubmitBatches();
		}
	}

	// step through all accumulated batches
	const std::list<CVertexBuffer*>& buffers=g_VBMan.GetBufferList();
	std::list<CVertexBuffer*>::const_iterator iter;
	for (iter=buffers.begin();iter!=buffers.end();++iter) {
		CVertexBuffer* buffer=*iter;
		
		// any batches in this VB?
		const std::vector<CVertexBuffer::Batch*>& batches=buffer->GetBatches();
		if (batches.size()>0) {
			u8* base=buffer->Bind();

			// setup data pointers
			u32 stride=sizeof(SVertex);
			glVertexPointer(3,GL_FLOAT,stride,base+offsetof(SVertex,m_Position));
			if (streamflags & STREAM_COLOR) glColorPointer(3,GL_FLOAT,stride,base+offsetof(SVertex,m_Color));
			if (streamflags & STREAM_UV0) glTexCoordPointer(2,GL_FLOAT,stride,base+offsetof(SVertex,m_UVs[0]));

			// render each batch
			for (i=0;i<batches.size();++i) {
				const CVertexBuffer::Batch* batch=batches[i];
				if (batch->m_IndexData.size()>0) {
					if (streamflags & STREAM_UV0) g_Renderer.BindTexture(0,tex_id(batch->m_Texture));
					for (uint j=0;j<batch->m_IndexData.size();j++) {
						glDrawElements(GL_TRIANGLES,(GLsizei)batch->m_IndexData[j].first,GL_UNSIGNED_SHORT,batch->m_IndexData[j].second);
						g_Renderer.m_Stats.m_DrawCalls++;
						g_Renderer.m_Stats.m_ModelTris+=(u32)batch->m_IndexData[j].first/2;
					}
				}
			}
		}
	}
	// everything rendered; empty out batch lists
	g_VBMan.ClearBatchIndices();
#else 
	for (i=0;i<m_Models.size();++i) {
		if (!flags || (m_Models[i]->GetFlags()&flags)) {
			CModelRData* modeldata=(CModelRData*) m_Models[i]->GetRenderData();
			modeldata->RenderStreams(streamflags);
		}
	}
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Submit: submit a model to render this frame
void CModelRData::Submit(CModel* model)
{
	CModelRData* data=(CModelRData*) model->GetRenderData();
	if (data==0) {
		// no renderdata for model, create it now
		data=new CModelRData(model);
		model->SetRenderData(data);
	} else {
		data->Update();
	}

	if (data->GetFlags() & MODELRDATA_FLAG_TRANSPARENT) {
		// add this mode to the transparency renderer for later processing - calculate
		// transform matrix
		g_TransparencyRenderer.Add(model);
	} else {
		// add to regular model list
		m_Models.push_back(model);
	}
}
