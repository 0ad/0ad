#include <algorithm>
#include "Renderer.h"
#include "TransparencyRenderer.h"
#include "terrain/Model.h"


CTransparencyRenderer g_TransparencyRenderer;


struct SortObjectsByDist {
	typedef CTransparencyRenderer::SObject SortObj;
	
	bool operator()(const SortObj& lhs,const SortObj& rhs) {
		return lhs.m_Dist>rhs.m_Dist? true : false;
	}
};

void CTransparencyRenderer::Render()
{
	// coarsely sort submitted objects in back to front manner
	std::sort(m_Objects.begin(),m_Objects.end(),SortObjectsByDist());

	// switch on wireframe if we need it
	if (g_Renderer.m_ModelRenderMode==WIREFRAME) {
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	} 

	// switch on client states
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// setup texture environment to modulate diffuse color with texture color
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

	// just pass through texture's alpha
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER,0.975f);

	uint i;
	for (i=0;i<m_Objects.size();++i) {
		CModel* model=m_Objects[i].m_Model;
		CModelRData* modeldata=(CModelRData*) model->GetRenderData();
		modeldata->RenderStreams(STREAM_POS|STREAM_COLOR|STREAM_UV0,model->GetTransform(),true);
	}

	glDepthMask(0);
	glAlphaFunc(GL_LEQUAL,0.975f);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	for (i=0;i<m_Objects.size();++i) {
		CModel* model=m_Objects[i].m_Model;
		CModelRData* modeldata=(CModelRData*) model->GetRenderData();
		modeldata->RenderStreams(STREAM_POS|STREAM_COLOR|STREAM_UV0,model->GetTransform(),true);
	}
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDepthMask(1);

	// switch off client states
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	if (g_Renderer.m_ModelRenderMode==WIREFRAME) {
		// switch wireframe off again
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	} else if (g_Renderer.m_ModelRenderMode==EDGED_FACES) {
		// edged faces: need to make a second pass over the data:
		// first switch on wireframe
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
		
		// setup some renderstate ..
		glDepthMask(0);
		g_Renderer.SetTexture(0,0);
		glColor4f(1,1,1,0.75f);
		glLineWidth(1.0f);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

		// .. and some client states
		glEnableClientState(GL_VERTEX_ARRAY);

		// render each model
		for (i=0;i<m_Objects.size();++i) {
			CModel* model=m_Objects[i].m_Model;
			CModelRData* modeldata=(CModelRData*) model->GetRenderData();
			modeldata->RenderStreams(STREAM_POS,model->GetTransform(),true);
		}

		// .. and switch off the client states
		glDisableClientState(GL_VERTEX_ARRAY);

		// .. and restore the renderstates
		glDisable(GL_BLEND);
		glDepthMask(1);

		// restore fill mode, and we're done
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	}

	// all transparent objects rendered; release them
	m_Objects.clear();
}

void CTransparencyRenderer::Add(CModel* model)
{	
	// resize array, get last object in list
	m_Objects.resize(m_Objects.size()+1);
	
	SObject& obj=m_Objects.back();
	obj.m_Model=model;

	// build transform from object to camera space 
	CMatrix3D objToCam,invcam;
	g_Renderer.m_Camera.m_Orientation.GetInverse(objToCam);
	objToCam*=model->GetTransform();

	// resort model indices from back to front, according to camera position - and store
	// the returned sqrd distance to the centre of the nearest triangle
	CModelRData* modeldata=(CModelRData*) model->GetRenderData();
	obj.m_Dist=modeldata->BackToFrontIndexSort(objToCam);
}
 