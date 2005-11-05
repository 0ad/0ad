/**
 * =========================================================================
 * File        : HWLightingModelRenderer.cpp
 * Project     : Pyrogenesis
 * Description : Implementation of HWLightingModelRenderer
 *
 * @author  Nicolai Hähnle <nicolai@wildfiregames.com>
 * =========================================================================
 */

#include "precompiled.h"

#include "ogl.h"
#include "lib/res/graphics/ogl_shader.h"
#include "Vector3D.h"
#include "Vector4D.h"

#include "ps/CLogger.h"

#include "graphics/Color.h"
#include "graphics/Model.h"
#include "graphics/ModelDef.h"

#include "renderer/HWLightingModelRenderer.h"
#include "renderer/Renderer.h"
#include "renderer/RenderModifiers.h"
#include "renderer/RenderPathVertexShader.h"
#include "renderer/SHCoeffs.h"
#include "renderer/VertexArray.h"


#define LOG_CATEGORY "graphics"


///////////////////////////////////////////////////////////////////////////////////////////////
// HWLightingModelRenderer implementation

struct HWLModelDef : public CModelDefRPrivate
{
	/// Indices are the same for all models, so share them
	u16* m_Indices;
	

	HWLModelDef(CModelDefPtr mdef);
	~HWLModelDef() { delete[] m_Indices; }
};


HWLModelDef::HWLModelDef(CModelDefPtr mdef)
{
	m_Indices = new u16[mdef->GetNumFaces()*3];
	ModelRenderer::BuildIndices(mdef, m_Indices);
}


struct HWLModel
{
	/// Dynamic per-CModel vertex array
	VertexArray m_Array;
	
	/// Position and normals are recalculated on CPU every frame
	VertexArray::Attribute m_Position;
	VertexArray::Attribute m_Normal;
	
	/// UV is stored per-CModel in order to avoid space wastage due to alignment
	VertexArray::Attribute m_UV;
	
	HWLModel() : m_Array(true) { }
};


struct HWLightingModelRendererInternals
{
	/// Currently used RenderModifier
	RenderModifierPtr modifier;
	
	/// Previously prepared modeldef
	HWLModelDef* hwlmodeldef;
};


// Construction and Destruction
HWLightingModelRenderer::HWLightingModelRenderer()
{
	m = new HWLightingModelRendererInternals;
	m->hwlmodeldef = 0;
}

HWLightingModelRenderer::~HWLightingModelRenderer()
{
	delete m;
}


// Check hardware support
bool HWLightingModelRenderer::IsAvailable()
{
	return g_Renderer.m_VertexShader != 0;
}


// Build model data (and modeldef data if necessary)
void* HWLightingModelRenderer::CreateModelData(CModel* model)
{
	CModelDefPtr mdef = model->GetModelDef();
	HWLModelDef* hwlmodeldef = (HWLModelDef*)mdef->GetRenderData(m);

	if (!hwlmodeldef)
	{
		hwlmodeldef = new HWLModelDef(mdef);
		mdef->SetRenderData(m, hwlmodeldef);
	}

	// Build the per-model data
	HWLModel* hwlmodel = new HWLModel;

	hwlmodel->m_Position.type = GL_FLOAT;
	hwlmodel->m_Position.elems = 3;
	hwlmodel->m_Array.AddAttribute(&hwlmodel->m_Position);

	hwlmodel->m_UV.type = GL_FLOAT;
	hwlmodel->m_UV.elems = 2;
	hwlmodel->m_Array.AddAttribute(&hwlmodel->m_UV);

	hwlmodel->m_Normal.type = GL_FLOAT;
	hwlmodel->m_Normal.elems = 3;
	hwlmodel->m_Array.AddAttribute(&hwlmodel->m_Normal);
	
	hwlmodel->m_Array.SetNumVertices(mdef->GetNumVertices());
	hwlmodel->m_Array.Layout();

	// Fill in static UV coordinates
	VertexArrayIterator<float[2]> UVit = hwlmodel->m_UV.GetIterator<float[2]>();
	
	ModelRenderer::BuildUV(mdef, UVit);

	return hwlmodel;
}


