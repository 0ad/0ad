/**
 * =========================================================================
 * File        : InstancingModelRenderer.cpp
 * Project     : Pyrogenesis
 * Description : Implementation of InstancingModelRenderer
 * =========================================================================
 */

#include "precompiled.h"

#include "lib/ogl.h"
#include "lib/res/graphics/ogl_shader.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"

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

	/// If true, primary color will only contain the diffuse term
	bool colorIsDiffuseOnly;

	/// After BeginPass, this points to the instancing matrix interface
	VS_Instancing* instancingConfig;
};


// Construction and Destruction
InstancingModelRenderer::InstancingModelRenderer(bool colorIsDiffuseOnly)
{
	m = new InstancingModelRendererInternals;
	m->imodeldef = 0;
	m->colorIsDiffuseOnly = colorIsDiffuseOnly;
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

	debug_assert(!model->IsSkinned());

	if (!imodeldef)
	{
		imodeldef = new IModelDef(mdef);
		mdef->SetRenderData(m, imodeldef);
	}

	return NULL;
}


void InstancingModelRenderer::UpdateModelData(CModel* UNUSED(model), void* UNUSED(data), int UNUSED(updateflags))
{
	// We have no per-CModel data
}


void InstancingModelRenderer::DestroyModelData(CModel* UNUSED(model), void* UNUSED(data))
{
	// We have no per-CModel data, and per-CModelDef data is deleted by the CModelDef
}


// Setup one rendering pass.
void InstancingModelRenderer::BeginPass(int streamflags, const CMatrix3D* texturematrix)
{
	debug_assert(streamflags == (streamflags & (STREAM_POS|STREAM_UV0|STREAM_COLOR|STREAM_TEXGENTOUV1)));

	RenderPathVertexShader* rpvs = g_Renderer.m_VertexShader;

	glEnableClientState(GL_VERTEX_ARRAY);

	if (streamflags & STREAM_UV0) glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	if (streamflags & STREAM_COLOR)
	{
		const CLightEnv& lightEnv = g_Renderer.GetLightEnv();
		VS_GlobalLight* lightConfig;

		if (streamflags & STREAM_TEXGENTOUV1)
		{
			ogl_program_use(rpvs->m_InstancingLightP);
			lightConfig = &rpvs->m_InstancingLightP_Light;
			m->instancingConfig = &rpvs->m_InstancingLightP_Instancing;

			rpvs->m_InstancingLightP_PosToUV1.SetMatrix(*texturematrix);
		}
		else
		{
			ogl_program_use(rpvs->m_InstancingLight);
			lightConfig = &rpvs->m_InstancingLight_Light;
			m->instancingConfig = &rpvs->m_InstancingLight_Instancing;
		}

		if (m->colorIsDiffuseOnly)
			lightConfig->SetAmbient(RGBColor(0,0,0));
		else
			lightConfig->SetAmbient(lightEnv.m_UnitsAmbientColor);
		lightConfig->SetSunDir(lightEnv.GetSunDir());
		lightConfig->SetSunColor(lightEnv.m_SunColor);

		glEnableClientState(GL_NORMAL_ARRAY);
	}
	else
	{
		if (streamflags & STREAM_TEXGENTOUV1)
		{
			ogl_program_use(rpvs->m_InstancingP);
			m->instancingConfig = &rpvs->m_InstancingP_Instancing;

			rpvs->m_InstancingP_PosToUV1.SetMatrix(*texturematrix);
		}
		else
		{
			ogl_program_use(rpvs->m_Instancing);
			m->instancingConfig = &rpvs->m_Instancing_Instancing;
		}
	}
}

// Cleanup rendering pass.
void InstancingModelRenderer::EndPass(int streamflags)
{
	if (streamflags & STREAM_UV0) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if (streamflags & STREAM_COLOR) glDisableClientState(GL_NORMAL_ARRAY);

	pglUseProgramObjectARB(0);
	glDisableClientState(GL_VERTEX_ARRAY);
}


// Prepare UV coordinates for this modeldef
void InstancingModelRenderer::PrepareModelDef(int streamflags, CModelDefPtr def)
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
void InstancingModelRenderer::RenderModel(int streamflags, CModel* model, void* UNUSED(data))
{
	CModelDefPtr mdldef = model->GetModelDef();
	const CMatrix3D& mat = model->GetTransform();

	if (streamflags & STREAM_COLOR)
	{
		CColor sc = model->GetShadingColor();
		glColor3f(sc.r, sc.g, sc.b);
	}

	m->instancingConfig->SetMatrix(mat);

	// render the lot
	size_t numFaces = mdldef->GetNumFaces();

	if (!g_Renderer.m_SkipSubmit) {
		pglDrawRangeElementsEXT(GL_TRIANGLES, 0, (GLuint)mdldef->GetNumVertices(),
				(GLsizei)numFaces*3, GL_UNSIGNED_SHORT, m->imodeldef->m_Indices);
	}

	// bump stats
	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_ModelTris += numFaces;
}


