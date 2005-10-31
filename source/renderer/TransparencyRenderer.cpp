/**
 * =========================================================================
 * File        : TransparencyRenderer.h
 * Project     : Pyrogenesis
 * Description : ModelRenderer implementation that sorts polygons based
 *             : on distance from viewer, for transparency rendering.
 *
 * @author Rich Cross <rich@wildfiregames.com>
 * @author Nicolai Hähnle <nicolai@wildfiregames.com>
 * =========================================================================
 */

#include "precompiled.h"

#include <algorithm>
#include <vector>

#include "ogl.h"
#include "MathUtil.h"
#include "Vector3D.h"
#include "Vector4D.h"

#include "graphics/Model.h"
#include "graphics/ModelDef.h"

#include "ps/Profile.h"

#include "renderer/Renderer.h"
#include "renderer/TransparencyRenderer.h"
#include "renderer/VertexArray.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
// TransparencyRenderer implementation

/**
 * Struct TModelDef: Per-CModelDef data for the transparency renderer
 */
struct TModelDef : public CModelDefRPrivate
{
	TModelDef(CModelDefPtr mdef);
	
	/// Static vertex array
	VertexArray m_Array;
	
	/// UV is static
	VertexArray::Attribute m_UV;
};

TModelDef::TModelDef(CModelDefPtr mdef)
	: m_Array(false)
{
	m_UV.type = GL_FLOAT;
	m_UV.elems = 2;
	m_Array.AddAttribute(&m_UV);
	
	m_Array.SetNumVertices(mdef->GetNumVertices());
	m_Array.Layout();

	VertexArrayIterator<float[2]> UVit = m_UV.GetIterator<float[2]>();
	
	ModelRenderer::BuildUV(mdef, UVit);
	
	m_Array.Upload();
	m_Array.FreeBackingStore();
}


/**
 * Struct TModel: Per-CModel data for the transparency renderer
 */
struct TModel : public CModelRData
{
	TModel(TransparencyRendererInternals* tri, CModel* model);
	
	/**
	 * BackToFrontIndexSort: Sort polygons by distance to camera for
	 * transparency rendering and fill the indices array appropriately.
	 *
	 * @param worldToCam World to camera coordinate space transform
	 *
	 * @return Square of the estimated distance to the nearest triangle.
	 */
	float BackToFrontIndexSort(const CMatrix3D& objToCam);
	
	/// Dynamic per-CModel vertex array
	VertexArray m_Array;
	
	/// Position and lighting are recalculated on CPU every frame
	VertexArray::Attribute m_Position;
	VertexArray::Attribute m_Color;

	/// Indices array (sorted on CPU based on distance to camera)
	u16* m_Indices;
};

TModel::TModel(TransparencyRendererInternals* tri, CModel* model)
	: CModelRData(tri, model), m_Array(true)
{
	CModelDefPtr mdef = model->GetModelDef();
	
	m_Position.type = GL_FLOAT;
	m_Position.elems = 3;
	m_Array.AddAttribute(&m_Position);

	m_Color.type = GL_UNSIGNED_BYTE;
	m_Color.elems = 4;
	m_Array.AddAttribute(&m_Color);
	
	m_Array.SetNumVertices(mdef->GetNumVertices());
	m_Array.Layout();
	
	m_Indices = new u16[mdef->GetNumFaces()*3];
}


typedef std::pair<int,float> IntFloatPair;

struct SortFacesByDist {
	bool operator()(const IntFloatPair& lhs,const IntFloatPair& rhs) {
		return lhs.second>rhs.second ? true : false;
	}
};

float TModel::BackToFrontIndexSort(const CMatrix3D& worldToCam)
{
	static std::vector<IntFloatPair> IndexSorter;

	CModelDefPtr mdef = GetModel()->GetModelDef();
	size_t numFaces = mdef->GetNumFaces();
	const SModelFace* faces = mdef->GetFaces();
	
	if (IndexSorter.size() < numFaces)
		IndexSorter.resize(numFaces);

	VertexArrayIterator<CVector3D> Position = m_Position.GetIterator<CVector3D>();
	CVector3D tmpvtx;

	for(size_t i = 0; i < numFaces; ++i)
	{
		tmpvtx = Position[faces[i].m_Verts[0]];
		tmpvtx += Position[faces[i].m_Verts[1]];
		tmpvtx += Position[faces[i].m_Verts[2]];
		tmpvtx *= 1.0f/3.0f;

		tmpvtx = worldToCam.Transform(tmpvtx);
		float distsqrd = SQR(tmpvtx.X)+SQR(tmpvtx.Y)+SQR(tmpvtx.Z);

		IndexSorter[i].first = (int)i;
		IndexSorter[i].second = distsqrd;
	}

	std::sort(IndexSorter.begin(),IndexSorter.begin()+numFaces,SortFacesByDist());
	
	// now build index list
	u32 idxidx = 0;
	for (size_t i = 0; i < numFaces; ++i) {
		const SModelFace& face = faces[IndexSorter[i].first];
		m_Indices[idxidx++] = (u16)(face.m_Verts[0]);
		m_Indices[idxidx++] = (u16)(face.m_Verts[1]);
		m_Indices[idxidx++] = (u16)(face.m_Verts[2]);
	}

	return IndexSorter[0].second;
}


