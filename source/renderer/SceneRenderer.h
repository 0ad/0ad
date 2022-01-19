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

#ifndef INCLUDED_RENDERER_SCENERENDERER
#define INCLUDED_RENDERER_SCENERENDERER

#include "graphics/Camera.h"
#include "graphics/ShaderDefines.h"
#include "graphics/ShaderProgramPtr.h"
#include "ps/Singleton.h"
#include "renderer/backend/gl/DeviceCommandContext.h"
#include "renderer/RenderingOptions.h"
#include "renderer/Scene.h"

#include <memory>

class CLightEnv;
class CMaterial;
class CMaterialManager;
class CModel;
class CParticleManager;
class CPatch;
class CSimulation2;
class ShadowMap;
class SkyManager;
class TerrainRenderer;
class WaterManager;

// rendering modes
enum ERenderMode { WIREFRAME, SOLID, EDGED_FACES };

// transparency modes
enum ETransparentMode { TRANSPARENT, TRANSPARENT_OPAQUE, TRANSPARENT_BLEND };

class CSceneRenderer : public SceneCollector
{
public:
	enum CullGroup
	{
		CULL_DEFAULT,
		CULL_SHADOWS_CASCADE_0,
		CULL_SHADOWS_CASCADE_1,
		CULL_SHADOWS_CASCADE_2,
		CULL_SHADOWS_CASCADE_3,
		CULL_REFLECTIONS,
		CULL_REFRACTIONS,
		CULL_SILHOUETTE_OCCLUDER,
		CULL_SILHOUETTE_CASTER,
		CULL_MAX
	};

	CSceneRenderer();
	~CSceneRenderer();

	void Initialize();
	void Resize(int width, int height);

	void BeginFrame();
	void EndFrame();

	/**
	 * Set simulation context for rendering purposes.
	 * Must be called at least once when the game has started and before
	 * frames are rendered.
	 */
	void SetSimulation(CSimulation2* simulation);

	// trigger a reload of shaders (when parameters they depend on have changed)
	void MakeShadersDirty();

	/**
	 * Set up the camera used for rendering the next scene; this includes
	 * setting OpenGL state like viewport, projection and modelview matrices.
	 *
	 * @param viewCamera this camera determines the eye position for rendering
	 * @param cullCamera this camera determines the frustum for culling in the renderer and
	 * for shadow calculations
	 */
	void SetSceneCamera(const CCamera& viewCamera, const CCamera& cullCamera);

	/**
	 * Render the given scene immediately.
	 * @param scene a Scene object describing what should be rendered.
	 */
	void RenderScene(Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext, Scene& scene);

	/**
	 * Return the scene that is currently being rendered.
	 * Only valid when the renderer is in a RenderScene call.
	 */
	Scene& GetScene();

	/**
	 * Render text overlays on top of the scene.
	 * Assumes the caller has set up the GL environment for orthographic rendering
	 * with texturing and blending.
	 */
	void RenderTextOverlays();

	// set the current lighting environment; (note: the passed pointer is just copied to a variable within the renderer,
	// so the lightenv passed must be scoped such that it is not destructed until after the renderer is no longer rendering)
	void SetLightEnv(CLightEnv* lightenv)
	{
		m_LightEnv = lightenv;
	}

	// set the mode to render subsequent terrain patches
	void SetTerrainRenderMode(ERenderMode mode) { m_TerrainRenderMode = mode; }
	// get the mode to render subsequent terrain patches
	ERenderMode GetTerrainRenderMode() const { return m_TerrainRenderMode; }

	// set the mode to render subsequent water patches
	void SetWaterRenderMode(ERenderMode mode) { m_WaterRenderMode = mode; }
	// get the mode to render subsequent water patches
	ERenderMode GetWaterRenderMode() const { return m_WaterRenderMode; }

	// set the mode to render subsequent models
	void SetModelRenderMode(ERenderMode mode) { m_ModelRenderMode = mode; }
	// get the mode to render subsequent models
	ERenderMode GetModelRenderMode() const { return m_ModelRenderMode; }

	// Get the mode to render subsequent overlays.
	ERenderMode GetOverlayRenderMode() const { return m_OverlayRenderMode; }
	// Set the mode to render subsequent overlays.
	void SetOverlayRenderMode(ERenderMode mode) { m_OverlayRenderMode = mode; }

