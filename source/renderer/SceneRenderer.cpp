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

#include "SceneRenderer.h"

#include "graphics/Camera.h"
#include "graphics/Decal.h"
#include "graphics/GameView.h"
#include "graphics/LightEnv.h"
#include "graphics/LOSTexture.h"
#include "graphics/MaterialManager.h"
#include "graphics/MiniMapTexture.h"
#include "graphics/Model.h"
#include "graphics/ModelDef.h"
#include "graphics/ParticleManager.h"
#include "graphics/Patch.h"
#include "graphics/ShaderManager.h"
#include "graphics/TerritoryTexture.h"
#include "graphics/Terrain.h"
#include "graphics/Texture.h"
#include "graphics/TextureManager.h"
#include "maths/Matrix3D.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/CStrInternStatic.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "ps/VideoMode.h"
#include "ps/World.h"
#include "renderer/backend/IDevice.h"
#include "renderer/DebugRenderer.h"
#include "renderer/HWLightingModelRenderer.h"
#include "renderer/InstancingModelRenderer.h"
#include "renderer/ModelRenderer.h"
#include "renderer/OverlayRenderer.h"
#include "renderer/ParticleRenderer.h"
#include "renderer/PostprocManager.h"
#include "renderer/Renderer.h"
#include "renderer/RenderingOptions.h"
#include "renderer/RenderModifiers.h"
#include "renderer/ShadowMap.h"
#include "renderer/SilhouetteRenderer.h"
#include "renderer/SkyManager.h"
#include "renderer/TerrainOverlay.h"
#include "renderer/TerrainRenderer.h"
#include "renderer/WaterManager.h"

#include <algorithm>

struct SScreenRect
{
	int x1, y1, x2, y2;
};

/**
 * Struct CSceneRendererInternals: Truly hide data that is supposed to be hidden
 * in this structure so it won't even appear in header files.
 */
class CSceneRenderer::Internals
{
	NONCOPYABLE(Internals);
public:
	Internals() = default;
	~Internals() = default;

	/// Water manager
	WaterManager waterManager;

	/// Sky manager
	SkyManager skyManager;

	/// Terrain renderer
	TerrainRenderer terrainRenderer;

	/// Overlay renderer
	OverlayRenderer overlayRenderer;

	/// Particle manager
	CParticleManager particleManager;

	/// Particle renderer
	ParticleRenderer particleRenderer;

	/// Material manager
	CMaterialManager materialManager;

	/// Shadow map
	ShadowMap shadow;

	SilhouetteRenderer silhouetteRenderer;

	/// Various model renderers
	struct Models
	{
		// NOTE: The current renderer design (with ModelRenderer, ModelVertexRenderer,
		// RenderModifier, etc) is mostly a relic of an older design that implemented
		// the different materials and rendering modes through extensive subclassing
		// and hooking objects together in various combinations.
		// The new design uses the CShaderManager API to abstract away the details
		// of rendering, and uses a data-driven approach to materials, so there are
		// now a small number of generic subclasses instead of many specialised subclasses,
		// but most of the old infrastructure hasn't been refactored out yet and leads to
		// some unwanted complexity.

		// Submitted models are split on two axes:
		//  - Normal vs Transp[arent] - alpha-blended models are stored in a separate
		//    list so we can draw them above/below the alpha-blended water plane correctly
		//  - Skinned vs Unskinned - with hardware lighting we don't need to
		//    duplicate mesh data per model instance (except for skinned models),
		//    so non-skinned models get different ModelVertexRenderers

		ModelRendererPtr NormalSkinned;
		ModelRendererPtr NormalUnskinned; // == NormalSkinned if unskinned shader instancing not supported
		ModelRendererPtr TranspSkinned;
		ModelRendererPtr TranspUnskinned; // == TranspSkinned if unskinned shader instancing not supported

		ModelVertexRendererPtr VertexRendererShader;
		ModelVertexRendererPtr VertexInstancingShader;
		ModelVertexRendererPtr VertexGPUSkinningShader;

		LitRenderModifierPtr ModShader;
	} Model;

	CShaderDefines globalContext;

	/**
	 * Renders all non-alpha-blended models with the given context.
	 */
	void CallModelRenderers(
		Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
		const CShaderDefines& context, int cullGroup, int flags)
	{
		CShaderDefines contextSkinned = context;
		if (g_RenderingOptions.GetGPUSkinning())
		{
			contextSkinned.Add(str_USE_INSTANCING, str_1);
			contextSkinned.Add(str_USE_GPU_SKINNING, str_1);
		}
		Model.NormalSkinned->Render(deviceCommandContext, Model.ModShader, contextSkinned, cullGroup, flags);

		if (Model.NormalUnskinned != Model.NormalSkinned)
		{
			CShaderDefines contextUnskinned = context;
			contextUnskinned.Add(str_USE_INSTANCING, str_1);
			Model.NormalUnskinned->Render(deviceCommandContext, Model.ModShader, contextUnskinned, cullGroup, flags);
		}
	}

	/**
	 * Renders all alpha-blended models with the given context.
	 */
	void CallTranspModelRenderers(
		Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
		const CShaderDefines& context, int cullGroup, int flags)
	{
		CShaderDefines contextSkinned = context;
		if (g_RenderingOptions.GetGPUSkinning())
		{
			contextSkinned.Add(str_USE_INSTANCING, str_1);
			contextSkinned.Add(str_USE_GPU_SKINNING, str_1);
		}
		Model.TranspSkinned->Render(deviceCommandContext, Model.ModShader, contextSkinned, cullGroup, flags);

		if (Model.TranspUnskinned != Model.TranspSkinned)
		{
			CShaderDefines contextUnskinned = context;
			contextUnskinned.Add(str_USE_INSTANCING, str_1);
			Model.TranspUnskinned->Render(deviceCommandContext, Model.ModShader, contextUnskinned, cullGroup, flags);
		}
	}
};

