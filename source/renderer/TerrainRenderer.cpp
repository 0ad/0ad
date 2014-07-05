/* Copyright (C) 2013 Wildfire Games.
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
#include "graphics/GameView.h"
#include "graphics/Model.h"
#include "graphics/ShaderManager.h"
#include "renderer/ShadowMap.h"
#include "renderer/SkyManager.h"
#include "graphics/TerritoryTexture.h"
#include "graphics/TextRenderer.h"

#include "maths/MathUtil.h"

#include "ps/Filesystem.h"
#include "ps/CLogger.h"
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

#include "tools/atlas/GameInterface/GameLoop.h"

extern GameLoopState* g_AtlasGameLoop;

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
	std::vector<CPatchRData*> visiblePatches[CRenderer::CULL_MAX];

	/// Decals that were submitted for this frame
	std::vector<CDecalRData*> visibleDecals[CRenderer::CULL_MAX];

	/// Fancy water shader
	CShaderProgramPtr fancyWaterShader;
	CShaderProgramPtr fancyEffectsShader;

	CSimulation2* simulation;
};



///////////////////////////////////////////////////////////////////
// Construction/Destruction
TerrainRenderer::TerrainRenderer()
{
	m = new TerrainRendererInternals();
	m->phase = Phase_Submit;
}

TerrainRenderer::~TerrainRenderer()
{
	delete m;
}

void TerrainRenderer::SetSimulation(CSimulation2* simulation)
{
	m->simulation = simulation;
}

///////////////////////////////////////////////////////////////////
// Submit a patch for rendering
void TerrainRenderer::Submit(int cullGroup, CPatch* patch)
{
	ENSURE(m->phase == Phase_Submit);

	CPatchRData* data = (CPatchRData*)patch->GetRenderData();
	if (data == 0)
	{
		// no renderdata for patch, create it now
		data = new CPatchRData(patch, m->simulation);
		patch->SetRenderData(data);
	}
	data->Update(m->simulation);

	m->visiblePatches[cullGroup].push_back(data);
}

///////////////////////////////////////////////////////////////////
// Submit a decal for rendering
void TerrainRenderer::Submit(int cullGroup, CModelDecal* decal)
{
	ENSURE(m->phase == Phase_Submit);

	CDecalRData* data = (CDecalRData*)decal->GetRenderData();
	if (data == 0)
	{
		// no renderdata for decal, create it now
		data = new CDecalRData(decal, m->simulation);
		decal->SetRenderData(data);
	}
	data->Update(m->simulation);

	m->visibleDecals[cullGroup].push_back(data);
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

	for (int i = 0; i < CRenderer::CULL_MAX; ++i)
	{
		m->visiblePatches[i].clear();
		m->visibleDecals[i].clear();
	}

	m->phase = Phase_Submit;
}


///////////////////////////////////////////////////////////////////
// Full-featured terrain rendering with blending and everything
void TerrainRenderer::RenderTerrain(int cullGroup)
{
#if CONFIG2_GLES
	UNUSED2(cullGroup);
#else
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];
	std::vector<CDecalRData*>& visibleDecals = m->visibleDecals[cullGroup];
	if (visiblePatches.empty() && visibleDecals.empty())
		return;

	CShaderProgramPtr dummyShader = g_Renderer.GetShaderManager().LoadProgram("fixed:dummy", CShaderDefines());
	dummyShader->Bind();

	// render the solid black sides of the map first
	g_Renderer.BindTexture(0, 0);
	glEnableClientState(GL_VERTEX_ARRAY);
	glColor3f(0, 0, 0);
	PROFILE_START("render terrain sides");
	for (size_t i = 0; i < visiblePatches.size(); ++i)
		visiblePatches[i]->RenderSides(dummyShader);
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
	CPatchRData::RenderBases(visiblePatches, CShaderDefines(), NULL, true, dummyShader);
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
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// no need to write to the depth buffer a second time
	glDepthMask(0);
	
	// The decal color array contains lighting data, which we don't want in this non-shader mode
	glDisableClientState(GL_COLOR_ARRAY);

	// render blend passes for each patch
	PROFILE_START("render terrain blends");
	CPatchRData::RenderBlends(visiblePatches, CShaderDefines(), NULL, true, dummyShader);
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
	CDecalRData::RenderDecals(visibleDecals, CShaderDefines(), NULL, true, dummyShader);
	PROFILE_END("render terrain decals");


	// Now apply lighting
	const CLightEnv& lightEnv = g_Renderer.GetLightEnv();

	pglClientActiveTextureARB(GL_TEXTURE0);
	glEnableClientState(GL_COLOR_ARRAY); // diffuse lighting colours

	// The vertex color is scaled by 0.5 to permit overbrightness without clamping.
	// We therefore need to draw clamp((texture*lighting)*2.0), where 'texture'
	// is what previous passes drew onto the framebuffer, and 'lighting' is the
	// color computed by this pass.
	// We can do that with blending by getting it to draw dst*src + src*dst:
	glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);

	// Scale the ambient color by 0.5 to match the vertex diffuse colors
	float terrainAmbientColor[4] = {
		lightEnv.m_TerrainAmbientColor.X * 0.5f,
		lightEnv.m_TerrainAmbientColor.Y * 0.5f,
		lightEnv.m_TerrainAmbientColor.Z * 0.5f,
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
	glLoadMatrixf(&losTexture.GetTextureMatrix()._11);
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
	CPatchRData::RenderStreams(visiblePatches, dummyShader, streamflags);
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

	dummyShader->Unbind();
#endif
}

void TerrainRenderer::RenderTerrainOverlayTexture(int cullGroup, CMatrix3D& textureMatrix)
{
#if CONFIG2_GLES
#warning TODO: implement TerrainRenderer::RenderTerrainOverlayTexture for GLES
	UNUSED2(cullGroup);
	UNUSED2(textureMatrix);
#else
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(0);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glMatrixMode(GL_TEXTURE);
	glLoadMatrixf(&textureMatrix._11);
	glMatrixMode(GL_MODELVIEW);

	CShaderProgramPtr dummyShader = g_Renderer.GetShaderManager().LoadProgram("fixed:dummy", CShaderDefines());
	dummyShader->Bind();
	CPatchRData::RenderStreams(visiblePatches, dummyShader, STREAM_POS|STREAM_POSTOUV0);
	dummyShader->Unbind();

	// To make the overlay visible over water, render an additional map-sized
	// water-height patch
	CBoundingBoxAligned waterBounds;
	for (size_t i = 0; i < visiblePatches.size(); ++i)
	{
		CPatchRData* data = visiblePatches[i];
		waterBounds += data->GetWaterBounds();
	}
	if (!waterBounds.IsEmpty())
	{
		float h = g_Renderer.GetWaterManager()->m_WaterHeight + 0.05f; // add a delta to avoid z-fighting
		float waterPos[] = {
			waterBounds[0].X, h, waterBounds[0].Z,
			waterBounds[1].X, h, waterBounds[0].Z,
			waterBounds[0].X, h, waterBounds[1].Z,
			waterBounds[1].X, h, waterBounds[1].Z
		};
		glVertexPointer(3, GL_FLOAT, 3*sizeof(float), waterPos);
		glTexCoordPointer(3, GL_FLOAT, 3*sizeof(float), waterPos);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	glDepthMask(1);
	glDisable(GL_BLEND);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
}


///////////////////////////////////////////////////////////////////

/**
 * Set up all the uniforms for a shader pass.
 */
