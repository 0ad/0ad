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
#include "renderer/backend/IDevice.h"
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
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	int cullGroup, CMatrix3D& textureMatrix,
	Renderer::Backend::ITexture* texture)
{
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];

	CShaderTechniquePtr debugOverlayTech =
		g_Renderer.GetShaderManager().LoadEffect(str_debug_overlay);
	deviceCommandContext->SetGraphicsPipelineState(
		debugOverlayTech->GetGraphicsPipelineStateDesc());
	deviceCommandContext->BeginPass();
	Renderer::Backend::IShaderProgram* debugOverlayShader = debugOverlayTech->GetShader();

	deviceCommandContext->SetTexture(
		debugOverlayShader->GetBindingSlot(str_baseTex), texture);
	const CMatrix3D transform =
		g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection();
	deviceCommandContext->SetUniform(
		debugOverlayShader->GetBindingSlot(str_transform), transform.AsFloatArray());
	deviceCommandContext->SetUniform(
		debugOverlayShader->GetBindingSlot(str_textureTransform), textureMatrix.AsFloatArray());
	CPatchRData::RenderStreams(deviceCommandContext, visiblePatches, true);

	// To make the overlay visible over water, render an additional map-sized
	// water-height patch.
	CBoundingBoxAligned waterBounds;
	for (CPatchRData* data : visiblePatches)
		waterBounds += data->GetWaterBounds();
	if (!waterBounds.IsEmpty())
	{
		// Add a delta to avoid z-fighting.
		const float height = g_Renderer.GetSceneRenderer().GetWaterManager().m_WaterHeight + 0.05f;
		const float waterPos[] =
		{
			waterBounds[0].X, height, waterBounds[0].Z,
			waterBounds[1].X, height, waterBounds[0].Z,
			waterBounds[1].X, height, waterBounds[1].Z,
			waterBounds[0].X, height, waterBounds[0].Z,
			waterBounds[1].X, height, waterBounds[1].Z,
			waterBounds[0].X, height, waterBounds[1].Z
		};

		deviceCommandContext->SetVertexAttributeFormat(
			Renderer::Backend::VertexAttributeStream::POSITION,
			Renderer::Backend::Format::R32G32B32_SFLOAT, 0, 0, 0);
		deviceCommandContext->SetVertexAttributeFormat(
			Renderer::Backend::VertexAttributeStream::UV0,
			Renderer::Backend::Format::R32G32B32_SFLOAT, 0, 0, 0);

		deviceCommandContext->SetVertexBufferData(0, waterPos);

		deviceCommandContext->Draw(0, 6);
	}

	deviceCommandContext->EndPass();
}


///////////////////////////////////////////////////////////////////

/**
 * Set up all the uniforms for a shader pass.
 */
void TerrainRenderer::PrepareShader(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	Renderer::Backend::IShaderProgram* shader, ShadowMap* shadow)
{
	CSceneRenderer& sceneRenderer = g_Renderer.GetSceneRenderer();

	const CMatrix3D transform = sceneRenderer.GetViewCamera().GetViewProjection();
	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_transform), transform.AsFloatArray());
	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_cameraPos),
		sceneRenderer.GetViewCamera().GetOrientation().GetTranslation().AsFloatArray());

	const CLightEnv& lightEnv = sceneRenderer.GetLightEnv();

	if (shadow)
		shadow->BindTo(deviceCommandContext, shader);

	CLOSTexture& los = sceneRenderer.GetScene().GetLOSTexture();
	deviceCommandContext->SetTexture(
		shader->GetBindingSlot(str_losTex), los.GetTextureSmooth());
	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_losTransform),
		los.GetTextureMatrix()[0], los.GetTextureMatrix()[12]);

	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_ambient),
		lightEnv.m_AmbientColor.AsFloatArray());
	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_sunColor),
		lightEnv.m_SunColor.AsFloatArray());
	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_sunDir),
		lightEnv.GetSunDir().AsFloatArray());

	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_fogColor),
		lightEnv.m_FogColor.AsFloatArray());
	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_fogParams),
		lightEnv.m_FogFactor, lightEnv.m_FogMax);
}

