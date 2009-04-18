/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * ModelRenderer implementation that sorts models and/or polygons based
 * on distance from viewer, for transparency rendering.
 */

#include "precompiled.h"

#include <algorithm>
#include <vector>

#include "lib/ogl.h"
#include "maths/MathUtil.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"

#include "graphics/LightEnv.h"
#include "graphics/Model.h"
#include "graphics/ModelDef.h"

#include "ps/Profile.h"

#include "renderer/Renderer.h"
#include "renderer/ShadowMap.h"
#include "renderer/TransparencyRenderer.h"
#include "renderer/VertexArray.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
// PolygonSortModelRenderer implementation


/**
 * Struct PSModelDef: Per-CModelDef data for the polygon sort vertex renderer
 */
struct PSModelDef : public CModelDefRPrivate
{
	PSModelDef(const CModelDefPtr& mdef);

	/// Static vertex array
	VertexArray m_Array;

	/// UV is static
	VertexArray::Attribute m_UV;
};

PSModelDef::PSModelDef(const CModelDefPtr& mdef)
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
 * Struct PSModel: Per-CModel data for the polygon sorting renderer
 */
struct PSModel
{
	PSModel(CModel* model);
	~PSModel();

	/**
	 * BackToFrontIndexSort: Sort polygons by distance to camera for
	 * transparency rendering and fill the indices array appropriately.
	 *
	 * @param worldToCam World to camera coordinate space transform
	 *
	 * @return Square of the estimated distance to the nearest triangle.
	 */
	float BackToFrontIndexSort(const CMatrix3D& objToCam);

	/// Back-link to the model
	CModel* m_Model;

	/// Dynamic per-CModel vertex array
	VertexArray m_Array;

	/// Position and lighting are recalculated on CPU every frame
	VertexArray::Attribute m_Position;
	VertexArray::Attribute m_Color;

	/// Indices array (sorted on CPU based on distance to camera)
	u16* m_Indices;
};