CSceneRenderer::CSceneRenderer()
{
	m = std::make_unique<Internals>();

	m_TerrainRenderMode = SOLID;
	m_WaterRenderMode = SOLID;
	m_ModelRenderMode = SOLID;
	m_OverlayRenderMode = SOLID;

	m_DisplayTerrainPriorities = false;

	m_LightEnv = nullptr;

	m_CurrentScene = nullptr;
}

CSceneRenderer::~CSceneRenderer()
{
	// We no longer UnloadWaterTextures here -
	// that is the responsibility of the module that asked for
	// them to be loaded (i.e. CGameView).
	m.reset();
}

void CSceneRenderer::ReloadShaders()
{
	m->globalContext = CShaderDefines();

	if (g_RenderingOptions.GetShadows())
	{
		m->globalContext.Add(str_USE_SHADOW, str_1);
		if (g_VideoMode.GetBackend() == CVideoMode::Backend::GL_ARB &&
			g_VideoMode.GetBackendDevice()->GetCapabilities().ARBShadersShadow)
		{
			m->globalContext.Add(str_USE_FP_SHADOW, str_1);
		}
		if (g_RenderingOptions.GetShadowPCF())
			m->globalContext.Add(str_USE_SHADOW_PCF, str_1);
		const int cascadeCount = m->shadow.GetCascadeCount();
		ENSURE(1 <= cascadeCount && cascadeCount <= 4);
		const CStrIntern cascadeCountStr[5] = {str_0, str_1, str_2, str_3, str_4};
		m->globalContext.Add(str_SHADOWS_CASCADE_COUNT, cascadeCountStr[cascadeCount]);
#if !CONFIG2_GLES
		m->globalContext.Add(str_USE_SHADOW_SAMPLER, str_1);
#endif
	}

	m->globalContext.Add(str_RENDER_DEBUG_MODE,
		RenderDebugModeEnum::ToString(g_RenderingOptions.GetRenderDebugMode()));

	if (g_VideoMode.GetBackend() != CVideoMode::Backend::GL_ARB && g_RenderingOptions.GetFog())
		m->globalContext.Add(str_USE_FOG, str_1);

	m->Model.ModShader = LitRenderModifierPtr(new ShaderRenderModifier());

	ENSURE(g_RenderingOptions.GetRenderPath() != RenderPath::FIXED);
	m->Model.VertexRendererShader = ModelVertexRendererPtr(new ShaderModelVertexRenderer());
	m->Model.VertexInstancingShader = ModelVertexRendererPtr(new InstancingModelRenderer(false, g_VideoMode.GetBackend() != CVideoMode::Backend::GL_ARB));

	if (g_RenderingOptions.GetGPUSkinning()) // TODO: should check caps and GLSL etc too
	{
		m->Model.VertexGPUSkinningShader = ModelVertexRendererPtr(new InstancingModelRenderer(true, g_VideoMode.GetBackend() != CVideoMode::Backend::GL_ARB));
		m->Model.NormalSkinned = ModelRendererPtr(new ShaderModelRenderer(m->Model.VertexGPUSkinningShader));
		m->Model.TranspSkinned = ModelRendererPtr(new ShaderModelRenderer(m->Model.VertexGPUSkinningShader));
	}
	else
	{
		m->Model.VertexGPUSkinningShader.reset();
		m->Model.NormalSkinned = ModelRendererPtr(new ShaderModelRenderer(m->Model.VertexRendererShader));
		m->Model.TranspSkinned = ModelRendererPtr(new ShaderModelRenderer(m->Model.VertexRendererShader));
	}

	m->Model.NormalUnskinned = ModelRendererPtr(new ShaderModelRenderer(m->Model.VertexInstancingShader));
	m->Model.TranspUnskinned = ModelRendererPtr(new ShaderModelRenderer(m->Model.VertexInstancingShader));
}

void CSceneRenderer::Initialize()
{
	// Let component renderers perform one-time initialization after graphics capabilities and
	// the shader path have been determined.
	m->overlayRenderer.Initialize();
}

// resize renderer view
void CSceneRenderer::Resize(int UNUSED(width), int UNUSED(height))
{
	// need to recreate the shadow map object to resize the shadow texture
	m->shadow.RecreateTexture();

	m->waterManager.RecreateOrLoadTexturesIfNeeded();
}

void CSceneRenderer::BeginFrame()
{
	// choose model renderers for this frame
	m->Model.ModShader->SetShadowMap(&m->shadow);
	m->Model.ModShader->SetLightEnv(m_LightEnv);
}

void CSceneRenderer::SetSimulation(CSimulation2* simulation)
{
	// set current simulation context for terrain renderer
	m->terrainRenderer.SetSimulation(simulation);
}

void CSceneRenderer::RenderShadowMap(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	const CShaderDefines& context)
{
	PROFILE3_GPU("shadow map");
	GPU_SCOPED_LABEL(deviceCommandContext, "Render shadow map");

	CShaderDefines shadowsContext = context;
	shadowsContext.Add(str_PASS_SHADOWS, str_1);

	CShaderDefines contextCast = shadowsContext;
	contextCast.Add(str_MODE_SHADOWCAST, str_1);

	m->shadow.BeginRender();

	const int cascadeCount = m->shadow.GetCascadeCount();
	ENSURE(0 <= cascadeCount && cascadeCount <= 4);
	for (int cascade = 0; cascade < cascadeCount; ++cascade)
	{
		m->shadow.PrepareCamera(cascade);

		const int cullGroup = CULL_SHADOWS_CASCADE_0 + cascade;
		{
			PROFILE("render patches");
			m->terrainRenderer.RenderPatches(deviceCommandContext, cullGroup, shadowsContext);
		}

		{
			PROFILE("render models");
			m->CallModelRenderers(deviceCommandContext, contextCast, cullGroup, MODELFLAG_CASTSHADOWS);
		}

		{
			PROFILE("render transparent models");
			m->CallTranspModelRenderers(deviceCommandContext, contextCast, cullGroup, MODELFLAG_CASTSHADOWS);
		}
	}

	m->shadow.EndRender();

	g_Renderer.SetViewport(m_ViewCamera.GetViewPort());
}

