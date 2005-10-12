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
#include "Profile.h"

#include "ps/CLogger.h"

#define LOG_CATEGORY "graphics"

CPlayerRenderer g_PlayerRenderer;


///////////////////////////////////////////////////////////////////////////////////////////////////
// RenderFast: Use single-pass, three-TMU rendering technique
void CPlayerRenderer::RenderFast()
{
	// Nice player color uses a single pass with three texture environments
	// Note: This uses ARB_texture_env_crossbar (which is checked in GameSetup)
	//
	// We calculate: Result = Color*Texture*(PlayerColor*(1-Texture.a) + 1.0*Texture.a)
	// Algebra gives us:
	// Result = (1 - ((1 - PlayerColor) * (1 - Texture.a)))*Texture*Color
	
	// TexEnv #0
	glActiveTextureARB(GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_ONE_MINUS_SRC_COLOR);
	
	// Don't care about alpha; set it to something harmless
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	
	// TexEnv #1
	glActiveTextureARB(GL_TEXTURE0+1);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_ONE_MINUS_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

	// Don't care about alpha; set it to something harmless
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	
	// TexEnv #2
	glActiveTextureARB(GL_TEXTURE0+2);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

	// Don't care about alpha; set it to something harmless
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	
	glActiveTextureARB(GL_TEXTURE0);

	// Render it!
	RenderObjectsStreams(STREAM_POS|STREAM_COLOR|STREAM_UV0, true);

	// Restore state
	glActiveTextureARB(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glActiveTextureARB(GL_TEXTURE2);
	glDisable(GL_TEXTURE_2D);
	glActiveTextureARB(GL_TEXTURE0);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// RenderSlow: Use multi-pass rendering technique, using two TMUs
void CPlayerRenderer::RenderSlow()
{
	// We calculate: Result = (Color*Texture)*Texture.a + (Color*Texture*PlayerColor)*(1-Texture.a)
	// Modulation is done via texture environments, the final interpolation is done via blending
	
	// FIRST PASS
	
	// TexEnv #0
	glActiveTextureARB(GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
	
	// Don't care about alpha; set it to something harmless
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	
	// Render it!
	RenderObjectsStreams(STREAM_POS|STREAM_COLOR|STREAM_UV0, false);
	
	
	// SECOND PASS
	
	// TexEnv #0
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
	
	// Alpha = Opacity of non-player colored layer
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	
	// TexEnv #1
	glActiveTextureARB(GL_TEXTURE1);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

	// Pass alpha unchanged
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	glActiveTextureARB(GL_TEXTURE0);
	
	// Setup blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_LESS, 1.0);
	glDepthMask(0);

	// Render it!
	RenderObjectsStreams(STREAM_POS|STREAM_COLOR|STREAM_UV0, true);

	// Restore state
	glActiveTextureARB(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glActiveTextureARB(GL_TEXTURE0);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDepthMask(1);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Render: render all deferred passes; call Sort before using to ensure passes
// are drawn in correct order
void CPlayerRenderer::Render()
{
	PROFILE( "render player models" );

	if (m_Objects.size()==0) return;

	if (g_Renderer.m_FastPlayerColor && ogl_max_tex_units < 3)
	{
		g_Renderer.m_FastPlayerColor = false;
		LOG(WARNING, LOG_CATEGORY, "Using fallback player color rendering (not enough TMUs).");
	}
	
	// switch on wireframe if we need it
	if (g_Renderer.m_ModelRenderMode==WIREFRAME) {
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	}

	CModelRData::SetupRender(STREAM_POS|STREAM_COLOR|STREAM_UV0);

	if (g_Renderer.m_FastPlayerColor)
	{
		RenderFast();
	}
	else
	{
		RenderSlow();
	}
	
	CModelRData::FinishRender(STREAM_POS|STREAM_COLOR|STREAM_UV0);

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

		// render each model
		CModelRData::SetupRender(STREAM_POS);
		RenderObjectsStreams(STREAM_POS);
		CModelRData::FinishRender(STREAM_POS);

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
	m_Objects.push_back(model);
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
	int tmus = 1;
	
	if (iscolorpass)
	{
		if (g_Renderer.m_FastPlayerColor)
			tmus = 3;
		else
			tmus = 2;
	}
	
	for (uint i=0;i<m_Objects.size();++i) {
		if (!mflags || (m_Objects[i]->GetFlags() & mflags)) {
			CModelRData* modeldata=(CModelRData*) m_Objects[i]->GetRenderData();

			// Pass the player color as a TexEnv constant when applicable
			if (iscolorpass)
			{
				// Get the player color
				SMaterialColor colour = m_Objects[i]->GetMaterial().GetPlayerColor();
				float* color = &colour.r; // because it's stored RGBA

				// Set the texture environment color the player color
				glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);
			}

			// Render the model
			modeldata->RenderStreams(streamflags, tmus);
		}
	}
}
