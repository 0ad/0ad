/* Copyright (C) 2011 Wildfire Games.
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
#include "graphics/Decal.h"
#include "graphics/LightEnv.h"
#include "graphics/LOSTexture.h"
#include "graphics/Patch.h"
#include "graphics/Terrain.h"
#include "graphics/GameView.h"
#include "graphics/Model.h"
#include "graphics/ShaderManager.h"

#include "maths/MathUtil.h"

#include "ps/Filesystem.h"
#include "ps/CLogger.h"
#include "ps/Font.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "ps/World.h"

#include "renderer/DecalRData.h"
#include "renderer/PatchRData.h"
#include "renderer/Renderer.h"
#include "renderer/ShadowMap.h"
#include "renderer/TerrainRenderer.h"
#include "renderer/VertexArray.h"
#include "renderer/WaterManager.h"

#include "lib/res/graphics/ogl_shader.h"


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

	/// Patches that were submitted for this frame
	std::vector<CPatchRData*> visiblePatches;
	std::vector<CPatchRData*> filteredPatches;

	/// Decals that were submitted for this frame
	std::vector<CDecalRData*> visibleDecals;
	std::vector<CDecalRData*> filteredDecals;

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
	ENSURE(m->phase == Phase_Submit);

	CPatchRData* data = (CPatchRData*)patch->GetRenderData();
	if (data == 0)
	{
		// no renderdata for patch, create it now
		data = new CPatchRData(patch);
		patch->SetRenderData(data);
	}
	data->Update();

	m->visiblePatches.push_back(data);
}

///////////////////////////////////////////////////////////////////
// Submit a decal for rendering
void TerrainRenderer::Submit(CModelDecal* decal)
{
	ENSURE(m->phase == Phase_Submit);

	CDecalRData* data = (CDecalRData*)decal->GetRenderData();
	if (data == 0)
	{
		// no renderdata for decal, create it now
		data = new CDecalRData(decal);
		decal->SetRenderData(data);
	}
	data->Update();

	m->visibleDecals.push_back(data);
}

///////////////////////////////////////////////////////////////////
// Prepare for rendering
void TerrainRenderer::PrepareForRendering()
{
	ENSURE(m->phase == Phase_Submit);

	m->phase = Phase_Render;
}

///////////////////////////////////////////////////////////////////
// Clear submissions lists
void TerrainRenderer::EndFrame()
{
	ENSURE(m->phase == Phase_Render || m->phase == Phase_Submit);

	m->visiblePatches.clear();
	m->visibleDecals.clear();

	m->phase = Phase_Submit;
}


///////////////////////////////////////////////////////////////////
// Culls patches and decals against a frustum.
bool TerrainRenderer::CullPatches(const CFrustum* frustum)
{
	m->filteredPatches.clear();
	for (std::vector<CPatchRData*>::iterator it = m->visiblePatches.begin(); it != m->visiblePatches.end(); it++)
	{
		if (frustum->IsBoxVisible(CVector3D(0, 0, 0), (*it)->GetPatch()->GetBounds()))
			m->filteredPatches.push_back(*it);
	}

	m->filteredDecals.clear();
	for (std::vector<CDecalRData*>::iterator it = m->visibleDecals.begin(); it != m->visibleDecals.end(); it++)
	{
		if (frustum->IsBoxVisible(CVector3D(0, 0, 0), (*it)->GetDecal()->GetBounds()))
			m->filteredDecals.push_back(*it);
	}

	return !m->filteredPatches.empty() || !m->filteredDecals.empty();
}

///////////////////////////////////////////////////////////////////
// Full-featured terrain rendering with blending and everything
void TerrainRenderer::RenderTerrain(bool filtered)
{
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = filtered ? m->filteredPatches : m->visiblePatches;
	std::vector<CDecalRData*>& visibleDecals = filtered ? m->filteredDecals : m->visibleDecals;
	if (visiblePatches.empty() && visibleDecals.empty())
		return;

	// render the solid black sides of the map first
	g_Renderer.BindTexture(0, 0);
	glEnableClientState(GL_VERTEX_ARRAY);
	glColor3f(0, 0, 0);
	PROFILE_START("render terrain sides");
	for (size_t i = 0; i < visiblePatches.size(); ++i)
		visiblePatches[i]->RenderSides();
	PROFILE_END("render terrain sides");

	// switch on required client states
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	// render everything fullbright
	// set up texture environment for base pass
	pglActiveTextureARB(GL_TEXTURE0);
	pglClientActiveTextureARB(GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);

	// Set alpha to 1.0
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	static const float one[4] = { 1.f, 1.f, 1.f, 1.f };
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, one);
	
	PROFILE_START("render terrain base");
	CPatchRData::RenderBases(visiblePatches);
	PROFILE_END("render terrain base");

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
	
	// The decal color array contains lighting data, which we don't want in this non-shader mode
	glDisableClientState(GL_COLOR_ARRAY);

	// render blend passes for each patch
	PROFILE_START("render terrain blends");
	CPatchRData::RenderBlends(visiblePatches);
	PROFILE_END("render terrain blends");

	// Disable second texcoord array
	pglClientActiveTextureARB(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);


	// Render terrain decals

	g_Renderer.BindTexture(1, 0);
	pglActiveTextureARB(GL_TEXTURE0);
	pglClientActiveTextureARB(GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	PROFILE_START("render terrain decals");
	for (size_t i = 0; i < visibleDecals.size(); ++i)
		visibleDecals[i]->Render(CShaderProgramPtr());
	PROFILE_END("render terrain decals");


	// Now apply lighting
	const CLightEnv& lightEnv = g_Renderer.GetLightEnv();

	pglClientActiveTextureARB(GL_TEXTURE0);
	glEnableClientState(GL_COLOR_ARRAY); // diffuse lighting colours

	glBlendFunc(GL_DST_COLOR, GL_ZERO);

	// GL_TEXTURE_ENV_COLOR requires four floats, so we shouldn't use the RGBColor directly
	float terrainAmbientColor[4] = {
		lightEnv.m_TerrainAmbientColor.X,
		lightEnv.m_TerrainAmbientColor.Y,
		lightEnv.m_TerrainAmbientColor.Z,
		1.f
	};

	CLOSTexture& losTexture = g_Renderer.GetScene().GetLOSTexture();

	int streamflags = STREAM_POS|STREAM_COLOR;

	pglActiveTextureARB(GL_TEXTURE0);
	// We're not going to use a texture here, but we have to have a valid texture
	// bound else the texture unit will be disabled.
	// We should still have a bound splat texture from some earlier rendering,
	// so assume that's still valid to use.
	// (TODO: That's a bit of an ugly hack.)

	// No shadows: (Ambient + Diffuse) * LOS
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

	losTexture.BindTexture(1);
	pglClientActiveTextureARB(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	streamflags |= STREAM_POSTOUV1;

	glMatrixMode(GL_TEXTURE);
	glLoadMatrixf(losTexture.GetTextureMatrix());
	glMatrixMode(GL_MODELVIEW);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	pglActiveTextureARB(GL_TEXTURE0);
	pglClientActiveTextureARB(GL_TEXTURE0);

	PROFILE_START("render terrain streams");
	CPatchRData::RenderStreams(visiblePatches, streamflags);
	PROFILE_END("render terrain streams");

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	// restore OpenGL state
	g_Renderer.BindTexture(1, 0);

	pglClientActiveTextureARB(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	pglClientActiveTextureARB(GL_TEXTURE0);
	pglActiveTextureARB(GL_TEXTURE0);
	glDepthMask(1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}


///////////////////////////////////////////////////////////////////

/**
 * Set up all the uniforms for a shader pass.
 */
