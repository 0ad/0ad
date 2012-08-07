/* Copyright (C) 2012 Wildfire Games.
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
#include "graphics/TerritoryTexture.h"
#include "graphics/TextRenderer.h"

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
	CShaderProgramPtr fancyWaterShader;
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
	for (std::vector<CPatchRData*>::iterator it = m->visiblePatches.begin(); it != m->visiblePatches.end(); ++it)
	{
		if (frustum->IsBoxVisible(CVector3D(0, 0, 0), (*it)->GetPatch()->GetWorldBounds()))
			m->filteredPatches.push_back(*it);
	}

	m->filteredDecals.clear();
	for (std::vector<CDecalRData*>::iterator it = m->visibleDecals.begin(); it != m->visibleDecals.end(); ++it)
	{
		if (frustum->IsBoxVisible(CVector3D(0, 0, 0), (*it)->GetDecal()->GetWorldBounds()))
			m->filteredDecals.push_back(*it);
	}

	return !m->filteredPatches.empty() || !m->filteredDecals.empty();
}

///////////////////////////////////////////////////////////////////
// Full-featured terrain rendering with blending and everything
void TerrainRenderer::RenderTerrain(bool filtered)
{
#if CONFIG2_GLES
	UNUSED2(filtered);
#else
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = filtered ? m->filteredPatches : m->visiblePatches;
	std::vector<CDecalRData*>& visibleDecals = filtered ? m->filteredDecals : m->visibleDecals;
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

void TerrainRenderer::RenderTerrainOverlayTexture(CMatrix3D& textureMatrix)
{
#if CONFIG2_GLES
#warning TODO: implement TerrainRenderer::RenderTerrainOverlayTexture for GLES
	UNUSED2(textureMatrix);
#else
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches;

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
	for (size_t i = 0; i < m->visiblePatches.size(); ++i)
	{
		CPatchRData* data = m->visiblePatches[i];
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
	shader->Uniform("transform", g_Renderer.GetViewCamera().GetViewProjection());
	shader->Uniform("cameraPos", g_Renderer.GetViewCamera().GetOrientation().GetTranslation());

	const CLightEnv& lightEnv = g_Renderer.GetLightEnv();

	if (shadow)
	{
		shader->BindTexture("shadowTex", shadow->GetTexture());
		shader->Uniform("shadowTransform", shadow->GetTextureMatrix());
		int width = shadow->GetWidth();
		int height = shadow->GetHeight();
		shader->Uniform("shadowScale", width, height, 1.0f / width, 1.0f / height);
	}

	shader->Uniform("ambient", lightEnv.m_UnitsAmbientColor);
	shader->Uniform("sunDir", lightEnv.GetSunDir());
	shader->Uniform("sunColor", lightEnv.m_SunColor);

	CLOSTexture& los = g_Renderer.GetScene().GetLOSTexture();
	shader->BindTexture("losTex", los.GetTextureSmooth());
	shader->Uniform("losTransform", los.GetTextureMatrix()[0], los.GetTextureMatrix()[12], 0.f, 0.f);

	shader->Uniform("ambient", lightEnv.m_TerrainAmbientColor);
	shader->Uniform("sunColor", lightEnv.m_SunColor);
}

void TerrainRenderer::RenderTerrainShader(const CShaderDefines& context, ShadowMap* shadow, bool filtered)
{
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = filtered ? m->filteredPatches : m->visiblePatches;
	std::vector<CDecalRData*>& visibleDecals = filtered ? m->filteredDecals : m->visibleDecals;
	if (visiblePatches.empty() && visibleDecals.empty())
		return;

	CShaderManager& shaderManager = g_Renderer.GetShaderManager();

	// render the solid black sides of the map first
	CShaderTechniquePtr techSolid = g_Renderer.GetShaderManager().LoadEffect("gui_solid");
	techSolid->BeginPass();
	CShaderProgramPtr shaderSolid = techSolid->GetShader();
	shaderSolid->Uniform("transform", g_Renderer.GetViewCamera().GetViewProjection());
	shaderSolid->Uniform("color", 0.0f, 0.0f, 0.0f, 1.0f);

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
void TerrainRenderer::RenderPatches(bool filtered)
{
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = filtered ? m->filteredPatches : m->visiblePatches;
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
void TerrainRenderer::RenderOutlines(bool filtered)
{
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = filtered ? m->filteredPatches : m->visiblePatches;
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
CBoundingBoxAligned TerrainRenderer::ScissorWater(const CMatrix3D &viewproj)
{
	CBoundingBoxAligned scissor;
	for (size_t i = 0; i < m->visiblePatches.size(); ++i)
	{
		CPatchRData* data = m->visiblePatches[i];
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
bool TerrainRenderer::RenderFancyWater()
{
	PROFILE3_GPU("fancy water");

	// If we're using fancy water, make sure its shader is loaded
	if (!m->fancyWaterShader)
	{
		m->fancyWaterShader = g_Renderer.GetShaderManager().LoadProgram("glsl/water_high", CShaderDefines());
		if (!m->fancyWaterShader)
		{
			LOGERROR(L"Failed to load water shader. Falling back to non-fancy water.\n");
			g_Renderer.m_Options.m_FancyWater = false;
			return false;
		}
	}

	WaterManager* WaterMgr = g_Renderer.GetWaterManager();
	CLOSTexture& losTexture = g_Renderer.GetScene().GetLOSTexture();
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	double time = WaterMgr->m_WaterTexTimer;
	double period = 1.6;
	int curTex = (int)(time*60/period) % 60;

	m->fancyWaterShader->Bind();

	m->fancyWaterShader->BindTexture("normalMap", WaterMgr->m_NormalMap[curTex]);

	// Shift the texture coordinates by these amounts to make the water "flow"
	float tx = -fmod(time, 81.0)/81.0;
	float ty = -fmod(time, 34.0)/34.0;
	float repeatPeriod = WaterMgr->m_RepeatPeriod;

	const CCamera& camera = g_Renderer.GetViewCamera();
	CVector3D camPos = camera.m_Orientation.GetTranslation();

	// Bind reflection and refraction textures
	m->fancyWaterShader->BindTexture("reflectionMap", WaterMgr->m_ReflectionTexture);
	m->fancyWaterShader->BindTexture("refractionMap", WaterMgr->m_RefractionTexture);

	m->fancyWaterShader->BindTexture("losMap", losTexture.GetTextureSmooth());

	const CLightEnv& lightEnv = g_Renderer.GetLightEnv();
	m->fancyWaterShader->Uniform("ambient", lightEnv.m_TerrainAmbientColor);
	m->fancyWaterShader->Uniform("sunDir", lightEnv.GetSunDir());
	m->fancyWaterShader->Uniform("sunColor", lightEnv.m_SunColor.X);
	m->fancyWaterShader->Uniform("shininess", WaterMgr->m_Shininess);
	m->fancyWaterShader->Uniform("specularStrength", WaterMgr->m_SpecularStrength);
	m->fancyWaterShader->Uniform("waviness", WaterMgr->m_Waviness);
	m->fancyWaterShader->Uniform("murkiness", WaterMgr->m_Murkiness);
	m->fancyWaterShader->Uniform("fullDepth", WaterMgr->m_WaterFullDepth);
	m->fancyWaterShader->Uniform("tint", WaterMgr->m_WaterTint);
	m->fancyWaterShader->Uniform("reflectionTintStrength", WaterMgr->m_ReflectionTintStrength);
	m->fancyWaterShader->Uniform("reflectionTint", WaterMgr->m_ReflectionTint);
	m->fancyWaterShader->Uniform("translation", tx, ty);
	m->fancyWaterShader->Uniform("repeatScale", 1.0f / repeatPeriod);
	m->fancyWaterShader->Uniform("reflectionMatrix", WaterMgr->m_ReflectionMatrix);
	m->fancyWaterShader->Uniform("refractionMatrix", WaterMgr->m_RefractionMatrix);
	m->fancyWaterShader->Uniform("losMatrix", losTexture.GetTextureMatrix());
	m->fancyWaterShader->Uniform("cameraPos", camPos);

	for (size_t i = 0; i < m->visiblePatches.size(); ++i)
	{
		CPatchRData* data = m->visiblePatches[i];
		data->RenderWater(m->fancyWaterShader);
	}

	m->fancyWaterShader->Unbind();

	pglActiveTextureARB(GL_TEXTURE0);

	glDisable(GL_BLEND);

	return true;
}

void TerrainRenderer::RenderSimpleWater()
{
#if !CONFIG2_GLES
	PROFILE3_GPU("simple water");

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
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	CShaderProgramPtr dummyShader = g_Renderer.GetShaderManager().LoadProgram("fixed:dummy", CShaderDefines());
	dummyShader->Bind();

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	for (size_t i = 0; i < m->visiblePatches.size(); ++i)
	{
		CPatchRData* data = m->visiblePatches[i];
		data->RenderWater(dummyShader);
	}

	glDisableClientState(GL_COLOR_ARRAY);
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

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
#endif
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
	PROFILE("priorities");

	ENSURE(m->phase == Phase_Render);

	CShaderTechniquePtr tech = g_Renderer.GetShaderManager().LoadEffect("gui_text");
	tech->BeginPass();
	CTextRenderer textRenderer(tech->GetShader());

	textRenderer.Font(L"mono-stroke-10");
	textRenderer.Color(1.0f, 1.0f, 0.0f);

	for (size_t i = 0; i < m->visiblePatches.size(); ++i)
		m->visiblePatches[i]->RenderPriorities(textRenderer);

	textRenderer.Render();
	tech->EndPass();
}