void CSceneRenderer::RenderPatches(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	const CShaderDefines& context, int cullGroup)
{
	PROFILE3_GPU("patches");
	GPU_SCOPED_LABEL(deviceCommandContext, "Render patches");

	// Switch on wireframe if we need it.
	CShaderDefines localContext = context;
	if (m_TerrainRenderMode == WIREFRAME)
		localContext.Add(str_MODE_WIREFRAME, str_1);

	// Render all the patches, including blend pass.
	m->terrainRenderer.RenderTerrainShader(deviceCommandContext, localContext, cullGroup,
		g_RenderingOptions.GetShadows() ? &m->shadow : nullptr);

	if (m_TerrainRenderMode == EDGED_FACES)
	{
		localContext.Add(str_MODE_WIREFRAME, str_1);
		// Edged faces: need to make a second pass over the data.

		// Render tiles edges.
		m->terrainRenderer.RenderPatches(
			deviceCommandContext, cullGroup, localContext, CColor(0.5f, 0.5f, 1.0f, 1.0f));

		// Render outline of each patch.
		m->terrainRenderer.RenderOutlines(deviceCommandContext, cullGroup);
	}
}

void CSceneRenderer::RenderModels(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	const CShaderDefines& context, int cullGroup)
{
	PROFILE3_GPU("models");
	GPU_SCOPED_LABEL(deviceCommandContext, "Render models");

	int flags = 0;

	CShaderDefines localContext = context;

	if (m_ModelRenderMode == WIREFRAME)
		localContext.Add(str_MODE_WIREFRAME, str_1);

	m->CallModelRenderers(deviceCommandContext, localContext, cullGroup, flags);

	if (m_ModelRenderMode == EDGED_FACES)
	{
		localContext.Add(str_MODE_WIREFRAME_SOLID, str_1);
		m->CallModelRenderers(deviceCommandContext, localContext, cullGroup, flags);
	}
}

void CSceneRenderer::RenderTransparentModels(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	const CShaderDefines& context, int cullGroup, ETransparentMode transparentMode)
{
	PROFILE3_GPU("transparent models");
	GPU_SCOPED_LABEL(deviceCommandContext, "Render transparent models");

	int flags = 0;

	CShaderDefines contextOpaque = context;
	contextOpaque.Add(str_ALPHABLEND_PASS_OPAQUE, str_1);

	CShaderDefines contextBlend = context;
	contextBlend.Add(str_ALPHABLEND_PASS_BLEND, str_1);

	if (m_ModelRenderMode == WIREFRAME)
	{
		contextOpaque.Add(str_MODE_WIREFRAME, str_1);
		contextBlend.Add(str_MODE_WIREFRAME, str_1);
	}

	if (transparentMode == TRANSPARENT || transparentMode == TRANSPARENT_OPAQUE)
		m->CallTranspModelRenderers(deviceCommandContext, contextOpaque, cullGroup, flags);

	if (transparentMode == TRANSPARENT || transparentMode == TRANSPARENT_BLEND)
		m->CallTranspModelRenderers(deviceCommandContext, contextBlend, cullGroup, flags);

	if (m_ModelRenderMode == EDGED_FACES)
	{
		CShaderDefines contextWireframe = contextOpaque;
		contextWireframe.Add(str_MODE_WIREFRAME, str_1);

		m->CallTranspModelRenderers(deviceCommandContext, contextWireframe, cullGroup, flags);
	}
}

// SetObliqueFrustumClipping: change the near plane to the given clip plane (in world space)
// Based on code from Game Programming Gems 5, from http://www.terathon.com/code/oblique.html
// - worldPlane is a clip plane in world space (worldPlane.Dot(v) >= 0 for any vector v passing the clipping test)
void CSceneRenderer::SetObliqueFrustumClipping(CCamera& camera, const CVector4D& worldPlane) const
{
	// First, we'll convert the given clip plane to camera space, then we'll
	// Get the view matrix and normal matrix (top 3x3 part of view matrix)
	CMatrix3D normalMatrix = camera.GetOrientation().GetTranspose();
	CVector4D camPlane = normalMatrix.Transform(worldPlane);

	CMatrix3D matrix = camera.GetProjection();

	// Calculate the clip-space corner point opposite the clipping plane
	// as (sgn(camPlane.x), sgn(camPlane.y), 1, 1) and
	// transform it into camera space by multiplying it
	// by the inverse of the projection matrix

	CVector4D q;
	q.X = (Sign(camPlane.X) - matrix[8] / matrix[11]) / matrix[0];
	q.Y = (Sign(camPlane.Y) - matrix[9] / matrix[11]) / matrix[5];
	q.Z = 1.0f / matrix[11];
	q.W = (1.0f - matrix[10] / matrix[11]) / matrix[14];

	// Calculate the scaled plane vector
	CVector4D c = camPlane * (2.0f * matrix[11] / camPlane.Dot(q));

	// Replace the third row of the projection matrix
	matrix[2] = c.X;
	matrix[6] = c.Y;
	matrix[10] = c.Z - matrix[11];
	matrix[14] = c.W;

	// Load it back into the camera
	camera.SetProjection(matrix);
}