void TerrainRenderer::PrepareShader(const CShaderProgramPtr& shader, ShadowMap* shadow)
{
	shader->Uniform(str_transform, g_Renderer.GetViewCamera().GetViewProjection());
	shader->Uniform(str_cameraPos, g_Renderer.GetViewCamera().GetOrientation().GetTranslation());

	const CLightEnv& lightEnv = g_Renderer.GetLightEnv();

	if (shadow)
	{
		shader->BindTexture(str_shadowTex, shadow->GetTexture());
		shader->Uniform(str_shadowTransform, shadow->GetTextureMatrix());
		int width = shadow->GetWidth();
		int height = shadow->GetHeight();
		shader->Uniform(str_shadowScale, width, height, 1.0f / width, 1.0f / height);
	}

	CLOSTexture& los = g_Renderer.GetScene().GetLOSTexture();
	shader->BindTexture(str_losTex, los.GetTextureSmooth());
	shader->Uniform(str_losTransform, los.GetTextureMatrix()[0], los.GetTextureMatrix()[12], 0.f, 0.f);

	shader->Uniform(str_ambient, lightEnv.m_TerrainAmbientColor);
	shader->Uniform(str_sunColor, lightEnv.m_SunColor);
	shader->Uniform(str_sunDir, lightEnv.GetSunDir());
	
	shader->Uniform(str_fogColor, lightEnv.m_FogColor);
	shader->Uniform(str_fogParams, lightEnv.m_FogFactor, lightEnv.m_FogMax, 0.f, 0.f);
}

