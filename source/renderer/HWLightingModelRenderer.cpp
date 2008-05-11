/**
 * =========================================================================
 * File        : HWLightingModelRenderer.cpp
 * Project     : Pyrogenesis
 * Description : Implementation of HWLightingModelRenderer
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

#include "renderer/HWLightingModelRenderer.h"
#include "renderer/Renderer.h"
#include "renderer/RenderModifiers.h"
#include "renderer/RenderPathVertexShader.h"
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

	/// If true, primary color will only contain the diffuse term
	bool colorIsDiffuseOnly;
};


// Construction and Destruction
HWLightingModelRenderer::HWLightingModelRenderer(bool colorIsDiffuseOnly)
{
	m = new HWLightingModelRendererInternals;
	m->hwlmodeldef = 0;
	m->colorIsDiffuseOnly = colorIsDiffuseOnly;
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
void HWLightingModelRenderer::UpdateModelData(CModel* model, void* data, int updateflags)
{
	HWLModel* hwlmodel = (HWLModel*)data;

	if (updateflags & RENDERDATA_UPDATE_VERTICES)
	{
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
void HWLightingModelRenderer::BeginPass(int streamflags, const CMatrix3D* texturematrix)
{
	debug_assert(streamflags == (streamflags & (STREAM_POS|STREAM_UV0|STREAM_COLOR|STREAM_TEXGENTOUV1)));

	glEnableClientState(GL_VERTEX_ARRAY);

	if (streamflags & STREAM_UV0) glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	if (streamflags & STREAM_COLOR)
	{
		RenderPathVertexShader* rpvs = g_Renderer.m_VertexShader;
		const CLightEnv& lightEnv = g_Renderer.GetLightEnv();
		VS_GlobalLight* lightConfig;

		if (streamflags & STREAM_TEXGENTOUV1)
		{
			ogl_program_use(rpvs->m_ModelLightP);
			lightConfig = &rpvs->m_ModelLightP_Light;

			rpvs->m_ModelLightP_PosToUV1.SetMatrix(*texturematrix);
		}
		else
		{
			ogl_program_use(rpvs->m_ModelLight);
			lightConfig = &rpvs->m_ModelLight_Light;
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
			pglActiveTextureARB(GL_TEXTURE1);

			float tmp[4];

			glEnable(GL_TEXTURE_GEN_S);
			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			tmp[0] = texturematrix->_11;
			tmp[1] = texturematrix->_12;
			tmp[2] = texturematrix->_13;
			tmp[3] = texturematrix->_14;
			glTexGenfv(GL_S, GL_OBJECT_PLANE, tmp);

			glEnable(GL_TEXTURE_GEN_T);
			glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			tmp[0] = texturematrix->_21;
			tmp[1] = texturematrix->_22;
			tmp[2] = texturematrix->_23;
			tmp[3] = texturematrix->_24;
			glTexGenfv(GL_T, GL_OBJECT_PLANE, tmp);

			glEnable(GL_TEXTURE_GEN_R);
			glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			tmp[0] = texturematrix->_31;
			tmp[1] = texturematrix->_32;
			tmp[2] = texturematrix->_33;
			tmp[3] = texturematrix->_34;
			glTexGenfv(GL_R, GL_OBJECT_PLANE, tmp);

			pglActiveTextureARB(GL_TEXTURE0);
		}
	}
}


// Cleanup one rendering pass
void HWLightingModelRenderer::EndPass(int streamflags)
{
	if (streamflags & STREAM_UV0) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if (streamflags & STREAM_COLOR)
	{
		pglUseProgramObjectARB(0);

		glDisableClientState(GL_NORMAL_ARRAY);
	}
	else
	{
		if (streamflags & STREAM_TEXGENTOUV1)
		{
			pglActiveTextureARB(GL_TEXTURE1);

			glDisable(GL_TEXTURE_GEN_S);
			glDisable(GL_TEXTURE_GEN_T);
			glDisable(GL_TEXTURE_GEN_R);

			pglActiveTextureARB(GL_TEXTURE0);
		}
	}

	glDisableClientState(GL_VERTEX_ARRAY);
}


// Prepare UV coordinates for this modeldef
void HWLightingModelRenderer::PrepareModelDef(int UNUSED(streamflags), CModelDefPtr def)
{
	m->hwlmodeldef = (HWLModelDef*)def->GetRenderData(m);

	debug_assert(m->hwlmodeldef);
}


// Render one model
void HWLightingModelRenderer::RenderModel(int streamflags, CModel* model, void* data)
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

	if (!g_Renderer.m_SkipSubmit) {
		pglDrawRangeElementsEXT(GL_TRIANGLES, 0, (GLuint)mdldef->GetNumVertices(),
					   (GLsizei)numFaces*3, GL_UNSIGNED_SHORT, m->hwlmodeldef->m_Indices);
	}

	// bump stats
	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_ModelTris += numFaces;
}