void CSceneRenderer::ComputeReflectionCamera(CCamera& camera, const CBoundingBoxAligned& scissor) const
{
	WaterManager& wm = m->waterManager;

	CMatrix3D projection;
	if (m_ViewCamera.GetProjectionType() == CCamera::ProjectionType::PERSPECTIVE)
	{
		const float aspectRatio = 1.0f;
		// Expand fov slightly since ripples can reflect parts of the scene that
		// are slightly outside the normal camera view, and we want to avoid any
		// noticeable edge-filtering artifacts
		projection.SetPerspective(m_ViewCamera.GetFOV() * 1.05f, aspectRatio, m_ViewCamera.GetNearPlane(), m_ViewCamera.GetFarPlane());
	}
	else
		projection = m_ViewCamera.GetProjection();

	camera = m_ViewCamera;

	// Temporarily change the camera to one that is reflected.
	// Also, for texturing purposes, make it render to a view port the size of the
	// water texture, stretch the image according to our aspect ratio so it covers
	// the whole screen despite being rendered into a square, and cover slightly more
	// of the view so we can see wavy reflections of slightly off-screen objects.
	camera.m_Orientation.Scale(1, -1, 1);
	camera.m_Orientation.Translate(0, 2 * wm.m_WaterHeight, 0);
	camera.UpdateFrustum(scissor);
	// Clip slightly above the water to improve reflections of objects on the water
	// when the reflections are distorted.
	camera.ClipFrustum(CVector4D(0, 1, 0, -wm.m_WaterHeight + 2.0f));

	SViewPort vp;
	vp.m_Height = wm.m_RefTextureSize;
	vp.m_Width = wm.m_RefTextureSize;
	vp.m_X = 0;
	vp.m_Y = 0;
	camera.SetViewPort(vp);
	camera.SetProjection(projection);
	CMatrix3D scaleMat;
	scaleMat.SetScaling(g_Renderer.GetHeight() / static_cast<float>(std::max(1, g_Renderer.GetWidth())), 1.0f, 1.0f);
	camera.SetProjection(scaleMat * camera.GetProjection());

	CVector4D camPlane(0, 1, 0, -wm.m_WaterHeight + 0.5f);
	SetObliqueFrustumClipping(camera, camPlane);
}

void CSceneRenderer::ComputeRefractionCamera(CCamera& camera, const CBoundingBoxAligned& scissor) const
{
	WaterManager& wm = m->waterManager;

	CMatrix3D projection;
	if (m_ViewCamera.GetProjectionType() == CCamera::ProjectionType::PERSPECTIVE)
	{
		const float aspectRatio = 1.0f;
		// Expand fov slightly since ripples can reflect parts of the scene that
		// are slightly outside the normal camera view, and we want to avoid any
		// noticeable edge-filtering artifacts
		projection.SetPerspective(m_ViewCamera.GetFOV() * 1.05f, aspectRatio, m_ViewCamera.GetNearPlane(), m_ViewCamera.GetFarPlane());
	}
	else
		projection = m_ViewCamera.GetProjection();

	camera = m_ViewCamera;

	// Temporarily change the camera to make it render to a view port the size of the
	// water texture, stretch the image according to our aspect ratio so it covers
	// the whole screen despite being rendered into a square, and cover slightly more
	// of the view so we can see wavy refractions of slightly off-screen objects.
	camera.UpdateFrustum(scissor);
	camera.ClipFrustum(CVector4D(0, -1, 0, wm.m_WaterHeight + 0.5f));	// add some to avoid artifacts near steep shores.

	SViewPort vp;
	vp.m_Height = wm.m_RefTextureSize;
	vp.m_Width = wm.m_RefTextureSize;
	vp.m_X = 0;
	vp.m_Y = 0;
	camera.SetViewPort(vp);
	camera.SetProjection(projection);
	CMatrix3D scaleMat;
	scaleMat.SetScaling(g_Renderer.GetHeight() / static_cast<float>(std::max(1, g_Renderer.GetWidth())), 1.0f, 1.0f);
	camera.SetProjection(scaleMat * camera.GetProjection());
}

// RenderReflections: render the water reflections to the reflection texture
void CSceneRenderer::RenderReflections(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	const CShaderDefines& context, const CBoundingBoxAligned& scissor)
{
	PROFILE3_GPU("water reflections");
	GPU_SCOPED_LABEL(deviceCommandContext, "Render water reflections");

	WaterManager& wm = m->waterManager;

	// Remember old camera
	CCamera normalCamera = m_ViewCamera;

	ComputeReflectionCamera(m_ViewCamera, scissor);
	const CBoundingBoxAligned reflectionScissor =
		m->terrainRenderer.ScissorWater(CULL_DEFAULT, m_ViewCamera);
	if (reflectionScissor.IsEmpty())
	{
		m_ViewCamera = normalCamera;
		return;
	}

	g_Renderer.SetViewport(m_ViewCamera.GetViewPort());

	// Save the model-view-projection matrix so the shaders can use it for projective texturing
	wm.m_ReflectionMatrix = m_ViewCamera.GetViewProjection();

	float vpHeight = wm.m_RefTextureSize;
	float vpWidth = wm.m_RefTextureSize;

	SScreenRect screenScissor;
	screenScissor.x1 = static_cast<int>(floor((reflectionScissor[0].X * 0.5f + 0.5f) * vpWidth));
	screenScissor.y1 = static_cast<int>(floor((reflectionScissor[0].Y * 0.5f + 0.5f) * vpHeight));
	screenScissor.x2 = static_cast<int>(ceil((reflectionScissor[1].X * 0.5f + 0.5f) * vpWidth));
	screenScissor.y2 = static_cast<int>(ceil((reflectionScissor[1].Y * 0.5f + 0.5f) * vpHeight));

	Renderer::Backend::IDeviceCommandContext::Rect scissorRect;
	scissorRect.x = screenScissor.x1;
	scissorRect.y = screenScissor.y1;
	scissorRect.width = screenScissor.x2 - screenScissor.x1;
	scissorRect.height = screenScissor.y2 - screenScissor.y1;
	deviceCommandContext->SetScissors(1, &scissorRect);

	deviceCommandContext->SetGraphicsPipelineState(
		Renderer::Backend::MakeDefaultGraphicsPipelineStateDesc());
	deviceCommandContext->SetFramebuffer(wm.m_ReflectionFramebuffer.get());
	deviceCommandContext->ClearFramebuffer();

	CShaderDefines reflectionsContext = context;
	reflectionsContext.Add(str_PASS_REFLECTIONS, str_1);

	// Render terrain and models
	RenderPatches(deviceCommandContext, reflectionsContext, CULL_REFLECTIONS);
	RenderModels(deviceCommandContext, reflectionsContext, CULL_REFLECTIONS);
	RenderTransparentModels(deviceCommandContext, reflectionsContext, CULL_REFLECTIONS, TRANSPARENT);

	// Particles are always oriented to face the camera in the vertex shader,
	// so they don't need the inverted cull face.
	if (g_RenderingOptions.GetParticles())
	{
		RenderParticles(deviceCommandContext, CULL_REFLECTIONS);
	}

	deviceCommandContext->SetScissors(0, nullptr);

	// Reset old camera
	m_ViewCamera = normalCamera;
	g_Renderer.SetViewport(m_ViewCamera.GetViewPort());
}

