/* Copyright (C) 2022 Wildfire Games.
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

#include "precompiled.h"

#include "renderer/TerrainRenderer.h"

#include "graphics/Camera.h"
#include "graphics/Canvas2D.h"
#include "graphics/Decal.h"
#include "graphics/GameView.h"
#include "graphics/LightEnv.h"
#include "graphics/LOSTexture.h"
#include "graphics/Patch.h"
#include "graphics/Model.h"
#include "graphics/ShaderManager.h"
#include "graphics/TerritoryTexture.h"
#include "graphics/TextRenderer.h"
#include "graphics/TextureManager.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/CStrInternStatic.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "ps/World.h"
#include "renderer/backend/gl/Device.h"
#include "renderer/DecalRData.h"
#include "renderer/PatchRData.h"
#include "renderer/Renderer.h"
#include "renderer/RenderingOptions.h"
#include "renderer/SceneRenderer.h"
#include "renderer/ShadowMap.h"
#include "renderer/SkyManager.h"
#include "renderer/VertexArray.h"
#include "renderer/WaterManager.h"

/**
 * TerrainRenderer keeps track of which phase it is in, to detect
 * when Submit, PrepareForRendering etc. are called in the wrong order.
 */
enum Phase
{
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
	std::vector<CPatchRData*> visiblePatches[CSceneRenderer::CULL_MAX];

	/// Decals that were submitted for this frame
	std::vector<CDecalRData*> visibleDecals[CSceneRenderer::CULL_MAX];

	/// Fancy water shader
	CShaderTechniquePtr fancyWaterTech;

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

	for (int i = 0; i < CSceneRenderer::CULL_MAX; ++i)
	{
		m->visiblePatches[i].clear();
		m->visibleDecals[i].clear();
	}

	m->phase = Phase_Submit;
}

void TerrainRenderer::RenderTerrainOverlayTexture(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	int cullGroup, CMatrix3D& textureMatrix,
	Renderer::Backend::GL::CTexture* texture)
{
#if CONFIG2_GLES
#warning TODO: implement TerrainRenderer::RenderTerrainOverlayTexture for GLES
	UNUSED2(deviceCommandContext);
	UNUSED2(cullGroup);
	UNUSED2(textureMatrix);
	UNUSED2(texture);
#else
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];

	CShaderTechniquePtr debugOverlayTech =
		g_Renderer.GetShaderManager().LoadEffect(str_debug_overlay);
	debugOverlayTech->BeginPass();
	deviceCommandContext->SetGraphicsPipelineState(
		debugOverlayTech->GetGraphicsPipelineStateDesc());
	const CShaderProgramPtr& debugOverlayShader = debugOverlayTech->GetShader();

	debugOverlayShader->BindTexture(str_baseTex, texture);
	debugOverlayShader->Uniform(str_transform, g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection());
	debugOverlayShader->Uniform(str_textureTransform, textureMatrix);
	CPatchRData::RenderStreams(visiblePatches, debugOverlayShader, STREAM_POS | STREAM_POSTOUV0);

	// To make the overlay visible over water, render an additional map-sized
	// water-height patch.
	CBoundingBoxAligned waterBounds;
	for (CPatchRData* data : visiblePatches)
		waterBounds += data->GetWaterBounds();
	if (!waterBounds.IsEmpty())
	{
		// Add a delta to avoid z-fighting.
		const float height = g_Renderer.GetSceneRenderer().GetWaterManager().m_WaterHeight + 0.05f;
		const float waterPos[] = {
			waterBounds[0].X, height, waterBounds[0].Z,
			waterBounds[1].X, height, waterBounds[0].Z,
			waterBounds[0].X, height, waterBounds[1].Z,
			waterBounds[1].X, height, waterBounds[1].Z
		};

		const GLsizei stride = sizeof(float) * 3;
		debugOverlayShader->VertexPointer(3, GL_FLOAT, stride, waterPos);
		debugOverlayShader->TexCoordPointer(GL_TEXTURE0, 3, GL_FLOAT, stride, waterPos);
		debugOverlayShader->AssertPointersBound();

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	debugOverlayTech->EndPass();
#endif
}


///////////////////////////////////////////////////////////////////

/**
 * Set up all the uniforms for a shader pass.
 */
