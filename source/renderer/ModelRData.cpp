#include "precompiled.h"


#include <algorithm>
#include "MathUtil.h"
#include "lib/res/graphics/ogl_tex.h"
#include "Renderer.h"
#include "TransparencyRenderer.h"
#include "PlayerRenderer.h"
#include "ModelRData.h"
#include "Model.h"
#include "ModelDef.h"
#include "MaterialManager.h"
#include "Profile.h"
#include "renderer/ModelDefRData.h"



///////////////////////////////////////////////////////////////////
// CModelRData constructor
CModelRData::CModelRData(CModel* model) 
	: m_Model(model), m_Normals(0), m_DynamicArray(true), m_Indices(0), m_Flags(0)
{
	debug_assert(model);
	// build all data now
	Build();
}

///////////////////////////////////////////////////////////////////
// CModelRData destructor
CModelRData::~CModelRData() 
{
	// clean up system copies of data
	delete[] m_Indices;
	delete[] m_Normals;
}

void CModelRData::Build()
{
	CModelDefPtr mdef = m_Model->GetModelDef();

	if (!mdef->GetRenderData())
	{
		mdef->SetRenderData(new CModelDefRData(&*mdef));
	}
	
	m_Position.type = GL_FLOAT;
	m_Position.elems = 3;
	m_DynamicArray.AddAttribute(&m_Position);
/*
	m_UV.type = GL_FLOAT;
	m_UV.elems = 2;
	m_DynamicArray.AddAttribute(&m_UV);
*/
	m_Color.type = GL_UNSIGNED_BYTE;
	m_Color.elems = 3;
	m_DynamicArray.AddAttribute(&m_Color);
	
	m_DynamicArray.SetNumVertices(mdef->GetNumVertices());
	m_DynamicArray.Layout();
	
	// build data
	BuildStaticVertices();
	BuildVertices();
	BuildIndices();
	// force a texture load on model's texture
	g_Renderer.LoadTexture(m_Model->GetTexture(),GL_CLAMP_TO_EDGE);
	// setup model render flags
	/*if (g_Renderer.IsTextureTransparent(m_Model->GetTexture())) {
		m_Flags|=MODELRDATA_FLAG_TRANSPARENT;
	}*/
	if(m_Model->GetMaterial().IsPlayer())
	{
		m_Flags |= MODELRDATA_FLAG_PLAYERCOLOR;
	}
	else if(m_Model->GetMaterial().UsesAlpha())
	{
		m_Flags |= MODELRDATA_FLAG_TRANSPARENT;
	}
}