// RenderRefractions: render the water refractions to the refraction texture
void CSceneRenderer::RenderRefractions(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	const CShaderDefines& context, const CBoundingBoxAligned &scissor)
{
	PROFILE3_GPU("water refractions");
	GPU_SCOPED_LABEL(deviceCommandContext, "Render water refractions");

	WaterManager& wm = m->waterManager;

	// Remember old camera
	CCamera normalCamera = m_ViewCamera;

	ComputeRefractionCamera(m_ViewCamera, scissor);
	const CBoundingBoxAligned refractionScissor =
		m->terrainRenderer.ScissorWater(CULL_DEFAULT, m_ViewCamera);
	if (refractionScissor.IsEmpty())
	{
		m_ViewCamera = normalCamera;
		return;
	}

	CVector4D camPlane(0, -1, 0, wm.m_WaterHeight + 2.0f);
	SetObliqueFrustumClipping(m_ViewCamera, camPlane);

	g_Renderer.SetViewport(m_ViewCamera.GetViewPort());

	// Save the model-view-projection matrix so the shaders can use it for projective texturing
	wm.m_RefractionMatrix = m_ViewCamera.GetViewProjection();
	wm.m_RefractionProjInvMatrix = m_ViewCamera.GetProjection().GetInverse();
	wm.m_RefractionViewInvMatrix = m_ViewCamera.GetOrientation();

	float vpHeight = wm.m_RefTextureSize;
	float vpWidth = wm.m_RefTextureSize;

	SScreenRect screenScissor;
	screenScissor.x1 = static_cast<int>(floor((refractionScissor[0].X * 0.5f + 0.5f) * vpWidth));
	screenScissor.y1 = static_cast<int>(floor((refractionScissor[0].Y * 0.5f + 0.5f) * vpHeight));
	screenScissor.x2 = static_cast<int>(ceil((refractionScissor[1].X * 0.5f + 0.5f) * vpWidth));
	screenScissor.y2 = static_cast<int>(ceil((refractionScissor[1].Y * 0.5f + 0.5f) * vpHeight));

	Renderer::Backend::IDeviceCommandContext::Rect scissorRect;
	scissorRect.x = screenScissor.x1;
	scissorRect.y = screenScissor.y1;
	scissorRect.width = screenScissor.x2 - screenScissor.x1;
	scissorRect.height = screenScissor.y2 - screenScissor.y1;
	deviceCommandContext->SetScissors(1, &scissorRect);

	deviceCommandContext->SetGraphicsPipelineState(
		Renderer::Backend::MakeDefaultGraphicsPipelineStateDesc());
	deviceCommandContext->SetFramebuffer(wm.m_RefractionFramebuffer.get());
	deviceCommandContext->ClearFramebuffer();

	// Render terrain and models
	RenderPatches(deviceCommandContext, context, CULL_REFRACTIONS);

	// Render debug-related terrain overlays to make it visible under water.
	ITerrainOverlay::RenderOverlaysBeforeWater(deviceCommandContext);

	RenderModels(deviceCommandContext, context, CULL_REFRACTIONS);
	RenderTransparentModels(deviceCommandContext, context, CULL_REFRACTIONS, TRANSPARENT_OPAQUE);

	deviceCommandContext->SetScissors(0, nullptr);

	// Reset old camera
	m_ViewCamera = normalCamera;
	g_Renderer.SetViewport(m_ViewCamera.GetViewPort());
}

void CSceneRenderer::RenderSilhouettes(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	const CShaderDefines& context)
{
	PROFILE3_GPU("silhouettes");
	GPU_SCOPED_LABEL(deviceCommandContext, "Render silhouettes");

	CShaderDefines contextOccluder = context;
	contextOccluder.Add(str_MODE_SILHOUETTEOCCLUDER, str_1);

	CShaderDefines contextDisplay = context;
	contextDisplay.Add(str_MODE_SILHOUETTEDISPLAY, str_1);

	// Render silhouettes of units hidden behind terrain or occluders.
	// To avoid breaking the standard rendering of alpha-blended objects, this
	// has to be done in a separate pass.
	// First we render all occluders into depth, then render all units with
	// inverted depth test so any behind an occluder will get drawn in a constant
	// color.

	deviceCommandContext->SetGraphicsPipelineState(
		Renderer::Backend::MakeDefaultGraphicsPipelineStateDesc());
	deviceCommandContext->ClearFramebuffer(false, true, true);

	// Render occluders:

	{
		PROFILE("render patches");
		m->terrainRenderer.RenderPatches(deviceCommandContext, CULL_SILHOUETTE_OCCLUDER, contextOccluder);
	}

	{
		PROFILE("render model occluders");
		m->CallModelRenderers(deviceCommandContext, contextOccluder, CULL_SILHOUETTE_OCCLUDER, 0);
	}

	{
		PROFILE("render transparent occluders");
		m->CallTranspModelRenderers(deviceCommandContext, contextOccluder, CULL_SILHOUETTE_OCCLUDER, 0);
	}

	// Since we can't sort, we'll use the stencil buffer to ensure we only draw
	// a pixel once (using the color of whatever model happens to be drawn first).
	{
		PROFILE("render model casters");
		m->CallModelRenderers(deviceCommandContext, contextDisplay, CULL_SILHOUETTE_CASTER, 0);
	}

	{
		PROFILE("render transparent casters");
		m->CallTranspModelRenderers(deviceCommandContext, contextDisplay, CULL_SILHOUETTE_CASTER, 0);
	}
}