void TerrainRenderer::PrepareShader(const CShaderProgramPtr& shader, ShadowMap* shadow)
{
	CSceneRenderer& sceneRenderer = g_Renderer.GetSceneRenderer();

	shader->Uniform(str_transform, sceneRenderer.GetViewCamera().GetViewProjection());
	shader->Uniform(str_cameraPos, sceneRenderer.GetViewCamera().GetOrientation().GetTranslation());

	const CLightEnv& lightEnv = sceneRenderer.GetLightEnv();

	if (shadow)
		shadow->BindTo(shader);

	CLOSTexture& los = sceneRenderer.GetScene().GetLOSTexture();
	shader->BindTexture(str_losTex, los.GetTextureSmooth());
	shader->Uniform(str_losTransform, los.GetTextureMatrix()[0], los.GetTextureMatrix()[12], 0.f, 0.f);

	shader->Uniform(str_ambient, lightEnv.m_AmbientColor);
	shader->Uniform(str_sunColor, lightEnv.m_SunColor);
	shader->Uniform(str_sunDir, lightEnv.GetSunDir());

	shader->Uniform(str_fogColor, lightEnv.m_FogColor);
	shader->Uniform(str_fogParams, lightEnv.m_FogFactor, lightEnv.m_FogMax, 0.f, 0.f);
}

void TerrainRenderer::RenderTerrainShader(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	const CShaderDefines& context, int cullGroup, ShadowMap* shadow)
{
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];
	std::vector<CDecalRData*>& visibleDecals = m->visibleDecals[cullGroup];
	if (visiblePatches.empty() && visibleDecals.empty())
		return;

	// render the solid black sides of the map first
	CShaderTechniquePtr techSolid = g_Renderer.GetShaderManager().LoadEffect(str_solid);
	techSolid->BeginPass();
	Renderer::Backend::GraphicsPipelineStateDesc solidPipelineStateDesc =
		techSolid->GetGraphicsPipelineStateDesc();
	solidPipelineStateDesc.rasterizationState.cullMode = Renderer::Backend::CullMode::NONE;
	deviceCommandContext->SetGraphicsPipelineState(solidPipelineStateDesc);

	const CShaderProgramPtr& shaderSolid = techSolid->GetShader();
	shaderSolid->Uniform(str_transform, g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection());
	shaderSolid->Uniform(str_color, 0.0f, 0.0f, 0.0f, 1.0f);

	CPatchRData::RenderSides(visiblePatches, shaderSolid);

	techSolid->EndPass();

	CPatchRData::RenderBases(deviceCommandContext, visiblePatches, context, shadow);

	// render blend passes for each patch
	CPatchRData::RenderBlends(deviceCommandContext, visiblePatches, context, shadow);

	CDecalRData::RenderDecals(deviceCommandContext, visibleDecals, context, shadow);

	// restore OpenGL state
	deviceCommandContext->BindTexture(3, GL_TEXTURE_2D, 0);
	deviceCommandContext->BindTexture(2, GL_TEXTURE_2D, 0);
	deviceCommandContext->BindTexture(1, GL_TEXTURE_2D, 0);
}


///////////////////////////////////////////////////////////////////
// Render un-textured patches as polygons
void TerrainRenderer::RenderPatches(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	int cullGroup, const CShaderDefines& defines, const CColor& color)
{
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];
	if (visiblePatches.empty())
		return;

#if CONFIG2_GLES
	UNUSED2(deviceCommandContext);
	UNUSED2(defines);
	UNUSED2(color);
	#warning TODO: implement TerrainRenderer::RenderPatches for GLES
#else

	CShaderTechniquePtr solidTech = g_Renderer.GetShaderManager().LoadEffect(str_terrain_solid, defines);
	solidTech->BeginPass();
	deviceCommandContext->SetGraphicsPipelineState(
		solidTech->GetGraphicsPipelineStateDesc());

	const CShaderProgramPtr& solidShader = solidTech->GetShader();
	solidShader->Uniform(str_transform, g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection());
	solidShader->Uniform(str_color, color);

	CPatchRData::RenderStreams(visiblePatches, solidShader, STREAM_POS);
	solidTech->EndPass();
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

	for (size_t i = 0; i < visiblePatches.size(); ++i)
		visiblePatches[i]->RenderOutline();
}


///////////////////////////////////////////////////////////////////
// Scissor rectangle of water patches
CBoundingBoxAligned TerrainRenderer::ScissorWater(int cullGroup, const CCamera& camera)
{
	CBoundingBoxAligned scissor;
	for (const CPatchRData* data : m->visiblePatches[cullGroup])
	{
		const CBoundingBoxAligned& waterBounds = data->GetWaterBounds();
		if (waterBounds.IsEmpty())
			continue;

		const CBoundingBoxAligned waterBoundsInViewPort =
			camera.GetBoundsInViewPort(waterBounds);
		if (!waterBoundsInViewPort.IsEmpty())
			scissor += waterBoundsInViewPort;
	}
	return CBoundingBoxAligned(
		CVector3D(Clamp(scissor[0].X, -1.0f, 1.0f), Clamp(scissor[0].Y, -1.0f, 1.0f), -1.0f),
		CVector3D(Clamp(scissor[1].X, -1.0f, 1.0f), Clamp(scissor[1].Y, -1.0f, 1.0f), 1.0f));
}