/**
 * Struct SObject: Pair of model and camera distance.
 */
struct SObject
{
	/// the transparent model
	TModel* m_Model;

	/// sqrd distance from camera to centre of nearest triangle
	float m_Dist;
	
	SObject(TModel* tmdl) : m_Model(tmdl), m_Dist(0) { }
};


/**
 * Struct TransparencyRendererInternals: Internal data structure of TransparencyRenderer
 */
struct TransparencyRendererInternals
{
	/// List of submitted models.
	std::vector<SObject> objects;
	
	/// Scratch space for normal vector calculation
	std::vector<CVector3D> normals;
};


// Construction / Destruction
TransparencyRenderer::TransparencyRenderer()
{
	m = new TransparencyRendererInternals;
}

TransparencyRenderer::~TransparencyRenderer()
{
	delete m;
}

// Submit a model: Create, but don't fill in, our own Model and ModelDef structures
void TransparencyRenderer::Submit(CModel* model)
{
	CModelRData* rdata = (CModelRData*)model->GetRenderData();
	TModel* tmdl;
	
	if (rdata && rdata->GetKey() == m)
	{
		tmdl = (TModel*)rdata;
	}
	else
	{
		CModelDefPtr mdef = model->GetModelDef();
		TModelDef* tmdef = (TModelDef*)mdef->GetRenderData(m);
		
		if (!tmdef)
		{
			tmdef = new TModelDef(mdef);
			mdef->SetRenderData(m, tmdef);
		}
	
		tmdl = new TModel(m, model);
		rdata = tmdl;
		model->SetRenderData(rdata);
		model->SetDirty(~0u);
		g_Renderer.LoadTexture(model->GetTexture(), GL_CLAMP_TO_EDGE);
	}
	
	m->objects.push_back(tmdl);
}


// Transform and sort all models
struct SortObjectsByDist {
	bool operator()(const SObject& lhs, const SObject& rhs) {
		return lhs.m_Dist>rhs.m_Dist? true : false;
	}
};

void TransparencyRenderer::PrepareModels()
{
	CMatrix3D worldToCam;
	
	if (m->objects.size() == 0)
		return;
	
	g_Renderer.m_Camera.m_Orientation.GetInverse(worldToCam);
	
	for(std::vector<SObject>::iterator it = m->objects.begin(); it != m->objects.end(); ++it)
	{
		TModel* tmdl = it->m_Model;
		CModel* model = tmdl->GetModel();
	
		debug_assert(model->GetRenderData() == tmdl);
		
		if (tmdl->m_UpdateFlags & RENDERDATA_UPDATE_VERTICES)
		{
			CModelDefPtr mdef = model->GetModelDef();
			size_t numVertices = mdef->GetNumVertices();
	
			// build vertices
			if (m->normals.size() < numVertices)
				m->normals.resize(numVertices);
		
			VertexArrayIterator<CVector3D> Position = tmdl->m_Position.GetIterator<CVector3D>();
			VertexArrayIterator<CVector3D> Normal = VertexArrayIterator<CVector3D>((char*)&m->normals[0], sizeof(CVector3D));
		
			BuildPositionAndNormals(model, Position, Normal);
		
			VertexArrayIterator<SColor4ub> Color = tmdl->m_Color.GetIterator<SColor4ub>();
		
			BuildColor4ub(model, Normal, Color);
	
			// upload everything to vertex buffer
			tmdl->m_Array.Upload();
		}
		tmdl->m_UpdateFlags = 0;
	
		// resort model indices from back to front, according to camera position - and store
		// the returned sqrd distance to the centre of the nearest triangle
		PROFILE_START( "sorting transparent" );
		it->m_Dist = tmdl->BackToFrontIndexSort(worldToCam);
		PROFILE_END( "sorting transparent" );
	}

	PROFILE_START( "sorting transparent" );
	std::sort(m->objects.begin(), m->objects.end(), SortObjectsByDist());
	PROFILE_END( "sorting transparent" );
}


// Render all models in order
void TransparencyRenderer::EndFrame()
{
	m->objects.clear();
}


// Return whether models have been submitted this frame
bool TransparencyRenderer::HaveSubmissions()
{
	return m->objects.size() != 0;
}