void CSceneRenderer::RenderParticles(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	int cullGroup)
{
	PROFILE3_GPU("particles");
	GPU_SCOPED_LABEL(deviceCommandContext, "Render particles");

	m->particleRenderer.RenderParticles(
		deviceCommandContext, cullGroup, m_ModelRenderMode == WIREFRAME);

	if (m_ModelRenderMode == EDGED_FACES)
	{
		m->particleRenderer.RenderParticles(
			deviceCommandContext, cullGroup, true);
		m->particleRenderer.RenderBounds(cullGroup);
	}
}

// RenderSubmissions: force rendering of any batched objects
void CSceneRenderer::RenderSubmissions(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	const CBoundingBoxAligned& waterScissor)
{
	PROFILE3("render submissions");
	GPU_SCOPED_LABEL(deviceCommandContext, "Render submissions");

	m->skyManager.LoadAndUploadSkyTexturesIfNeeded(deviceCommandContext);

	GetScene().GetLOSTexture().InterpolateLOS(deviceCommandContext);
	GetScene().GetTerritoryTexture().UpdateIfNeeded(deviceCommandContext);
	GetScene().GetMiniMapTexture().Render(deviceCommandContext);

	CShaderDefines context = m->globalContext;

	int cullGroup = CULL_DEFAULT;

	// Set the camera
	g_Renderer.SetViewport(m_ViewCamera.GetViewPort());

	// Prepare model renderers
	{
	PROFILE3("prepare models");
	m->Model.NormalSkinned->PrepareModels();
	m->Model.TranspSkinned->PrepareModels();
	if (m->Model.NormalUnskinned != m->Model.NormalSkinned)
		m->Model.NormalUnskinned->PrepareModels();
	if (m->Model.TranspUnskinned != m->Model.TranspSkinned)
		m->Model.TranspUnskinned->PrepareModels();
	}

	m->terrainRenderer.PrepareForRendering();

	m->overlayRenderer.PrepareForRendering();

	m->particleRenderer.PrepareForRendering(context);

	if (g_RenderingOptions.GetShadows())
	{
		RenderShadowMap(deviceCommandContext, context);
	}

	if (m->waterManager.m_RenderWater)
	{
		if (waterScissor.GetVolume() > 0 && m->waterManager.WillRenderFancyWater())
		{
			m->waterManager.UpdateQuality();

			PROFILE3_GPU("water scissor");
			if (g_RenderingOptions.GetWaterReflection())
				RenderReflections(deviceCommandContext, context, waterScissor);

			if (g_RenderingOptions.GetWaterRefraction())
				RenderRefractions(deviceCommandContext, context, waterScissor);

			if (g_RenderingOptions.GetWaterFancyEffects())
				m->terrainRenderer.RenderWaterFoamOccluders(deviceCommandContext, cullGroup);
		}
	}

	deviceCommandContext->SetGraphicsPipelineState(
		Renderer::Backend::MakeDefaultGraphicsPipelineStateDesc());

	CPostprocManager& postprocManager = g_Renderer.GetPostprocManager();
	if (postprocManager.IsEnabled())
	{
		// We have to update the post process manager with real near/far planes
		// that we use for the scene rendering.
		postprocManager.SetDepthBufferClipPlanes(
			m_ViewCamera.GetNearPlane(), m_ViewCamera.GetFarPlane()
		);
		postprocManager.Initialize();
		postprocManager.CaptureRenderOutput(deviceCommandContext);
	}
	else
	{
		deviceCommandContext->SetFramebuffer(
			deviceCommandContext->GetDevice()->GetCurrentBackbuffer());
	}

	{
		PROFILE3_GPU("clear buffers");
		// We don't need to clear the color attachment of the framebuffer if the sky
		// is going to be rendered. Because it covers the whole view.
		deviceCommandContext->ClearFramebuffer(!m->skyManager.IsSkyVisible(), true, true);
	}

	m->skyManager.RenderSky(deviceCommandContext);

	// render submitted patches and models
	RenderPatches(deviceCommandContext, context, cullGroup);

	// render debug-related terrain overlays
	ITerrainOverlay::RenderOverlaysBeforeWater(deviceCommandContext);

	// render other debug-related overlays before water (so they can be seen when underwater)
	m->overlayRenderer.RenderOverlaysBeforeWater(deviceCommandContext);

	RenderModels(deviceCommandContext, context, cullGroup);

	// render water
	if (m->waterManager.m_RenderWater && g_Game && waterScissor.GetVolume() > 0)
	{
		if (m->waterManager.WillRenderFancyWater())
		{
			// Render transparent stuff, but only the solid parts that can occlude block water.
			RenderTransparentModels(deviceCommandContext, context, cullGroup, TRANSPARENT_OPAQUE);

			m->terrainRenderer.RenderWater(deviceCommandContext, context, cullGroup, &m->shadow);

			// Render transparent stuff again, but only the blended parts that overlap water.
			RenderTransparentModels(deviceCommandContext, context, cullGroup, TRANSPARENT_BLEND);
		}
		else
		{
			m->terrainRenderer.RenderWater(deviceCommandContext, context, cullGroup, &m->shadow);

			// Render transparent stuff, so it can overlap models/terrain.
			RenderTransparentModels(deviceCommandContext, context, cullGroup, TRANSPARENT);
		}
	}
	else
	{
		// render transparent stuff, so it can overlap models/terrain
		RenderTransparentModels(deviceCommandContext, context, cullGroup, TRANSPARENT);
	}

	// render debug-related terrain overlays
	ITerrainOverlay::RenderOverlaysAfterWater(deviceCommandContext, cullGroup);

	// render some other overlays after water (so they can be displayed on top of water)
	m->overlayRenderer.RenderOverlaysAfterWater(deviceCommandContext);

	// particles are transparent so render after water
	if (g_RenderingOptions.GetParticles())
	{
		RenderParticles(deviceCommandContext, cullGroup);
	}

	if (postprocManager.IsEnabled())
	{
		if (g_Renderer.GetPostprocManager().IsMultisampleEnabled())
			g_Renderer.GetPostprocManager().ResolveMultisampleFramebuffer(deviceCommandContext);

		postprocManager.ApplyPostproc(deviceCommandContext);
		postprocManager.ReleaseRenderOutput(deviceCommandContext);
	}

	if (g_RenderingOptions.GetSilhouettes())
	{
		RenderSilhouettes(deviceCommandContext, context);
	}

	// render debug lines
	if (g_RenderingOptions.GetDisplayFrustum())
		DisplayFrustum();

	if (g_RenderingOptions.GetDisplayShadowsFrustum())
		m->shadow.RenderDebugBounds();

	m->silhouetteRenderer.RenderDebugBounds(deviceCommandContext);
	m->silhouetteRenderer.RenderDebugOverlays(deviceCommandContext);

	// render overlays that should appear on top of all other objects
	m->overlayRenderer.RenderForegroundOverlays(deviceCommandContext, m_ViewCamera);
}