	// debugging
	void SetDisplayTerrainPriorities(bool enabled) { m_DisplayTerrainPriorities = enabled; }

	// return the current light environment
	const CLightEnv &GetLightEnv() { return *m_LightEnv; }

	// return the current view camera
	const CCamera& GetViewCamera() const { return m_ViewCamera; }
	// replace the current view camera
	void SetViewCamera(const CCamera& camera) { m_ViewCamera = camera; }

	// return the current cull camera
	const CCamera& GetCullCamera() const { return m_CullCamera; }

	/**
	 * GetWaterManager: Return the renderer's water manager.
	 *
	 * @return the WaterManager object used by the renderer
	 */
	WaterManager& GetWaterManager();

	/**
	 * GetSkyManager: Return the renderer's sky manager.
	 *
	 * @return the SkyManager object used by the renderer
	 */
	SkyManager& GetSkyManager();

	CParticleManager& GetParticleManager();

	TerrainRenderer& GetTerrainRenderer();

	CMaterialManager& GetMaterialManager();

	ShadowMap& GetShadowMap();

	/**
	 * Resets the render state to default, that was before a game started
	 */
	void ResetState();

	void ReloadShaders();

protected:
	void Submit(CPatch* patch) override;
	void Submit(SOverlayLine* overlay) override;
	void Submit(SOverlayTexturedLine* overlay) override;
	void Submit(SOverlaySprite* overlay) override;
	void Submit(SOverlayQuad* overlay) override;
	void Submit(CModelDecal* decal) override;
	void Submit(CParticleEmitter* emitter) override;
	void Submit(SOverlaySphere* overlay) override;
	void SubmitNonRecursive(CModel* model) override;

	// render any batched objects
	void RenderSubmissions(
		Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
		const CBoundingBoxAligned& waterScissor);

	// patch rendering stuff
	void RenderPatches(
		Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
		const CShaderDefines& context, int cullGroup);

	// model rendering stuff
	void RenderModels(
		Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
		const CShaderDefines& context, int cullGroup);
	void RenderTransparentModels(
		Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
		const CShaderDefines& context, int cullGroup, ETransparentMode transparentMode, bool disableFaceCulling);

	void RenderSilhouettes(
		Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
		const CShaderDefines& context);

	void RenderParticles(
		Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
		int cullGroup);

	// shadow rendering stuff
	void RenderShadowMap(
		Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
		const CShaderDefines& context);

	// render water reflection and refraction textures
	void RenderReflections(
		Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
		const CShaderDefines& context, const CBoundingBoxAligned& scissor);
	void RenderRefractions(
		Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
		const CShaderDefines& context, const CBoundingBoxAligned& scissor);

	void ComputeReflectionCamera(CCamera& camera, const CBoundingBoxAligned& scissor) const;
	void ComputeRefractionCamera(CCamera& camera, const CBoundingBoxAligned& scissor) const;

	// debugging
	void DisplayFrustum();

	// enable oblique frustum clipping with the given clip plane
	void SetObliqueFrustumClipping(CCamera& camera, const CVector4D& clipPlane) const;

	// Private data that is not needed by inline functions.
	class Internals;
	std::unique_ptr<Internals> m;

	// Current terrain rendering mode.
	ERenderMode m_TerrainRenderMode;
	// Current water rendering mode.
	ERenderMode m_WaterRenderMode;
	// Current model rendering mode.
	ERenderMode m_ModelRenderMode;
	// Current overlay rendering mode.
	ERenderMode m_OverlayRenderMode;

	/**
	 * m_ViewCamera: determines the eye position for rendering
	 *
	 * @see CGameView::m_ViewCamera
	 */
	CCamera m_ViewCamera;

	/**
	 * m_CullCamera: determines the frustum for culling and shadowmap calculations
	 *
	 * @see CGameView::m_ViewCamera
	 */
	CCamera m_CullCamera;

	// only valid inside a call to RenderScene
	Scene* m_CurrentScene;
	int m_CurrentCullGroup;

	// color used to clear screen in BeginFrame
	float m_ClearColor[4];
	// current lighting setup
	CLightEnv* m_LightEnv;

	/**
	 * Enable rendering of terrain tile priority text overlay, for debugging.
	 */
	bool m_DisplayTerrainPriorities;
};

#endif // INCLUDED_RENDERER_SCENERENDERER
