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
 * Terrain rendering (everything related to patches and water) is
 * encapsulated in TerrainRenderer
 */

#include "precompiled.h"

#include "graphics/Camera.h"
#include "graphics/LightEnv.h"
#include "graphics/Patch.h"
#include "graphics/Terrain.h"
#include "graphics/GameView.h"

#include "maths/MathUtil.h"

#include "ps/Filesystem.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "ps/Pyrogenesis.h"	// MICROLOG
#include "ps/World.h"

#include "simulation2/Simulation2.h"

#include "renderer/PatchRData.h"
#include "renderer/Renderer.h"
#include "renderer/ShadowMap.h"
#include "renderer/TerrainRenderer.h"
#include "renderer/WaterManager.h"

#include "lib/res/graphics/ogl_shader.h"

#define LOG_CATEGORY L"graphics"


///////////////////////////////////////////////////////////////////////////////////////////////
// TerrainRenderer implementation


/**
 * TerrainRenderer keeps track of which phase it is in, to detect
 * when Submit, PrepareForRendering etc. are called in the wrong order.
 */
enum Phase {
	Phase_Submit,
	Phase_Render
};


/**
 * Struct TerrainRendererInternals: Internal variables used by the TerrainRenderer class.
 */
struct TerrainRendererInternals
{
	/// Which phase (submitting or rendering patches) are we in right now?
	Phase phase;

	/**
	 * VisiblePatches: Patches that were submitted for this frame
	 *
	 * @todo Merge this list with CPatchRData list
	 */
	std::vector<CPatch*> visiblePatches;

	/// Fancy water shader
	Handle fancyWaterShader;
};



///////////////////////////////////////////////////////////////////
// Construction/Destruction
TerrainRenderer::TerrainRenderer()
{
	m = new TerrainRendererInternals();
	m->phase = Phase_Submit;
	m->fancyWaterShader = 0;
}

TerrainRenderer::~TerrainRenderer()
{
	if( m->fancyWaterShader )
	{
		ogl_program_free( m->fancyWaterShader );
	}
	delete m;
}


///////////////////////////////////////////////////////////////////
// Submit a patch for rendering
void TerrainRenderer::Submit(CPatch* patch)
{
	debug_assert(m->phase == Phase_Submit);

	CPatchRData* data=(CPatchRData*) patch->GetRenderData();
	if (data == 0)
	{
		// no renderdata for patch, create it now
		data = new CPatchRData(patch);
		patch->SetRenderData(data);
	}
	data->Update();

	m->visiblePatches.push_back(patch);
}


///////////////////////////////////////////////////////////////////
// Prepare for rendering
void TerrainRenderer::PrepareForRendering()
{
	debug_assert(m->phase == Phase_Submit);

	m->phase = Phase_Render;
}

///////////////////////////////////////////////////////////////////
// Clear submissions lists
void TerrainRenderer::EndFrame()
{
	debug_assert(m->phase == Phase_Render);

	m->visiblePatches.clear();

	m->phase = Phase_Submit;
}


///////////////////////////////////////////////////////////////////
// Query if patches have been submitted this frame
bool TerrainRenderer::HaveSubmissions()
{
	return !m->visiblePatches.empty();
}


