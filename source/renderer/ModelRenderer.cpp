/**
 * =========================================================================
 * File        : ModelRenderer.cpp
 * Project     : Pyrogenesis
 * Description : Implementation of ModelRenderer and BatchModelRenderer
 * =========================================================================
 */

#include "precompiled.h"

#include "lib/ogl.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"

#include "ps/CLogger.h"
#include "ps/Profile.h"

#include "graphics/Color.h"
#include "graphics/LightEnv.h"
#include "graphics/Model.h"
#include "graphics/ModelDef.h"

#include "renderer/ModelRenderer.h"
#include "renderer/ModelVertexRenderer.h"
#include "renderer/Renderer.h"
#include "renderer/RenderModifiers.h"

#include <boost/weak_ptr.hpp>

#define LOG_CATEGORY "graphics"


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

	if (model->IsSkinned())
	{
		// boned model - calculate skinned vertex positions/normals
		PROFILE( "skinning bones" );

		// Avoid the noisy warnings that occur inside SkinPoint/SkinNormal in
		// some broken situations
		if (numVertices && vertices[0].m_Blend.m_Bone[0] == 0xff)
		{
			LOG_ONCE(CLogger::Error, LOG_CATEGORY, "Model %s is boned with unboned animation", mdef->GetName().c_str());
			return;
		}

		for (size_t j=0; j<numVertices; j++)
		{
			Position[j] = CModelDef::SkinPoint(vertices[j], model->GetAnimatedBoneMatrices(), model->GetInverseBindBoneMatrices());
			Normal[j] = CModelDef::SkinNormal(vertices[j], model->GetAnimatedBoneMatrices(), model->GetInverseBindBoneMatrices());
		}
	}
	else
	{
		PROFILE( "software transform" );
		// just copy regular positions, transform normals to world space
		const CMatrix3D& transform = model->GetTransform();
		const CMatrix3D& invtransform = model->GetInvTransform();
		for (size_t j=0; j<numVertices; j++)
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
		VertexArrayIterator<SColor4ub> Color,
		bool onlyDiffuse)
{
	PROFILE( "lighting vertices" );

	CModelDefPtr mdef = model->GetModelDef();
	size_t numVertices = mdef->GetNumVertices();
	const CLightEnv& lightEnv = g_Renderer.GetLightEnv();
	CColor shadingColor = model->GetShadingColor();
	RGBColor tempcolor;

	if (onlyDiffuse)
	{
		for (size_t j=0; j<numVertices; j++)
		{
			lightEnv.EvaluateDirect(Normal[j], tempcolor);
			tempcolor.X *= shadingColor.r;
			tempcolor.Y *= shadingColor.g;
			tempcolor.Z *= shadingColor.b;
			Color[j] = ConvertRGBColorTo4ub(tempcolor);
		}
	}
	else
	{
		for (size_t j=0; j<numVertices; j++)
		{
			lightEnv.EvaluateUnit(Normal[j], tempcolor);
			tempcolor.X *= shadingColor.r;
			tempcolor.Y *= shadingColor.g;
			tempcolor.Z *= shadingColor.b;
			Color[j] = ConvertRGBColorTo4ub(tempcolor);
		}
	}
}


// Copy UV coordinates
void ModelRenderer::BuildUV(
		CModelDefPtr mdef,
		VertexArrayIterator<float[2]> UV)
{
	size_t numVertices = mdef->GetNumVertices();
	SModelVertex* vertices = mdef->GetVertices();

	for (size_t j=0; j < numVertices; ++j, ++UV)
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
	size_t idxidx = 0;
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
	boost::weak_ptr<CModelDef> m_ModelDef;

	/// Pointer to the next ModelDefTracker that has submitted models.
	BMRModelDefTracker* m_Next;

	/// Number of slots used in m_ModelSlots
	size_t m_Slots;

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

	/// ModelVertexRenderer used for vertex transformations
	ModelVertexRendererPtr vertexRenderer;

	/// Track the current "phase" of the frame (only for debugging purposes)
	BMRPhase phase;

	/// Linked list of ModelDefTrackers that have submitted models
	BMRModelDefTracker* submissions;

	/// Helper functions
	void ThunkDestroyModelData(CModel* model, void* data)
	{
		vertexRenderer->DestroyModelData(model, data);
	}

	void RenderAllModels(RenderModifierPtr modifier, int filterflags, int pass, int streamflags);
};