void TerrainRenderer::RenderTerrainShader(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	const CShaderDefines& context, int cullGroup, ShadowMap* shadow)
{
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];
	std::vector<CDecalRData*>& visibleDecals = m->visibleDecals[cullGroup];
	if (visiblePatches.empty() && visibleDecals.empty())
		return;

	// render the solid black sides of the map first
	CShaderTechniquePtr techSolid = g_Renderer.GetShaderManager().LoadEffect(str_solid);
	Renderer::Backend::GraphicsPipelineStateDesc solidPipelineStateDesc =
		techSolid->GetGraphicsPipelineStateDesc();
	solidPipelineStateDesc.rasterizationState.cullMode = Renderer::Backend::CullMode::NONE;
	deviceCommandContext->SetGraphicsPipelineState(solidPipelineStateDesc);
	deviceCommandContext->BeginPass();

	Renderer::Backend::IShaderProgram* shaderSolid = techSolid->GetShader();
	const CMatrix3D transform =
		g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection();
	deviceCommandContext->SetUniform(
		shaderSolid->GetBindingSlot(str_transform), transform.AsFloatArray());
	deviceCommandContext->SetUniform(
		shaderSolid->GetBindingSlot(str_color), 0.0f, 0.0f, 0.0f, 1.0f);

	CPatchRData::RenderSides(deviceCommandContext, visiblePatches);

	deviceCommandContext->EndPass();

	CPatchRData::RenderBases(deviceCommandContext, visiblePatches, context, shadow);

	// render blend passes for each patch
	CPatchRData::RenderBlends(deviceCommandContext, visiblePatches, context, shadow);

	CDecalRData::RenderDecals(deviceCommandContext, visibleDecals, context, shadow);
}

///////////////////////////////////////////////////////////////////
// Render un-textured patches as polygons
void TerrainRenderer::RenderPatches(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	int cullGroup, const CShaderDefines& defines, const CColor& color)
{
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];
	if (visiblePatches.empty())
		return;

	GPU_SCOPED_LABEL(deviceCommandContext, "Render terrain patches");

	CShaderTechniquePtr solidTech = g_Renderer.GetShaderManager().LoadEffect(str_terrain_solid, defines);
	deviceCommandContext->SetGraphicsPipelineState(
		solidTech->GetGraphicsPipelineStateDesc());
	deviceCommandContext->BeginPass();

	Renderer::Backend::IShaderProgram* solidShader = solidTech->GetShader();

	const CMatrix3D transform =
		g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection();
	deviceCommandContext->SetUniform(
		solidShader->GetBindingSlot(str_transform), transform.AsFloatArray());
	deviceCommandContext->SetUniform(
		solidShader->GetBindingSlot(str_color), color.AsFloatArray());

	CPatchRData::RenderStreams(deviceCommandContext, visiblePatches, false);
	deviceCommandContext->EndPass();
}