///////////////////////////////////////////////////////////////////
// Full-featured terrain rendering with blending and everything
void TerrainRenderer::RenderTerrain(ShadowMap* shadow)
{
	debug_assert(m->phase == Phase_Render);

	// switch on required client states
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	// render everything fullbright
	// set up texture environment for base pass
	MICROLOG(L"base splat textures");
	pglActiveTextureARB(GL_TEXTURE0);
	pglClientActiveTextureARB(GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

	// Set alpha to 1.0
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	static const float one[4] = { 1.f, 1.f, 1.f, 1.f };
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, one);
	
	for(size_t i = 0; i < m->visiblePatches.size(); ++i)
	{
		CPatchRData* patchdata = (CPatchRData*)m->visiblePatches[i]->GetRenderData();
		patchdata->RenderBase(true); // with LOS color
	}
	
	// render blends
	// switch on the composite alpha map texture
	(void)ogl_tex_bind(g_Renderer.m_hCompositeAlphaMap, 1);

	// switch on second uv set
	pglClientActiveTextureARB(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// setup additional texenv required by blend pass
	pglActiveTextureARB(GL_TEXTURE1);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_ONE_MINUS_SRC_ALPHA);

	// switch on blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	// no need to write to the depth buffer a second time
	glDepthMask(0);
	
	// render blend passes for each patch
	for(size_t i = 0; i < m->visiblePatches.size(); ++i)
	{
		CPatchRData* patchdata = (CPatchRData*)m->visiblePatches[i]->GetRenderData();
		patchdata->RenderBlends();
	}
	
	// Disable second texcoord array
	pglClientActiveTextureARB(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	// Now apply lighting
	const CLightEnv& lightEnv = g_Renderer.GetLightEnv();

	glBlendFunc(GL_DST_COLOR, GL_ZERO);

	// GL_TEXTURE_ENV_COLOR requires four floats, so we shouldn't use the RGBColor directly
	float terrainAmbientColor[4] = {
		lightEnv.m_TerrainAmbientColor.X,
		lightEnv.m_TerrainAmbientColor.Y,
		lightEnv.m_TerrainAmbientColor.Z,
		1.f
	};
	
	if (!shadow)
	{
		pglActiveTextureARB(GL_TEXTURE0);
		// We're not going to use a texture here, but we have to have a valid texture
		// bound else the texture unit will be disabled.
		// We should still have a bound splat texture from some earlier rendering,
		// so assume that's still valid to use.
		// (TODO: That's a bit of an ugly hack.)

		// Shadow rendering disabled: Ambient + Diffuse
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, terrainAmbientColor);
		
	}
	else
	{
		const CMatrix3D& texturematrix = shadow->GetTextureMatrix();

		pglActiveTextureARB(GL_TEXTURE0);
		glMatrixMode(GL_TEXTURE);
		glLoadMatrixf(&texturematrix._11);
		glMatrixMode(GL_MODELVIEW);

		glBindTexture(GL_TEXTURE_2D, shadow->GetTexture());

		if (shadow->GetUseDepthTexture())
		{
			// Ambient + ShTranslucency * Diffuse * (1 - Shadow) + Diffuse * Shadow
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

			float shadowTransp = g_Renderer.GetLightEnv().GetTerrainShadowTransparency();
			float color[4] = { shadowTransp, shadowTransp, shadowTransp, 1.0 };
			glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);

			pglActiveTextureARB(GL_TEXTURE1);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_INTERPOLATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_TEXTURE0);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

			pglActiveTextureARB(GL_TEXTURE2);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, shadow->GetTexture()); // Need a valid texture or the unit will be disabled
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

			glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, terrainAmbientColor);
			
		}
		else
		{
			// Ambient + Diffuse * Shadow
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

			pglActiveTextureARB(GL_TEXTURE1); // + Ambient
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

			glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, terrainAmbientColor);
			
		}
	}

	pglActiveTextureARB(GL_TEXTURE0);
	pglClientActiveTextureARB(GL_TEXTURE0);

	for (size_t i = 0; i < m->visiblePatches.size(); ++i)
	{
		CPatchRData* patchdata = (CPatchRData*)m->visiblePatches[i]->GetRenderData();
		patchdata->RenderStreams(STREAM_POS|STREAM_COLOR|STREAM_POSTOUV0, false);
	}
	
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	// restore OpenGL state
	if (shadow)
	{
		if (shadow->GetUseDepthTexture())
			g_Renderer.BindTexture(2,0);
	}
	g_Renderer.BindTexture(1,0);

	pglClientActiveTextureARB(GL_TEXTURE0);
	pglActiveTextureARB(GL_TEXTURE0);
	glDepthMask(1);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}