// Render fancy water
bool TerrainRenderer::RenderFancyWater(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	const CShaderDefines& context, int cullGroup, ShadowMap* shadow)
{
	PROFILE3_GPU("fancy water");
	OGL_SCOPED_DEBUG_GROUP("Render Fancy Water");

	CSceneRenderer& sceneRenderer = g_Renderer.GetSceneRenderer();

	WaterManager& waterManager = sceneRenderer.GetWaterManager();
	CShaderDefines defines = context;

	// If we're using fancy water, make sure its shader is loaded
	if (!m->fancyWaterTech || waterManager.m_NeedsReloading)
	{
		if (waterManager.m_WaterRealDepth)
			defines.Add(str_USE_REAL_DEPTH, str_1);
		if (waterManager.m_WaterFancyEffects)
			defines.Add(str_USE_FANCY_EFFECTS, str_1);
		if (waterManager.m_WaterRefraction)
			defines.Add(str_USE_REFRACTION, str_1);
		if (waterManager.m_WaterReflection)
			defines.Add(str_USE_REFLECTION, str_1);

		m->fancyWaterTech = g_Renderer.GetShaderManager().LoadEffect(str_water_high, defines);

		if (!m->fancyWaterTech)
		{
			LOGERROR("Failed to load water shader. Falling back to a simple water.\n");
			waterManager.m_RenderWater = false;
			return false;
		}
		waterManager.m_NeedsReloading = false;
	}

	CLOSTexture& losTexture = sceneRenderer.GetScene().GetLOSTexture();

	// Calculating the advanced informations about Foam and all if the quality calls for it.
	/*if (WaterMgr->m_NeedInfoUpdate && (WaterMgr->m_WaterFoam || WaterMgr->m_WaterCoastalWaves))
	{
		WaterMgr->m_NeedInfoUpdate = false;
		WaterMgr->CreateSuperfancyInfo();
	}*/

	const double time = waterManager.m_WaterTexTimer;
	const float repeatPeriod = waterManager.m_RepeatPeriod;

#if !CONFIG2_GLES
	if (g_Renderer.GetSceneRenderer().GetWaterRenderMode() == WIREFRAME)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif

	m->fancyWaterTech->BeginPass();
	deviceCommandContext->SetGraphicsPipelineState(
		m->fancyWaterTech->GetGraphicsPipelineStateDesc());
	const CShaderProgramPtr& fancyWaterShader = m->fancyWaterTech->GetShader();

	const CCamera& camera = g_Renderer.GetSceneRenderer().GetViewCamera();

	const double period = 8.0;
	// TODO: move uploading to a prepare function during loading.
	const CTexturePtr& currentNormalTexture = waterManager.m_NormalMap[waterManager.GetCurrentTextureIndex(period)];
	const CTexturePtr& nextNormalTexture = waterManager.m_NormalMap[waterManager.GetNextTextureIndex(period)];
	currentNormalTexture->UploadBackendTextureIfNeeded(deviceCommandContext);
	nextNormalTexture->UploadBackendTextureIfNeeded(deviceCommandContext);
	fancyWaterShader->BindTexture(str_normalMap, currentNormalTexture->GetBackendTexture());
	fancyWaterShader->BindTexture(str_normalMap2, nextNormalTexture->GetBackendTexture());

	if (waterManager.m_WaterFancyEffects)
	{
		fancyWaterShader->BindTexture(str_waterEffectsTex, waterManager.m_FancyTexture.get());
	}

	if (waterManager.m_WaterRefraction && waterManager.m_WaterRealDepth)
	{
		fancyWaterShader->BindTexture(str_depthTex, waterManager.m_RefrFboDepthTexture.get());
		fancyWaterShader->Uniform(str_projInvTransform, waterManager.m_RefractionProjInvMatrix);
		fancyWaterShader->Uniform(str_viewInvTransform, waterManager.m_RefractionViewInvMatrix);
	}

	if (waterManager.m_WaterRefraction)
		fancyWaterShader->BindTexture(str_refractionMap, waterManager.m_RefractionTexture.get());
	if (waterManager.m_WaterReflection)
		fancyWaterShader->BindTexture(str_reflectionMap, waterManager.m_ReflectionTexture.get());
	fancyWaterShader->BindTexture(str_losTex, losTexture.GetTextureSmooth());

	const CLightEnv& lightEnv = sceneRenderer.GetLightEnv();

	fancyWaterShader->Uniform(str_transform, sceneRenderer.GetViewCamera().GetViewProjection());

	fancyWaterShader->BindTexture(str_skyCube, sceneRenderer.GetSkyManager().GetSkyCube());
	// TODO: check that this rotates in the right direction.
	CMatrix3D skyBoxRotation;
	skyBoxRotation.SetIdentity();
	skyBoxRotation.RotateY(M_PI + lightEnv.GetRotation());
	fancyWaterShader->Uniform(str_skyBoxRot, skyBoxRotation);

	if (waterManager.m_WaterRefraction)
		fancyWaterShader->Uniform(str_refractionMatrix, waterManager.m_RefractionMatrix);
	if (waterManager.m_WaterReflection)
		fancyWaterShader->Uniform(str_reflectionMatrix, waterManager.m_ReflectionMatrix);

	fancyWaterShader->Uniform(str_ambient, lightEnv.m_AmbientColor);
	fancyWaterShader->Uniform(str_sunDir, lightEnv.GetSunDir());
	fancyWaterShader->Uniform(str_sunColor, lightEnv.m_SunColor);
	fancyWaterShader->Uniform(str_color, waterManager.m_WaterColor);
	fancyWaterShader->Uniform(str_tint, waterManager.m_WaterTint);
	fancyWaterShader->Uniform(str_waviness, waterManager.m_Waviness);
	fancyWaterShader->Uniform(str_murkiness, waterManager.m_Murkiness);
	fancyWaterShader->Uniform(str_windAngle, waterManager.m_WindAngle);
	fancyWaterShader->Uniform(str_repeatScale, 1.0f / repeatPeriod);
	fancyWaterShader->Uniform(str_losTransform, losTexture.GetTextureMatrix()[0], losTexture.GetTextureMatrix()[12], 0.f, 0.f);

	fancyWaterShader->Uniform(str_cameraPos, camera.GetOrientation().GetTranslation());

	fancyWaterShader->Uniform(str_fogColor, lightEnv.m_FogColor);
	fancyWaterShader->Uniform(str_fogParams, lightEnv.m_FogFactor, lightEnv.m_FogMax, 0.f, 0.f);
	fancyWaterShader->Uniform(str_time, (float)time);
	fancyWaterShader->Uniform(str_screenSize, (float)g_Renderer.GetWidth(), (float)g_Renderer.GetHeight(), 0.0f, 0.0f);

	if (waterManager.m_WaterType == L"clap")
	{
		fancyWaterShader->Uniform(str_waveParams1, 30.0f,1.5f,20.0f,0.03f);
		fancyWaterShader->Uniform(str_waveParams2, 0.5f,0.0f,0.0f,0.0f);
	}
	else if (waterManager.m_WaterType == L"lake")
	{
		fancyWaterShader->Uniform(str_waveParams1, 8.5f,1.5f,15.0f,0.03f);
		fancyWaterShader->Uniform(str_waveParams2, 0.2f,0.0f,0.0f,0.07f);
	}
	else
	{
		fancyWaterShader->Uniform(str_waveParams1, 15.0f,0.8f,10.0f,0.1f);
		fancyWaterShader->Uniform(str_waveParams2, 0.3f,0.0f,0.1f,0.3f);
	}

	if (shadow)
		shadow->BindTo(fancyWaterShader);

	for (CPatchRData* data : m->visiblePatches[cullGroup])
	{
		data->RenderWaterSurface(fancyWaterShader, true);
		if (waterManager.m_WaterFancyEffects)
			data->RenderWaterShore(fancyWaterShader);
	}
	m->fancyWaterTech->EndPass();

#if !CONFIG2_GLES
	if (g_Renderer.GetSceneRenderer().GetWaterRenderMode() == WIREFRAME)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

	return true;
}

