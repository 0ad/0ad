#include <assert.h>
#include <algorithm>
#include "res/tex.h"
#include "Renderer.h"
#include "TransparencyRenderer.h"
#include "ModelRData.h"
#include "terrain/Model.h"


CModelRData::CModelRData(CModel* model) : m_Model(model), m_Vertices(0), m_Normals(0), m_Indices(0), m_VB(0) 
{
	assert(model);
	// build all data now
	Build();
}

CModelRData::~CModelRData() 
{
}

void CModelRData::Build()
{
	BuildVertices();
	BuildIndices();
}

void CModelRData::BuildIndices()
{
	CModelDef* mdef=m_Model->GetModelDef();

	// allocate indices if we haven't got any already
	if (!m_Indices) {
		m_Indices=new u16[mdef->GetNumFaces()*3];
	}

	// build indices
	u32 indices=0;
	SModelFace* faces=mdef->GetFaces();
	for (int j=0; j<mdef->GetNumFaces(); j++) {
		SModelFace& face=faces[j];
		m_Indices[indices++]=face.m_Verts[0];
		m_Indices[indices++]=face.m_Verts[1];
		m_Indices[indices++]=face.m_Verts[2];
	}
}

inline int clamp(int x,int min,int max)
{
	if (x<min) return min;
	else if (x>max) return max;
	else return x;
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

static CVector3D SkinPoint(const SModelVertex& vertex,const CMatrix3D* matrices)
{
	CVector3D result(0,0,0),tmp;

	for (u32 i=0;vertex.m_Blend.m_Bone[i]!=0xff && i<SVertexBlend::SIZE;i++) {
		const CMatrix3D& m=matrices[vertex.m_Blend.m_Bone[i]];
		m.Transform(vertex.m_Coords,tmp);
		result+=tmp*vertex.m_Blend.m_Weight[i];
	}

	return result;
}

static CVector3D SkinNormal(const SModelVertex& vertex,const CMatrix3D* invmatrices)
{
	CVector3D result(0,0,0),tmp;

	for (u32 i=0;vertex.m_Blend.m_Bone[i]!=0xff && i<SVertexBlend::SIZE;i++) {
		const CMatrix3D& m=invmatrices[vertex.m_Blend.m_Bone[i]];
		m.RotateTransposed(vertex.m_Norm,tmp);
		result+=tmp*vertex.m_Blend.m_Weight[i];
	}

	return result;
}

void CModelRData::BuildVertices()
{
	CModelDef* mdef=m_Model->GetModelDef();

	// allocate vertices if we haven't got any already
	if (!m_Vertices) {
		m_Vertices=new SVertex[mdef->GetNumVertices()];
		m_Normals=new CVector3D[mdef->GetNumVertices()];
	}

	// build vertices
	u32 numVertices=mdef->GetNumVertices();
	SModelVertex* vertices=mdef->GetVertices();
	if (m_Model->GetBoneMatrices()) {
		// boned model - calculate skinned vertex positions/normals
		for (uint j=0; j<numVertices; j++) {
			m_Vertices[j].m_Position=SkinPoint(vertices[j],m_Model->GetBoneMatrices());
			m_Normals[j]=SkinNormal(vertices[j],m_Model->GetInvBoneMatrices());
		}
	} else {
		// just copy regular positions
		for (uint j=0; j<numVertices; j++) {
			m_Vertices[j].m_Position=vertices[j].m_Coords;
			m_Normals[j]=vertices[j].m_Norm;
		}
	}
	
	// now fill in UV and vertex colour data
	for (uint j=0; j<numVertices; j++) {
		m_Vertices[j].m_UVs[0]=vertices[j].m_U;
		m_Vertices[j].m_UVs[1]=1-vertices[j].m_V;

		RGBColor c;
		g_Renderer.m_SHCoeffsUnits.Evaluate(m_Normals[j],c);

		m_Vertices[j].m_Color=ConvertColor(c);
	}

	if (g_Renderer.m_Caps.m_VBO) {
		if (!m_VB) {
			glGenBuffersARB(1,(GLuint*) &m_VB);
			glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_VB);
			glBufferDataARB(GL_ARRAY_BUFFER_ARB,mdef->GetNumVertices()*sizeof(SVertex),0,mdef->GetNumBones() ? GL_DYNAMIC_DRAW_ARB : GL_STATIC_DRAW_ARB);
		}
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_VB);
		glBufferSubDataARB(GL_ARRAY_BUFFER_ARB,0,mdef->GetNumVertices()*sizeof(SVertex),m_Vertices);
	} 
}


void CModelRData::RenderStreams(u32 streamflags,const CMatrix3D& transform,bool transparentPass)
{	
	// ignore transparent passes 
	if (!transparentPass && g_Renderer.IsTextureTransparent(m_Model->GetTexture())) {
		return;
	}
	
	CModelDef* mdldef=(CModelDef*) m_Model->GetModelDef();
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	CMatrix3D tmp;
	glMultMatrixf(&transform._11);	

	if (streamflags & STREAM_UV0) g_Renderer.SetTexture(0,m_Model->GetTexture(),GL_CLAMP_TO_EDGE);

	u8* base;
	if (g_Renderer.m_Caps.m_VBO) {
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_VB);
		base=0;
	} else {
		base=(u8*) &m_Vertices[0];
	}

	// set vertex pointers
	u32 stride=sizeof(SVertex);
	glVertexPointer(3,GL_FLOAT,stride,base+offsetof(SVertex,m_Position));
	if (streamflags & STREAM_COLOR) glColorPointer(4,GL_UNSIGNED_BYTE,stride,base+offsetof(SVertex,m_Color));
	if (streamflags & STREAM_UV0) glTexCoordPointer(2,GL_FLOAT,stride,base+offsetof(SVertex,m_UVs));

	// render the lot
	u32 numFaces=mdldef->GetNumFaces();
	glDrawElements(GL_TRIANGLES,numFaces*3,GL_UNSIGNED_SHORT,m_Indices);
	
	// bump stats
	g_Renderer.m_Stats.m_DrawCalls++;
	if (transparentPass) {
		g_Renderer.m_Stats.m_TransparentTris+=numFaces;
	} else {
		g_Renderer.m_Stats.m_ModelTris+=numFaces;
	}

	glPopMatrix();
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

	CModelDef* mdldef=(CModelDef*) m_Model->GetModelDef();

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
		m_Indices[indices++]=face.m_Verts[0];
		m_Indices[indices++]=face.m_Verts[1];
		m_Indices[indices++]=face.m_Verts[2];
	}

	// clear list for next call
	IndexSorter.clear();

	return mindist;
}