///////////////////////////////////////////////////////////////////
// Render un-textured patches as polygons
void TerrainRenderer::RenderPatches()
{
	debug_assert(m->phase == Phase_Render);

	glEnableClientState(GL_VERTEX_ARRAY);
	for(size_t i = 0; i < m->visiblePatches.size(); ++i)
	{
		CPatchRData* patchdata = (CPatchRData*)m->visiblePatches[i]->GetRenderData();
		patchdata->RenderStreams(STREAM_POS, true);
	}
	glDisableClientState(GL_VERTEX_ARRAY);
}


///////////////////////////////////////////////////////////////////
// Render outlines of submitted patches as lines
void TerrainRenderer::RenderOutlines()
{
	glEnableClientState(GL_VERTEX_ARRAY);
	for(size_t i = 0; i < m->visiblePatches.size(); ++i)
	{
		CPatchRData* patchdata = (CPatchRData*)m->visiblePatches[i]->GetRenderData();
		patchdata->RenderOutline();
	}
	glDisableClientState(GL_VERTEX_ARRAY);
}


///////////////////////////////////////////////////////////////////
// Render water that is part of the terrain
void TerrainRenderer::RenderWater()
{
	PROFILE( "render water" );

	WaterManager* WaterMgr = g_Renderer.GetWaterManager();

	bool fancy = WaterMgr->WillRenderFancyWater();

	// If we're using fancy water, make sure its shader is loaded
	if(fancy && !m->fancyWaterShader)
	{
		Handle h = ogl_program_load(g_VFS, L"shaders/water_high.xml");
		if (h < 0)
		{
			LOG(CLogger::Error, LOG_CATEGORY, L"Failed to load water shader. Falling back to non-fancy water.\n");
			g_Renderer.m_Options.m_FancyWater = false;
			fancy = false;
		}
		else
		{
			m->fancyWaterShader = h;
		}
	}

	//(Crappy) fresnel effect
	CCamera* Camera=g_Game->GetView()->GetCamera();
	CVector3D CamFace=Camera->m_Orientation.GetIn();
	CamFace.Normalize();
	float FresnelScalar = CamFace.Dot( CVector3D(0.0f, -1.0f, 0.0f) );
	//Invert and set boundaries
	FresnelScalar = (1 - FresnelScalar) * 0.4f + 0.6f;

	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	int mapSize = terrain->GetVerticesPerSide();
//	CLOSManager* losMgr = g_Game->GetWorld()->GetLOSManager();
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	double time = WaterMgr->m_WaterTexTimer;

	double period = 1.6;
	int curTex = (int)(time*60/period) % 60;

	if(fancy)
	{
		ogl_tex_bind(WaterMgr->m_NormalMap[curTex], 0);
	}
	else
	{
		ogl_tex_bind(WaterMgr->m_WaterTexture[curTex], 0);
	}

	// Shift the texture coordinates by these amounts to make the water "flow"
	float tx = -fmod(time, 81.0)/81.0;
	float ty = -fmod(time, 34.0)/34.0;

	if(!fancy)
	{
		// Perform the shifting by modifying the texture matrix
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glTranslatef(tx, ty, 0);

		// Set up texture environment to multiply vertex RGB by texture RGB and use vertex alpha
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PRIMARY_COLOR_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	}

	// Set the proper LOD bias
	glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, g_Renderer.m_Options.m_LodBias);
	
	// Some offsets used to go around counterclockwise while keeping code concise
	const int DX[] = {1,1,0,0};
	const int DZ[] = {0,1,1,0};

	GLint vertexDepth = 0;	// water depth attribute, if using fancy water
	GLint losMultiplier = 0;	// LOS multiplier, if using fancy water

	if(fancy)
	{
		// Bind reflection and refraction textures on texture units 1 and 2
		pglActiveTextureARB( GL_TEXTURE1_ARB );
		glEnable( GL_TEXTURE_2D );
		glBindTexture( GL_TEXTURE_2D, WaterMgr->m_ReflectionTexture );
		pglActiveTextureARB( GL_TEXTURE2_ARB );
		glEnable( GL_TEXTURE_2D );
		glBindTexture( GL_TEXTURE_2D, WaterMgr->m_RefractionTexture );

		// Bind water shader and set arguments
		ogl_program_use( m->fancyWaterShader );

		GLint ambient = ogl_program_get_uniform_location( m->fancyWaterShader, "ambient" );
		GLint sunDir = ogl_program_get_uniform_location( m->fancyWaterShader, "sunDir" );
		GLint sunColor = ogl_program_get_uniform_location( m->fancyWaterShader, "sunColor" );
		GLint cameraPos = ogl_program_get_uniform_location( m->fancyWaterShader, "cameraPos" );
		GLint shininess = ogl_program_get_uniform_location( m->fancyWaterShader, "shininess" );
		GLint specularStrength = ogl_program_get_uniform_location( m->fancyWaterShader, "specularStrength" );
		GLint waviness = ogl_program_get_uniform_location( m->fancyWaterShader, "waviness" );
		GLint murkiness = ogl_program_get_uniform_location( m->fancyWaterShader, "murkiness" );
		GLint fullDepth = ogl_program_get_uniform_location( m->fancyWaterShader, "fullDepth" );
		GLint tint = ogl_program_get_uniform_location( m->fancyWaterShader, "tint" );
		GLint reflectionTint = ogl_program_get_uniform_location( m->fancyWaterShader, "reflectionTint" );
		GLint reflectionTintStrength = ogl_program_get_uniform_location( m->fancyWaterShader, "reflectionTintStrength" );
		GLint translation = ogl_program_get_uniform_location( m->fancyWaterShader, "translation" );
		GLint reflectionMatrix = ogl_program_get_uniform_location( m->fancyWaterShader, "reflectionMatrix" );
		GLint refractionMatrix = ogl_program_get_uniform_location( m->fancyWaterShader, "refractionMatrix" );
		GLint normalMap = ogl_program_get_uniform_location( m->fancyWaterShader, "normalMap" );
		GLint reflectionMap = ogl_program_get_uniform_location( m->fancyWaterShader, "reflectionMap" );
		GLint refractionMap = ogl_program_get_uniform_location( m->fancyWaterShader, "refractionMap" );

		const CLightEnv& lightEnv = g_Renderer.GetLightEnv();
		pglUniform3fvARB( ambient, 1, &lightEnv.m_TerrainAmbientColor.X );
		pglUniform3fvARB( sunDir, 1, &lightEnv.GetSunDir().X );
		pglUniform3fvARB( sunColor, 1, &lightEnv.m_SunColor.X );
		pglUniform1fARB( shininess, WaterMgr->m_Shininess );
		pglUniform1fARB( specularStrength, WaterMgr->m_SpecularStrength );
		pglUniform1fARB( waviness, WaterMgr->m_Waviness );
		pglUniform1fARB( murkiness, WaterMgr->m_Murkiness );
		pglUniform1fARB( fullDepth, WaterMgr->m_WaterFullDepth );
		pglUniform3fvARB( tint, 1, WaterMgr->m_WaterTint.FloatArray() );
		pglUniform1fARB( reflectionTintStrength, WaterMgr->m_ReflectionTintStrength );
		pglUniform3fvARB( reflectionTint, 1, WaterMgr->m_ReflectionTint.FloatArray() );
		pglUniform4fARB( translation, tx, ty, 0, 0 );
		pglUniformMatrix4fvARB( reflectionMatrix, 1, false, &WaterMgr->m_ReflectionMatrix._11 );
		pglUniformMatrix4fvARB( refractionMatrix, 1, false, &WaterMgr->m_RefractionMatrix._11 );
		pglUniform1iARB( normalMap, 0 );		// texture unit 0
		pglUniform1iARB( reflectionMap, 1 );	// texture unit 1
		pglUniform1iARB( refractionMap, 2 );	// texture unit 2

		const CCamera& camera = g_Renderer.GetViewCamera();
		CVector3D camPos = camera.m_Orientation.GetTranslation();
		pglUniform3fvARB( cameraPos, 1, &camPos.X );

		vertexDepth = ogl_program_get_attrib_location( m->fancyWaterShader, "vertexDepth" );
		losMultiplier = ogl_program_get_attrib_location( m->fancyWaterShader, "losMultiplier" );
	}
	
	float repeatPeriod = (fancy ? WaterMgr->m_RepeatPeriod : 16.0f);

	glBegin(GL_QUADS);

	for(size_t i=0; i<m->visiblePatches.size(); i++)
	{
		CPatch* patch = m->visiblePatches[i];

		for(ssize_t dx=0; dx<PATCH_SIZE; dx++)
		{
			for(ssize_t dz=0; dz<PATCH_SIZE; dz++)
			{
				ssize_t x = (patch->m_X*PATCH_SIZE + dx);
				ssize_t z = (patch->m_Z*PATCH_SIZE + dz);

				// is any corner of the tile below the water height? if not, no point rendering it
				bool shouldRender = false;
				for(int j=0; j<4; j++)
				{
					float terrainHeight = terrain->GetVertexGroundLevel(x + DX[j], z + DZ[j]);
					if( terrainHeight < WaterMgr->m_WaterHeight )
					{
						shouldRender = true;
						break;
					}
				}
				if(!shouldRender)
				{
					continue;
				}

				for(int j=0; j<4; j++)
				{
					int ix = x + DX[j];
					int iz = z + DZ[j];

					float vertX = ix * CELL_SIZE;
					float vertZ = iz * CELL_SIZE;

					float terrainHeight = terrain->GetVertexGroundLevel(ix, iz);

					float alpha = clamp(
						(WaterMgr->m_WaterHeight - terrainHeight) / WaterMgr->m_WaterFullDepth + WaterMgr->m_WaterAlphaOffset,
						-100.0f, WaterMgr->m_WaterMaxAlpha);

					float losMod = 1.0f;
					/*
					if (false) // XXX: need to implement this for new sim system
					{
						for(size_t k=0; k<4; k++)
						{
							ssize_t tx = ix - DX[k];
							ssize_t tz = iz - DZ[k];

							if(tx >= 0 && tz >= 0 && tx <= mapSize-2 && tz <= mapSize-2)
							{
								ELOSStatus s = losMgr->GetStatus(tx, tz, g_Game->GetLocalPlayer());
								if(s == LOS_EXPLORED && losMod > 0.7f)
									losMod = 0.7f;
								else if(s==LOS_UNEXPLORED && losMod > 0.0f)
									losMod = 0.0f;
							}
						}
					}
					*/

					if(fancy)
					{
						pglVertexAttrib1fARB( vertexDepth, WaterMgr->m_WaterHeight - terrainHeight );
						pglVertexAttrib1fARB( losMultiplier, losMod );
					}
					else
					{
						glColor4f(WaterMgr->m_WaterColor.r*losMod, WaterMgr->m_WaterColor.g*losMod, 
							WaterMgr->m_WaterColor.b*losMod, alpha * FresnelScalar);
					}
					pglMultiTexCoord2fARB(GL_TEXTURE0, vertX/repeatPeriod, vertZ/repeatPeriod);
					glVertex3f(vertX, WaterMgr->m_WaterHeight, vertZ);
				}
			}	//end of x loop
		}	//end of z loop
	}
	glEnd();

	if(fancy)
	{
		// Unbind the refraction/reflection textures and the shader

		pglActiveTextureARB( GL_TEXTURE1_ARB );
		glBindTexture( GL_TEXTURE_2D, 0 );
		glDisable( GL_TEXTURE_2D );

		pglActiveTextureARB( GL_TEXTURE2_ARB );
		glBindTexture( GL_TEXTURE_2D, 0 );
		glDisable( GL_TEXTURE_2D );
		
		pglActiveTextureARB( GL_TEXTURE0_ARB );

		ogl_program_use( 0 );
	}

	if(!fancy)
	{
		// Clean up the texture matrix and blend mode
		glLoadIdentity();
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}

	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}