BMRModelData::~BMRModelData()
{
	m_BMRI->ThunkDestroyModelData(GetModel(), m_Data);
}


// Construction/Destruction
BatchModelRenderer::BatchModelRenderer(ModelVertexRendererPtr vertexrenderer)
{
	m = new BatchModelRendererInternals(this);
	m->vertexRenderer = vertexrenderer;
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

	ogl_WarnIfError();

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
		bmrdata->m_Data = m->vertexRenderer->CreateModelData(model);
		rdata = bmrdata;
		model->SetRenderData(bmrdata);
		model->SetDirty(~0u);
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
	size_t idx;

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

	ogl_WarnIfError();
}


// Call update for all submitted models and enter the rendering phase
void BatchModelRenderer::PrepareModels()
{
	debug_assert(m->phase == BMRSubmit);

	for(BMRModelDefTracker* mdeftracker = m->submissions; mdeftracker; mdeftracker = mdeftracker->m_Next)
	{
		for(size_t idx = 0; idx < mdeftracker->m_Slots; ++idx)
		{
			for(BMRModelData* bmrdata = mdeftracker->m_ModelSlots[idx]; bmrdata; bmrdata = bmrdata->m_Next)
			{
				CModel* model = bmrdata->GetModel();

				debug_assert(model->GetRenderData() == bmrdata);

				m->vertexRenderer->UpdateModelData(
						model, bmrdata->m_Data,
						bmrdata->m_UpdateFlags);
				bmrdata->m_UpdateFlags = 0;
			}
		}
	}

	m->phase = BMRRender;
}


// Clear the submissions list
void BatchModelRenderer::EndFrame()
{
	static size_t mostslots = 1;

	for(BMRModelDefTracker* mdeftracker = m->submissions; mdeftracker; mdeftracker = mdeftracker->m_Next)
	{
		if (mdeftracker->m_Slots > mostslots)
		{
			mostslots = mdeftracker->m_Slots;
			//debug_printf("BatchModelRenderer: SubmissionSlots maximum: %u\n", mostslots);
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


// Render models, outer loop for multi-passing
void BatchModelRenderer::Render(RenderModifierPtr modifier, int flags)
{
	debug_assert(m->phase == BMRRender);

	if (!HaveSubmissions())
		return;

	int pass = 0;

	do
	{
		int streamflags = modifier->BeginPass(pass);
		const CMatrix3D* texturematrix = 0;

		if (streamflags & STREAM_TEXGENTOUV1)
			texturematrix = modifier->GetTexGenMatrix(pass);

		m->vertexRenderer->BeginPass(streamflags, texturematrix);

		m->RenderAllModels(modifier, flags, pass, streamflags);

		m->vertexRenderer->EndPass(streamflags);
	} while(!modifier->EndPass(pass++));
}


// Render one frame worth of models
void BatchModelRendererInternals::RenderAllModels(
		RenderModifierPtr modifier, int filterflags,
		int pass, int streamflags)
{
	for(BMRModelDefTracker* mdeftracker = submissions; mdeftracker; mdeftracker = mdeftracker->m_Next)
	{
		vertexRenderer->PrepareModelDef(streamflags, mdeftracker->m_ModelDef.lock());

		for(size_t idx = 0; idx < mdeftracker->m_Slots; ++idx)
		{
			BMRModelData* bmrdata = mdeftracker->m_ModelSlots[idx];

			modifier->PrepareTexture(pass, bmrdata->GetModel()->GetTexture());

			for(; bmrdata; bmrdata = bmrdata->m_Next)
			{
				CModel* model = bmrdata->GetModel();

				debug_assert(bmrdata->GetKey() == this);

				if (filterflags && !(model->GetFlags()&filterflags))
					continue;

				modifier->PrepareModel(pass, model);
				vertexRenderer->RenderModel(streamflags, model, bmrdata->m_Data);
			}
		}
	}
}


