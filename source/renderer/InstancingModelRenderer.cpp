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
#include "graphics/LightEnv.h"
#include "graphics/Model.h"
#include "graphics/ModelDef.h"

#include "renderer/InstancingModelRenderer.h"
#include "renderer/Renderer.h"
#include "renderer/RenderModifiers.h"
#include "renderer/RenderPathVertexShader.h"
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
	~IModelDef() { delete[] m_Indices; }
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

	/// Previously prepared modeldef
	IModelDef* imodeldef;
};


// Construction and Destruction
InstancingModelRenderer::InstancingModelRenderer()
{
	m = new InstancingModelRendererInternals;
	m->imodeldef = 0;
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

	return NULL;
}


void InstancingModelRenderer::UpdateModelData(CModel* UNUSED(model), void* UNUSED(data), u32 UNUSED(updateflags))
{
	// We have no per-CModel data
}


void InstancingModelRenderer::DestroyModelData(CModel* UNUSED(model), void* UNUSED(data))
{
	// We have no per-CModel data, and per-CModelDef data is deleted by the CModelDef
}


// Setup one rendering pass.
void InstancingModelRenderer::BeginPass(uint streamflags)
{
	glEnableClientState(GL_VERTEX_ARRAY);

	if (streamflags & STREAM_UV0) glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	if (streamflags & STREAM_COLOR)
	{
		const CLightEnv& lightEnv = g_Renderer.GetLightEnv();
		int idx;

		ogl_program_use(g_Renderer.m_VertexShader->m_InstancingLight);
		idx = g_Renderer.m_VertexShader->m_InstancingLight_Ambient;
		pglUniform3fvARB(idx, 1, &lightEnv.m_UnitsAmbientColor.X);
		idx = g_Renderer.m_VertexShader->m_InstancingLight_SunDir;
		pglUniform3fvARB(idx, 1, &lightEnv.GetSunDir().X);
		idx = g_Renderer.m_VertexShader->m_InstancingLight_SunColor;
		pglUniform3fvARB(idx, 1, &lightEnv.m_SunColor.X);

		glEnableClientState(GL_NORMAL_ARRAY);
	}
	else
	{
		ogl_program_use(g_Renderer.m_VertexShader->m_Instancing);
	}
}

// Cleanup rendering pass.
void InstancingModelRenderer::EndPass(uint streamflags)
{
	if (streamflags & STREAM_UV0) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if (streamflags & STREAM_COLOR) glDisableClientState(GL_NORMAL_ARRAY);

	pglUseProgramObjectARB(0);
	glDisableClientState(GL_VERTEX_ARRAY);
}


// Prepare UV coordinates for this modeldef
void InstancingModelRenderer::PrepareModelDef(uint streamflags, CModelDefPtr def)
{
	m->imodeldef = (IModelDef*)def->GetRenderData(m);

	debug_assert(m->imodeldef);

	u8* base = m->imodeldef->m_Array.Bind();
	GLsizei stride = (GLsizei)m->imodeldef->m_Array.GetStride();

	glVertexPointer(3, GL_FLOAT, stride, base + m->imodeldef->m_Position.offset);
	if (streamflags & STREAM_COLOR)
	{
		glNormalPointer(GL_FLOAT, stride, base + m->imodeldef->m_Normal.offset);
	}
	if (streamflags & STREAM_UV0)
	{
		glTexCoordPointer(2, GL_FLOAT, stride, base + m->imodeldef->m_UV.offset);
	}
}


// Render one model
void InstancingModelRenderer::RenderModel(uint streamflags, CModel* model, void* UNUSED(data))
{
	CModelDefPtr mdldef = model->GetModelDef();
	const CMatrix3D& mat = model->GetTransform();
	RenderPathVertexShader* rpvs = g_Renderer.m_VertexShader;

	if (streamflags & STREAM_COLOR)
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
	pglDrawRangeElementsEXT(GL_TRIANGLES, 0, (GLuint)mdldef->GetNumVertices(),
			(GLsizei)numFaces*3, GL_UNSIGNED_SHORT, m->imodeldef->m_Indices);

	// bump stats
	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_ModelTris += numFaces;
}