void TerrainRenderer::RenderTerrainShader(const CShaderDefines& context, int cullGroup, ShadowMap* shadow)
{
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];
	std::vector<CDecalRData*>& visibleDecals = m->visibleDecals[cullGroup];
	if (visiblePatches.empty() && visibleDecals.empty())
		return;

	// render the solid black sides of the map first
	CShaderTechniquePtr techSolid = g_Renderer.GetShaderManager().LoadEffect(str_gui_solid);
	techSolid->BeginPass();
	CShaderProgramPtr shaderSolid = techSolid->GetShader();
	shaderSolid->Uniform(str_transform, g_Renderer.GetViewCamera().GetViewProjection());
	shaderSolid->Uniform(str_color, 0.0f, 0.0f, 0.0f, 1.0f);

	PROFILE_START("render terrain sides");
	for (size_t i = 0; i < visiblePatches.size(); ++i)
		visiblePatches[i]->RenderSides(shaderSolid);
	PROFILE_END("render terrain sides");

	techSolid->EndPass();

	PROFILE_START("render terrain base");
	CPatchRData::RenderBases(visiblePatches, context, shadow);
	PROFILE_END("render terrain base");

	// no need to write to the depth buffer a second time
	glDepthMask(0);

	// render blend passes for each patch
	PROFILE_START("render terrain blends");
	CPatchRData::RenderBlends(visiblePatches, context, shadow, false);
	PROFILE_END("render terrain blends");

	PROFILE_START("render terrain decals");
	CDecalRData::RenderDecals(visibleDecals, context, shadow, false);
	PROFILE_END("render terrain decals");

	// restore OpenGL state
	g_Renderer.BindTexture(1, 0);
	g_Renderer.BindTexture(2, 0);
	g_Renderer.BindTexture(3, 0);

	glDepthMask(1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
}


///////////////////////////////////////////////////////////////////
// Render un-textured patches as polygons
void TerrainRenderer::RenderPatches(int cullGroup)
{
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];
	if (visiblePatches.empty())
		return;

#if CONFIG2_GLES
#warning TODO: implement TerrainRenderer::RenderPatches for GLES
#else
	CShaderProgramPtr dummyShader = g_Renderer.GetShaderManager().LoadProgram("fixed:dummy", CShaderDefines());
	dummyShader->Bind();

	glEnableClientState(GL_VERTEX_ARRAY);
	CPatchRData::RenderStreams(visiblePatches, dummyShader, STREAM_POS);
	glDisableClientState(GL_VERTEX_ARRAY);

	dummyShader->Unbind();
#endif
}


///////////////////////////////////////////////////////////////////
// Render outlines of submitted patches as lines
void TerrainRenderer::RenderOutlines(int cullGroup)
{
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];
	if (visiblePatches.empty())
		return;

#if CONFIG2_GLES
#warning TODO: implement TerrainRenderer::RenderOutlines for GLES
#else
	glEnableClientState(GL_VERTEX_ARRAY);
	for (size_t i = 0; i < visiblePatches.size(); ++i)
		visiblePatches[i]->RenderOutline();
	glDisableClientState(GL_VERTEX_ARRAY);
#endif
}


