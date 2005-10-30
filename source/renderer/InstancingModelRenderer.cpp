/**
 * =========================================================================
 * File        : InstancingModelRenderer.cpp
 * Project     : Pyrogenesis
 * Description : Implementation of InstancingModelRenderer
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

#include "renderer/InstancingModelRenderer.h"
#include "renderer/Renderer.h"
#include "renderer/RenderModifiers.h"
#include "renderer/RenderPathVertexShader.h"
#include "renderer/SHCoeffs.h"
#include "renderer/VertexArray.h"


#define LOG_CATEGORY "graphics"


///////////////////////////////////////////////////////////////////////////////////////////////
// InstancingModelRenderer implementation

struct IModelDef : public CModelDefRPrivate
{
	/// Static per-CModel vertex array
	VertexArray m_Array;
	
	/// Position, normals and UV are all static
	VertexArray::Attribute m_Position;
	VertexArray::Attribute m_Normal;
	VertexArray::Attribute m_UV;
	
	/// Indices are the same for all models, so share them
	u16* m_Indices;
	

	IModelDef(CModelDefPtr mdef);
	~IModelDef() { delete m_Indices; }
};


IModelDef::IModelDef(CModelDefPtr mdef)
	: m_Array(false)
{
	size_t numVertices = mdef->GetNumVertices();
	
	m_Position.type = GL_FLOAT;
	m_Position.elems = 3;
	m_Array.AddAttribute(&m_Position);
	
	m_Normal.type = GL_FLOAT;
	m_Normal.elems = 3;
	m_Array.AddAttribute(&m_Normal);
	
	m_UV.type = GL_FLOAT;
	m_UV.elems = 2;
	m_Array.AddAttribute(&m_UV);
	
	m_Array.SetNumVertices(numVertices);
	m_Array.Layout();

	VertexArrayIterator<CVector3D> Position = m_Position.GetIterator<CVector3D>();
	VertexArrayIterator<CVector3D> Normal = m_Normal.GetIterator<CVector3D>();
	VertexArrayIterator<float[2]> UVit = m_UV.GetIterator<float[2]>();
	
	ModelRenderer::CopyPositionAndNormals(mdef, Position, Normal);
	ModelRenderer::BuildUV(mdef, UVit);
	
	m_Array.Upload();
	m_Array.FreeBackingStore();
	
	m_Indices = new u16[mdef->GetNumFaces()*3];
	ModelRenderer::BuildIndices(mdef, m_Indices);
}


struct InstancingModelRendererInternals
{
	/// Currently used RenderModifier
	RenderModifierPtr modifier;
	
	/// Current rendering pass
	uint pass;
	
	/// Streamflags required in this pass
	uint streamflags;
	
	/// Previously prepared modeldef
	IModelDef* imodeldef;
};


// Construction and Destruction
InstancingModelRenderer::InstancingModelRenderer()
{
	m = new InstancingModelRendererInternals;
}

InstancingModelRenderer::~InstancingModelRenderer()
{
	delete m;
}


// Check hardware support
bool InstancingModelRenderer::IsAvailable()
{
	return g_Renderer.m_VertexShader != 0;
}


// Render submitted models.
void InstancingModelRenderer::Render(RenderModifierPtr modifier, u32 flags)
{
	if (!HaveSubmissions())
		return;
	
	// Save for later
	m->modifier = modifier;
	
	glEnableClientState(GL_VERTEX_ARRAY);
	
	m->pass = 0;
	do
	{
		m->streamflags = modifier->BeginPass(m->pass);
		
		if (m->streamflags & STREAM_UV0) glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		if (m->streamflags & STREAM_COLOR)
		{
			const RGBColor* coeffs = g_Renderer.m_SHCoeffsUnits.GetCoefficients();
			int idx;
			
			ogl_program_use(g_Renderer.m_VertexShader->m_InstancingLight);
			idx = g_Renderer.m_VertexShader->m_InstancingLight_SHCoefficients;
			pglUniform3fvARB(idx, 9, (float*)coeffs);

			glEnableClientState(GL_NORMAL_ARRAY);
		}
		else
		{
			ogl_program_use(g_Renderer.m_VertexShader->m_Instancing);
		}
		
		RenderAllModels(flags);

		if (m->streamflags & STREAM_UV0) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		if (m->streamflags & STREAM_COLOR) glDisableClientState(GL_NORMAL_ARRAY);
	
		pglUseProgramObjectARB(0);

	} while(!modifier->EndPass(m->pass++));
	
	glDisableClientState(GL_VERTEX_ARRAY);
}


// Build modeldef data if necessary - we have no per-CModel data
void* InstancingModelRenderer::CreateModelData(CModel* model)
{
	CModelDefPtr mdef = model->GetModelDef();
	IModelDef* imodeldef = (IModelDef*)mdef->GetRenderData(m);

	debug_assert(!model->GetBoneMatrices());
	
	if (!imodeldef)
	{
		imodeldef = new IModelDef(mdef);
		mdef->SetRenderData(m, imodeldef);
	}
}


void InstancingModelRenderer::UpdateModelData(CModel* model, void* data, u32 updateflags)
{
	// We have no per-CModel data
}


void InstancingModelRenderer::DestroyModelData(CModel* model, void* data)
{
	// We have no per-CModel data, and per-CModelDef data is deleted by the CModelDef
}


// Prepare UV coordinates for this modeldef
void InstancingModelRenderer::PrepareModelDef(CModelDefPtr def)
{
	m->imodeldef = (IModelDef*)def->GetRenderData(m);
	
	debug_assert(m->imodeldef);

	u8* base = m->imodeldef->m_Array.Bind();
	GLsizei stride = (GLsizei)m->imodeldef->m_Array.GetStride();
	
	glVertexPointer(3, GL_FLOAT, stride, base + m->imodeldef->m_Position.offset);
	if (m->streamflags & STREAM_COLOR)
	{
		glNormalPointer(GL_FLOAT, stride, base + m->imodeldef->m_Normal.offset);
	}
	if (m->streamflags & STREAM_UV0)
	{
		glTexCoordPointer(2, GL_FLOAT, stride, base + m->imodeldef->m_UV.offset);
	}
}


// Call the modifier to prepare the given texture
void InstancingModelRenderer::PrepareTexture(CTexture* texture)
{
	m->modifier->PrepareTexture(m->pass, texture);
}


// Render one model
void InstancingModelRenderer::RenderModel(CModel* model, void* data)
{
	m->modifier->PrepareModel(m->pass, model);
	
	CModelDefPtr mdldef = model->GetModelDef();
	const CMatrix3D& mat = model->GetTransform();
	RenderPathVertexShader* rpvs = g_Renderer.m_VertexShader;
	
	if (m->streamflags & STREAM_COLOR)
	{
		CColor sc = model->GetShadingColor();
		glColor3f(sc.r, sc.g, sc.b);
		
		pglVertexAttrib4fARB(rpvs->m_InstancingLight_Instancing1, mat._11, mat._12, mat._13, mat._14);
		pglVertexAttrib4fARB(rpvs->m_InstancingLight_Instancing2, mat._21, mat._22, mat._23, mat._24);
		pglVertexAttrib4fARB(rpvs->m_InstancingLight_Instancing3, mat._31, mat._32, mat._33, mat._34);
	}
	else
	{
		pglVertexAttrib4fARB(rpvs->m_Instancing_Instancing1, mat._11, mat._12, mat._13, mat._14);
		pglVertexAttrib4fARB(rpvs->m_Instancing_Instancing2, mat._21, mat._22, mat._23, mat._24);
		pglVertexAttrib4fARB(rpvs->m_Instancing_Instancing3, mat._31, mat._32, mat._33, mat._34);
	}

	// render the lot
	size_t numFaces = mdldef->GetNumFaces();
	pglDrawRangeElementsEXT(GL_TRIANGLES, 0, mdldef->GetNumVertices(),
			numFaces*3, GL_UNSIGNED_SHORT, m->imodeldef->m_Indices);

	// bump stats
	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_ModelTris += numFaces;
}

