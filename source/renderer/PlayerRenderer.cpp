/***************************************************************************************
	AUTHOR:			John M. Mena
	EMAIL:			JohnMMena@hotmail.com
	FILE:			PlayerRenderer.cpp
	CREATED:		1/23/05
	COMPLETED:		NULL

	DESCRIPTION:	Handles rendering all of the player objects.
					The structure and overall design was inherited from Rich Cross' Transparency Renderer.
****************************************************************************************/

#include "precompiled.h"

#include <algorithm>
#include "Renderer.h"
#include "PlayerRenderer.h"
#include "Model.h"
#include "Game.h"

CPlayerRenderer g_PlayerRenderer;

///////////////////////////////////////////////////////////////////////////////////////////////////
// SetupColorRenderStates: setup the render states for the player color pass.
void CPlayerRenderer::SetupColorRenderStates()
{
	// Set up second pass: first texture unit carries on doing texture*lighting,
	// but passes alpha through inverted; the second texture unit modulates
	// with the player colour.

	glActiveTexture(GL_TEXTURE0);

	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_ONE_MINUS_SRC_ALPHA);

	glActiveTexture(GL_TEXTURE1);

	glEnable(GL_TEXTURE_2D);

	// t1 = t0 * playercolor
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

	// Continue passing through alpha from texture
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Render: render all deferred passes; call Sort before using to ensure passes
// are drawn in correct order
void CPlayerRenderer::Render()
{
	if (m_Objects.size()==0) return;

	// switch on wireframe if we need it
	if (g_Renderer.m_ModelRenderMode==WIREFRAME) {
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	} 


	// set up texture environment for base pass - modulate texture and primary color
	glActiveTexture(GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

	// Set the proper LOD bias
	glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS,  g_Renderer.m_Options.m_LodBias);

	// Render two passes: first, render the unit as normal. Second,
	// render it again but modulated with the player-colour, using
	// the alpha channel as a mask.
	// EDIT: The second pass resides in SetupColorRenderStates() [John M. Mena]
	//
	// This really ought to be done in a single pass on hardware that
	// supports register combiners / fragment programs / etc (since it
	// would only need a single pass and no blending)

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	
	// First pass should ignore the alpha channel and render the whole model
	glDisable(GL_ALPHA_TEST);

	RenderObjectsStreams(STREAM_POS|STREAM_COLOR|STREAM_UV0);

	// Render the second pass:

	// Second pass uses the alpha channel to blend the coloured model
	// with the first pass's solid model
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0);

	RenderObjectsStreams(STREAM_POS|STREAM_COLOR|STREAM_UV0, true);


	// Restore states
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);

	glActiveTexture(GL_TEXTURE0);

	// switch off client states
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

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

void CPlayerRenderer::Clear()
{
	// all transparent objects rendered; release them
	m_Objects.clear();
}

void CPlayerRenderer::Add(CModel* model)
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

//TODO: Correctly implement shadows for the players
void CPlayerRenderer::RenderShadows()
{
	if (m_Objects.size()==0) return;

	RenderObjectsStreams(STREAM_POS|STREAM_UV0, false, MODELFLAG_CASTSHADOWS);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// RenderObjectsStreams: render given streams on all objects
void CPlayerRenderer::RenderObjectsStreams(u32 streamflags, bool iscolorpass, u32 mflags)
{
	for (uint i=0;i<m_Objects.size();++i) {
		if (!mflags || (m_Objects[i].m_Model->GetFlags() & mflags)) {
			CModelRData* modeldata=(CModelRData*) m_Objects[i].m_Model->GetRenderData();

			// Setup the render states to apply the second texture ( i.e. player color )
			if (iscolorpass)
			{
				// I, John Mena, don't think that both passes need a color applied.
				// If I am wrong, then just move everything except for the 
				// SetupColorRenderStates() below this if statement.

				// Get the models player ID
				PS_uint playerid = m_Objects[i].m_Model->GetPlayerID();

				// Get the player color
				const SPlayerColour& colour = g_Game->GetPlayer( playerid )->GetColour();
				float color[] = { colour.r, colour.g, colour.b, colour.a };

				// Just like it says, Sets up the player color render states
				SetupColorRenderStates();

				// Set the texture environment color the player color
				glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);
			}

			// Render the model
			modeldata->RenderStreams(streamflags, iscolorpass);
		}
	}
}