void TerrainRenderer::RenderSimpleWater(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	int cullGroup)
{
#if CONFIG2_GLES
	UNUSED2(deviceCommandContext);
	UNUSED2(cullGroup);
#else
	PROFILE3_GPU("simple water");
	OGL_SCOPED_DEBUG_GROUP("Render Simple Water");

	const WaterManager& waterManager = g_Renderer.GetSceneRenderer().GetWaterManager();
	CLOSTexture& losTexture = g_Game->GetView()->GetLOSTexture();

	if (g_Renderer.GetSceneRenderer().GetWaterRenderMode() == WIREFRAME)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	const double time = waterManager.m_WaterTexTimer;

	CShaderTechniquePtr waterSimpleTech =
		g_Renderer.GetShaderManager().LoadEffect(str_water_simple);
	waterSimpleTech->BeginPass();
	deviceCommandContext->SetGraphicsPipelineState(
		waterSimpleTech->GetGraphicsPipelineStateDesc());
	const CShaderProgramPtr& waterSimpleShader = waterSimpleTech->GetShader();

	const CTexturePtr& waterTexture = waterManager.m_WaterTexture[waterManager.GetCurrentTextureIndex(1.6)];
	waterTexture->UploadBackendTextureIfNeeded(deviceCommandContext);
	waterSimpleShader->BindTexture(str_baseTex, waterTexture->GetBackendTexture());
	waterSimpleShader->BindTexture(str_losTex, losTexture.GetTextureSmooth());
	waterSimpleShader->Uniform(str_transform, g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection());
	waterSimpleShader->Uniform(str_losTransform, losTexture.GetTextureMatrix()[0], losTexture.GetTextureMatrix()[12], 0.f, 0.f);
	waterSimpleShader->Uniform(str_time, static_cast<float>(time));
	waterSimpleShader->Uniform(str_color, waterManager.m_WaterColor);

	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];
	for (size_t i = 0; i < visiblePatches.size(); ++i)
	{
		CPatchRData* data = visiblePatches[i];
		data->RenderWaterSurface(waterSimpleShader, false);
	}

	deviceCommandContext->BindTexture(1, GL_TEXTURE_2D, 0);

	waterSimpleTech->EndPass();

	if (g_Renderer.GetSceneRenderer().GetWaterRenderMode() == WIREFRAME)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
}