///////////////////////////////////////////////////////////////////
// Render outlines of submitted patches as lines
void TerrainRenderer::RenderOutlines(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	int cullGroup)
{
	ENSURE(m->phase == Phase_Render);

	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];
	if (visiblePatches.empty())
		return;

	GPU_SCOPED_LABEL(deviceCommandContext, "Render terrain outlines");

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
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	const CShaderDefines& context, int cullGroup, ShadowMap* shadow)
{
	PROFILE3_GPU("fancy water");
	GPU_SCOPED_LABEL(deviceCommandContext, "Render fancy water");

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

	deviceCommandContext->SetGraphicsPipelineState(
		m->fancyWaterTech->GetGraphicsPipelineStateDesc());
	deviceCommandContext->BeginPass();
	Renderer::Backend::IShaderProgram* fancyWaterShader = m->fancyWaterTech->GetShader();

	const CCamera& camera = g_Renderer.GetSceneRenderer().GetViewCamera();

	const double period = 8.0;
	// TODO: move uploading to a prepare function during loading.
	const CTexturePtr& currentNormalTexture = waterManager.m_NormalMap[waterManager.GetCurrentTextureIndex(period)];
	const CTexturePtr& nextNormalTexture = waterManager.m_NormalMap[waterManager.GetNextTextureIndex(period)];

	currentNormalTexture->UploadBackendTextureIfNeeded(deviceCommandContext);
	nextNormalTexture->UploadBackendTextureIfNeeded(deviceCommandContext);

	deviceCommandContext->SetTexture(
		fancyWaterShader->GetBindingSlot(str_normalMap),
		currentNormalTexture->GetBackendTexture());
	deviceCommandContext->SetTexture(
		fancyWaterShader->GetBindingSlot(str_normalMap2),
		nextNormalTexture->GetBackendTexture());

	if (waterManager.m_WaterFancyEffects)
	{
		deviceCommandContext->SetTexture(
			fancyWaterShader->GetBindingSlot(str_waterEffectsTex),
			waterManager.m_FancyTexture.get());
	}

	if (waterManager.m_WaterRefraction && waterManager.m_WaterRealDepth)
	{
		deviceCommandContext->SetTexture(
			fancyWaterShader->GetBindingSlot(str_depthTex),
			waterManager.m_RefrFboDepthTexture.get());
		deviceCommandContext->SetUniform(
			fancyWaterShader->GetBindingSlot(str_projInvTransform),
			waterManager.m_RefractionProjInvMatrix.AsFloatArray());
		deviceCommandContext->SetUniform(
			fancyWaterShader->GetBindingSlot(str_viewInvTransform),
			waterManager.m_RefractionViewInvMatrix.AsFloatArray());
	}

	if (waterManager.m_WaterRefraction)
	{
		deviceCommandContext->SetTexture(
			fancyWaterShader->GetBindingSlot(str_refractionMap),
			waterManager.m_RefractionTexture.get());
	}
	if (waterManager.m_WaterReflection)
	{
		deviceCommandContext->SetTexture(
			fancyWaterShader->GetBindingSlot(str_reflectionMap),
			waterManager.m_ReflectionTexture.get());
	}
	deviceCommandContext->SetTexture(
		fancyWaterShader->GetBindingSlot(str_losTex), losTexture.GetTextureSmooth());

	const CLightEnv& lightEnv = sceneRenderer.GetLightEnv();

	const CMatrix3D transform = sceneRenderer.GetViewCamera().GetViewProjection();
	deviceCommandContext->SetUniform(
		fancyWaterShader->GetBindingSlot(str_transform), transform.AsFloatArray());

	deviceCommandContext->SetTexture(
		fancyWaterShader->GetBindingSlot(str_skyCube),
		sceneRenderer.GetSkyManager().GetSkyCube());
	// TODO: check that this rotates in the right direction.
	CMatrix3D skyBoxRotation;
	skyBoxRotation.SetIdentity();
	skyBoxRotation.RotateY(M_PI + lightEnv.GetRotation());
	deviceCommandContext->SetUniform(
		fancyWaterShader->GetBindingSlot(str_skyBoxRot),
		skyBoxRotation.AsFloatArray());

	if (waterManager.m_WaterRefraction)
	{
		deviceCommandContext->SetUniform(
			fancyWaterShader->GetBindingSlot(str_refractionMatrix),
			waterManager.m_RefractionMatrix.AsFloatArray());
	}
	if (waterManager.m_WaterReflection)
	{
		deviceCommandContext->SetUniform(
			fancyWaterShader->GetBindingSlot(str_reflectionMatrix),
			waterManager.m_ReflectionMatrix.AsFloatArray());
	}

	deviceCommandContext->SetUniform(
		fancyWaterShader->GetBindingSlot(str_ambient), lightEnv.m_AmbientColor.AsFloatArray());
	deviceCommandContext->SetUniform(
		fancyWaterShader->GetBindingSlot(str_sunDir), lightEnv.GetSunDir().AsFloatArray());
	deviceCommandContext->SetUniform(
		fancyWaterShader->GetBindingSlot(str_sunColor), lightEnv.m_SunColor.AsFloatArray());
	deviceCommandContext->SetUniform(
		fancyWaterShader->GetBindingSlot(str_color), waterManager.m_WaterColor.AsFloatArray());
	deviceCommandContext->SetUniform(
		fancyWaterShader->GetBindingSlot(str_tint), waterManager.m_WaterTint.AsFloatArray());
	deviceCommandContext->SetUniform(
		fancyWaterShader->GetBindingSlot(str_waviness), waterManager.m_Waviness);
	deviceCommandContext->SetUniform(
		fancyWaterShader->GetBindingSlot(str_murkiness), waterManager.m_Murkiness);
	deviceCommandContext->SetUniform(
		fancyWaterShader->GetBindingSlot(str_windAngle), waterManager.m_WindAngle);
	deviceCommandContext->SetUniform(
		fancyWaterShader->GetBindingSlot(str_repeatScale), 1.0f / repeatPeriod);

	deviceCommandContext->SetUniform(
		fancyWaterShader->GetBindingSlot(str_losTransform),
		losTexture.GetTextureMatrix()[0], losTexture.GetTextureMatrix()[12]);
	deviceCommandContext->SetUniform(
		fancyWaterShader->GetBindingSlot(str_cameraPos),
		camera.GetOrientation().GetTranslation().AsFloatArray());

	deviceCommandContext->SetUniform(
		fancyWaterShader->GetBindingSlot(str_fogColor),
		lightEnv.m_FogColor.AsFloatArray());
	deviceCommandContext->SetUniform(
		fancyWaterShader->GetBindingSlot(str_fogParams),
		lightEnv.m_FogFactor, lightEnv.m_FogMax);
	deviceCommandContext->SetUniform(
		fancyWaterShader->GetBindingSlot(str_time), static_cast<float>(time));
	deviceCommandContext->SetUniform(
		fancyWaterShader->GetBindingSlot(str_screenSize),
		static_cast<float>(g_Renderer.GetWidth()),
		static_cast<float>(g_Renderer.GetHeight()));

	if (waterManager.m_WaterType == L"clap")
	{
		deviceCommandContext->SetUniform(
			fancyWaterShader->GetBindingSlot(str_waveParams1),
			30.0f, 1.5f, 20.0f, 0.03f);
		deviceCommandContext->SetUniform(
			fancyWaterShader->GetBindingSlot(str_waveParams2),
			0.5f, 0.0f, 0.0f, 0.0f);
	}
	else if (waterManager.m_WaterType == L"lake")
	{
		deviceCommandContext->SetUniform(
			fancyWaterShader->GetBindingSlot(str_waveParams1),
			8.5f, 1.5f, 15.0f, 0.03f);
		deviceCommandContext->SetUniform(
			fancyWaterShader->GetBindingSlot(str_waveParams2),
			0.2f, 0.0f, 0.0f, 0.07f);
	}
	else
	{
		deviceCommandContext->SetUniform(
			fancyWaterShader->GetBindingSlot(str_waveParams1),
			15.0f, 0.8f, 10.0f, 0.1f);
		deviceCommandContext->SetUniform(
			fancyWaterShader->GetBindingSlot(str_waveParams2),
			0.3f, 0.0f, 0.1f, 0.3f);
	}

	if (shadow)
		shadow->BindTo(deviceCommandContext, fancyWaterShader);

	for (CPatchRData* data : m->visiblePatches[cullGroup])
	{
		data->RenderWaterSurface(deviceCommandContext, true);
		if (waterManager.m_WaterFancyEffects)
			data->RenderWaterShore(deviceCommandContext);
	}
	deviceCommandContext->EndPass();

	return true;
}