PSModel::PSModel(CModel* model)
	: m_Model(model), m_Array(true)
{
	CModelDefPtr mdef = m_Model->GetModelDef();

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

PSModel::~PSModel()
{
	delete[] m_Indices;
}


typedef std::pair<int,float> IntFloatPair;

struct SortFacesByDist {
	bool operator()(const IntFloatPair& lhs,const IntFloatPair& rhs) {
		return lhs.second>rhs.second ? true : false;
	}
};

float PSModel::BackToFrontIndexSort(const CMatrix3D& worldToCam)
{
	static std::vector<IntFloatPair> IndexSorter;

	CModelDefPtr mdef = m_Model->GetModelDef();
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
	size_t idxidx = 0;
	for (size_t i = 0; i < numFaces; ++i) {
		const SModelFace& face = faces[IndexSorter[i].first];
		m_Indices[idxidx++] = (u16)(face.m_Verts[0]);
		m_Indices[idxidx++] = (u16)(face.m_Verts[1]);
		m_Indices[idxidx++] = (u16)(face.m_Verts[2]);
	}

	return IndexSorter[0].second;
}


/**
 * Struct PolygonSortModelRendererInternals: Internal data structure of
 * PolygonSortModelRenderer
 */
struct PolygonSortModelRendererInternals
{
	/// Scratch space for normal vector calculation
	std::vector<CVector3D> normals;
};


// Construction / Destruction
PolygonSortModelRenderer::PolygonSortModelRenderer()
{
	m = new PolygonSortModelRendererInternals;
}

PolygonSortModelRenderer::~PolygonSortModelRenderer()
{
	delete m;
}

// Create per-CModel data for the model (and per-CModelDef data if necessary)
void* PolygonSortModelRenderer::CreateModelData(CModel* model)
{
	CModelDefPtr mdef = model->GetModelDef();
	PSModelDef* psmdef = (PSModelDef*)mdef->GetRenderData(m);

	if (!psmdef)
	{
		psmdef = new PSModelDef(mdef);
		mdef->SetRenderData(m, psmdef);
	}

	return new PSModel(model);
}


// Updated transforms
void PolygonSortModelRenderer::UpdateModelData(CModel* model, void* data, int updateflags)
{
	PSModel* psmdl = (PSModel*)data;

	if (updateflags & (RENDERDATA_UPDATE_VERTICES|RENDERDATA_UPDATE_COLOR))
	{
		CModelDefPtr mdef = model->GetModelDef();
		size_t numVertices = mdef->GetNumVertices();

		// build vertices
		if (m->normals.size() < numVertices)
			m->normals.resize(numVertices);

		VertexArrayIterator<CVector3D> Position = psmdl->m_Position.GetIterator<CVector3D>();
		VertexArrayIterator<CVector3D> Normal = VertexArrayIterator<CVector3D>((char*)&m->normals[0], sizeof(CVector3D));

		ModelRenderer::BuildPositionAndNormals(model, Position, Normal);

		VertexArrayIterator<SColor4ub> Color = psmdl->m_Color.GetIterator<SColor4ub>();

		ModelRenderer::BuildColor4ub(model, Normal, Color, false);

		// upload everything to vertex buffer
		psmdl->m_Array.Upload();
	}

	// resort model indices from back to front, according to the view camera position - and store
	// the returned sqrd distance to the centre of the nearest triangle
	// Use the view camera instead of the cull camera because:
	//  a) polygon sorting implicitly uses the view camera (and changing that would be costly)
	//  b) using the cull camera is likely not interesting from a debugging POV
	PROFILE_START( "sorting transparent" );

	CMatrix3D worldToCam;
	g_Renderer.GetViewCamera().m_Orientation.GetInverse(worldToCam);

	psmdl->BackToFrontIndexSort(worldToCam);
	PROFILE_END( "sorting transparent" );
}


// Cleanup per-CModel data
void PolygonSortModelRenderer::DestroyModelData(CModel* UNUSED(model), void* data)
{
	PSModel* psmdl = (PSModel*)data;

	delete psmdl;
}


// Prepare for one rendering pass
void PolygonSortModelRenderer::BeginPass(int streamflags, const CMatrix3D* texturematrix)
{
	debug_assert(streamflags == (streamflags & (STREAM_POS|STREAM_COLOR|STREAM_UV0|STREAM_TEXGENTOUV1)));

	glEnableClientState(GL_VERTEX_ARRAY);

	if (streamflags & STREAM_UV0) glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	if (streamflags & STREAM_COLOR) glEnableClientState(GL_COLOR_ARRAY);
	if (streamflags & STREAM_TEXGENTOUV1)
	{
		pglActiveTextureARB(GL_TEXTURE1);
		pglClientActiveTextureARB(GL_TEXTURE1);

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		glMatrixMode(GL_TEXTURE);
		glLoadMatrixf(&texturematrix->_11);
		glMatrixMode(GL_MODELVIEW);

		pglActiveTextureARB(GL_TEXTURE0);
		pglClientActiveTextureARB(GL_TEXTURE0);
	}
}


// Cleanup rendering
void PolygonSortModelRenderer::EndPass(int streamflags)
{
	if (streamflags & STREAM_UV0) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if (streamflags & STREAM_COLOR) glDisableClientState(GL_COLOR_ARRAY);
	if (streamflags & STREAM_TEXGENTOUV1)
	{
		pglActiveTextureARB(GL_TEXTURE1);
		pglClientActiveTextureARB(GL_TEXTURE1);

		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);

		pglActiveTextureARB(GL_TEXTURE0);
		pglClientActiveTextureARB(GL_TEXTURE0);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
}


// Prepare for rendering models using this CModelDef
void PolygonSortModelRenderer::PrepareModelDef(int streamflags, const CModelDefPtr& def)
{
	if (streamflags & STREAM_UV0)
	{
		PSModelDef* psmdef = (PSModelDef*)def->GetRenderData(m);

		debug_assert(psmdef);

		u8* base = psmdef->m_Array.Bind();
		GLsizei stride = (GLsizei)psmdef->m_Array.GetStride();

		glTexCoordPointer(2, GL_FLOAT, stride, base + psmdef->m_UV.offset);
	}
}


// Render one model
void PolygonSortModelRenderer::RenderModel(int streamflags, CModel* model, void* data)
{
	CModelDefPtr mdef = model->GetModelDef();
	PSModel* psmdl = (PSModel*)data;

	// Setup per-CModel arrays
	u8* base = psmdl->m_Array.Bind();
	GLsizei stride = (GLsizei)psmdl->m_Array.GetStride();

	glVertexPointer(3, GL_FLOAT, stride, base + psmdl->m_Position.offset);
	if (streamflags & STREAM_COLOR)
		glColorPointer(3, psmdl->m_Color.type, stride, base + psmdl->m_Color.offset);
	if (streamflags & STREAM_TEXGENTOUV1)
	{
		pglClientActiveTextureARB(GL_TEXTURE1);
		pglActiveTextureARB(GL_TEXTURE1);
		glTexCoordPointer(3, GL_FLOAT, stride, base + psmdl->m_Position.offset);
		pglClientActiveTextureARB(GL_TEXTURE0);
		pglActiveTextureARB(GL_TEXTURE0);
	}

	// render the lot
	size_t numFaces = mdef->GetNumFaces();

	if (!g_Renderer.m_SkipSubmit) {
		pglDrawRangeElementsEXT(GL_TRIANGLES, 0, (GLuint)mdef->GetNumVertices(),
					(GLsizei)numFaces*3, GL_UNSIGNED_SHORT, psmdl->m_Indices);
	}

	// bump stats
	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_ModelTris += numFaces;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
// SortModelRenderer implementation

/**
 * Struct SModel: Per-CModel data for the model-sorting renderer
 */
struct SModel : public CModelRData
{
	SModel(SortModelRendererInternals* tri, CModel* model);
	~SModel();

	// Back-link to the Model renderer
	SortModelRendererInternals* m_SMRI;

	// Private data of the ModelVertexRenderer
	void* m_Data;

	// Distance to camera (for sorting)
	float m_Distance;
};


/**
 * Struct SortModelRendererInternals: Internal data structure of SortModelRenderer
 */
struct SortModelRendererInternals
{
	/// Vertex renderer used for transform and lighting
	ModelVertexRendererPtr vertexRenderer;

	/// List of submitted models.
	std::vector<SModel*> models;
};



SModel::SModel(SortModelRendererInternals* smri, CModel* model)
	: CModelRData(smri, model), m_SMRI(smri)
{
	m_Data = m_SMRI->vertexRenderer->CreateModelData(model);
	m_Distance = 0;
}

SModel::~SModel()
{
	m_SMRI->vertexRenderer->DestroyModelData(GetModel(), m_Data);
}



// Construction / Destruction
SortModelRenderer::SortModelRenderer(ModelVertexRendererPtr vertexRenderer)
{
	m = new SortModelRendererInternals;
	m->vertexRenderer = vertexRenderer;
}

SortModelRenderer::~SortModelRenderer()
{
	delete m;
}

// Submit a model: Create, but don't fill in, our own Model and ModelDef structures
void SortModelRenderer::Submit(CModel* model)
{
	CModelRData* rdata = (CModelRData*)model->GetRenderData();
	SModel* smdl;

	if (rdata && rdata->GetKey() == m)
	{
		smdl = (SModel*)rdata;
	}
	else
	{
		smdl = new SModel(m, model);
		rdata = smdl;
		model->SetRenderData(rdata);
		model->SetDirty(~0u);
		g_Renderer.LoadTexture(model->GetTexture(), GL_CLAMP_TO_EDGE);
	}

	m->models.push_back(smdl);
}


// Transform and sort all models
struct SortModelsByDist {
	bool operator()(SModel* lhs, SModel* rhs) {
		// Sort by distance, and break ties by comparing pointers
		return lhs->m_Distance > rhs->m_Distance ? true
			: lhs->m_Distance < rhs->m_Distance ? false
			: (lhs > rhs);
	}
};

void SortModelRenderer::PrepareModels()
{
	CMatrix3D worldToCam;

	if (m->models.size() == 0)
		return;

	g_Renderer.GetViewCamera().m_Orientation.GetInverse(worldToCam);

	for(std::vector<SModel*>::iterator it = m->models.begin(); it != m->models.end(); ++it)
	{
		SModel* smdl = *it;
		CModel* model = smdl->GetModel();

		debug_assert(model->GetRenderData() == smdl);

		m->vertexRenderer->UpdateModelData(model, smdl->m_Data, smdl->m_UpdateFlags);
		smdl->m_UpdateFlags = 0;

		CVector3D modelpos = model->GetTransform().GetTranslation();

		modelpos = worldToCam.Transform(modelpos);

		smdl->m_Distance = modelpos.Z;
	}

	PROFILE_START( "sorting transparent" );
	std::sort(m->models.begin(), m->models.end(), SortModelsByDist());
	PROFILE_END( "sorting transparent" );
}


// Cleanup per-frame model list
void SortModelRenderer::EndFrame()
{
	m->models.clear();
}


// Return whether models have been submitted this frame
bool SortModelRenderer::HaveSubmissions()
{
	return m->models.size() != 0;
}


// Render submitted models (filtered by flags) using the given modifier
void SortModelRenderer::Render(const RenderModifierPtr& modifier, int flags)
{
	int pass = 0;

	if (m->models.size() == 0)
		return;

	do
	{
		int streamflags = modifier->BeginPass(pass);
		const CMatrix3D* texturematrix = 0;
		CModelDefPtr lastmdef;
		CTexture* lasttex = 0;

		if (streamflags & STREAM_TEXGENTOUV1)
			texturematrix = modifier->GetTexGenMatrix(pass);

		m->vertexRenderer->BeginPass(streamflags, texturematrix);

		for(std::vector<SModel*>::iterator it = m->models.begin(); it != m->models.end(); ++it)
		{
			SModel* smdl = *it;
			CModel* mdl = smdl->GetModel();

			if (flags & !(mdl->GetFlags() & flags))
				continue;

			debug_assert(smdl->GetKey() == m);

			CModelDefPtr mdef = mdl->GetModelDef();
			CTexture* tex = mdl->GetTexture();

			// Prepare per-CModelDef data if changed
			if (mdef != lastmdef)
			{
				m->vertexRenderer->PrepareModelDef(streamflags, mdef);
				lastmdef = mdef;
			}

			// Prepare necessary RenderModifier stuff
			if (tex != lasttex)
			{
				modifier->PrepareTexture(pass, tex);
				lasttex = tex;
			}

			modifier->PrepareModel(pass, mdl);

			// Render the model
			m->vertexRenderer->RenderModel(streamflags, mdl, smdl->m_Data);
		}

		m->vertexRenderer->EndPass(streamflags);
	} while(!modifier->EndPass(pass++));
}



///////////////////////////////////////////////////////////////////////////////////////////////////
// TransparentRenderModifier implementation

TransparentRenderModifier::TransparentRenderModifier()
{
}

TransparentRenderModifier::~TransparentRenderModifier()
{
}

int TransparentRenderModifier::BeginPass(int pass)
{
	if (pass == 0)
	{
		// First pass: Put down Z for opaque parts of the model,
		// don't touch the color buffer.

		glDepthMask(1);

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

bool TransparentRenderModifier::EndPass(int pass)
{
	if (pass == 0)
		return false; // multi-pass

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDepthMask(1);

	return true;
}

void TransparentRenderModifier::PrepareTexture(int UNUSED(pass), CTexture* texture)
{
	g_Renderer.SetTexture(0, texture);
}

void TransparentRenderModifier::PrepareModel(int UNUSED(pass), CModel* UNUSED(model))
{
	// No per-model setup necessary
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// LitTransparentRenderModifier implementation

LitTransparentRenderModifier::LitTransparentRenderModifier()
{
}

LitTransparentRenderModifier::~LitTransparentRenderModifier()
{
}

int LitTransparentRenderModifier::BeginPass(int pass)
{
	debug_assert(GetShadowMap() && GetShadowMap()->GetUseDepthTexture());

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

		// Ambient + Diffuse * Shadow
		pglActiveTextureARB(GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE1);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

		pglActiveTextureARB(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, GetShadowMap()->GetTexture());
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &GetLightEnv()->m_UnitsAmbientColor.X);

		// Incoming color is ambient + diffuse light
		pglActiveTextureARB(GL_TEXTURE2);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, GetShadowMap()->GetTexture());
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

		pglActiveTextureARB(GL_TEXTURE0);

		glAlphaFunc(GL_GREATER,0);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

		// Set the proper LOD bias
		glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, g_Renderer.m_Options.m_LodBias);

		return STREAM_POS|STREAM_COLOR|STREAM_UV0|STREAM_TEXGENTOUV1;
	}
}

bool LitTransparentRenderModifier::EndPass(int pass)
{
	if (pass == 0)
		return false; // multi-pass

	pglActiveTextureARB(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	pglActiveTextureARB(GL_TEXTURE2);
	glDisable(GL_TEXTURE_2D);
	pglActiveTextureARB(GL_TEXTURE0);

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDepthMask(1);

	return true;
}

const CMatrix3D* LitTransparentRenderModifier::GetTexGenMatrix(int UNUSED(pass))
{
	return &GetShadowMap()->GetTextureMatrix();
}

void LitTransparentRenderModifier::PrepareTexture(int UNUSED(pass), CTexture* texture)
{
	g_Renderer.SetTexture(0, texture);
}

void LitTransparentRenderModifier::PrepareModel(int UNUSED(pass), CModel* UNUSED(model))
{
	// No per-model setup necessary
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// TransparentShadowRenderModifier implementation

TransparentShadowRenderModifier::TransparentShadowRenderModifier()
{
}

TransparentShadowRenderModifier::~TransparentShadowRenderModifier()
{
}

int TransparentShadowRenderModifier::BeginPass(int pass)
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

bool TransparentShadowRenderModifier::EndPass(int UNUSED(pass))
{
	glDepthMask(1);
	glDisable(GL_BLEND);

	return true;
}

void TransparentShadowRenderModifier::PrepareTexture(int UNUSED(pass), CTexture* texture)
{
	g_Renderer.SetTexture(0, texture);
}

void TransparentShadowRenderModifier::PrepareModel(int UNUSED(pass), CModel* UNUSED(model))
{
	// No per-model setup necessary
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// TransparentDepthShadowModifier implementation

TransparentDepthShadowModifier::TransparentDepthShadowModifier()
{
}

TransparentDepthShadowModifier::~TransparentDepthShadowModifier()
{
}

int TransparentDepthShadowModifier::BeginPass(int pass)
{
	debug_assert(pass == 0);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.4f);

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

bool TransparentDepthShadowModifier::EndPass(int UNUSED(pass))
{
	glDisable(GL_ALPHA_TEST);

	return true;
}

void TransparentDepthShadowModifier::PrepareTexture(int UNUSED(pass), CTexture* texture)
{
	g_Renderer.SetTexture(0, texture);
}

void TransparentDepthShadowModifier::PrepareModel(int UNUSED(pass), CModel* UNUSED(model))
{
	// No per-model setup necessary
}