void CSceneRenderer::EndFrame()
{
	// empty lists
	m->terrainRenderer.EndFrame();
	m->overlayRenderer.EndFrame();
	m->particleRenderer.EndFrame();
	m->silhouetteRenderer.EndFrame();

	// Finish model renderers
	m->Model.NormalSkinned->EndFrame();
	m->Model.TranspSkinned->EndFrame();
	if (m->Model.NormalUnskinned != m->Model.NormalSkinned)
		m->Model.NormalUnskinned->EndFrame();
	if (m->Model.TranspUnskinned != m->Model.TranspSkinned)
		m->Model.TranspUnskinned->EndFrame();
}

void CSceneRenderer::DisplayFrustum()
{
	g_Renderer.GetDebugRenderer().DrawCameraFrustum(m_CullCamera, CColor(1.0f, 1.0f, 1.0f, 0.25f), 2);
	g_Renderer.GetDebugRenderer().DrawCameraFrustum(m_CullCamera, CColor(1.0f, 1.0f, 1.0f, 1.0f), 2, true);
}

// Text overlay rendering
void CSceneRenderer::RenderTextOverlays(CCanvas2D& canvas)
{
	PROFILE3_GPU("text overlays");

	if (m_DisplayTerrainPriorities)
		m->terrainRenderer.RenderPriorities(canvas, CULL_DEFAULT);
}

// SetSceneCamera: setup projection and transform of camera and adjust viewport to current view
// The camera always represents the actual camera used to render a scene, not any virtual camera
// used for shadow rendering or reflections.
void CSceneRenderer::SetSceneCamera(const CCamera& viewCamera, const CCamera& cullCamera)
{
	m_ViewCamera = viewCamera;
	m_CullCamera = cullCamera;

	if (g_RenderingOptions.GetShadows())
		m->shadow.SetupFrame(m_CullCamera, m_LightEnv->GetSunDir());
}

void CSceneRenderer::Submit(CPatch* patch)
{
	if (m_CurrentCullGroup == CULL_DEFAULT)
	{
		m->shadow.AddShadowReceiverBound(patch->GetWorldBounds());
		m->silhouetteRenderer.AddOccluder(patch);
	}

	if (CULL_SHADOWS_CASCADE_0 <= m_CurrentCullGroup && m_CurrentCullGroup <= CULL_SHADOWS_CASCADE_3)
	{
		const int cascade = m_CurrentCullGroup - CULL_SHADOWS_CASCADE_0;
		m->shadow.AddShadowCasterBound(cascade, patch->GetWorldBounds());
	}

	m->terrainRenderer.Submit(m_CurrentCullGroup, patch);
}

void CSceneRenderer::Submit(SOverlayLine* overlay)
{
	// Overlays are only needed in the default cull group for now,
	// so just ignore submissions to any other group
	if (m_CurrentCullGroup == CULL_DEFAULT)
		m->overlayRenderer.Submit(overlay);
}

void CSceneRenderer::Submit(SOverlayTexturedLine* overlay)
{
	if (m_CurrentCullGroup == CULL_DEFAULT)
		m->overlayRenderer.Submit(overlay);
}

void CSceneRenderer::Submit(SOverlaySprite* overlay)
{
	if (m_CurrentCullGroup == CULL_DEFAULT)
		m->overlayRenderer.Submit(overlay);
}

void CSceneRenderer::Submit(SOverlayQuad* overlay)
{
	if (m_CurrentCullGroup == CULL_DEFAULT)
		m->overlayRenderer.Submit(overlay);
}

void CSceneRenderer::Submit(SOverlaySphere* overlay)
{
	if (m_CurrentCullGroup == CULL_DEFAULT)
		m->overlayRenderer.Submit(overlay);
}