///////////////////////////////////////////////////////////////////
// Scissor rectangle of water patches
CBoundingBoxAligned TerrainRenderer::ScissorWater(int cullGroup, const CMatrix3D &viewproj)
{
	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];

	CBoundingBoxAligned scissor;
	for (size_t i = 0; i < visiblePatches.size(); ++i)
	{
		CPatchRData* data = visiblePatches[i];
		const CBoundingBoxAligned& waterBounds = data->GetWaterBounds();
		if (waterBounds.IsEmpty())
			continue;

		CVector4D v1 = viewproj.Transform(CVector4D(waterBounds[0].X, waterBounds[1].Y, waterBounds[0].Z, 1.0f));
		CVector4D v2 = viewproj.Transform(CVector4D(waterBounds[1].X, waterBounds[1].Y, waterBounds[0].Z, 1.0f));
		CVector4D v3 = viewproj.Transform(CVector4D(waterBounds[0].X, waterBounds[1].Y, waterBounds[1].Z, 1.0f));
		CVector4D v4 = viewproj.Transform(CVector4D(waterBounds[1].X, waterBounds[1].Y, waterBounds[1].Z, 1.0f));
		CBoundingBoxAligned screenBounds;
		#define ADDBOUND(v1, v2, v3, v4) \
			if (v1.Z >= -v1.W) \
				screenBounds += CVector3D(v1.X, v1.Y, v1.Z) * (1.0f / v1.W); \
			else \
			{ \
				float t = v1.Z + v1.W; \
				if (v2.Z > -v2.W) \
				{ \
					CVector4D c2 = v1 + (v2 - v1) * (t / (t - (v2.Z + v2.W))); \
					screenBounds += CVector3D(c2.X, c2.Y, c2.Z) * (1.0f / c2.W); \
				} \
				if (v3.Z > -v3.W) \
				{ \
					CVector4D c3 = v1 + (v3 - v1) * (t / (t - (v3.Z + v3.W))); \
					screenBounds += CVector3D(c3.X, c3.Y, c3.Z) * (1.0f / c3.W); \
				} \
				if (v4.Z > -v4.W) \
				{ \
					CVector4D c4 = v1 + (v4 - v1) * (t / (t - (v4.Z + v4.W))); \
					screenBounds += CVector3D(c4.X, c4.Y, c4.Z) * (1.0f / c4.W); \
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
	return CBoundingBoxAligned(CVector3D(clamp(scissor[0].X, -1.0f, 1.0f), clamp(scissor[0].Y, -1.0f, 1.0f), -1.0f),
				  CVector3D(clamp(scissor[1].X, -1.0f, 1.0f), clamp(scissor[1].Y, -1.0f, 1.0f), 1.0f));
}

// Render fancy water
bool TerrainRenderer::RenderFancyWater(const CShaderDefines& context, int cullGroup, ShadowMap* shadow)
{
	PROFILE3_GPU("fancy water");
	
	WaterManager* WaterMgr = g_Renderer.GetWaterManager();
	CShaderDefines defines = context;
	
	// If we're using fancy water, make sure its shader is loaded
	if (!m->fancyWaterShader || WaterMgr->m_NeedsReloading)
	{
		if (WaterMgr->m_WaterRealDepth)
			defines.Add(str_USE_REAL_DEPTH, str_1);
		if (WaterMgr->m_WaterFancyEffects)
			defines.Add(str_USE_FANCY_EFFECTS, str_1);
		if (WaterMgr->m_WaterRefraction)
			defines.Add(str_USE_REFRACTION, str_1);
		if (WaterMgr->m_WaterReflection)
			defines.Add(str_USE_REFLECTION, str_1);
		if (shadow && WaterMgr->m_WaterShadows)
			defines.Add(str_USE_SHADOWS_ON_WATER, str_1);

		m->fancyEffectsShader = g_Renderer.GetShaderManager().LoadProgram("glsl/water_effects", defines);
		if (!m->fancyEffectsShader)
		{
			LOGERROR(L"Failed to load Fancy effects shader. Deactivating fancy effects.\n");
			g_Renderer.SetOptionBool(CRenderer::OPT_WATERFANCYEFFECTS, false);
			defines.Add(str_USE_FANCY_EFFECTS, str_0);
		}
		
		// haven't updated the ARB shader yet so I'll always load the GLSL
		/*if (!g_Renderer.m_Options.m_PreferGLSL && !superFancy)
			m->fancyWaterShader = g_Renderer.GetShaderManager().LoadProgram("arb/water_high", defines);
		else*/
			m->fancyWaterShader = g_Renderer.GetShaderManager().LoadProgram("glsl/water_high", defines);
		
		if (!m->fancyWaterShader)
		{
			LOGERROR(L"Failed to load water shader. Falling back to fixed pipeline water.\n");
			WaterMgr->m_RenderWater = false;
			return false;
		}
		WaterMgr->m_NeedsReloading = false;
	}
	
	CLOSTexture& losTexture = g_Renderer.GetScene().GetLOSTexture();

	GLuint depthTex;
	// creating the real depth texture using the depth buffer.
	if (WaterMgr->m_WaterRealDepth)
	{
		if (WaterMgr->m_depthTT == 0)
		{
			glGenTextures(1, (GLuint*)&depthTex);
			WaterMgr->m_depthTT = depthTex;
			glBindTexture(GL_TEXTURE_2D, WaterMgr->m_depthTT);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, g_Renderer.GetWidth(), g_Renderer.GetHeight(), 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE,NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		} else {
			glBindTexture(GL_TEXTURE_2D, WaterMgr->m_depthTT);
			glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 0, 0, g_Renderer.GetWidth(), g_Renderer.GetHeight(), 0);
		}
		
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	// Calculating the advanced informations about Foam and all if the quality calls for it.
	/*if (WaterMgr->m_NeedInfoUpdate && (WaterMgr->m_WaterFoam || WaterMgr->m_WaterCoastalWaves))
	{
		WaterMgr->m_NeedInfoUpdate = false;
		WaterMgr->CreateSuperfancyInfo();
	}*/
	
	double time = WaterMgr->m_WaterTexTimer;
	double period = 8;
	int curTex = (int)(time*60/period) % 60;
	int nexTex = (curTex + 1) % 60;
	
	float repeatPeriod = WaterMgr->m_RepeatPeriod;
	
	GLuint FramebufferName = 0;

	// Render normals and foam to a framebuffer if we're in fancy effects
	if (WaterMgr->m_WaterFancyEffects)
	{
		// Save the post-processing framebuffer.
		GLint fbo;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &fbo);
		
		// Generate our framebuffer
		pglGenFramebuffersEXT(1, &FramebufferName);
		pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, FramebufferName);
		
		GLuint renderedTexture;
		if (WaterMgr->m_FancyTexture == 0)
		{
			glGenTextures(1, &renderedTexture);
			WaterMgr->m_FancyTexture = renderedTexture;
			
			glBindTexture(GL_TEXTURE_2D, WaterMgr->m_FancyTexture);
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, (float)g_Renderer.GetWidth(), (float)g_Renderer.GetHeight(), 0,GL_RGBA, GL_UNSIGNED_BYTE, 0);
		} else
			glBindTexture(GL_TEXTURE_2D, WaterMgr->m_FancyTexture);
				
		pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, WaterMgr->m_FancyTexture, 0);
				
		// rendering
		m->fancyEffectsShader->Bind();
		m->fancyEffectsShader->BindTexture(str_normalMap, WaterMgr->m_NormalMap[curTex]);
		m->fancyEffectsShader->BindTexture(str_normalMap2, WaterMgr->m_NormalMap[nexTex]);
		m->fancyEffectsShader->Uniform(str_waviness, WaterMgr->m_Waviness);
		m->fancyEffectsShader->Uniform(str_repeatScale, 1.0f / repeatPeriod);
		m->fancyEffectsShader->Uniform(str_time, (float)time);
		m->fancyEffectsShader->Uniform(str_windAngle, (float)WaterMgr->m_WindAngle);
		m->fancyEffectsShader->Uniform(str_screenSize, (float)g_Renderer.GetWidth(), (float)g_Renderer.GetHeight(), 0.0f, 0.0f);
		m->fancyEffectsShader->Uniform(str_mapSize, (float)(WaterMgr->m_MapSize));

		if (WaterMgr->m_WaterType == L"clap")
		{
			m->fancyEffectsShader->Uniform(str_waveParams1, 30.0f,1.5f,20.0f,0.03f);
			m->fancyEffectsShader->Uniform(str_waveParams2, 0.5f,0.0f,0.0f,0.0f);
		}
		else if (WaterMgr->m_WaterType == L"lake")
		{
			m->fancyEffectsShader->Uniform(str_waveParams1, 8.5f,1.5f,15.0f,0.03f);
			m->fancyEffectsShader->Uniform(str_waveParams2, 0.2f,0.0f,0.0f,0.07f);
		}
		else
		{
			m->fancyEffectsShader->Uniform(str_waveParams1, 15.0f,0.8f,10.0f,0.1f);
			m->fancyEffectsShader->Uniform(str_waveParams2, 0.3f,0.0f,0.1f,0.3f);
		}
		
		std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];
		for (size_t i = 0; i < visiblePatches.size(); ++i)
		{
			CPatchRData* data = visiblePatches[i];
			data->RenderWater(m->fancyEffectsShader);
		}
		
		m->fancyEffectsShader->Unbind();
		
		// rebind post-processing frambuffer.
		pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	
	m->fancyWaterShader->Bind();
		
	const CCamera& camera = g_Renderer.GetViewCamera();
	CVector3D camPos = camera.m_Orientation.GetTranslation();

	m->fancyWaterShader->BindTexture(str_normalMap, WaterMgr->m_NormalMap[curTex]);
	m->fancyWaterShader->BindTexture(str_normalMap2, WaterMgr->m_NormalMap[nexTex]);
	
	if (WaterMgr->m_WaterFancyEffects)
		m->fancyWaterShader->BindTexture(str_waterEffectsTex, WaterMgr->m_FancyTexture);
	if (WaterMgr->m_WaterRealDepth)
		m->fancyWaterShader->BindTexture(str_depthTex, WaterMgr->m_depthTT);
	if (WaterMgr->m_WaterReflection)
		m->fancyWaterShader->BindTexture(str_reflectionMap, WaterMgr->m_ReflectionTexture);
	if (WaterMgr->m_WaterRefraction)
		m->fancyWaterShader->BindTexture(str_refractionMap, WaterMgr->m_RefractionTexture);

	m->fancyWaterShader->BindTexture(str_losMap, losTexture.GetTextureSmooth());
	
	m->fancyWaterShader->BindTexture(str_skyCube, g_Renderer.GetSkyManager()->GetSkyCube());

	const CLightEnv& lightEnv = g_Renderer.GetLightEnv();

	// TODO: only bind what's really needed for that.
	m->fancyWaterShader->Uniform(str_sunDir, lightEnv.GetSunDir());
	m->fancyWaterShader->Uniform(str_sunColor, lightEnv.m_SunColor.X);
	m->fancyWaterShader->Uniform(str_color, WaterMgr->m_WaterColor);
	m->fancyWaterShader->Uniform(str_tint, WaterMgr->m_WaterTint);
	m->fancyWaterShader->Uniform(str_waviness, WaterMgr->m_Waviness);
	m->fancyWaterShader->Uniform(str_murkiness, WaterMgr->m_Murkiness);
	m->fancyWaterShader->Uniform(str_windAngle, WaterMgr->m_WindAngle);
	m->fancyWaterShader->Uniform(str_repeatScale, 1.0f / repeatPeriod);
	m->fancyWaterShader->Uniform(str_reflectionMatrix, WaterMgr->m_ReflectionMatrix);
	m->fancyWaterShader->Uniform(str_refractionMatrix, WaterMgr->m_RefractionMatrix);
	m->fancyWaterShader->Uniform(str_losMatrix, losTexture.GetTextureMatrix());
	m->fancyWaterShader->Uniform(str_cameraPos, camPos);
	m->fancyWaterShader->Uniform(str_fogColor, lightEnv.m_FogColor);
	m->fancyWaterShader->Uniform(str_fogParams, lightEnv.m_FogFactor, lightEnv.m_FogMax, 0.f, 0.f);
	m->fancyWaterShader->Uniform(str_time, (float)time);
	m->fancyWaterShader->Uniform(str_screenSize, (float)g_Renderer.GetWidth(), (float)g_Renderer.GetHeight(), 0.0f, 0.0f);
	
	if (!WaterMgr->m_WaterFancyEffects)
	{
		if (WaterMgr->m_WaterType == L"clap")
		{
			m->fancyWaterShader->Uniform(str_waveParams1, 30.0f,1.5f,20.0f,0.03f);
			m->fancyWaterShader->Uniform(str_waveParams2, 0.5f,0.0f,0.0f,0.0f);
		}
		else if (WaterMgr->m_WaterType == L"lake")
		{
			m->fancyWaterShader->Uniform(str_waveParams1, 8.5f,1.5f,15.0f,0.03f);
			m->fancyWaterShader->Uniform(str_waveParams2, 0.2f,0.0f,0.0f,0.07f);
		}
		else
		{
			m->fancyWaterShader->Uniform(str_waveParams1, 15.0f,0.8f,10.0f,0.1f);
			m->fancyWaterShader->Uniform(str_waveParams2, 0.3f,0.0f,0.1f,0.3f);
		}
	}
	
	if (shadow && WaterMgr->m_WaterShadows)
	{
		m->fancyWaterShader->BindTexture(str_shadowTex, shadow->GetTexture());
		m->fancyWaterShader->Uniform(str_shadowTransform, shadow->GetTextureMatrix());
		int width = shadow->GetWidth();
		int height = shadow->GetHeight();
		m->fancyWaterShader->Uniform(str_shadowScale, width, height, 1.0f / width, 1.0f / height);
	}

	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];
	for (size_t i = 0; i < visiblePatches.size(); ++i)
	{
		CPatchRData* data = visiblePatches[i];
		data->RenderWater(m->fancyWaterShader);
	}

	m->fancyWaterShader->Unbind();

	pglActiveTextureARB(GL_TEXTURE0);
	pglDeleteFramebuffersEXT(1, &FramebufferName);

	glDisable(GL_BLEND);

	return true;
}

