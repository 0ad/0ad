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
 * higher level interface on top of OpenGL to render basic objects:
 * terrain, models, sprites, particles etc.
 */

#ifndef INCLUDED_RENDERER
#define INCLUDED_RENDERER

#include "graphics/Camera.h"
#include "graphics/SColor.h"
#include "graphics/ShaderProgram.h"
#include "lib/res/handle.h"
#include "ps/Singleton.h"
#include "scripting/ScriptableObject.h"

#include "renderer/Scene.h"

// necessary declarations
class CPatch;
class CMaterial;
class CModel;
class CLightEnv;

class RenderPathVertexShader;
class WaterManager;
class SkyManager;
class CTextureManager;
class CShaderManager;
class CParticleManager;

// rendering modes
enum ERenderMode { WIREFRAME, SOLID, EDGED_FACES };

// transparency modes
enum ETransparentMode { TRANSPARENT, TRANSPARENT_OPAQUE, TRANSPARENT_BLEND };

// access to sole renderer object
#define g_Renderer CRenderer::GetSingleton()

struct SScreenRect
{
	GLint x1, y1, x2, y2;
};

///////////////////////////////////////////////////////////////////////////////////////////
// CRenderer: base renderer class - primary interface to the rendering engine
struct CRendererInternals;

class CRenderer :
	public Singleton<CRenderer>,
	public CJSObject<CRenderer>,
	private SceneCollector
{
public:
	// various enumerations and renderer related constants
	enum { NumAlphaMaps=14 };
	enum Option {
		OPT_NOVBO,
		OPT_SHADOWS,
		OPT_FANCYWATER,
		OPT_LODBIAS,
		OPT_SHADOWPCF
	};

	enum RenderPath {
		// If no rendering path is configured explicitly, the renderer
		// will choose the path when Open() is called.
		RP_DEFAULT,

		// Classic fixed function.
		RP_FIXED,

		// Use new ARB/GLSL system
		RP_SHADER
	};

	// stats class - per frame counts of number of draw calls, poly counts etc
	struct Stats {
		// set all stats to zero
		void Reset() { memset(this, 0, sizeof(*this)); }
		// number of draw calls per frame - total DrawElements + Begin/End immediate mode loops
		size_t m_DrawCalls;
		// number of terrain triangles drawn
		size_t m_TerrainTris;
		// number of water triangles drawn
		size_t m_WaterTris;
		// number of (non-transparent) model triangles drawn
		size_t m_ModelTris;
		// number of overlay triangles drawn
		size_t m_OverlayTris;
		// number of splat passes for alphamapping
		size_t m_BlendSplats;
		// number of particles
		size_t m_Particles;
	};

	// renderer options
	struct Options {
		bool m_NoVBO;
		bool m_Shadows;
		bool m_FancyWater;
		float m_LodBias;
		RenderPath m_RenderPath;
		bool m_ShadowAlphaFix;
		bool m_ARBProgramShadow;
		bool m_ShadowPCF;
		bool m_PreferGLSL;
	} m_Options;

	struct Caps {
		bool m_VBO;
		bool m_ARBProgram;
		bool m_ARBProgramShadow;
		bool m_VertexShader;
		bool m_FragmentShader;
		bool m_Shadows;
	};

public:
	// constructor, destructor
	CRenderer();
	~CRenderer();

	// open up the renderer: performs any necessary initialisation
	bool Open(int width,int height);

	// resize renderer view
	void Resize(int width,int height);

	// set/get boolean renderer option
	void SetOptionBool(enum Option opt,bool value);
	bool GetOptionBool(enum Option opt) const;
	void SetOptionFloat(enum Option opt, float val);
	void SetRenderPath(RenderPath rp);
	RenderPath GetRenderPath() const { return m_Options.m_RenderPath; }
	static CStr GetRenderPathName(RenderPath rp);
	static RenderPath GetRenderPathByName(const CStr& name);

	// return view width
	int GetWidth() const { return m_Width; }
	// return view height
	int GetHeight() const { return m_Height; }
	// return view aspect ratio
	float GetAspect() const { return float(m_Width)/float(m_Height); }

	// signal frame start
	void BeginFrame();
	// signal frame end
	void EndFrame();

	// set color used to clear screen in BeginFrame()
	void SetClearColor(SColor4ub color);

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

	// set the viewport
	void SetViewport(const SViewPort &);

	/**
	 * Render the given scene immediately.
	 * @param scene a Scene object describing what should be rendered.
	 */
	void RenderScene(Scene& scene);

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
	void SetLightEnv(CLightEnv* lightenv) {
		m_LightEnv=lightenv;
	}

	// set the mode to render subsequent terrain patches
	void SetTerrainRenderMode(ERenderMode mode) { m_TerrainRenderMode=mode; }
	// get the mode to render subsequent terrain patches
	ERenderMode GetTerrainRenderMode() const { return m_TerrainRenderMode; }

	// set the mode to render subsequent models
	void SetModelRenderMode(ERenderMode mode) { m_ModelRenderMode=mode; }
	// get the mode to render subsequent models
	ERenderMode GetModelRenderMode() const { return m_ModelRenderMode; }

	// debugging
	void SetDisplayTerrainPriorities(bool enabled) { m_DisplayTerrainPriorities = enabled; }

	// bind a GL texture object to active unit
	void BindTexture(int unit, unsigned int tex);

	// load the default set of alphamaps.
	// return a negative error code if anything along the way fails.
	// called via delay-load mechanism.
	int LoadAlphaMaps();
	void UnloadAlphaMaps();

	// return stats accumulated for current frame
	Stats& GetStats() { return m_Stats; }

	// return the current light environment
	const CLightEnv &GetLightEnv() { return *m_LightEnv; }

	// return the current view camera
	const CCamera& GetViewCamera() const { return m_ViewCamera; }
	// return the current cull camera
	const CCamera& GetCullCamera() const { return m_CullCamera; }

	/**
	 * GetWaterManager: Return the renderer's water manager.
	 *
	 * @return the WaterManager object used by the renderer
	 */
	WaterManager* GetWaterManager() { return m_WaterManager; }

	/**
	 * GetSkyManager: Return the renderer's sky manager.
	 *
	 * @return the SkyManager object used by the renderer
	 */
	SkyManager* GetSkyManager() { return m_SkyManager; }

	CTextureManager& GetTextureManager();

	CShaderManager& GetShaderManager();

	CParticleManager& GetParticleManager();

	/**
	 * SetFastPlayerColor: Tell the renderer which path to take for
	 * player colored models. Both paths should provide the same visual
	 * quality, however the slow path runs on older hardware using multi-pass.
	 *
	 * @param fast true if the fast path should be used from now on. If fast
	 * is true but the OpenGL implementation does not support it, a warning
	 * is printed and the slow path is used instead.
	 */
	void SetFastPlayerColor(bool fast);

	/**
	 * GetCapabilities: Return which OpenGL capabilities are available and enabled.
	 *
	 * @return capabilities structure
	 */
	const Caps& GetCapabilities() const { return m_Caps; }

	bool GetDisableCopyShadow() const { return m_DisableCopyShadow; }

	static void ScriptingInit();

protected:
	friend struct CRendererInternals;
	friend class CVertexBuffer;
	friend class CPatchRData;
	friend class CDecalRData;
	friend class FixedFunctionModelRenderer;
	friend class ModelRenderer;
	friend class PolygonSortModelRenderer;
	friend class SortModelRenderer;
	friend class RenderPathVertexShader;
	friend class HWLightingModelRenderer;
	friend class ShaderModelRenderer;
	friend class InstancingModelRenderer;
	friend class ShaderInstancingModelRenderer;
	friend class TerrainRenderer;

	// scripting
	// TODO: Perhaps we could have a version of AddLocalProperty for function-driven
	// properties? Then we could hide these function in the private implementation class.
	jsval JSI_GetFastPlayerColor(JSContext*);
	void JSI_SetFastPlayerColor(JSContext* ctx, jsval newval);
	jsval JSI_GetRenderPath(JSContext*);
	void JSI_SetRenderPath(JSContext* ctx, jsval newval);
	jsval JSI_GetDepthTextureBits(JSContext*);
	void JSI_SetDepthTextureBits(JSContext* ctx, jsval newval);
	jsval JSI_GetShadows(JSContext*);
	void JSI_SetShadows(JSContext* ctx, jsval newval);
	jsval JSI_GetShadowAlphaFix(JSContext*);
	void JSI_SetShadowAlphaFix(JSContext* ctx, jsval newval);
	jsval JSI_GetShadowPCF(JSContext*);
	void JSI_SetShadowPCF(JSContext* ctx, jsval newval);
	jsval JSI_GetPreferGLSL(JSContext*);
	void JSI_SetPreferGLSL(JSContext* ctx, jsval newval);
	jsval JSI_GetSky(JSContext*);
	void JSI_SetSky(JSContext* ctx, jsval newval);

	//BEGIN: Implementation of SceneCollector
	void Submit(CPatch* patch);
	void Submit(SOverlayLine* overlay);
	void Submit(SOverlayTexturedLine* overlay);
	void Submit(SOverlaySprite* overlay);
	void Submit(CModelDecal* decal);
	void Submit(CParticleEmitter* emitter);
	void SubmitNonRecursive(CModel* model);
	//END: Implementation of SceneCollector

	// render any batched objects
	void RenderSubmissions();

	// patch rendering stuff
	void RenderPatches(const CFrustum* frustum = 0);

	// model rendering stuff
	void RenderModels(const CFrustum* frustum = 0);
	void RenderTransparentModels(ETransparentMode transparentMode, const CFrustum* frustum = 0);

	void RenderSilhouettes();

	void RenderParticles();

	// shadow rendering stuff
	void RenderShadowMap();

	// render water reflection and refraction textures
	SScreenRect RenderReflections(const CBoundingBoxAligned& scissor);
	SScreenRect RenderRefractions(const CBoundingBoxAligned& scissor);

	// debugging
	void DisplayFrustum();

	// enable oblique frustum clipping with the given clip plane
	void SetObliqueFrustumClipping(const CVector4D& clipPlane);

	void ReloadShaders();

	// hotloading
	static Status ReloadChangedFileCB(void* param, const VfsPath& path);

	// RENDERER DATA:
	/// Private data that is not needed by inline functions
	CRendererInternals* m;
	// view width
	int m_Width;
	// view height
	int m_Height;
	// current terrain rendering mode
	ERenderMode m_TerrainRenderMode;
	// current model rendering mode
	ERenderMode m_ModelRenderMode;

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

	// color used to clear screen in BeginFrame
	float m_ClearColor[4];
	// current lighting setup
	CLightEnv* m_LightEnv;
	// ogl_tex handle of composite alpha map (all the alpha maps packed into one texture)
	Handle m_hCompositeAlphaMap;
	// coordinates of each (untransformed) alpha map within the packed texture
	struct {
		float u0,u1,v0,v1;
	} m_AlphaMapCoords[NumAlphaMaps];
	// card capabilities
	Caps m_Caps;
	// build card cap bits
	void EnumCaps();
	// per-frame renderer stats
	Stats m_Stats;

	/// If false, use a multipass fallback for player colors.
	bool m_FastPlayerColor;

	/**
	 * m_WaterManager: the WaterManager object used for water textures and settings
	 * (e.g. water color, water height)
	 */
	WaterManager* m_WaterManager;

	/**
	 * m_SkyManager: the SkyManager object used for sky textures and settings
	 */
	SkyManager* m_SkyManager;

	/**
	 * m_SortAllTransparent: If true, all transparent models are
	 * rendered using the TransparencyRenderer which performs sorting.
	 *
	 * Otherwise, transparent models are rendered using the faster
	 * batching renderer when possible.
	 */
	bool m_SortAllTransparent;

	/**
	 * m_DisplayFrustum: Render the cull frustum and other data that may be interesting
	 * to evaluate culling and shadow map calculations
	 *
	 * Can be controlled from JS via renderer.displayFrustum
	 */
	bool m_DisplayFrustum;

	/**
	 * m_DisableCopyShadow: For debugging purpose:
	 * Disable copying of shadow data into the shadow texture (when EXT_fbo is not available)
	 */
	bool m_DisableCopyShadow;

	/**
	 * Enable rendering of terrain tile priority text overlay, for debugging.
	 */
	bool m_DisplayTerrainPriorities;

public:
	/**
	 * m_ShadowZBias: Z bias used when rendering shadows into a depth texture.
	 * This can be used to control shadowing artifacts.
	 *
	 * Can be accessed via JS as renderer.shadowZBias
	 * ShadowMap uses this for matrix calculation.
	 */
	float m_ShadowZBias;

	/**
	 * m_ShadowMapSize: Size of shadow map, or 0 for default. Typically slow but useful
	 * for high-quality rendering. Changes don't take effect until the shadow map
	 * is regenerated.
	 *
	 * Can be accessed via JS as renderer.shadowMapSize
	 */
	int m_ShadowMapSize;

	/**
	 * m_SkipSubmit: Disable the actual submission of rendering commands to OpenGL.
	 * All state setup is still performed as usual.
	 *
	 * Can be accessed via JS as renderer.skipSubmit
	 */
	bool m_SkipSubmit;
};


#endif