void TerrainRenderer::PrepareShader(const CShaderProgramPtr& shader, ShadowMap* shadow)
{
	const CLightEnv& lightEnv = g_Renderer.GetLightEnv();

	if (shadow)
	{
		shader->BindTexture("shadowTex", shadow->GetTexture());
		shader->Uniform("shadowTransform", shadow->GetTextureMatrix());

		const float* offsets = shadow->GetFilterOffsets();
		shader->Uniform("shadowOffsets1", offsets[0], offsets[1], offsets[2], offsets[3]);
		shader->Uniform("shadowOffsets2", offsets[4], offsets[5], offsets[6], offsets[7]);
	}

	CLOSTexture& los = g_Renderer.GetScene().GetLOSTexture();
	shader->BindTexture("losTex", los.GetTexture());
	shader->Uniform("losTransform", los.GetTextureMatrix()[0], los.GetTextureMatrix()[12], 0.f, 0.f);

	shader->Uniform("ambient", lightEnv.m_TerrainAmbientColor);
	shader->Uniform("sunColor", lightEnv.m_SunColor);
}

void TerrainRenderer::RenderTerrainShader(ShadowMap* shadow, bool filtered)
{
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = filtered ? m->filteredPatches : m->visiblePatches;
	std::vector<CDecalRData*>& visibleDecals = filtered ? m->filteredDecals : m->visibleDecals;
	if (visiblePatches.empty() && visibleDecals.empty())
		return;

	CShaderManager& shaderManager = g_Renderer.GetShaderManager();

	typedef std::map<CStr, CStr> Defines;
	Defines defBasic;
	if (shadow)
	{
		defBasic["USE_SHADOW"] = "1";
		if (g_Renderer.m_Caps.m_ARBProgramShadow && g_Renderer.m_Options.m_ARBProgramShadow)
			defBasic["USE_FP_SHADOW"] = "1";
		if (g_Renderer.m_Options.m_ShadowPCF)
			defBasic["USE_SHADOW_PCF"] = "1";
	}

	defBasic["LIGHTING_MODEL_" + g_Renderer.GetLightEnv().GetLightingModel()] = "1";

	CShaderProgramPtr shaderBase(shaderManager.LoadProgram("terrain_base", defBasic));
	CShaderProgramPtr shaderBlend(shaderManager.LoadProgram("terrain_blend", defBasic));
	CShaderProgramPtr shaderDecal(shaderManager.LoadProgram("terrain_decal", defBasic));

	// render the solid black sides of the map first
	g_Renderer.BindTexture(0, 0);
	glEnableClientState(GL_VERTEX_ARRAY);
	glColor3f(0, 0, 0);
	PROFILE_START("render terrain sides");
	for (size_t i = 0; i < visiblePatches.size(); ++i)
		visiblePatches[i]->RenderSides();
	PROFILE_END("render terrain sides");

	// switch on required client states
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY); // diffuse lighting colours

	shaderBase->Bind();
	PrepareShader(shaderBase, shadow);

	PROFILE_START("render terrain base");
	CPatchRData::RenderBases(visiblePatches);
	PROFILE_END("render terrain base");

	shaderBase->Unbind();

	// render blends

	shaderBlend->Bind();
	PrepareShader(shaderBlend, shadow);

	// switch on the composite alpha map texture
	(void)ogl_tex_bind(g_Renderer.m_hCompositeAlphaMap, 1);

	// switch on second uv set
	pglClientActiveTextureARB(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// switch on blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// no need to write to the depth buffer a second time
	glDepthMask(0);

	// render blend passes for each patch
	PROFILE_START("render terrain blends");
	CPatchRData::RenderBlends(visiblePatches);
	PROFILE_END("render terrain blends");

	// Disable second texcoord array
	pglClientActiveTextureARB(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	shaderBlend->Unbind();

	// Render terrain decals

	shaderDecal->Bind();
	PrepareShader(shaderDecal, shadow);

	g_Renderer.BindTexture(1, 0);
	pglActiveTextureARB(GL_TEXTURE0);
	pglClientActiveTextureARB(GL_TEXTURE0);

	PROFILE_START("render terrain decals");
	for (size_t i = 0; i < visibleDecals.size(); ++i)
		visibleDecals[i]->Render(shaderDecal);
	PROFILE_END("render terrain decals");

	shaderDecal->Unbind();

	// restore OpenGL state
	g_Renderer.BindTexture(1, 0);
	g_Renderer.BindTexture(2, 0);
	g_Renderer.BindTexture(3, 0);

	pglClientActiveTextureARB(GL_TEXTURE0);
	pglActiveTextureARB(GL_TEXTURE0);
	glDepthMask(1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}


///////////////////////////////////////////////////////////////////
// Render un-textured patches as polygons
void TerrainRenderer::RenderPatches(bool filtered)
{
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = filtered ? m->filteredPatches : m->visiblePatches;
	if (visiblePatches.empty())
		return;

	glEnableClientState(GL_VERTEX_ARRAY);
	CPatchRData::RenderStreams(visiblePatches, STREAM_POS);
	glDisableClientState(GL_VERTEX_ARRAY);
}


///////////////////////////////////////////////////////////////////
// Render outlines of submitted patches as lines
void TerrainRenderer::RenderOutlines(bool filtered)
{
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = filtered ? m->filteredPatches : m->visiblePatches;
	if (visiblePatches.empty())
		return;

	glEnableClientState(GL_VERTEX_ARRAY);
	for (size_t i = 0; i < visiblePatches.size(); ++i)
		visiblePatches[i]->RenderOutline();
	glDisableClientState(GL_VERTEX_ARRAY);
}


///////////////////////////////////////////////////////////////////
// Scissor rectangle of water patches
CBound TerrainRenderer::ScissorWater(const CMatrix3D &viewproj)
{
	CBound scissor;
	for (size_t i = 0; i < m->visiblePatches.size(); ++i)
	{
		CPatchRData* data = m->visiblePatches[i];
		const CBound& waterBounds = data->GetWaterBounds();
		if (waterBounds.IsEmpty())
			continue;

		CVector4D v1 = viewproj.Transform(CVector4D(waterBounds[0].X, waterBounds[1].Y, waterBounds[0].Z, 1.0f));
		CVector4D v2 = viewproj.Transform(CVector4D(waterBounds[1].X, waterBounds[1].Y, waterBounds[0].Z, 1.0f));
		CVector4D v3 = viewproj.Transform(CVector4D(waterBounds[0].X, waterBounds[1].Y, waterBounds[1].Z, 1.0f));
		CVector4D v4 = viewproj.Transform(CVector4D(waterBounds[1].X, waterBounds[1].Y, waterBounds[1].Z, 1.0f));
		CBound screenBounds;
		#define ADDBOUND(v1, v2, v3, v4) \
			if (v1[2] >= -v1[3]) \
				screenBounds += CVector3D(v1[0], v1[1], v1[2]) * (1.0f / v1[3]); \
			else \
			{ \
				float t = v1[2] + v1[3]; \
				if (v2[2] > -v2[3]) \
				{ \
					CVector4D c2 = v1 + (v2 - v1) * (t / (t - (v2[2] + v2[3]))); \
					screenBounds += CVector3D(c2[0], c2[1], c2[2]) * (1.0f / c2[3]); \
				} \
				if (v3[2] > -v3[3]) \
				{ \
					CVector4D c3 = v1 + (v3 - v1) * (t / (t - (v3[2] + v3[3]))); \
					screenBounds += CVector3D(c3[0], c3[1], c3[2]) * (1.0f / c3[3]); \
				} \
				if (v4[2] > -v4[3]) \
				{ \
					CVector4D c4 = v1 + (v4 - v1) * (t / (t - (v4[2] + v4[3]))); \
					screenBounds += CVector3D(c4[0], c4[1], c4[2]) * (1.0f / c4[3]); \
				} \
			}
		ADDBOUND(v1, v2, v3, v4);
		ADDBOUND(v2, v1, v3, v4);
		ADDBOUND(v3, v1, v2, v4);
		ADDBOUND(v4, v1, v2, v3);
		#undef ADDBOUND
		if (screenBounds[0].X >= 1.0f || screenBounds[1].X <= -1.0f || screenBounds[0].Y >= 1.0f || screenBounds[1].Y <= -1.0f)
			continue;
		scissor += screenBounds;
	}
	return CBound(CVector3D(clamp(scissor[0].X, -1.0f, 1.0f), clamp(scissor[0].Y, -1.0f, 1.0f), -1.0f),
				  CVector3D(clamp(scissor[1].X, -1.0f, 1.0f), clamp(scissor[1].Y, -1.0f, 1.0f), 1.0f));
}

// Render fancy water
bool TerrainRenderer::RenderFancyWater()
{
	PROFILE("render fancy water");

	// If we're using fancy water, make sure its shader is loaded
	if (!m->fancyWaterShader)
	{
		Handle h = ogl_program_load(g_VFS, L"shaders/water_high.xml");
		if (h < 0)
		{
			LOGERROR(L"Failed to load water shader. Falling back to non-fancy water.\n");
			g_Renderer.m_Options.m_FancyWater = false;
			return false;
		}
		else
		{
			m->fancyWaterShader = h;
		}
	}

	WaterManager* WaterMgr = g_Renderer.GetWaterManager();
	CLOSTexture& losTexture = g_Renderer.GetScene().GetLOSTexture();
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	double time = WaterMgr->m_WaterTexTimer;
	double period = 1.6;
	int curTex = (int)(time*60/period) % 60;

	WaterMgr->m_NormalMap[curTex]->Bind();

	// Shift the texture coordinates by these amounts to make the water "flow"
	float tx = -fmod(time, 81.0)/81.0;
	float ty = -fmod(time, 34.0)/34.0;
	float repeatPeriod = WaterMgr->m_RepeatPeriod;

	// Set the proper LOD bias
	glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, g_Renderer.m_Options.m_LodBias);

	const CCamera& camera = g_Renderer.GetViewCamera();
	CVector3D camPos = camera.m_Orientation.GetTranslation();

	// Bind reflection and refraction textures on texture units 1 and 2
	pglActiveTextureARB(GL_TEXTURE1_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, WaterMgr->m_ReflectionTexture);
	pglActiveTextureARB(GL_TEXTURE2_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, WaterMgr->m_RefractionTexture);

	losTexture.BindTexture(3);

	// Bind water shader and set arguments
	ogl_program_use(m->fancyWaterShader);

	GLint ambient = ogl_program_get_uniform_location(m->fancyWaterShader, "ambient");
	GLint sunDir = ogl_program_get_uniform_location(m->fancyWaterShader, "sunDir");
	GLint sunColor = ogl_program_get_uniform_location(m->fancyWaterShader, "sunColor");
	GLint cameraPos = ogl_program_get_uniform_location(m->fancyWaterShader, "cameraPos");
	GLint shininess = ogl_program_get_uniform_location(m->fancyWaterShader, "shininess");
	GLint specularStrength = ogl_program_get_uniform_location(m->fancyWaterShader, "specularStrength");
	GLint waviness = ogl_program_get_uniform_location(m->fancyWaterShader, "waviness");
	GLint murkiness = ogl_program_get_uniform_location(m->fancyWaterShader, "murkiness");
	GLint fullDepth = ogl_program_get_uniform_location(m->fancyWaterShader, "fullDepth");
	GLint tint = ogl_program_get_uniform_location(m->fancyWaterShader, "tint");
	GLint reflectionTint = ogl_program_get_uniform_location(m->fancyWaterShader, "reflectionTint");
	GLint reflectionTintStrength = ogl_program_get_uniform_location(m->fancyWaterShader, "reflectionTintStrength");
	GLint translation = ogl_program_get_uniform_location(m->fancyWaterShader, "translation");
	GLint repeatScale = ogl_program_get_uniform_location(m->fancyWaterShader, "repeatScale");
	GLint reflectionMatrix = ogl_program_get_uniform_location(m->fancyWaterShader, "reflectionMatrix");
	GLint refractionMatrix = ogl_program_get_uniform_location(m->fancyWaterShader, "refractionMatrix");
	GLint losMatrix = ogl_program_get_uniform_location(m->fancyWaterShader, "losMatrix");
	GLint normalMap = ogl_program_get_uniform_location(m->fancyWaterShader, "normalMap");
	GLint reflectionMap = ogl_program_get_uniform_location(m->fancyWaterShader, "reflectionMap");
	GLint refractionMap = ogl_program_get_uniform_location(m->fancyWaterShader, "refractionMap");
	GLint losMap = ogl_program_get_uniform_location(m->fancyWaterShader, "losMap");

	const CLightEnv& lightEnv = g_Renderer.GetLightEnv();
	pglUniform3fvARB(ambient, 1, &lightEnv.m_TerrainAmbientColor.X);
	pglUniform3fvARB(sunDir, 1, &lightEnv.GetSunDir().X);
	pglUniform3fvARB(sunColor, 1, &lightEnv.m_SunColor.X);
	pglUniform1fARB(shininess, WaterMgr->m_Shininess);
	pglUniform1fARB(specularStrength, WaterMgr->m_SpecularStrength);
	pglUniform1fARB(waviness, WaterMgr->m_Waviness);
	pglUniform1fARB(murkiness, WaterMgr->m_Murkiness);
	pglUniform1fARB(fullDepth, WaterMgr->m_WaterFullDepth);
	pglUniform3fvARB(tint, 1, WaterMgr->m_WaterTint.FloatArray());
	pglUniform1fARB(reflectionTintStrength, WaterMgr->m_ReflectionTintStrength);
	pglUniform3fvARB(reflectionTint, 1, WaterMgr->m_ReflectionTint.FloatArray());
	pglUniform2fARB(translation, tx, ty);
	pglUniform1fARB(repeatScale, 1.0f / repeatPeriod);
	pglUniformMatrix4fvARB(reflectionMatrix, 1, false, &WaterMgr->m_ReflectionMatrix._11);
	pglUniformMatrix4fvARB(refractionMatrix, 1, false, &WaterMgr->m_RefractionMatrix._11);
	pglUniformMatrix4fvARB(losMatrix, 1, false, losTexture.GetTextureMatrix());
	pglUniform1iARB(normalMap, 0);		// texture unit 0
	pglUniform1iARB(reflectionMap, 1);	// texture unit 1
	pglUniform1iARB(refractionMap, 2);	// texture unit 2
	pglUniform1iARB(losMap, 3);			// texture unit 3
	pglUniform3fvARB(cameraPos, 1, &camPos.X);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	for (size_t i = 0; i < m->visiblePatches.size(); ++i)
	{
		CPatchRData* data = m->visiblePatches[i];
		data->RenderWater();
	}

	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	// Unbind the LOS/refraction/reflection textures and the shader
	g_Renderer.BindTexture(3, 0);
	g_Renderer.BindTexture(2, 0);
	g_Renderer.BindTexture(1, 0);

	pglActiveTextureARB(GL_TEXTURE0_ARB);

	ogl_program_use(0);

	glDisable(GL_BLEND);

	return true;
}

void TerrainRenderer::RenderSimpleWater()
{
	PROFILE("render simple water");

	WaterManager* WaterMgr = g_Renderer.GetWaterManager();
	CLOSTexture& losTexture = g_Game->GetView()->GetLOSTexture();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	double time = WaterMgr->m_WaterTexTimer;
	double period = 1.6f;
	int curTex = (int)(time*60/period) % 60;

	WaterMgr->m_WaterTexture[curTex]->Bind();

	// Shift the texture coordinates by these amounts to make the water "flow"
	float tx = -fmod(time, 81.0)/81.0;
	float ty = -fmod(time, 34.0)/34.0;
	float repeatPeriod = 16.0f;

	// Perform the shifting by using texture coordinate generation
	GLfloat texgenS0[4] = { 1/repeatPeriod, 0, 0, tx };
	GLfloat texgenT0[4] = { 0, 0, 1/repeatPeriod, ty };
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, texgenS0);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, texgenT0);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);

	// Set up texture environment to multiply vertex RGB by texture RGB and use vertex alpha
	GLfloat waterColor[4] = { WaterMgr->m_WaterColor.r, WaterMgr->m_WaterColor.g, WaterMgr->m_WaterColor.b, 1.0f };
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, waterColor);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	// Multiply by LOS texture
	losTexture.BindTexture(1);
	const float *losMatrix = losTexture.GetTextureMatrix();
	GLfloat texgenS1[4] = { losMatrix[0], losMatrix[4], losMatrix[8], losMatrix[12] };
	GLfloat texgenT1[4] = { losMatrix[1], losMatrix[5], losMatrix[9], losMatrix[13] };
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, texgenS1);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, texgenT1);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	// Set the proper LOD bias
	glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, g_Renderer.m_Options.m_LodBias);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	for (size_t i = 0; i < m->visiblePatches.size(); ++i)
	{
		CPatchRData* data = m->visiblePatches[i];
		data->RenderWater();
	}

	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	g_Renderer.BindTexture(1, 0);

	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	pglActiveTextureARB(GL_TEXTURE0_ARB);

	// Clean up the texture matrix and blend mode
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

///////////////////////////////////////////////////////////////////
// Render water that is part of the terrain
void TerrainRenderer::RenderWater()
{
	WaterManager* WaterMgr = g_Renderer.GetWaterManager();

	if (!WaterMgr->WillRenderFancyWater() || !RenderFancyWater())
		RenderSimpleWater();
}

void TerrainRenderer::RenderPriorities()
{
	PROFILE("render priorities");

	ENSURE(m->phase == Phase_Render);

	CFont font(L"mono-stroke-10");
	font.Bind();

	glColor3f(1, 1, 0);

	for (size_t i = 0; i < m->visiblePatches.size(); ++i)
		m->visiblePatches[i]->RenderPriorities();
}