void TerrainRenderer::RenderSimpleWater(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	int cullGroup)
{
	PROFILE3_GPU("simple water");
	GPU_SCOPED_LABEL(deviceCommandContext, "Render Simple Water");

	const WaterManager& waterManager = g_Renderer.GetSceneRenderer().GetWaterManager();
	CLOSTexture& losTexture = g_Game->GetView()->GetLOSTexture();

	const double time = waterManager.m_WaterTexTimer;

	CShaderDefines context;
	if (g_Renderer.GetSceneRenderer().GetWaterRenderMode() == WIREFRAME)
		context.Add(str_MODE_WIREFRAME, str_1);

	CShaderTechniquePtr waterSimpleTech =
		g_Renderer.GetShaderManager().LoadEffect(str_water_simple, context);
	deviceCommandContext->SetGraphicsPipelineState(
		waterSimpleTech->GetGraphicsPipelineStateDesc());
	deviceCommandContext->BeginPass();
	Renderer::Backend::IShaderProgram* waterSimpleShader = waterSimpleTech->GetShader();

	const CTexturePtr& waterTexture = waterManager.m_WaterTexture[waterManager.GetCurrentTextureIndex(1.6)];
	waterTexture->UploadBackendTextureIfNeeded(deviceCommandContext);

	deviceCommandContext->SetTexture(
		waterSimpleShader->GetBindingSlot(str_baseTex), waterTexture->GetBackendTexture());
	deviceCommandContext->SetTexture(
		waterSimpleShader->GetBindingSlot(str_losTex), losTexture.GetTextureSmooth());

	const CMatrix3D transform =
		g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection();
	deviceCommandContext->SetUniform(
		waterSimpleShader->GetBindingSlot(str_transform), transform.AsFloatArray());

	deviceCommandContext->SetUniform(
		waterSimpleShader->GetBindingSlot(str_losTransform),
		losTexture.GetTextureMatrix()[0], losTexture.GetTextureMatrix()[12]);
	deviceCommandContext->SetUniform(
		waterSimpleShader->GetBindingSlot(str_time), static_cast<float>(time));
	deviceCommandContext->SetUniform(
		waterSimpleShader->GetBindingSlot(str_color), waterManager.m_WaterColor.AsFloatArray());

	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];
	for (size_t i = 0; i < visiblePatches.size(); ++i)
	{
		CPatchRData* data = visiblePatches[i];
		data->RenderWaterSurface(deviceCommandContext, false);
	}

	deviceCommandContext->EndPass();
}