///////////////////////////////////////////////////////////////////
// Render water that is part of the terrain
void TerrainRenderer::RenderWater(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	const CShaderDefines& context, int cullGroup, ShadowMap* shadow)
{
	const WaterManager& waterManager = g_Renderer.GetSceneRenderer().GetWaterManager();

	if (!waterManager.WillRenderFancyWater())
		RenderSimpleWater(deviceCommandContext, cullGroup);
	else
		RenderFancyWater(deviceCommandContext, context, cullGroup, shadow);
}

void TerrainRenderer::RenderWaterFoamOccluders(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	int cullGroup)
{
	CSceneRenderer& sceneRenderer = g_Renderer.GetSceneRenderer();
	const WaterManager& waterManager = sceneRenderer.GetWaterManager();
	if (!waterManager.WillRenderFancyWater())
		return;

	// Render normals and foam to a framebuffer if we're using fancy effects.
	deviceCommandContext->SetFramebuffer(waterManager.m_FancyEffectsFramebuffer.get());

	// Overwrite waves that would be behind the ground.
	CShaderTechniquePtr dummyTech = g_Renderer.GetShaderManager().LoadEffect(str_solid);
	dummyTech->BeginPass();
	Renderer::Backend::GraphicsPipelineStateDesc pipelineStateDesc =
		dummyTech->GetGraphicsPipelineStateDesc();
	pipelineStateDesc.depthStencilState.depthTestEnabled = true;
	pipelineStateDesc.rasterizationState.cullMode = Renderer::Backend::CullMode::NONE;
	deviceCommandContext->SetGraphicsPipelineState(pipelineStateDesc);
	const CShaderProgramPtr& dummyShader = dummyTech->GetShader();

	dummyShader->Uniform(str_transform, sceneRenderer.GetViewCamera().GetViewProjection());
	dummyShader->Uniform(str_color, 0.0f, 0.0f, 0.0f, 0.0f);
	for (CPatchRData* data : m->visiblePatches[cullGroup])
		data->RenderWaterShore(dummyShader);
	dummyTech->EndPass();

	deviceCommandContext->SetFramebuffer(
		deviceCommandContext->GetDevice()->GetCurrentBackbuffer());
}

void TerrainRenderer::RenderPriorities(int cullGroup)
{
	PROFILE("priorities");

	ENSURE(m->phase == Phase_Render);

	CCanvas2D canvas;
	CTextRenderer textRenderer;
	textRenderer.SetCurrentFont(CStrIntern("mono-stroke-10"));
	textRenderer.SetCurrentColor(CColor(1.0f, 1.0f, 0.0f, 1.0f));

	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];
	for (size_t i = 0; i < visiblePatches.size(); ++i)
		visiblePatches[i]->RenderPriorities(textRenderer);

	canvas.DrawText(textRenderer);
}