void TerrainRenderer::RenderSimpleWater(int cullGroup)
{
#if CONFIG2_GLES
	UNUSED2(cullGroup);
#else
	PROFILE3_GPU("simple water");

	WaterManager* WaterMgr = g_Renderer.GetWaterManager();
	CLOSTexture& losTexture = g_Game->GetView()->GetLOSTexture();

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

	// Set up texture environment to multiply vertex RGB by texture RGB.
	GLfloat waterColor[4] = { WaterMgr->m_WaterColor.r, WaterMgr->m_WaterColor.g, WaterMgr->m_WaterColor.b, 1.0f };
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, waterColor);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);


	// Multiply by LOS texture
	losTexture.BindTexture(1);
	CMatrix3D losMatrix = losTexture.GetTextureMatrix();
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

	CShaderProgramPtr dummyShader = g_Renderer.GetShaderManager().LoadProgram("fixed:dummy", CShaderDefines());
	dummyShader->Bind();

	glEnableClientState(GL_VERTEX_ARRAY);

	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];
	for (size_t i = 0; i < visiblePatches.size(); ++i)
	{
		CPatchRData* data = visiblePatches[i];
		data->RenderWater(dummyShader, true);
	}

	glDisableClientState(GL_VERTEX_ARRAY);

	dummyShader->Unbind();

	g_Renderer.BindTexture(1, 0);

	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	pglActiveTextureARB(GL_TEXTURE0_ARB);

	// Clean up the texture matrix and blend mode
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glDisable(GL_TEXTURE_2D);
#endif
}

///////////////////////////////////////////////////////////////////
// Render water that is part of the terrain
void TerrainRenderer::RenderWater(const CShaderDefines& context, int cullGroup, ShadowMap* shadow)
{
	WaterManager* WaterMgr = g_Renderer.GetWaterManager();

	WaterMgr->UpdateQuality();

	if (!WaterMgr->WillRenderFancyWater())
		RenderSimpleWater(cullGroup);
	else
		RenderFancyWater(context, cullGroup, shadow);
}

void TerrainRenderer::RenderPriorities(int cullGroup)
{
	PROFILE("priorities");

	ENSURE(m->phase == Phase_Render);

	CShaderTechniquePtr tech = g_Renderer.GetShaderManager().LoadEffect(str_gui_text);
	tech->BeginPass();
	CTextRenderer textRenderer(tech->GetShader());

	textRenderer.Font(CStrIntern("mono-stroke-10"));
	textRenderer.Color(1.0f, 1.0f, 0.0f);

	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];
	for (size_t i = 0; i < visiblePatches.size(); ++i)
		visiblePatches[i]->RenderPriorities(textRenderer);

	textRenderer.Render();
	tech->EndPass();
}