///////////////////////////////////////////////////////////////////
// Render water that is part of the terrain
void TerrainRenderer::RenderWater(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	const CShaderDefines& context, int cullGroup, ShadowMap* shadow)
{
	const WaterManager& waterManager = g_Renderer.GetSceneRenderer().GetWaterManager();

	if (!waterManager.WillRenderFancyWater())
		RenderSimpleWater(deviceCommandContext, cullGroup);
	else
		RenderFancyWater(deviceCommandContext, context, cullGroup, shadow);
}

void TerrainRenderer::RenderWaterFoamOccluders(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	int cullGroup)
{
	CSceneRenderer& sceneRenderer = g_Renderer.GetSceneRenderer();
	const WaterManager& waterManager = sceneRenderer.GetWaterManager();
	if (!waterManager.WillRenderFancyWater())
		return;

	GPU_SCOPED_LABEL(deviceCommandContext, "Render water foam occluders");

	// Render normals and foam to a framebuffer if we're using fancy effects.
	deviceCommandContext->SetFramebuffer(waterManager.m_FancyEffectsFramebuffer.get());

	// Overwrite waves that would be behind the ground.
	CShaderTechniquePtr dummyTech = g_Renderer.GetShaderManager().LoadEffect(str_solid);
	Renderer::Backend::GraphicsPipelineStateDesc pipelineStateDesc =
		dummyTech->GetGraphicsPipelineStateDesc();
	pipelineStateDesc.depthStencilState.depthTestEnabled = true;
	pipelineStateDesc.rasterizationState.cullMode = Renderer::Backend::CullMode::NONE;
	deviceCommandContext->SetGraphicsPipelineState(pipelineStateDesc);
	deviceCommandContext->BeginPass();

	Renderer::Backend::IShaderProgram* dummyShader = dummyTech->GetShader();

	const CMatrix3D transform = sceneRenderer.GetViewCamera().GetViewProjection();
	deviceCommandContext->SetUniform(
		dummyShader->GetBindingSlot(str_transform), transform.AsFloatArray());
	deviceCommandContext->SetUniform(
		dummyShader->GetBindingSlot(str_color), 0.0f, 0.0f, 0.0f, 0.0f);

	for (CPatchRData* data : m->visiblePatches[cullGroup])
		data->RenderWaterShore(deviceCommandContext);

	deviceCommandContext->EndPass();

	deviceCommandContext->SetFramebuffer(
		deviceCommandContext->GetDevice()->GetCurrentBackbuffer());
}

void TerrainRenderer::RenderPriorities(CCanvas2D& canvas, int cullGroup)
{
	PROFILE("priorities");

	ENSURE(m->phase == Phase_Render);

	CTextRenderer textRenderer;
	textRenderer.SetCurrentFont(CStrIntern("mono-stroke-10"));
	textRenderer.SetCurrentColor(CColor(1.0f, 1.0f, 0.0f, 1.0f));

	std::vector<CPatchRData*>& visiblePatches = m->visiblePatches[cullGroup];
	for (size_t i = 0; i < visiblePatches.size(); ++i)
		visiblePatches[i]->RenderPriorities(textRenderer);

	canvas.DrawText(textRenderer);
}
