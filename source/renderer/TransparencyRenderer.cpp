///////////////////////////////////////////////////////////////////////////////
//
// Name:		TransparencyRenderer.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#include "precompiled.h"

#include <algorithm>
#include "Renderer.h"
#include "TransparencyRenderer.h"
#include "Model.h"


CTransparencyRenderer g_TransparencyRenderer;


///////////////////////////////////////////////////////////////////////////////////////////////////
// SortObjectsByDist: sorting class used for back-to-front sort of transparent passes
struct SortObjectsByDist {
	typedef CTransparencyRenderer::SObject SortObj;
	
	bool operator()(const SortObj& lhs,const SortObj& rhs) {
		return lhs.m_Dist>rhs.m_Dist? true : false;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Sort: coarsely sort submitted objects in back to front manner
void CTransparencyRenderer::Sort()
{
	std::sort(m_Objects.begin(),m_Objects.end(),SortObjectsByDist());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Render: render all deferred passes; call Sort before using to ensure passes
// are drawn in correct order
void CTransparencyRenderer::Render()
{
	if (m_Objects.size()==0) return;

	// switch on wireframe if we need it
	if (g_Renderer.m_ModelRenderMode==WIREFRAME) {
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	} 

	// switch on client states
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);

	// just pass through texture's alpha
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER,0.975f);

	// render everything with color writes off to setup depth buffer correctly
	glColorMask(0,0,0,0);
	RenderObjectsStreams(STREAM_POS|STREAM_UV0);
	glColorMask(1,1,1,1);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	glDepthMask(0);
	glAlphaFunc(GL_GREATER,0);

	// setup texture environment to modulate diffuse color with texture color
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

	RenderObjectsStreams(STREAM_POS|STREAM_COLOR|STREAM_UV0);

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
		RenderObjectsStreams(STREAM_POS);

		// .. and switch off the client states
		glDisableClientState(GL_VERTEX_ARRAY);

		// .. and restore the renderstates
		glDisable(GL_BLEND);
		glDepthMask(1);

		// restore fill mode, and we're done
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	}
}

void CTransparencyRenderer::Clear()
{
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
 
void CTransparencyRenderer::RenderShadows()
{
	if (m_Objects.size()==0) return;

	// switch on client states
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

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

	RenderObjectsStreams(STREAM_POS|STREAM_UV0,MODELFLAG_CASTSHADOWS);

	glDepthMask(1);
	glDisable(GL_BLEND);

	// switch off client states
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// RenderObjectsStreams: render given streams on all objects
void CTransparencyRenderer::RenderObjectsStreams(u32 streamflags,u32 mflags)
{
	for (uint i=0;i<m_Objects.size();++i) {
		if (!mflags || (m_Objects[i].m_Model->GetFlags() & mflags)) {
			CModelRData* modeldata=(CModelRData*) m_Objects[i].m_Model->GetRenderData();
			modeldata->RenderStreams(streamflags);
		}
	}
}
