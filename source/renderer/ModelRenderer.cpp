/**
 * =========================================================================
 * File        : ModelRenderer.cpp
 * Project     : Pyrogenesis
 * Description : Implementation of ModelRenderer and BatchModelRenderer
 *
 * @author Nicolai Hähnle <nicolai@wildfiregames.com>
 * =========================================================================
 */

#include "precompiled.h"

#include "ogl.h"
#include "Vector3D.h"
#include "Vector4D.h"

#include "ps/CLogger.h"
#include "ps/Profile.h"

#include "graphics/Color.h"
#include "graphics/Model.h"
#include "graphics/ModelDef.h"

#include "renderer/ModelRenderer.h"
#include "renderer/Renderer.h"
#include "renderer/SHCoeffs.h"


#define LOG_CATEGORY "graphics"


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
static void SkinNormal(const SModelVertex& vertex, const CMatrix3D* invtranspmatrices, CVector3D& result)
{
	CVector3D tmp;
	const SVertexBlend& blend=vertex.m_Blend;

	// must have at least one valid bone if we're using SkinNormal
	debug_assert(blend.m_Bone[0]!=0xff);

	const CMatrix3D& m = invtranspmatrices[blend.m_Bone[0]];
	m.Rotate(vertex.m_Norm, result);
	result*=blend.m_Weight[0];

	for (u32 i=1; i<SVertexBlend::SIZE && vertex.m_Blend.m_Bone[i]!=0xff; i++) {
		const CMatrix3D& m = invtranspmatrices[blend.m_Bone[i]];
		m.Rotate(vertex.m_Norm,tmp);
		result+=tmp*blend.m_Weight[i];
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
// ModelRenderer implementation

// Helper function to copy object-space position and normal vectors into arrays.
void ModelRenderer::CopyPositionAndNormals(
		CModelDefPtr mdef,
		VertexArrayIterator<CVector3D> Position,
		VertexArrayIterator<CVector3D> Normal)
{
	size_t numVertices = mdef->GetNumVertices();
	SModelVertex* vertices = mdef->GetVertices();
	
	for(size_t j = 0; j < numVertices; ++j)
	{
		Position[j] = vertices[j].m_Coords;
		Normal[j] = vertices[j].m_Norm;
	}
}

// Helper function to transform position and normal vectors into world-space.
void ModelRenderer::BuildPositionAndNormals(
		CModel* model,
		VertexArrayIterator<CVector3D> Position,
		VertexArrayIterator<CVector3D> Normal)
{
	CModelDefPtr mdef = model->GetModelDef();
	size_t numVertices = mdef->GetNumVertices();
	SModelVertex* vertices=mdef->GetVertices();
	
	const CMatrix3D* bonematrices = model->GetBoneMatrices();
	if (bonematrices)
	{
		// boned model - calculate skinned vertex positions/normals
		PROFILE( "skinning bones" );
		const CMatrix3D* invtranspbonematrices;
		
		// Analytic geometry tells us that normal vectors need to be
		// multiplied by the inverse of the transpose. However, calculating
		// the inverse is slow, and analytic geometry also tells us that
		// for orthogonal matrices, the inverse is equal to the transpose,
		// so the inverse of the transpose is, in fact, the original matrix.
		//
		// The "fast normals" code assumes that bone transformation contain
		// no "weird" transformations like shears or non-uniform scaling
		// (actually, the entire code assumes no scaling) and thus gets
		// around the slow calculation of the inverse.
		if (g_Renderer.m_FastNormals)
			invtranspbonematrices = bonematrices;
		else
			invtranspbonematrices = model->GetInvTranspBoneMatrices();
		
		for (size_t j=0; j<numVertices; j++)
		{
			SkinPoint(vertices[j],bonematrices,Position[j]);
			SkinNormal(vertices[j],invtranspbonematrices,Normal[j]);
		}
	}
	else
	{
		PROFILE( "software transform" );
		// just copy regular positions, transform normals to world space
		const CMatrix3D& transform = model->GetTransform();
		const CMatrix3D& invtransform = model->GetInvTransform();
		for (uint j=0; j<numVertices; j++)
		{
			transform.Transform(vertices[j].m_Coords,Position[j]);
			invtransform.RotateTransposed(vertices[j].m_Norm,Normal[j]);
		}
	}
}


// Helper function for lighting
void ModelRenderer::BuildColor4ub(
		CModel* model,
		VertexArrayIterator<CVector3D> Normal,
		VertexArrayIterator<SColor4ub> Color)
{
	PROFILE( "lighting vertices" );
	
	CModelDefPtr mdef = model->GetModelDef();
	size_t numVertices = mdef->GetNumVertices();
	CSHCoeffs& shcoeffs = g_Renderer.m_SHCoeffsUnits;
	CColor sc = model->GetShadingColor();
	RGBColor shadingcolor(sc.r, sc.g, sc.b);
	RGBColor tempcolor;
	
	for (uint j=0; j<numVertices; j++)
	{
		shcoeffs.Evaluate(Normal[j], tempcolor, shadingcolor);
		*(u32*)&Color[j] = ConvertRGBColorTo4ub(tempcolor);
	}
}


// Copy UV coordinates
void ModelRenderer::BuildUV(
		CModelDefPtr mdef,
		VertexArrayIterator<float[2]> UV)
{
	size_t numVertices = mdef->GetNumVertices();
	SModelVertex* vertices = mdef->GetVertices();
		
	for (uint j=0; j < numVertices; ++j, ++UV)
	{
		(*UV)[0] = vertices[j].m_U;
		(*UV)[1] = 1.0-vertices[j].m_V;
	}
}


// Build default indices array.
void ModelRenderer::BuildIndices(
		CModelDefPtr mdef,
		u16* Indices)
{
	u32 idxidx = 0;
	SModelFace* faces = mdef->GetFaces();
	
	for (size_t j = 0; j < mdef->GetNumFaces(); ++j) {
		SModelFace& face=faces[j];
		Indices[idxidx++]=face.m_Verts[0];
		Indices[idxidx++]=face.m_Verts[1];
		Indices[idxidx++]=face.m_Verts[2];
	}
}



///////////////////////////////////////////////////////////////////////////////////////////////
// BatchModelRenderer implementation


/// See BatchModelRendererInternals::phase
enum BMRPhase {
	/// Currently allow calls to Submit and PrepareModels
	BMRSubmit,
	
	/// Allow calls to rendering and EndFrame
	BMRRender
};


/**
 * Struct BMRModelData: Per-CModel render data used by the BatchModelRenderer.
 */
struct BMRModelData : public CModelRData
{
	BMRModelData(BatchModelRendererInternals* bmri, CModel* model)
	: CModelRData(bmri, model), m_BMRI(bmri), m_Data(0), m_Next(0) { }
	virtual ~BMRModelData();
	
	/// Back-link to "our" modelrenderer
	BatchModelRendererInternals* m_BMRI;
	
	/// Private data created by derived class' CreateModelData
	void* m_Data;
	
	/// Next model in the per-ModelDefTracker-slot linked list.
	BMRModelData* m_Next;
};


/**
 * Class BMRModelDefTracker: Per-CModelDef data used by the BatchModelRenderer.
 * 
 * Note that classes that derive from BatchModelRenderer should use
 * their own per-CModelDef data if necessary.
 */
struct BMRModelDefTracker : public CModelDefRPrivate
{
	BMRModelDefTracker(CModelDefPtr mdef)
	: m_ModelDef(mdef), m_Next(0), m_Slots(0) { }

	/// Back-link to the CModelDef object
	CModelDefPtr m_ModelDef;

	/// Pointer to the next ModelDefTracker that has submitted models.
	BMRModelDefTracker* m_Next;
	
	/// Number of slots used in m_ModelSlots
	uint m_Slots;
	
	/// Each slot contains a linked list of model data objects, up to m_Slots-1
	// At the end of the frame, m_Slots is reset to 0, but m_ModelSlots stays
	// the same size (we assume the same number of slots is going to be used
	// next frame)
	std::vector<BMRModelData*> m_ModelSlots;
};



/**
 * Struct BatchModelRendererInternals: Internal data of the BatchModelRenderer
 * 
 * Separated into the source file to increase implementation hiding (and to 
 * avoid some causes of recompiles).
 */
struct BatchModelRendererInternals
{
	BatchModelRendererInternals(BatchModelRenderer* r) : m_Renderer(r) { }
	
	/// Back-link to "our" renderer
	BatchModelRenderer* m_Renderer;
	
	/// Track the current "phase" of the frame (only for debugging purposes)
	BMRPhase phase;
	
	/// Linked list of ModelDefTrackers that have submitted models
	BMRModelDefTracker* submissions;
	
	/// Helper functions
	void ThunkDestroyModelData(CModel* model, void* data)
	{
		m_Renderer->DestroyModelData(model, data);
	}
};

BMRModelData::~BMRModelData()
{
	m_BMRI->ThunkDestroyModelData(GetModel(), m_Data);
}


// Construction/Destruction
BatchModelRenderer::BatchModelRenderer()
{
	m = new BatchModelRendererInternals(this);
	m->phase = BMRSubmit;
	m->submissions = 0;
}

BatchModelRenderer::~BatchModelRenderer()
{
	delete m;
}

// Submit one model.
void BatchModelRenderer::Submit(CModel* model)
{
	debug_assert(m->phase == BMRSubmit);
	
	CModelDefPtr mdef = model->GetModelDef();
	BMRModelDefTracker* mdeftracker = (BMRModelDefTracker*)mdef->GetRenderData(m);
	CModelRData* rdata = (CModelRData*)model->GetRenderData();
	BMRModelData* bmrdata = 0;
	
	// Ensure model def data and model data exist
	if (!mdeftracker)
	{
		mdeftracker = new BMRModelDefTracker(mdef);
		mdef->SetRenderData(m, mdeftracker);
	}
	
	if (rdata && rdata->GetKey() == m)
	{
		bmrdata = (BMRModelData*)rdata;
	}
	else
	{
		bmrdata = new BMRModelData(m, model);
		bmrdata->m_Data = CreateModelData(model);
		rdata = bmrdata;
		model->SetRenderData(bmrdata);
		model->SetDirty(~0);
		g_Renderer.LoadTexture(model->GetTexture(), GL_CLAMP_TO_EDGE);
	}

	// Add the model def tracker to the submission list if necessary
	if (!mdeftracker->m_Slots)
	{
		mdeftracker->m_Next = m->submissions;
		m->submissions = mdeftracker;
	}

	// Add the bmrdata to the modeldef list
	Handle htex = model->GetTexture()->GetHandle();
	uint idx;
	
	for(idx = 0; idx < mdeftracker->m_Slots; ++idx)
	{
		BMRModelData* in = mdeftracker->m_ModelSlots[idx];
		
		if (in->GetModel()->GetTexture()->GetHandle() == htex)
			break;
	}
	
	if (idx >= mdeftracker->m_Slots)
	{
		++mdeftracker->m_Slots;
		if (mdeftracker->m_Slots > mdeftracker->m_ModelSlots.size())
		{
			mdeftracker->m_ModelSlots.push_back(0);
			debug_assert(mdeftracker->m_ModelSlots.size() == mdeftracker->m_Slots);
		}
		mdeftracker->m_ModelSlots[idx] = 0;
	}

	bmrdata->m_Next = mdeftracker->m_ModelSlots[idx];
	mdeftracker->m_ModelSlots[idx] = bmrdata;
}


// Call update for all submitted models and enter the rendering phase
void BatchModelRenderer::PrepareModels()
{
	debug_assert(m->phase == BMRSubmit);
	
	for(BMRModelDefTracker* mdeftracker = m->submissions; mdeftracker; mdeftracker = mdeftracker->m_Next)
	{
		for(uint idx = 0; idx < mdeftracker->m_Slots; ++idx)
		{
			for(BMRModelData* bmrdata = mdeftracker->m_ModelSlots[idx]; bmrdata; bmrdata = bmrdata->m_Next)
			{
				debug_assert(bmrdata->GetModel()->GetRenderData() == bmrdata);
				
				UpdateModelData(bmrdata->GetModel(), bmrdata->m_Data, bmrdata->m_UpdateFlags);
				bmrdata->m_UpdateFlags = 0;
			}
		}
	}
	
	m->phase = BMRRender;
}


// Clear the submissions list
void BatchModelRenderer::EndFrame()
{
	static uint mostslots = 1;
	
	for(BMRModelDefTracker* mdeftracker = m->submissions; mdeftracker; mdeftracker = mdeftracker->m_Next)
	{
		if (mdeftracker->m_Slots > mostslots)
		{
			mostslots = mdeftracker->m_Slots;
			debug_printf("BatchModelRenderer: SubmissionSlots maximum: %u\n", mostslots);
		}
		mdeftracker->m_Slots = 0;
	}
	m->submissions = 0;
	
	m->phase = BMRSubmit;
}


// Return whether we models have been submitted this frame
bool BatchModelRenderer::HaveSubmissions()
{
	return m->submissions != 0;
}


// Walk through the submissions list and call PrepareXYZ and RenderModel as necessary
void BatchModelRenderer::RenderAllModels(u32 flags)
{
	debug_assert(m->phase == BMRRender);
	
	for(BMRModelDefTracker* mdeftracker = m->submissions; mdeftracker; mdeftracker = mdeftracker->m_Next)
	{
		PrepareModelDef(mdeftracker->m_ModelDef);
		
		for(uint idx = 0; idx < mdeftracker->m_Slots; ++idx)
		{
			BMRModelData* bmrdata = mdeftracker->m_ModelSlots[idx];
			
			PrepareTexture(bmrdata->GetModel()->GetTexture());

			for(; bmrdata; bmrdata = bmrdata->m_Next)
			{
				CModel* model = bmrdata->GetModel();
				
				debug_assert(bmrdata->GetKey() == m);
				
				if (flags && !(model->GetFlags()&flags))
					continue;
			
				RenderModel(model, bmrdata->m_Data);
			}
		}
	}
}