// Render submitted models (filtered by flags) using the given modifier
void TransparencyRenderer::Render(RenderModifierPtr modifier, u32 flags)
{
	uint pass = 0;
	
	if (m->objects.size() == 0)
		return;
	
	glEnableClientState(GL_VERTEX_ARRAY);
	
	do
	{
		u32 streamflags = modifier->BeginPass(pass);
		CModelDefPtr lastmdef;
		CTexture* lasttex = 0;
		
		if (streamflags & STREAM_UV0) glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		if (streamflags & STREAM_COLOR) glEnableClientState(GL_COLOR_ARRAY);
		
		for(std::vector<SObject>::iterator it = m->objects.begin(); it != m->objects.end(); ++it)
		{
			if (flags & !(it->m_Model->GetModel()->GetFlags()&flags))
				continue;
			
			TModel* tmdl = it->m_Model;
			CModel* mdl = tmdl->GetModel();
			CModelDefPtr mdef = mdl->GetModelDef();
			CTexture* tex = mdl->GetTexture();
			
			// Prepare per-CModelDef data if changed
			if (mdef != lastmdef)
			{
				TModelDef* tmdef = (TModelDef*)mdef->GetRenderData(m);
				
				if (streamflags & STREAM_UV0)
				{
					u8* base = tmdef->m_Array.Bind();
					GLsizei stride = (GLsizei)tmdef->m_Array.GetStride();
	
					glTexCoordPointer(2, GL_FLOAT, stride, base + tmdef->m_UV.offset);
				}
				
				lastmdef = mdef;
			}
			
			// Prepare necessary RenderModifier stuff
			if (tex != lasttex)
			{
				modifier->PrepareTexture(pass, tex);
				lasttex = tex;
			}
			
			modifier->PrepareModel(pass, mdl);
		
			// Setup per-CModel arrays
			u8* base = tmdl->m_Array.Bind();
			GLsizei stride = (GLsizei)tmdl->m_Array.GetStride();
	
			glVertexPointer(3, GL_FLOAT, stride, base + tmdl->m_Position.offset);
			if (streamflags & STREAM_COLOR)
				glColorPointer(3, tmdl->m_Color.type, stride, base + tmdl->m_Color.offset);	

			// render the lot
			size_t numFaces = mdef->GetNumFaces();
			pglDrawRangeElementsEXT(GL_TRIANGLES, 0, (GLuint)mdef->GetNumVertices(),
					       (GLsizei)numFaces*3, GL_UNSIGNED_SHORT, tmdl->m_Indices);

			// bump stats
			g_Renderer.m_Stats.m_DrawCalls++;
			g_Renderer.m_Stats.m_ModelTris += numFaces;
		}
	
		if (streamflags & STREAM_UV0) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		if (streamflags & STREAM_COLOR) glDisableClientState(GL_COLOR_ARRAY);
	} while(!modifier->EndPass(pass++));
	
	glDisableClientState(GL_VERTEX_ARRAY);
}



///////////////////////////////////////////////////////////////////////////////////////////////////
// TransparentRenderModifier implementation

TransparentRenderModifier::TransparentRenderModifier()
{
}

TransparentRenderModifier::~TransparentRenderModifier()
{
}

u32 TransparentRenderModifier::BeginPass(uint pass)
{
	if (pass == 0)
	{
		// First pass: Put down Z for opaque parts of the model,
		// don't touch the color buffer.
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_CONSTANT);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);

		// just pass through texture's alpha
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

		// Set the proper LOD bias
		glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, g_Renderer.m_Options.m_LodBias);
		
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER,0.975f);

		// render everything with color writes off to setup depth buffer correctly
		glColorMask(0,0,0,0);
		
		return STREAM_POS|STREAM_UV0;
	}
	else
	{
		// Second pass: Put down color, disable Z write
		glColorMask(1,1,1,1);

		glDepthMask(0);

		// setup texture environment to modulate diffuse color with texture color
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

		glAlphaFunc(GL_GREATER,0);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	
		// Set the proper LOD bias
		glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, g_Renderer.m_Options.m_LodBias);
		
		return STREAM_POS|STREAM_COLOR|STREAM_UV0;
	}
}

bool TransparentRenderModifier::EndPass(uint pass)
{
	if (pass == 0)
		return false; // multi-pass
	
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDepthMask(1);
	
	return true;
}

void TransparentRenderModifier::PrepareTexture(uint UNUSED(pass), CTexture* texture)
{
	g_Renderer.SetTexture(0, texture);
}

void TransparentRenderModifier::PrepareModel(uint UNUSED(pass), CModel* UNUSED(model))
{
	// No per-model setup nececssary
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// TransparentShadowRenderModifier implementation

TransparentShadowRenderModifier::TransparentShadowRenderModifier()
{
}

TransparentShadowRenderModifier::~TransparentShadowRenderModifier()
{
}

u32 TransparentShadowRenderModifier::BeginPass(uint pass)
{
	debug_assert(pass == 0);
	
	glDepthMask(0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	// Set the proper LOD bias
	glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, g_Renderer.m_Options.m_LodBias);

	return STREAM_POS|STREAM_UV0;
}

bool TransparentShadowRenderModifier::EndPass(uint UNUSED(pass))
{
	glDepthMask(1);
	glDisable(GL_BLEND);
	
	return true;
}

void TransparentShadowRenderModifier::PrepareTexture(uint UNUSED(pass), CTexture* texture)
{
	g_Renderer.SetTexture(0, texture);
}

void TransparentShadowRenderModifier::PrepareModel(uint UNUSED(pass), CModel* UNUSED(model))
{
	// No per-model setup nececssary
}