// Fill in and upload dynamic vertex array
void HWLightingModelRenderer::UpdateModelData(CModel* model, void* data, u32 updateflags)
{
	HWLModel* hwlmodel = (HWLModel*)data;
	
	if (updateflags & RENDERDATA_UPDATE_VERTICES)
	{
		CModelDefPtr mdef = model->GetModelDef();
	
		// build vertices
		VertexArrayIterator<CVector3D> Position = hwlmodel->m_Position.GetIterator<CVector3D>();
		VertexArrayIterator<CVector3D> Normal = hwlmodel->m_Normal.GetIterator<CVector3D>();
		
		ModelRenderer::BuildPositionAndNormals(model, Position, Normal);
	
		// upload everything to vertex buffer
		hwlmodel->m_Array.Upload();
	}
}


// Cleanup per-model data.
// Note that per-CModelDef data is deleted by the CModelDef itself.
void HWLightingModelRenderer::DestroyModelData(CModel* UNUSED(model), void* data)
{
	HWLModel* hwlmodel = (HWLModel*)data;
	
	delete hwlmodel;
}


// Setup one rendering pass
void HWLightingModelRenderer::BeginPass(uint streamflags)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	
	if (streamflags & STREAM_UV0) glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	if (streamflags & STREAM_COLOR)
	{
		const RGBColor* coeffs = g_Renderer.m_SHCoeffsUnits.GetCoefficients();
		int idx;
		
		ogl_program_use(g_Renderer.m_VertexShader->m_ModelLight);
		idx = g_Renderer.m_VertexShader->m_ModelLight_SHCoefficients;
		pglUniform3fvARB(idx, 9, (float*)coeffs);

		glEnableClientState(GL_NORMAL_ARRAY);
	}
}


// Cleanup one rendering pass
void HWLightingModelRenderer::EndPass(uint streamflags)
{
	if (streamflags & STREAM_UV0) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if (streamflags & STREAM_COLOR)
	{
		pglUseProgramObjectARB(0);

		glDisableClientState(GL_NORMAL_ARRAY);
	}
	
	glDisableClientState(GL_VERTEX_ARRAY);
}


// Prepare UV coordinates for this modeldef
void HWLightingModelRenderer::PrepareModelDef(uint UNUSED(streamflags), CModelDefPtr def)
{
	m->hwlmodeldef = (HWLModelDef*)def->GetRenderData(m);
	
	debug_assert(m->hwlmodeldef);
}


// Render one model
void HWLightingModelRenderer::RenderModel(uint streamflags, CModel* model, void* data)
{
	CModelDefPtr mdldef = model->GetModelDef();
	HWLModel* hwlmodel = (HWLModel*)data;
	
	u8* base = hwlmodel->m_Array.Bind();
	GLsizei stride = (GLsizei)hwlmodel->m_Array.GetStride();
	
	glVertexPointer(3, GL_FLOAT, stride, base + hwlmodel->m_Position.offset);
	if (streamflags & STREAM_COLOR)
	{
		CColor sc = model->GetShadingColor();
		glColor3f(sc.r, sc.g, sc.b);
		
		glNormalPointer(GL_FLOAT, stride, base + hwlmodel->m_Normal.offset);
	}
	if (streamflags & STREAM_UV0)
	{
		glTexCoordPointer(2, GL_FLOAT, stride, base + hwlmodel->m_UV.offset);
	}

	// render the lot
	size_t numFaces = mdldef->GetNumFaces();
	pglDrawRangeElementsEXT(GL_TRIANGLES, 0, (GLuint)mdldef->GetNumVertices(),
			       (GLsizei)numFaces*3, GL_UNSIGNED_SHORT, m->hwlmodeldef->m_Indices);

	// bump stats
	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_ModelTris += numFaces;
}

