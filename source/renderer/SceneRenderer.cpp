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

#include "maths/Matrix3D.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/CStrInternStatic.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "ps/World.h"
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
#include "graphics/Terrain.h"
#include "graphics/Texture.h"
#include "graphics/TextureManager.h"
#include "ps/VideoMode.h"
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
	GLint x1, y1, x2, y2;
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
	void CallModelRenderers(const CShaderDefines& context, int cullGroup, int flags)
	{
		CShaderDefines contextSkinned = context;
		if (g_RenderingOptions.GetGPUSkinning())
		{
			contextSkinned.Add(str_USE_INSTANCING, str_1);
			contextSkinned.Add(str_USE_GPU_SKINNING, str_1);
		}
		Model.NormalSkinned->Render(Model.ModShader, contextSkinned, cullGroup, flags);

		if (Model.NormalUnskinned != Model.NormalSkinned)
		{
			CShaderDefines contextUnskinned = context;
			contextUnskinned.Add(str_USE_INSTANCING, str_1);
			Model.NormalUnskinned->Render(Model.ModShader, contextUnskinned, cullGroup, flags);
		}
	}

	/**
	 * Renders all alpha-blended models with the given context.
	 */
	void CallTranspModelRenderers(const CShaderDefines& context, int cullGroup, int flags)
	{
		CShaderDefines contextSkinned = context;
		if (g_RenderingOptions.GetGPUSkinning())
		{
			contextSkinned.Add(str_USE_INSTANCING, str_1);
			contextSkinned.Add(str_USE_GPU_SKINNING, str_1);
		}
		Model.TranspSkinned->Render(Model.ModShader, contextSkinned, cullGroup, flags);

		if (Model.TranspUnskinned != Model.TranspSkinned)
		{
			CShaderDefines contextUnskinned = context;
			contextUnskinned.Add(str_USE_INSTANCING, str_1);
			Model.TranspUnskinned->Render(Model.ModShader, contextUnskinned, cullGroup, flags);
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
	m_ClearColor[0] = m_ClearColor[1] = m_ClearColor[2] = m_ClearColor[3] = 0;

	m_DisplayTerrainPriorities = false;

	CStr skystring = "0 0 0";
	CColor skycolor;
	CFG_GET_VAL("skycolor", skystring);
	if (skycolor.ParseString(skystring, 255.f))
	{
		m_ClearColor[0] = skycolor.r;
		m_ClearColor[1] = skycolor.g;
		m_ClearColor[2] = skycolor.b;
		m_ClearColor[3] = skycolor.a;
	}

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

	const CRenderer::Caps& capabilities = g_Renderer.GetCapabilities();
	if (capabilities.m_Shadows && g_RenderingOptions.GetShadows())
	{
		m->globalContext.Add(str_USE_SHADOW, str_1);
		if (capabilities.m_ARBProgramShadow && g_RenderingOptions.GetARBProgramShadow())
			m->globalContext.Add(str_USE_FP_SHADOW, str_1);
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

	m->waterManager.Resize();
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

void CSceneRenderer::RenderShadowMap(const CShaderDefines& context)
{
	PROFILE3_GPU("shadow map");
	OGL_SCOPED_DEBUG_GROUP("Render shadow map");

	CShaderDefines contextCast = context;
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
			glCullFace(GL_FRONT);
			glEnable(GL_CULL_FACE);
			m->terrainRenderer.RenderPatches(cullGroup);
			glCullFace(GL_BACK);
		}

		{
			PROFILE("render models");
			m->CallModelRenderers(contextCast, cullGroup, MODELFLAG_CASTSHADOWS);
		}

		{
			PROFILE("render transparent models");
			// disable face-culling for two-sided models
			glDisable(GL_CULL_FACE);
			m->CallTranspModelRenderers(contextCast, cullGroup, MODELFLAG_CASTSHADOWS);
			glEnable(GL_CULL_FACE);
		}
	}

	m->shadow.EndRender();

	g_Renderer.SetViewport(m_ViewCamera.GetViewPort());
}

void CSceneRenderer::RenderPatches(const CShaderDefines& context, int cullGroup)
{
	PROFILE3_GPU("patches");
	OGL_SCOPED_DEBUG_GROUP("Render patches");

#if CONFIG2_GLES
#warning TODO: implement wireface/edged rendering mode GLES
#else
	// switch on wireframe if we need it
	if (m_TerrainRenderMode == WIREFRAME)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
#endif

	// render all the patches, including blend pass
	const CRenderer::Caps& capabilities = g_Renderer.GetCapabilities();
	m->terrainRenderer.RenderTerrainShader(context, cullGroup,
		(capabilities.m_Shadows && g_RenderingOptions.GetShadows()) ? &m->shadow : 0);

#if !CONFIG2_GLES
	if (m_TerrainRenderMode == WIREFRAME)
	{
		// switch wireframe off again
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	else if (m_TerrainRenderMode == EDGED_FACES)
	{
		// edged faces: need to make a second pass over the data:
		// first switch on wireframe
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		// setup some renderstate ..
		glActiveTextureARB(GL_TEXTURE0);
		glLineWidth(2.0f);

		// render tiles edges
		m->terrainRenderer.RenderPatches(cullGroup, CColor(0.5f, 0.5f, 1.0f, 1.0f));

		glLineWidth(4.0f);

		// render outline of each patch
		m->terrainRenderer.RenderOutlines(cullGroup);

		// .. and restore the renderstates
		glLineWidth(1.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
#endif
}

void CSceneRenderer::RenderModels(const CShaderDefines& context, int cullGroup)
{
	PROFILE3_GPU("models");
	OGL_SCOPED_DEBUG_GROUP("Render models");

	int flags = 0;

#if !CONFIG2_GLES
	if (m_ModelRenderMode == WIREFRAME)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
#endif

	m->CallModelRenderers(context, cullGroup, flags);

#if !CONFIG2_GLES
	if (m_ModelRenderMode == WIREFRAME)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	else if (m_ModelRenderMode == EDGED_FACES)
	{
		CShaderDefines contextWireframe = context;
		contextWireframe.Add(str_MODE_WIREFRAME, str_1);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		m->CallModelRenderers(contextWireframe, cullGroup, flags);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
#endif
}

void CSceneRenderer::RenderTransparentModels(const CShaderDefines& context, int cullGroup, ETransparentMode transparentMode, bool disableFaceCulling)
{
	PROFILE3_GPU("transparent models");
	OGL_SCOPED_DEBUG_GROUP("Render transparent models");

	int flags = 0;

#if !CONFIG2_GLES
	// switch on wireframe if we need it
	if (m_ModelRenderMode == WIREFRAME)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
#endif

	// disable face culling for two-sided models in sub-renders
	if (disableFaceCulling)
		glDisable(GL_CULL_FACE);

	CShaderDefines contextOpaque = context;
	contextOpaque.Add(str_ALPHABLEND_PASS_OPAQUE, str_1);

	CShaderDefines contextBlend = context;
	contextBlend.Add(str_ALPHABLEND_PASS_BLEND, str_1);

	if (transparentMode == TRANSPARENT || transparentMode == TRANSPARENT_OPAQUE)
		m->CallTranspModelRenderers(contextOpaque, cullGroup, flags);

	if (transparentMode == TRANSPARENT || transparentMode == TRANSPARENT_BLEND)
		m->CallTranspModelRenderers(contextBlend, cullGroup, flags);

	if (disableFaceCulling)
		glEnable(GL_CULL_FACE);

#if !CONFIG2_GLES
	if (m_ModelRenderMode == WIREFRAME)
	{
		// switch wireframe off again
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	else if (m_ModelRenderMode == EDGED_FACES)
	{
		CShaderDefines contextWireframe = contextOpaque;
		contextWireframe.Add(str_MODE_WIREFRAME, str_1);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		m->CallTranspModelRenderers(contextWireframe, cullGroup, flags);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
#endif
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
	q.X = (sgn(camPlane.X) - matrix[8]/matrix[11]) / matrix[0];
	q.Y = (sgn(camPlane.Y) - matrix[9]/matrix[11]) / matrix[5];
	q.Z = 1.0f/matrix[11];
	q.W = (1.0f - matrix[10]/matrix[11]) / matrix[14];

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
void CSceneRenderer::RenderReflections(const CShaderDefines& context, const CBoundingBoxAligned& scissor)
{
	PROFILE3_GPU("water reflections");
	OGL_SCOPED_DEBUG_GROUP("Render water reflections");

	WaterManager& wm = m->waterManager;

	// Remember old camera
	CCamera normalCamera = m_ViewCamera;

	ComputeReflectionCamera(m_ViewCamera, scissor);
	const CBoundingBoxAligned reflectionScissor =
		m->terrainRenderer.ScissorWater(CULL_DEFAULT, m_ViewCamera);

	g_Renderer.SetViewport(m_ViewCamera.GetViewPort());

	// Save the model-view-projection matrix so the shaders can use it for projective texturing
	wm.m_ReflectionMatrix = m_ViewCamera.GetViewProjection();

	float vpHeight = wm.m_RefTextureSize;
	float vpWidth = wm.m_RefTextureSize;

	SScreenRect screenScissor;
	screenScissor.x1 = (GLint)floor((reflectionScissor[0].X*0.5f+0.5f)*vpWidth);
	screenScissor.y1 = (GLint)floor((reflectionScissor[0].Y*0.5f+0.5f)*vpHeight);
	screenScissor.x2 = (GLint)ceil((reflectionScissor[1].X*0.5f+0.5f)*vpWidth);
	screenScissor.y2 = (GLint)ceil((reflectionScissor[1].Y*0.5f+0.5f)*vpHeight);

	glEnable(GL_SCISSOR_TEST);
	glScissor(screenScissor.x1, screenScissor.y1, screenScissor.x2 - screenScissor.x1, screenScissor.y2 - screenScissor.y1);

	// try binding the framebuffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, wm.m_ReflectionFbo);

	glClearColor(0.5f, 0.5f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glFrontFace(GL_CW);

	if (!g_RenderingOptions.GetWaterReflection())
	{
		m->skyManager.RenderSky();
		ogl_WarnIfError();
	}
	else
	{
		// Render terrain and models
		RenderPatches(context, CULL_REFLECTIONS);
		ogl_WarnIfError();
		RenderModels(context, CULL_REFLECTIONS);
		ogl_WarnIfError();
		RenderTransparentModels(context, CULL_REFLECTIONS, TRANSPARENT, true);
		ogl_WarnIfError();
	}
	glFrontFace(GL_CCW);

	// Particles are always oriented to face the camera in the vertex shader,
	// so they don't need the inverted glFrontFace
	if (g_RenderingOptions.GetParticles())
	{
		RenderParticles(CULL_REFLECTIONS);
		ogl_WarnIfError();
	}

	glDisable(GL_SCISSOR_TEST);

	// Reset old camera
	m_ViewCamera = normalCamera;
	g_Renderer.SetViewport(m_ViewCamera.GetViewPort());

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

// RenderRefractions: render the water refractions to the refraction texture
void CSceneRenderer::RenderRefractions(const CShaderDefines& context, const CBoundingBoxAligned &scissor)
{
	PROFILE3_GPU("water refractions");
	OGL_SCOPED_DEBUG_GROUP("Render water refractions");

	WaterManager& wm = m->waterManager;

	// Remember old camera
	CCamera normalCamera = m_ViewCamera;

	ComputeRefractionCamera(m_ViewCamera, scissor);
	const CBoundingBoxAligned refractionScissor =
		m->terrainRenderer.ScissorWater(CULL_DEFAULT, m_ViewCamera);

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
	screenScissor.x1 = (GLint)floor((refractionScissor[0].X*0.5f+0.5f)*vpWidth);
	screenScissor.y1 = (GLint)floor((refractionScissor[0].Y*0.5f+0.5f)*vpHeight);
	screenScissor.x2 = (GLint)ceil((refractionScissor[1].X*0.5f+0.5f)*vpWidth);
	screenScissor.y2 = (GLint)ceil((refractionScissor[1].Y*0.5f+0.5f)*vpHeight);

	glEnable(GL_SCISSOR_TEST);
	glScissor(screenScissor.x1, screenScissor.y1, screenScissor.x2 - screenScissor.x1, screenScissor.y2 - screenScissor.y1);

	// try binding the framebuffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, wm.m_RefractionFbo);

	glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Render terrain and models
	RenderPatches(context, CULL_REFRACTIONS);
	ogl_WarnIfError();
	RenderModels(context, CULL_REFRACTIONS);
	ogl_WarnIfError();
	RenderTransparentModels(context, CULL_REFRACTIONS, TRANSPARENT_OPAQUE, false);
	ogl_WarnIfError();

	glDisable(GL_SCISSOR_TEST);

	// Reset old camera
	m_ViewCamera = normalCamera;
	g_Renderer.SetViewport(m_ViewCamera.GetViewPort());

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void CSceneRenderer::RenderSilhouettes(const CShaderDefines& context)
{
	PROFILE3_GPU("silhouettes");
	OGL_SCOPED_DEBUG_GROUP("Render water silhouettes");

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

	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glColorMask(0, 0, 0, 0);

	// Render occluders:

	{
		PROFILE("render patches");

		// To prevent units displaying silhouettes when parts of their model
		// protrude into the ground, only occlude with the back faces of the
		// terrain (so silhouettes will still display when behind hills)
		glCullFace(GL_FRONT);
		m->terrainRenderer.RenderPatches(CULL_SILHOUETTE_OCCLUDER);
		glCullFace(GL_BACK);
	}

	{
		PROFILE("render model occluders");
		m->CallModelRenderers(contextOccluder, CULL_SILHOUETTE_OCCLUDER, 0);
	}

	{
		PROFILE("render transparent occluders");
		m->CallTranspModelRenderers(contextOccluder, CULL_SILHOUETTE_OCCLUDER, 0);
	}

	glDepthFunc(GL_GEQUAL);
	glColorMask(1, 1, 1, 1);

	// Since we can't sort, we'll use the stencil buffer to ensure we only draw
	// a pixel once (using the color of whatever model happens to be drawn first).
	glEnable(GL_BLEND);
	glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
	const float silhouetteAlpha = 0.75f;
	glBlendColorEXT(0, 0, 0, silhouetteAlpha);

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 1, (GLuint)-1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	{
		PROFILE("render model casters");
		m->CallModelRenderers(contextDisplay, CULL_SILHOUETTE_CASTER, 0);
	}

	{
		PROFILE("render transparent casters");
		m->CallTranspModelRenderers(contextDisplay, CULL_SILHOUETTE_CASTER, 0);
	}

	// Restore state
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendColorEXT(0, 0, 0, 0);
	glDisable(GL_STENCIL_TEST);
}

void CSceneRenderer::RenderParticles(int cullGroup)
{
	PROFILE3_GPU("particles");
	OGL_SCOPED_DEBUG_GROUP("Render particles");

	m->particleRenderer.RenderParticles(cullGroup);

#if !CONFIG2_GLES
	if (m_ModelRenderMode == EDGED_FACES)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		m->particleRenderer.RenderParticles(true);
		m->particleRenderer.RenderBounds(cullGroup);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
#endif
}

// RenderSubmissions: force rendering of any batched objects
void CSceneRenderer::RenderSubmissions(const CBoundingBoxAligned& waterScissor)
{
	PROFILE3("render submissions");
	OGL_SCOPED_DEBUG_GROUP("Render submissions");

	GetScene().GetLOSTexture().InterpolateLOS();

	GetScene().GetMiniMapTexture().Render();

	CShaderDefines context = m->globalContext;

	int cullGroup = CULL_DEFAULT;

	ogl_WarnIfError();

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

	const CRenderer::Caps& capabilities = g_Renderer.GetCapabilities();
	if (capabilities.m_Shadows && g_RenderingOptions.GetShadows())
	{
		RenderShadowMap(context);
	}

	ogl_WarnIfError();

	if (m->waterManager.m_RenderWater)
	{
		if (waterScissor.GetVolume() > 0 && m->waterManager.WillRenderFancyWater())
		{
			PROFILE3_GPU("water scissor");
			RenderReflections(context, waterScissor);

			if (g_RenderingOptions.GetWaterRefraction())
				RenderRefractions(context, waterScissor);
		}
	}

	CPostprocManager& postprocManager = g_Renderer.GetPostprocManager();
	if (g_RenderingOptions.GetPostProc())
	{
		// We have to update the post process manager with real near/far planes
		// that we use for the scene rendering.
		postprocManager.SetDepthBufferClipPlanes(
			m_ViewCamera.GetNearPlane(), m_ViewCamera.GetFarPlane()
		);
		postprocManager.Initialize();
		postprocManager.CaptureRenderOutput();
	}

	{
		PROFILE3_GPU("clear buffers");
		glClearColor(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	m->skyManager.RenderSky();

	// render submitted patches and models
	RenderPatches(context, cullGroup);
	ogl_WarnIfError();

	// render debug-related terrain overlays
	ITerrainOverlay::RenderOverlaysBeforeWater();
	ogl_WarnIfError();

	// render other debug-related overlays before water (so they can be seen when underwater)
	m->overlayRenderer.RenderOverlaysBeforeWater();
	ogl_WarnIfError();

	RenderModels(context, cullGroup);
	ogl_WarnIfError();

	// render water
	if (m->waterManager.m_RenderWater && g_Game && waterScissor.GetVolume() > 0)
	{
		if (m->waterManager.WillRenderFancyWater())
		{
			// Render transparent stuff, but only the solid parts that can occlude block water.
			RenderTransparentModels(context, cullGroup, TRANSPARENT_OPAQUE, false);
			ogl_WarnIfError();

			m->terrainRenderer.RenderWater(context, cullGroup, &m->shadow);
			ogl_WarnIfError();

			// Render transparent stuff again, but only the blended parts that overlap water.
			RenderTransparentModels(context, cullGroup, TRANSPARENT_BLEND, false);
			ogl_WarnIfError();
		}
		else
		{
			m->terrainRenderer.RenderWater(context, cullGroup, &m->shadow);
			ogl_WarnIfError();

			// Render transparent stuff, so it can overlap models/terrain.
			RenderTransparentModels(context, cullGroup, TRANSPARENT, false);
			ogl_WarnIfError();
		}
	}
	else
	{
		// render transparent stuff, so it can overlap models/terrain
		RenderTransparentModels(context, cullGroup, TRANSPARENT, false);
		ogl_WarnIfError();
	}

	// render debug-related terrain overlays
	ITerrainOverlay::RenderOverlaysAfterWater(cullGroup);
	ogl_WarnIfError();

	// render some other overlays after water (so they can be displayed on top of water)
	m->overlayRenderer.RenderOverlaysAfterWater();
	ogl_WarnIfError();

	// particles are transparent so render after water
	if (g_RenderingOptions.GetParticles())
	{
		RenderParticles(cullGroup);
		ogl_WarnIfError();
	}

	if (g_RenderingOptions.GetPostProc())
	{
		if (g_Renderer.GetPostprocManager().IsMultisampleEnabled())
			g_Renderer.GetPostprocManager().ResolveMultisampleFramebuffer();

		postprocManager.ApplyPostproc();
		postprocManager.ReleaseRenderOutput();
	}

	if (g_RenderingOptions.GetSilhouettes())
	{
		RenderSilhouettes(context);
	}

	// render debug lines
	if (g_RenderingOptions.GetDisplayFrustum())
		DisplayFrustum();

	if (g_RenderingOptions.GetDisplayShadowsFrustum())
	{
		m->shadow.RenderDebugBounds();
		m->shadow.RenderDebugTexture();
	}

	m->silhouetteRenderer.RenderDebugOverlays(m_ViewCamera);

	// render overlays that should appear on top of all other objects
	m->overlayRenderer.RenderForegroundOverlays(m_ViewCamera);
	ogl_WarnIfError();

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

// DisplayFrustum: debug displays
//  - white: cull camera frustum
//  - red: bounds of shadow casting objects
void CSceneRenderer::DisplayFrustum()
{
#if CONFIG2_GLES
#warning TODO: implement CSceneRenderer::DisplayFrustum for GLES
#else
	glDepthMask(0);
	glDisable(GL_CULL_FACE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	g_Renderer.GetDebugRenderer().DrawCameraFrustum(m_CullCamera, CColor(1.0f, 1.0f, 1.0f, 0.25f), 2);
	glDisable(GL_BLEND);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	g_Renderer.GetDebugRenderer().DrawCameraFrustum(m_CullCamera, CColor(1.0f, 1.0f, 1.0f, 1.0f), 2);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glEnable(GL_CULL_FACE);
	glDepthMask(1);
#endif

	ogl_WarnIfError();
}

// Text overlay rendering
void CSceneRenderer::RenderTextOverlays()
{
	PROFILE3_GPU("text overlays");

	if (m_DisplayTerrainPriorities)
		m->terrainRenderer.RenderPriorities(CULL_DEFAULT);

	ogl_WarnIfError();
}

// SetSceneCamera: setup projection and transform of camera and adjust viewport to current view
// The camera always represents the actual camera used to render a scene, not any virtual camera
// used for shadow rendering or reflections.
void CSceneRenderer::SetSceneCamera(const CCamera& viewCamera, const CCamera& cullCamera)
{
	m_ViewCamera = viewCamera;
	m_CullCamera = cullCamera;
	
	const CRenderer::Caps& capabilities = g_Renderer.GetCapabilities();
	if (capabilities.m_Shadows && g_RenderingOptions.GetShadows())
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
void CSceneRenderer::RenderScene(Scene& scene)
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

	const CRenderer::Caps& capabilities = g_Renderer.GetCapabilities();
	if (capabilities.m_Shadows && g_RenderingOptions.GetShadows())
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
			m->waterManager.RenderWaves(frustum);
		}
	}

	m_CurrentCullGroup = -1;

	ogl_WarnIfError();

	RenderSubmissions(waterScissor);

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
