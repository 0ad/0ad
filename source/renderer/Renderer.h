/* Copyright (C) 2014 Wildfire Games.
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
#include "graphics/ShaderProgramPtr.h"
#include "lib/res/handle.h"
#include "ps/Singleton.h"

#include "graphics/ShaderDefines.h"
#include "renderer/PostprocManager.h"
#include "renderer/Scene.h"
#include "renderer/TimeManager.h"
#include "scriptinterface/ScriptInterface.h"

// necessary declarations
class CFontManager;
class CLightEnv;
class CMaterial;
class CMaterialManager;
class CModel;
class CParticleManager;
class CPatch;
class CShaderManager;
class CSimulation2;
class CTextureManager;
class CTimeManager;
class RenderPathVertexShader;
class SkyManager;
class TerrainRenderer;
class WaterManager;

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
	private SceneCollector
{
public:
	// various enumerations and renderer related constants
	enum { NumAlphaMaps=14 };
	enum Option {
		OPT_NOVBO,
		OPT_SHADOWS,
		OPT_WATERUGLY,
		OPT_WATERFANCYEFFECTS,
		OPT_WATERREALDEPTH,
		OPT_WATERREFLECTION,
		OPT_WATERREFRACTION,
		OPT_SHADOWSONWATER,
		OPT_SHADOWPCF,
		OPT_PARTICLES,
		OPT_GENTANGENTS,
		OPT_PREFERGLSL,
		OPT_SILHOUETTES,
		OPT_SHOWSKY,
		OPT_SMOOTHLOS,
		OPT_POSTPROC,
		OPT_DISPLAYFRUSTUM,
	};

	enum CullGroup {
		CULL_DEFAULT,
		CULL_SHADOWS,
		CULL_REFLECTIONS,
		CULL_REFRACTIONS,
		CULL_SILHOUETTE_OCCLUDER,
		CULL_SILHOUETTE_CASTER,
		CULL_MAX
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
		
		bool m_WaterUgly;
		bool m_WaterFancyEffects;
		bool m_WaterRealDepth;
		bool m_WaterRefraction;
		bool m_WaterReflection;
		bool m_WaterShadows;

		RenderPath m_RenderPath;
		bool m_ShadowAlphaFix;
		bool m_ARBProgramShadow;
		bool m_ShadowPCF;
		bool m_Particles;
		bool m_PreferGLSL;
		bool m_ForceAlphaTest;
		bool m_GPUSkinning;
		bool m_Silhouettes;
		bool m_GenTangents;
		bool m_SmoothLOS;
		bool m_ShowSky;
		bool m_Postproc;
		bool m_DisplayFrustum;
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
	void SetOptionBool(enum Option opt, bool value);
	bool GetOptionBool(enum Option opt) const;
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

	/**
	 * Set simulation context for rendering purposes.
	 * Must be called at least once when the game has started and before
	 * frames are rendered.
	 */
	void SetSimulation(CSimulation2* simulation);

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

	// get the last viewport
	SViewPort GetViewport();

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
	// replace the current view camera
	void SetViewCamera(const CCamera& camera) { m_ViewCamera = camera; }

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

	TerrainRenderer& GetTerrainRenderer();

	CMaterialManager& GetMaterialManager();

	CFontManager& GetFontManager();

	CShaderDefines GetSystemShaderDefines() { return m_SystemShaderDefines; }
	
	CTimeManager& GetTimeManager();
	
	CPostprocManager& GetPostprocManager();

	/**
	 * GetCapabilities: Return which OpenGL capabilities are available and enabled.
	 *
	 * @return capabilities structure
	 */
	const Caps& GetCapabilities() const { return m_Caps; }

	static void RegisterScriptFunctions(ScriptInterface& scriptInterface);

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
	friend class ShaderModelVertexRenderer;
	friend class InstancingModelRenderer;
	friend class ShaderInstancingModelRenderer;
	friend class TerrainRenderer;
	friend class WaterRenderer;

	//BEGIN: Implementation of SceneCollector
	void Submit(CPatch* patch);
	void Submit(SOverlayLine* overlay);
	void Submit(SOverlayTexturedLine* overlay);
	void Submit(SOverlaySprite* overlay);
	void Submit(SOverlayQuad* overlay);
	void Submit(CModelDecal* decal);
	void Submit(CParticleEmitter* emitter);
	void Submit(SOverlaySphere* overlay);
	void SubmitNonRecursive(CModel* model);
	//END: Implementation of SceneCollector

	// render any batched objects
	void RenderSubmissions(const CBoundingBoxAligned& waterScissor);

	// patch rendering stuff
	void RenderPatches(const CShaderDefines& context, int cullGroup);

	// model rendering stuff
	void RenderModels(const CShaderDefines& context, int cullGroup);
	void RenderTransparentModels(const CShaderDefines& context, int cullGroup, ETransparentMode transparentMode, bool disableFaceCulling);

	void RenderSilhouettes(const CShaderDefines& context);

	void RenderParticles(int cullGroup);

	// shadow rendering stuff
	void RenderShadowMap(const CShaderDefines& context);

	// render water reflection and refraction textures
	void RenderReflections(const CShaderDefines& context, const CBoundingBoxAligned& scissor);
	void RenderRefractions(const CShaderDefines& context, const CBoundingBoxAligned& scissor);

	void ComputeReflectionCamera(CCamera& camera, const CBoundingBoxAligned& scissor) const;
	void ComputeRefractionCamera(CCamera& camera, const CBoundingBoxAligned& scissor) const;

	// debugging
	void DisplayFrustum();

	// enable oblique frustum clipping with the given clip plane
	void SetObliqueFrustumClipping(CCamera& camera, const CVector4D& clipPlane) const;

	void ReloadShaders();
	void RecomputeSystemShaderDefines();

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

	CShaderDefines m_SystemShaderDefines;

	SViewPort m_Viewport;

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