void CSceneRenderer::Submit(CModelDecal* decal)
{
	// Decals can't cast shadows since they're flat on the terrain.
	// They can receive shadows, but the terrain under them will have
	// already been passed to AddShadowCasterBound, so don't bother
	// doing it again here.

	m->terrainRenderer.Submit(m_CurrentCullGroup, decal);
}

void CSceneRenderer::Submit(CParticleEmitter* emitter)
{
	m->particleRenderer.Submit(m_CurrentCullGroup, emitter);
}

void CSceneRenderer::SubmitNonRecursive(CModel* model)
{
	if (m_CurrentCullGroup == CULL_DEFAULT)
	{
		m->shadow.AddShadowReceiverBound(model->GetWorldBounds());

		if (model->GetFlags() & MODELFLAG_SILHOUETTE_OCCLUDER)
			m->silhouetteRenderer.AddOccluder(model);
		if (model->GetFlags() & MODELFLAG_SILHOUETTE_DISPLAY)
			m->silhouetteRenderer.AddCaster(model);
	}

	if (CULL_SHADOWS_CASCADE_0 <= m_CurrentCullGroup && m_CurrentCullGroup <= CULL_SHADOWS_CASCADE_3)
	{
		if (!(model->GetFlags() & MODELFLAG_CASTSHADOWS))
			return;

		const int cascade = m_CurrentCullGroup - CULL_SHADOWS_CASCADE_0;
		m->shadow.AddShadowCasterBound(cascade, model->GetWorldBounds());
	}

	bool requiresSkinning = (model->GetModelDef()->GetNumBones() != 0);

	if (model->GetMaterial().UsesAlphaBlending())
	{
		if (requiresSkinning)
			m->Model.TranspSkinned->Submit(m_CurrentCullGroup, model);
		else
			m->Model.TranspUnskinned->Submit(m_CurrentCullGroup, model);
	}
	else
	{
		if (requiresSkinning)
			m->Model.NormalSkinned->Submit(m_CurrentCullGroup, model);
		else
			m->Model.NormalUnskinned->Submit(m_CurrentCullGroup, model);
	}
}

// Render the given scene
void CSceneRenderer::RenderScene(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext, Scene& scene)
{
	m_CurrentScene = &scene;

	CFrustum frustum = m_CullCamera.GetFrustum();

	m_CurrentCullGroup = CULL_DEFAULT;

	scene.EnumerateObjects(frustum, this);

	m->particleManager.RenderSubmit(*this, frustum);

	if (g_RenderingOptions.GetSilhouettes())
	{
		m->silhouetteRenderer.ComputeSubmissions(m_ViewCamera);

		m_CurrentCullGroup = CULL_DEFAULT;
		m->silhouetteRenderer.RenderSubmitOverlays(*this);

		m_CurrentCullGroup = CULL_SILHOUETTE_OCCLUDER;
		m->silhouetteRenderer.RenderSubmitOccluders(*this);

		m_CurrentCullGroup = CULL_SILHOUETTE_CASTER;
		m->silhouetteRenderer.RenderSubmitCasters(*this);
	}

	if (g_RenderingOptions.GetShadows())
	{
		for (int cascade = 0; cascade <= m->shadow.GetCascadeCount(); ++cascade)
		{
			m_CurrentCullGroup = CULL_SHADOWS_CASCADE_0 + cascade;
			const CFrustum shadowFrustum = m->shadow.GetShadowCasterCullFrustum(cascade);
			scene.EnumerateObjects(shadowFrustum, this);
		}
	}

	CBoundingBoxAligned waterScissor;
	if (m->waterManager.m_RenderWater)
	{
		waterScissor = m->terrainRenderer.ScissorWater(CULL_DEFAULT, m_ViewCamera);

		if (waterScissor.GetVolume() > 0 && m->waterManager.WillRenderFancyWater())
		{
			if (g_RenderingOptions.GetWaterReflection())
			{
				m_CurrentCullGroup = CULL_REFLECTIONS;

				CCamera reflectionCamera;
				ComputeReflectionCamera(reflectionCamera, waterScissor);

				scene.EnumerateObjects(reflectionCamera.GetFrustum(), this);
			}

			if (g_RenderingOptions.GetWaterRefraction())
			{
				m_CurrentCullGroup = CULL_REFRACTIONS;

				CCamera refractionCamera;
				ComputeRefractionCamera(refractionCamera, waterScissor);

				scene.EnumerateObjects(refractionCamera.GetFrustum(), this);
			}

			// Render the waves to the Fancy effects texture
			m->waterManager.RenderWaves(deviceCommandContext, frustum);
		}
	}

	m_CurrentCullGroup = -1;

	RenderSubmissions(deviceCommandContext, waterScissor);

	m_CurrentScene = NULL;
}

Scene& CSceneRenderer::GetScene()
{
	ENSURE(m_CurrentScene);
	return *m_CurrentScene;
}

void CSceneRenderer::MakeShadersDirty()
{
	m->waterManager.m_NeedsReloading = true;
}

WaterManager& CSceneRenderer::GetWaterManager()
{
	return m->waterManager;
}

SkyManager& CSceneRenderer::GetSkyManager()
{
	return m->skyManager;
}

CParticleManager& CSceneRenderer::GetParticleManager()
{
	return m->particleManager;
}

TerrainRenderer& CSceneRenderer::GetTerrainRenderer()
{
	return m->terrainRenderer;
}

CMaterialManager& CSceneRenderer::GetMaterialManager()
{
	return m->materialManager;
}

ShadowMap& CSceneRenderer::GetShadowMap()
{
	return m->shadow;
}

void CSceneRenderer::ResetState()
{
	// Clear all emitters, that were created in previous games
	GetParticleManager().ClearUnattachedEmitters();
}