void CModelRData::BuildIndices()
{
	CModelDefPtr mdef=m_Model->GetModelDef();
	debug_assert(mdef);
	
	// allocate indices if we haven't got any already
	if (!m_Indices) {
		m_Indices=new u16[mdef->GetNumFaces()*3];
	}

	// build indices
	u32 indices=0;
	SModelFace* faces=mdef->GetFaces();
	for (size_t j=0; j<mdef->GetNumFaces(); j++) {
		SModelFace& face=faces[j];
		m_Indices[indices++]=face.m_Verts[0];
		m_Indices[indices++]=face.m_Verts[1];
		m_Indices[indices++]=face.m_Verts[2];
	}
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SkinPoint: skin the vertex position using it's blend data and given bone matrices
static void SkinPoint(const SModelVertex& vertex,const CMatrix3D* matrices,CVector3D& result)
{
	CVector3D tmp;
	const SVertexBlend& blend=vertex.m_Blend;

	// must have at least one valid bone if we're using SkinPoint
	debug_assert(blend.m_Bone[0]!=0xff);

	const CMatrix3D& m=matrices[blend.m_Bone[0]];
	m.Transform(vertex.m_Coords,result);
	result*=blend.m_Weight[0];

	for (u32 i=1; i<SVertexBlend::SIZE && blend.m_Bone[i]!=0xff; i++) {
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
	debug_assert(blend.m_Bone[0]!=0xff);

	const CMatrix3D& m=invmatrices[blend.m_Bone[0]];
	m.RotateTransposed(vertex.m_Norm,result);
	result*=blend.m_Weight[0];

	for (u32 i=1; i<SVertexBlend::SIZE && vertex.m_Blend.m_Bone[i]!=0xff; i++) {
		const CMatrix3D& m=invmatrices[blend.m_Bone[i]];
		m.RotateTransposed(vertex.m_Norm,tmp);
		result+=tmp*blend.m_Weight[i];
	}
}

void CModelRData::BuildStaticVertices()
{
/*
	CModelDefPtr mdef = m_Model->GetModelDef();
	size_t numVertices = mdef->GetNumVertices();
	SModelVertex* vertices = mdef->GetVertices();
	VertexArrayIterator<float[]> UVit = m_UV.GetIterator<float[]>();
	
	for (uint j=0; j < numVertices; ++j, ++UVit) {
		(*UVit)[0] = vertices[j].m_U;
		(*UVit)[1] = 1.0-vertices[j].m_V;
	}
*/
}

void CModelRData::BuildVertices()
{
	CModelDefPtr mdef=m_Model->GetModelDef();
	size_t numVertices=mdef->GetNumVertices();
	SModelVertex* vertices=mdef->GetVertices();

	// allocate vertices if we haven't got any already and 
	// fill in data that never changes
	if (!m_Normals)
		m_Normals=new CVector3D[mdef->GetNumVertices()];

	// build vertices
	VertexArrayIterator<CVector3D> Position = m_Position.GetIterator<CVector3D>();
	VertexArrayIterator<SColor3ub> Color = m_Color.GetIterator<SColor3ub>();
	const CMatrix3D* bonematrices=m_Model->GetBoneMatrices();
	if (bonematrices) {
		// boned model - calculate skinned vertex positions/normals
		PROFILE( "skinning bones" );
		const CMatrix3D* invbonematrices=m_Model->GetInvBoneMatrices();
		for (size_t j=0; j<numVertices; j++) {
			SkinPoint(vertices[j],bonematrices,Position[j]);
			SkinNormal(vertices[j],invbonematrices,m_Normals[j]);
		}
	} else {
		// just copy regular positions, transform normals to world space
		const CMatrix3D& transform=m_Model->GetTransform();
		const CMatrix3D& invtransform=m_Model->GetInvTransform();
		for (uint j=0; j<numVertices; j++) {
			transform.Transform(vertices[j].m_Coords,Position[j]);
			invtransform.RotateTransposed(vertices[j].m_Norm,m_Normals[j]);
		}
	}
	
	PROFILE_START( "lighting vertices" );
	// now fill in UV and vertex colour data
	CSHCoeffs& shcoeffs = g_Renderer.m_SHCoeffsUnits;
	CColor sc = m_Model->GetShadingColor();
	RGBColor shadingcolor(sc.r, sc.g, sc.b);
	RGBColor tempcolor;
	for (uint j=0; j<numVertices; j++) {
		shcoeffs.Evaluate(m_Normals[j], tempcolor, shadingcolor);
		*(u32*)&Color[j] = ConvertRGBColorTo4ub(tempcolor);
	}
	PROFILE_END( "lighting vertices" );

	// upload everything to vertex buffer
	m_DynamicArray.Upload();
}


void CModelRData::RenderStreams(u32 streamflags, bool isplayer)
{	
	CModelDefPtr mdldef=m_Model->GetModelDef();
	
	if (streamflags & STREAM_UV0)
	{
		if(!isplayer)
			m_Model->GetMaterial().Bind();
		else
			g_Renderer.SetTexture(1,m_Model->GetTexture());

		g_Renderer.SetTexture(0,m_Model->GetTexture());
	}

	((CModelDefRData*)mdldef->GetRenderData())->PrepareStream(streamflags);
	
	u8* base = m_DynamicArray.Bind();
	size_t stride = m_DynamicArray.GetStride();
	
	glVertexPointer(3, GL_FLOAT, stride, base + m_Position.offset);
	if (streamflags & STREAM_COLOR) glColorPointer(3, m_Color.type, stride, base + m_Color.offset);
#if 0
	{
		uint a = (uint)this;
		uint b = (uint)m_Model->GetTexture();
		uint hash = ((a >> 16) ^ (b & 0xffff)) | ((a & 0xffff0000) ^ (b << 16));
		hash = (hash * 65537) + 17;
		hash |= 0xff000000;
		glColor4ubv((GLubyte*)&hash);
		glDisableClientState(GL_COLOR_ARRAY);
	}
#endif

	// render the lot
	size_t numFaces=mdldef->GetNumFaces();
	glDrawRangeElementsEXT(GL_TRIANGLES,0,mdldef->GetNumVertices(),numFaces*3,GL_UNSIGNED_SHORT,m_Indices);

	if(streamflags & STREAM_UV0 & !isplayer)
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
	
	size_t numFaces=mdldef->GetNumFaces();
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
		m_Indices[indices++]=(u16)(face.m_Verts[0]);
		m_Indices[indices++]=(u16)(face.m_Verts[1]);
		m_Indices[indices++]=(u16)(face.m_Verts[2]);
	}

	// clear list for next call
	IndexSorter.clear();

	return mindist;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
// RenderModels: render all submitted models; assumes necessary client states already enabled,
// and texture environment already setup as required
void CModelRData::RenderModels(u32 streamflags, u32 flags)
{
	for(CModelDefRData* mdefdata = CModelDefRData::m_Submissions; 
	    mdefdata;
	    mdefdata = mdefdata->m_SubmissionNext)
	{
		mdefdata->PrepareStream(streamflags);
		
		for(uint idx = 0; idx < mdefdata->m_SubmissionSlots; ++idx)
		{
			CModelRData* modeldata = mdefdata->m_SubmissionModels[idx];
			
			if (streamflags & STREAM_UV0)
				g_Renderer.SetTexture(0, modeldata->GetModel()->GetTexture());

			for(; modeldata; modeldata = modeldata->m_SubmissionNext)
			{
				if (flags && !(modeldata->GetModel()->GetFlags()&flags))
					continue;
				
				CModelDefPtr mdldef = modeldata->GetModel()->GetModelDef();
	
				u8* base = modeldata->m_DynamicArray.Bind();
				size_t stride = modeldata->m_DynamicArray.GetStride();
	
				glVertexPointer(3, GL_FLOAT, stride, 
						base + modeldata->m_Position.offset);
				if (streamflags & STREAM_COLOR)
					glColorPointer(3, modeldata->m_Color.type, stride, 
							base + modeldata->m_Color.offset);
			
				// render the lot
				size_t numFaces=mdldef->GetNumFaces();
				glDrawRangeElementsEXT(GL_TRIANGLES, 0, mdldef->GetNumVertices(),
						numFaces*3, GL_UNSIGNED_SHORT, modeldata->m_Indices);
			
				// bump stats
				g_Renderer.m_Stats.m_DrawCalls++;
				g_Renderer.m_Stats.m_ModelTris+=numFaces;
			}
		}
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////
// Submit: submit a model to render this frame
void CModelRData::Submit(CModel* model)
{
	CModelRData* data=(CModelRData*) model->GetRenderData();
	if (data==0) {
		// no renderdata for model, create it now
		PROFILE( "create render data" );
		data=new CModelRData(model);
		model->SetRenderData(data);
	} else {
		PROFILE( "update render data" );
		data->Update();
	}

	if (data->GetFlags() & MODELRDATA_FLAG_TRANSPARENT) {
		// add this mode to the transparency renderer for later processing - calculate
		// transform matrix
		g_TransparencyRenderer.Add(model);
	} else if (data->GetFlags() & MODELRDATA_FLAG_PLAYERCOLOR) {
		// add this model to the player renderer
		g_PlayerRenderer.Add(model);
	} else {
		CModelDefPtr mdldef = model->GetModelDef();
		CModelDefRData* mdefdata = (CModelDefRData*)mdldef->GetRenderData();
		
		debug_assert(mdefdata != 0);
		
		mdefdata->Submit(data);
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////
// ClearSubmissions: Clear the submissions list
// TODO: This is asymmetrical: It only clears CModelRData lists, but no player/transparency renderer lists
void CModelRData::ClearSubmissions()
{
	for(CModelDefRData* mdefdata = CModelDefRData::m_Submissions; mdefdata; mdefdata = mdefdata->m_SubmissionNext)
	{
		mdefdata->ClearSubmissions();
	}
	CModelDefRData::m_Submissions = 0;
}
