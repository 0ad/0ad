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
 * higher level interface on top of OpenGL to render basic objects:
 * terrain, models, sprites, particles etc.
 */

#include "precompiled.h"

#include <map>
#include <set>
#include <algorithm>

#include <boost/algorithm/string.hpp>

#include "Renderer.h"

#include "lib/bits.h"	// is_pow2
#include "lib/res/graphics/ogl_tex.h"
#include "lib/allocators/shared_ptr.h"
#include "maths/Matrix3D.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "ps/Filesystem.h"
#include "ps/World.h"
#include "ps/Loader.h"
#include "ps/ProfileViewer.h"
#include "graphics/Camera.h"
#include "graphics/GameView.h"
#include "graphics/LightEnv.h"
#include "graphics/LOSTexture.h"
#include "graphics/MaterialManager.h"
#include "graphics/Model.h"
#include "graphics/ModelDef.h"
#include "graphics/ParticleManager.h"
#include "graphics/ShaderManager.h"
#include "graphics/Terrain.h"
#include "graphics/Texture.h"
#include "graphics/TextureManager.h"
#include "renderer/HWLightingModelRenderer.h"
#include "renderer/InstancingModelRenderer.h"
#include "renderer/ModelRenderer.h"
#include "renderer/OverlayRenderer.h"
#include "renderer/ParticleRenderer.h"
#include "renderer/RenderModifiers.h"
#include "renderer/ShadowMap.h"
#include "renderer/SkyManager.h"
#include "renderer/TerrainOverlay.h"
#include "renderer/TerrainRenderer.h"
#include "renderer/TimeManager.h"
#include "renderer/VertexBufferManager.h"
#include "renderer/WaterManager.h"

extern bool g_GameRestarted;

///////////////////////////////////////////////////////////////////////////////////
// CRendererStatsTable - Profile display of rendering stats

/**
 * Class CRendererStatsTable: Implementation of AbstractProfileTable to
 * display the renderer stats in-game.
 *
 * Accesses CRenderer::m_Stats by keeping the reference passed to the
 * constructor.
 */
class CRendererStatsTable : public AbstractProfileTable
{
	NONCOPYABLE(CRendererStatsTable);
public:
	CRendererStatsTable(const CRenderer::Stats& st);

	// Implementation of AbstractProfileTable interface
	CStr GetName();
	CStr GetTitle();
	size_t GetNumberRows();
	const std::vector<ProfileColumn>& GetColumns();
	CStr GetCellText(size_t row, size_t col);
	AbstractProfileTable* GetChild(size_t row);

private:
	/// Reference to the renderer singleton's stats
	const CRenderer::Stats& Stats;

	/// Column descriptions
	std::vector<ProfileColumn> columnDescriptions;

	enum {
		Row_DrawCalls = 0,
		Row_TerrainTris,
		Row_WaterTris,
		Row_ModelTris,
		Row_OverlayTris,
		Row_BlendSplats,
		Row_Particles,
		Row_VBReserved,
		Row_VBAllocated,
		Row_ShadersLoaded,

		// Must be last to count number of rows
		NumberRows
	};
};

// Construction
CRendererStatsTable::CRendererStatsTable(const CRenderer::Stats& st)
	: Stats(st)
{
	columnDescriptions.push_back(ProfileColumn("Name", 230));
	columnDescriptions.push_back(ProfileColumn("Value", 100));
}

// Implementation of AbstractProfileTable interface
CStr CRendererStatsTable::GetName()
{
	return "renderer";
}

CStr CRendererStatsTable::GetTitle()
{
	return "Renderer statistics";
}

size_t CRendererStatsTable::GetNumberRows()
{
	return NumberRows;
}

const std::vector<ProfileColumn>& CRendererStatsTable::GetColumns()
{
	return columnDescriptions;
}

CStr CRendererStatsTable::GetCellText(size_t row, size_t col)
{
	char buf[256];

	switch(row)
	{
	case Row_DrawCalls:
		if (col == 0)
			return "# draw calls";
		sprintf_s(buf, sizeof(buf), "%lu", (unsigned long)Stats.m_DrawCalls);
		return buf;

	case Row_TerrainTris:
		if (col == 0)
			return "# terrain tris";
		sprintf_s(buf, sizeof(buf), "%lu", (unsigned long)Stats.m_TerrainTris);
		return buf;

	case Row_WaterTris:
		if (col == 0)
			return "# water tris";
		sprintf_s(buf, sizeof(buf), "%lu", (unsigned long)Stats.m_WaterTris);
		return buf;

	case Row_ModelTris:
		if (col == 0)
			return "# model tris";
		sprintf_s(buf, sizeof(buf), "%lu", (unsigned long)Stats.m_ModelTris);
		return buf;

	case Row_OverlayTris:
		if (col == 0)
			return "# overlay tris";
		sprintf_s(buf, sizeof(buf), "%lu", (unsigned long)Stats.m_OverlayTris);
		return buf;

	case Row_BlendSplats:
		if (col == 0)
			return "# blend splats";
		sprintf_s(buf, sizeof(buf), "%lu", (unsigned long)Stats.m_BlendSplats);
		return buf;

	case Row_Particles:
		if (col == 0)
			return "# particles";
		sprintf_s(buf, sizeof(buf), "%lu", (unsigned long)Stats.m_Particles);
		return buf;

	case Row_VBReserved:
		if (col == 0)
			return "VB bytes reserved";
		sprintf_s(buf, sizeof(buf), "%lu", (unsigned long)g_VBMan.GetBytesReserved());
		return buf;

	case Row_VBAllocated:
		if (col == 0)
			return "VB bytes allocated";
		sprintf_s(buf, sizeof(buf), "%lu", (unsigned long)g_VBMan.GetBytesAllocated());
		return buf;

	case Row_ShadersLoaded:
		if (col == 0)
			return "shader effects loaded";
		sprintf_s(buf, sizeof(buf), "%lu", (unsigned long)g_Renderer.GetShaderManager().GetNumEffectsLoaded());
		return buf;

	default:
		return "???";
	}
}

AbstractProfileTable* CRendererStatsTable::GetChild(size_t UNUSED(row))
{
	return 0;
}


///////////////////////////////////////////////////////////////////////////////////
// CRenderer implementation

/**
 * Struct CRendererInternals: Truly hide data that is supposed to be hidden
 * in this structure so it won't even appear in header files.
 */
struct CRendererInternals
{
	NONCOPYABLE(CRendererInternals);
public:
	/// true if CRenderer::Open has been called
	bool IsOpen;

	/// true if shaders need to be reloaded
	bool ShadersDirty;

	/// Table to display renderer stats in-game via profile system
	CRendererStatsTable profileTable;

	/// Shader manager
	CShaderManager shaderManager;

	/// Water manager
	WaterManager waterManager;

	/// Sky manager
	SkyManager skyManager;

	/// Texture manager
	CTextureManager textureManager;

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
	
	/// Time manager
	CTimeManager timeManager;

	/// Shadow map
	ShadowMap shadow;
	
	/// Postprocessing effect manager
	CPostprocManager postprocManager;

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

	CRendererInternals() :
		IsOpen(false), ShadersDirty(true), profileTable(g_Renderer.m_Stats), textureManager(g_VFS, false, false)
	{
	}

	/**
	 * Load the OpenGL projection and modelview matrices and the viewport according
	 * to the given camera.
	 */
	void SetOpenGLCamera(const CCamera& camera)
	{
		CMatrix3D view;
		camera.m_Orientation.GetInverse(view);
		const CMatrix3D& proj = camera.GetProjection();

#if CONFIG2_GLES
#warning TODO: fix CRenderer camera handling for GLES (do not use global matrixes)
#else
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(&proj._11);

		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(&view._11);
#endif

		const SViewPort &vp = camera.GetViewPort();
		glViewport((GLint)vp.m_X,(GLint)vp.m_Y,(GLsizei)vp.m_Width,(GLsizei)vp.m_Height);
	}

	/**
	 * Renders all non-alpha-blended models with the given context.
	 */
	void CallModelRenderers(const CShaderDefines& context, int flags)
	{
		CShaderDefines contextSkinned = context;
		if (g_Renderer.m_Options.m_GPUSkinning)
		{
			contextSkinned.Add("USE_INSTANCING", "1");
			contextSkinned.Add("USE_GPU_SKINNING", "1");
		}
		Model.NormalSkinned->Render(Model.ModShader, contextSkinned, flags);

		if (Model.NormalUnskinned != Model.NormalSkinned)
		{
			CShaderDefines contextUnskinned = context;
			contextUnskinned.Add("USE_INSTANCING", "1");
			Model.NormalUnskinned->Render(Model.ModShader, contextUnskinned, flags);
		}
	}

	/**
	 * Renders all alpha-blended models with the given context.
	 */
	void CallTranspModelRenderers(const CShaderDefines& context, int flags)
	{
		CShaderDefines contextSkinned = context;
		if (g_Renderer.m_Options.m_GPUSkinning)
		{
			contextSkinned.Add("USE_INSTANCING", "1");
			contextSkinned.Add("USE_GPU_SKINNING", "1");
		}
		Model.TranspSkinned->Render(Model.ModShader, contextSkinned, flags);

		if (Model.TranspUnskinned != Model.TranspSkinned)
		{
			CShaderDefines contextUnskinned = context;
			contextUnskinned.Add("USE_INSTANCING", "1");
			Model.TranspUnskinned->Render(Model.ModShader, contextUnskinned, flags);
		}
	}

	/**
	 * Filters all non-alpha-blended models.
	 */
	void FilterModels(CModelFilter& filter, int passed, int flags = 0)
	{
		Model.NormalSkinned->Filter(filter, passed, flags);
		if (Model.NormalUnskinned != Model.NormalSkinned)
			Model.NormalUnskinned->Filter(filter, passed, flags);
	}

	/**
	 * Filters all alpha-blended models.
	 */
	void FilterTranspModels(CModelFilter& filter, int passed, int flags = 0)
	{
		Model.TranspSkinned->Filter(filter, passed, flags);
		if (Model.TranspUnskinned != Model.TranspSkinned)
			Model.TranspUnskinned->Filter(filter, passed, flags);
	}
};

///////////////////////////////////////////////////////////////////////////////////
// CRenderer constructor
CRenderer::CRenderer()
{
	m = new CRendererInternals;
	m_WaterManager = &m->waterManager;
	m_SkyManager = &m->skyManager;

	g_ProfileViewer.AddRootTable(&m->profileTable);

	m_Width = 0;
	m_Height = 0;
	m_TerrainRenderMode = SOLID;
	m_ModelRenderMode = SOLID;
	m_ClearColor[0] = m_ClearColor[1] = m_ClearColor[2] = m_ClearColor[3] = 0;

	m_DisplayFrustum = false;
	m_DisplayTerrainPriorities = false;
	m_SkipSubmit = false;

	m_Options.m_NoVBO = false;
	m_Options.m_RenderPath = RP_DEFAULT;
	m_Options.m_Shadows = false;
	m_Options.m_ShadowAlphaFix = true;
	m_Options.m_ARBProgramShadow = true;
	m_Options.m_ShadowPCF = false;
	m_Options.m_Particles = false;
	m_Options.m_Silhouettes = false;
	m_Options.m_PreferGLSL = false;
	m_Options.m_ForceAlphaTest = false;
	m_Options.m_GPUSkinning = false;
	m_Options.m_GenTangents = false;
	m_Options.m_SmoothLOS = false;
	m_Options.m_Postproc = false;
	m_Options.m_ShowSky = false;

	// TODO: be more consistent in use of the config system
	CFG_GET_VAL("preferglsl", Bool, m_Options.m_PreferGLSL);
	CFG_GET_VAL("forcealphatest", Bool, m_Options.m_ForceAlphaTest);
	CFG_GET_VAL("gpuskinning", Bool, m_Options.m_GPUSkinning);
	CFG_GET_VAL("gentangents", Bool, m_Options.m_GenTangents);
	CFG_GET_VAL("smoothlos", Bool, m_Options.m_SmoothLOS);
	CFG_GET_VAL("postproc", Bool, m_Options.m_Postproc);

	CStr skystring = "0 0 0";
	CColor skycolor;
	CFG_GET_VAL("skycolor", String, skystring);
	if (skycolor.ParseString(skystring, 255.f))
		SetClearColor(skycolor.AsSColor4ub());

#if CONFIG2_GLES
	// Override config option since GLES only supports GLSL
	m_Options.m_PreferGLSL = true;
#endif

	m_ShadowZBias = 0.02f;
	m_ShadowMapSize = 0;

	m_LightEnv = NULL;

	m_CurrentScene = NULL;

	m_hCompositeAlphaMap = 0;

	m_Stats.Reset();
	AddLocalProperty(L"particles", &m_Options.m_Particles, false);

	AddLocalProperty(L"waternormal", &m_Options.m_WaterNormal, false);
	AddLocalProperty(L"waterrealdepth", &m_Options.m_WaterRealDepth, false);
	AddLocalProperty(L"waterreflection", &m_Options.m_WaterReflection, false);
	AddLocalProperty(L"waterrefraction", &m_Options.m_WaterRefraction, false);
	AddLocalProperty(L"waterfoam", &m_Options.m_WaterFoam, false);
	AddLocalProperty(L"watercoastalwaves", &m_Options.m_WaterCoastalWaves, false);
	AddLocalProperty(L"watershadow", &m_Options.m_WaterShadow, false);

	AddLocalProperty(L"horizonHeight", &m->skyManager.m_HorizonHeight, false);
	AddLocalProperty(L"waterMurkiness", &m->waterManager.m_Murkiness, false);
	AddLocalProperty(L"waterReflTintStrength", &m->waterManager.m_ReflectionTintStrength, false);
	AddLocalProperty(L"waterRepeatPeriod", &m->waterManager.m_RepeatPeriod, false);
	AddLocalProperty(L"waterShininess", &m->waterManager.m_Shininess, false);
	AddLocalProperty(L"waterSpecularStrength", &m->waterManager.m_SpecularStrength, false);
	AddLocalProperty(L"waterWaviness", &m->waterManager.m_Waviness, false);
	AddLocalProperty(L"silhouettes", &m_Options.m_Silhouettes, false);
	AddLocalProperty(L"showsky", &m_Options.m_ShowSky, false);

	RegisterFileReloadFunc(ReloadChangedFileCB, this);
}

///////////////////////////////////////////////////////////////////////////////////
// CRenderer destructor
CRenderer::~CRenderer()
{
	UnregisterFileReloadFunc(ReloadChangedFileCB, this);

	// we no longer UnloadAlphaMaps / UnloadWaterTextures here -
	// that is the responsibility of the module that asked for
	// them to be loaded (i.e. CGameView).
	delete m;
}


///////////////////////////////////////////////////////////////////////////////////
// EnumCaps: build card cap bits
void CRenderer::EnumCaps()
{
	// assume support for nothing
	m_Caps.m_VBO = false;
	m_Caps.m_ARBProgram = false;
	m_Caps.m_ARBProgramShadow = false;
	m_Caps.m_VertexShader = false;
	m_Caps.m_FragmentShader = false;
	m_Caps.m_Shadows = false;

	// now start querying extensions
	if (!m_Options.m_NoVBO) {
		if (ogl_HaveExtension("GL_ARB_vertex_buffer_object")) {
			m_Caps.m_VBO=true;
		}
	}

	if (0 == ogl_HaveExtensions(0, "GL_ARB_vertex_program", "GL_ARB_fragment_program", NULL))
	{
		m_Caps.m_ARBProgram = true;
		if (ogl_HaveExtension("GL_ARB_fragment_program_shadow"))
			m_Caps.m_ARBProgramShadow = true;
	}

	if (0 == ogl_HaveExtensions(0, "GL_ARB_shader_objects", "GL_ARB_shading_language_100", NULL))
	{
		if (ogl_HaveExtension("GL_ARB_vertex_shader"))
			m_Caps.m_VertexShader = true;
		if (ogl_HaveExtension("GL_ARB_fragment_shader"))
			m_Caps.m_FragmentShader = true;
	}

#if CONFIG2_GLES
	m_Caps.m_Shadows = true;
#else
	if (0 == ogl_HaveExtensions(0, "GL_ARB_shadow", "GL_ARB_depth_texture", "GL_EXT_framebuffer_object", NULL))
	{
		if (ogl_max_tex_units >= 4)
			m_Caps.m_Shadows = true;
	}
#endif
}

CShaderDefines CRenderer::GetSystemShaderDefines()
{
	CShaderDefines defines;

	if (GetRenderPath() == RP_SHADER && m_Caps.m_ARBProgram)
		defines.Add("SYS_HAS_ARB", "1");

	if (GetRenderPath() == RP_SHADER && m_Caps.m_VertexShader && m_Caps.m_FragmentShader)
		defines.Add("SYS_HAS_GLSL", "1");

	if (m_Options.m_PreferGLSL)
		defines.Add("SYS_PREFER_GLSL", "1");

	return defines;
}

void CRenderer::ReloadShaders()
{
	ENSURE(m->IsOpen);

	m->globalContext = GetSystemShaderDefines();

	if (m_Caps.m_Shadows && m_Options.m_Shadows)
	{
		m->globalContext.Add("USE_SHADOW", "1");
		if (m_Caps.m_ARBProgramShadow && m_Options.m_ARBProgramShadow)
			m->globalContext.Add("USE_FP_SHADOW", "1");
		if (m_Options.m_ShadowPCF)
			m->globalContext.Add("USE_SHADOW_PCF", "1");
#if !CONFIG2_GLES
		m->globalContext.Add("USE_SHADOW_SAMPLER", "1");
#endif
	}

	if (m_LightEnv)
		m->globalContext.Add(("LIGHTING_MODEL_" + m_LightEnv->GetLightingModel()).c_str(), "1");

	m->Model.ModShader = LitRenderModifierPtr(new ShaderRenderModifier());

	bool cpuLighting = (GetRenderPath() == RP_FIXED);
	m->Model.VertexRendererShader = ModelVertexRendererPtr(new ShaderModelVertexRenderer(cpuLighting));
	m->Model.VertexInstancingShader = ModelVertexRendererPtr(new InstancingModelRenderer(false, m_Options.m_GenTangents));

	if (GetRenderPath() == RP_SHADER && m_Options.m_GPUSkinning) // TODO: should check caps and GLSL etc too
	{
		m->Model.VertexGPUSkinningShader = ModelVertexRendererPtr(new InstancingModelRenderer(true, m_Options.m_GenTangents));
		m->Model.NormalSkinned = ModelRendererPtr(new ShaderModelRenderer(m->Model.VertexGPUSkinningShader));
		m->Model.TranspSkinned = ModelRendererPtr(new ShaderModelRenderer(m->Model.VertexGPUSkinningShader));
	}
	else
	{
		m->Model.VertexGPUSkinningShader.reset();
		m->Model.NormalSkinned = ModelRendererPtr(new ShaderModelRenderer(m->Model.VertexRendererShader));
		m->Model.TranspSkinned = ModelRendererPtr(new ShaderModelRenderer(m->Model.VertexRendererShader));
	}

	// Use instancing renderers in shader mode
	if (GetRenderPath() == RP_SHADER)
	{
		m->Model.NormalUnskinned = ModelRendererPtr(new ShaderModelRenderer(m->Model.VertexInstancingShader));
		m->Model.TranspUnskinned = ModelRendererPtr(new ShaderModelRenderer(m->Model.VertexInstancingShader));
	}
	else
	{
		m->Model.NormalUnskinned = m->Model.NormalSkinned;
		m->Model.TranspUnskinned = m->Model.TranspSkinned;
	}

	m->ShadersDirty = false;
}

bool CRenderer::Open(int width, int height)
{
	m->IsOpen = true;

	// Must query card capabilities before creating renderers that depend
	// on card capabilities.
	EnumCaps();

	// Dimensions
	m_Width = width;
	m_Height = height;

	// set packing parameters
	glPixelStorei(GL_PACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);

	// setup default state
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);

	GLint bits;
	glGetIntegerv(GL_DEPTH_BITS,&bits);
	LOGMESSAGE(L"CRenderer::Open: depth bits %d",bits);
	glGetIntegerv(GL_STENCIL_BITS,&bits);
	LOGMESSAGE(L"CRenderer::Open: stencil bits %d",bits);
	glGetIntegerv(GL_ALPHA_BITS,&bits);
	LOGMESSAGE(L"CRenderer::Open: alpha bits %d",bits);

	// Validate the currently selected render path
	SetRenderPath(m_Options.m_RenderPath);

	// Let component renderers perform one-time initialization after graphics capabilities and
	// the shader path have been determined.
	m->overlayRenderer.Initialize();
	
	if (m_Options.m_Postproc)
		m->postprocManager.Initialize();

	return true;
}

// resize renderer view
void CRenderer::Resize(int width,int height)
{
	// need to recreate the shadow map object to resize the shadow texture
	m->shadow.RecreateTexture();

	m_Width = width;
	m_Height = height;
	
	if (m_Options.m_Postproc)
		m->postprocManager.RecreateBuffers();
}

//////////////////////////////////////////////////////////////////////////////////////////
// SetOptionBool: set boolean renderer option
void CRenderer::SetOptionBool(enum Option opt,bool value)
{
	switch (opt) {
		case OPT_NOVBO:
			m_Options.m_NoVBO = value;
			break;
		case OPT_SHADOWS:
			m_Options.m_Shadows = value;
			MakeShadersDirty();
			break;
		case OPT_WATERNORMAL:
			m_Options.m_WaterNormal = value;
			break;
		case OPT_WATERREALDEPTH:
			m_Options.m_WaterRealDepth = value;
			break;
		case OPT_WATERFOAM:
			m_Options.m_WaterFoam = value;
			break;
		case OPT_WATERCOASTALWAVES:
			m_Options.m_WaterCoastalWaves = value;
			break;
		case OPT_WATERREFLECTION:
			m_Options.m_WaterReflection = value;
			break;
		case OPT_WATERREFRACTION:
			m_Options.m_WaterRefraction = value;
			break;
		case OPT_WATERSHADOW:
			m_Options.m_WaterShadow = value;
			break;
		case OPT_SHADOWPCF:
			m_Options.m_ShadowPCF = value;
			MakeShadersDirty();
			break;
		case OPT_PARTICLES:
			m_Options.m_Particles = value;
			break;
		case OPT_SILHOUETTES:
			m_Options.m_Silhouettes = value;
			break;
		case OPT_SHOWSKY:
			m_Options.m_ShowSky = value;
			break;
		default:
			debug_warn(L"CRenderer::SetOptionBool: unknown option");
			break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// GetOptionBool: get boolean renderer option
bool CRenderer::GetOptionBool(enum Option opt) const
{
	switch (opt) {
		case OPT_NOVBO:
			return m_Options.m_NoVBO;
		case OPT_SHADOWS:
			return m_Options.m_Shadows;
		case OPT_WATERNORMAL:
			return m_Options.m_WaterNormal;
		case OPT_WATERREALDEPTH:
			return m_Options.m_WaterRealDepth;
		case OPT_WATERFOAM:
			return m_Options.m_WaterFoam;
		case OPT_WATERCOASTALWAVES:
			return m_Options.m_WaterCoastalWaves;
		case OPT_WATERREFLECTION:
			return m_Options.m_WaterReflection;
		case OPT_WATERREFRACTION:
			return m_Options.m_WaterRefraction;
		case OPT_WATERSHADOW:
			return m_Options.m_WaterShadow;
		case OPT_SHADOWPCF:
			return m_Options.m_ShadowPCF;
		case OPT_PARTICLES:
			return m_Options.m_Particles;
		case OPT_SILHOUETTES:
			return m_Options.m_Silhouettes;
		case OPT_SHOWSKY:
			return m_Options.m_ShowSky;
		default:
			debug_warn(L"CRenderer::GetOptionBool: unknown option");
			break;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
// SetRenderPath: Select the preferred render path.
// This may only be called before Open(), because the layout of vertex arrays and other
// data may depend on the chosen render path.
void CRenderer::SetRenderPath(RenderPath rp)
{
	if (!m->IsOpen)
	{
		// Delay until Open() is called.
		m_Options.m_RenderPath = rp;
		return;
	}

	// Renderer has been opened, so validate the selected renderpath
	if (rp == RP_DEFAULT)
	{
		if (m_Caps.m_ARBProgram || (m_Caps.m_VertexShader && m_Caps.m_FragmentShader && m_Options.m_PreferGLSL))
			rp = RP_SHADER;
		else
			rp = RP_FIXED;
	}

	if (rp == RP_SHADER)
	{
		if (!(m_Caps.m_ARBProgram || (m_Caps.m_VertexShader && m_Caps.m_FragmentShader && m_Options.m_PreferGLSL)))
		{
			LOGWARNING(L"Falling back to fixed function\n");
			rp = RP_FIXED;
		}
	}

	m_Options.m_RenderPath = rp;

	MakeShadersDirty();

	// We might need to regenerate some render data after changing path
	if (g_Game)
		g_Game->GetWorld()->GetTerrain()->MakeDirty(RENDERDATA_UPDATE_COLOR);
}


CStr CRenderer::GetRenderPathName(RenderPath rp)
{
	switch(rp) {
	case RP_DEFAULT: return "default";
	case RP_FIXED: return "fixed";
	case RP_SHADER: return "shader";
	default: return "(invalid)";
	}
}

CRenderer::RenderPath CRenderer::GetRenderPathByName(const CStr& name)
{
	if (name == "fixed")
		return RP_FIXED;
	if (name == "shader")
		return RP_SHADER;
	if (name == "default")
		return RP_DEFAULT;

	LOGWARNING(L"Unknown render path name '%hs', assuming 'default'", name.c_str());
	return RP_DEFAULT;
}


//////////////////////////////////////////////////////////////////////////////////////////
// BeginFrame: signal frame start
void CRenderer::BeginFrame()
{
	PROFILE("begin frame");

	// zero out all the per-frame stats
	m_Stats.Reset();

	// choose model renderers for this frame

	if (m->ShadersDirty)
		ReloadShaders();
	
	m->Model.ModShader->SetShadowMap(&m->shadow);
	m->Model.ModShader->SetLightEnv(m_LightEnv);
}

//////////////////////////////////////////////////////////////////////////////////////////
void CRenderer::SetSimulation(CSimulation2* simulation)
{
	// set current simulation context for terrain renderer
	m->terrainRenderer.SetSimulation(simulation);
}

// SetClearColor: set color used to clear screen in BeginFrame()
void CRenderer::SetClearColor(SColor4ub color)
{
	m_ClearColor[0] = float(color.R) / 255.0f;
	m_ClearColor[1] = float(color.G) / 255.0f;
	m_ClearColor[2] = float(color.B) / 255.0f;
	m_ClearColor[3] = float(color.A) / 255.0f;
}

void CRenderer::RenderShadowMap(const CShaderDefines& context)
{
	PROFILE3_GPU("shadow map");

	m->shadow.BeginRender();

	{
		PROFILE("render patches");
		glCullFace(GL_FRONT);
		glEnable(GL_CULL_FACE);
		m->terrainRenderer.RenderPatches();
		glCullFace(GL_BACK);
	}

	CShaderDefines contextCast = context;
	contextCast.Add("MODE_SHADOWCAST", "1");

	{
		PROFILE("render models");
		m->CallModelRenderers(contextCast, MODELFLAG_CASTSHADOWS);
	}

	{
		PROFILE("render transparent models");
		// disable face-culling for two-sided models
		glDisable(GL_CULL_FACE);
		m->CallTranspModelRenderers(contextCast, MODELFLAG_CASTSHADOWS);
		glEnable(GL_CULL_FACE);
	}

	m->shadow.EndRender();

	m->SetOpenGLCamera(m_ViewCamera);
}

void CRenderer::RenderPatches(const CShaderDefines& context, const CFrustum* frustum)
{
	PROFILE3_GPU("patches");

	bool filtered = false;
	if (frustum)
	{
		if (!m->terrainRenderer.CullPatches(frustum))
			return;

		filtered = true;
	}

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
	if (GetRenderPath() == RP_SHADER)
		m->terrainRenderer.RenderTerrainShader(context, (m_Caps.m_Shadows && m_Options.m_Shadows) ? &m->shadow : 0, filtered);
	else
		m->terrainRenderer.RenderTerrain(filtered);


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
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);

		// setup some renderstate ..
		pglActiveTextureARB(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);
		glColor3f(0.5f, 0.5f, 1.0f);
		glLineWidth(2.0f);

		// render tiles edges
		m->terrainRenderer.RenderPatches(filtered);

		// set color for outline
		glColor3f(0, 0, 1);
		glLineWidth(4.0f);

		// render outline of each patch
		m->terrainRenderer.RenderOutlines(filtered);

		// .. and restore the renderstates
		glLineWidth(1.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
#endif
}

class CModelCuller : public CModelFilter
{
public:
	CModelCuller(const CFrustum& frustum) : m_Frustum(frustum) { }

	bool Filter(CModel *model)
	{
		return m_Frustum.IsBoxVisible(CVector3D(0, 0, 0), model->GetWorldBoundsRec());
	}

private:
	const CFrustum& m_Frustum;
};

void CRenderer::RenderModels(const CShaderDefines& context, const CFrustum* frustum)
{
	PROFILE3_GPU("models");

	int flags = 0;
	if (frustum)
	{
		flags = MODELFLAG_FILTERED;
		CModelCuller culler(*frustum);
		m->FilterModels(culler, flags);
	}

#if !CONFIG2_GLES
	if (m_ModelRenderMode == WIREFRAME)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
#endif

	m->CallModelRenderers(context, flags);

#if !CONFIG2_GLES
	if (m_ModelRenderMode == WIREFRAME)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	else if (m_ModelRenderMode == EDGED_FACES)
	{
		CShaderDefines contextWireframe = context;
		contextWireframe.Add("MODE_WIREFRAME", "1");

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_TEXTURE_2D);

		m->CallModelRenderers(contextWireframe, flags);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
#endif
}

void CRenderer::RenderTransparentModels(const CShaderDefines& context, ETransparentMode transparentMode, const CFrustum* frustum)
{
	PROFILE3_GPU("transparent models");

	int flags = 0;
	if (frustum)
	{
		flags = MODELFLAG_FILTERED;
		CModelCuller culler(*frustum);
		m->FilterTranspModels(culler, flags);
	}

#if !CONFIG2_GLES
	// switch on wireframe if we need it
	if (m_ModelRenderMode == WIREFRAME)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
#endif

	// disable face culling for two-sided models in sub-renders
	if (flags)
		glDisable(GL_CULL_FACE);

	CShaderDefines contextOpaque = context;
	contextOpaque.Add("ALPHABLEND_PASS_OPAQUE", "1");

	CShaderDefines contextBlend = context;
	contextBlend.Add("ALPHABLEND_PASS_BLEND", "1");

	if (transparentMode == TRANSPARENT || transparentMode == TRANSPARENT_OPAQUE)
		m->CallTranspModelRenderers(contextOpaque, flags);

	if (transparentMode == TRANSPARENT || transparentMode == TRANSPARENT_BLEND)
		m->CallTranspModelRenderers(contextBlend, flags);

	if (flags)
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
		contextWireframe.Add("MODE_WIREFRAME", "1");

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_TEXTURE_2D);

		m->CallTranspModelRenderers(contextWireframe, flags);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
#endif
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// SetObliqueFrustumClipping: change the near plane to the given clip plane (in world space)
// Based on code from Game Programming Gems 5, from http://www.terathon.com/code/oblique.html
// - worldPlane is a clip plane in world space (worldPlane.Dot(v) >= 0 for any vector v passing the clipping test)
void CRenderer::SetObliqueFrustumClipping(const CVector4D& worldPlane)
{
	// First, we'll convert the given clip plane to camera space, then we'll 
	// Get the view matrix and normal matrix (top 3x3 part of view matrix)
	CMatrix3D normalMatrix = m_ViewCamera.m_Orientation.GetTranspose();
	CVector4D camPlane = normalMatrix.Transform(worldPlane);

	CMatrix3D matrix = m_ViewCamera.GetProjection();

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
	m_ViewCamera.SetProjection(matrix);
	m->SetOpenGLCamera(m_ViewCamera);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// RenderReflections: render the water reflections to the reflection texture
SScreenRect CRenderer::RenderReflections(const CShaderDefines& context, const CBoundingBoxAligned& scissor)
{
	PROFILE3_GPU("water reflections");

	WaterManager& wm = m->waterManager;

	// Remember old camera
	CCamera normalCamera = m_ViewCamera;

	// Temporarily change the camera to one that is reflected.
	// Also, for texturing purposes, make it render to a view port the size of the
	// water texture, stretch the image according to our aspect ratio so it covers
	// the whole screen despite being rendered into a square, and cover slightly more
	// of the view so we can see wavy reflections of slightly off-screen objects.
	m_ViewCamera.m_Orientation.Scale(1, -1, 1);
	m_ViewCamera.m_Orientation.Translate(0, 2*wm.m_WaterHeight, 0);
	m_ViewCamera.UpdateFrustum(scissor);
	m_ViewCamera.ClipFrustum(CVector4D(0, 1, 0, -wm.m_WaterHeight));

	SViewPort vp;
	vp.m_Height = wm.m_ReflectionTextureSize;
	vp.m_Width = wm.m_ReflectionTextureSize;
	vp.m_X = 0;
	vp.m_Y = 0;
	m_ViewCamera.SetViewPort(vp);
	m_ViewCamera.SetProjection(normalCamera.GetNearPlane(), normalCamera.GetFarPlane(), normalCamera.GetFOV()*1.05f); // Slightly higher than view FOV
	CMatrix3D scaleMat;
	scaleMat.SetScaling(m_Height/float(std::max(1, m_Width)), 1.0f, 1.0f);
	m_ViewCamera.m_ProjMat = scaleMat * m_ViewCamera.m_ProjMat;

	m->SetOpenGLCamera(m_ViewCamera);

	CVector4D camPlane(0, 1, 0, -wm.m_WaterHeight);
	SetObliqueFrustumClipping(camPlane);

	// Save the model-view-projection matrix so the shaders can use it for projective texturing
	wm.m_ReflectionMatrix = m_ViewCamera.GetViewProjection();

	SScreenRect screenScissor;
	screenScissor.x1 = (GLint)floor((scissor[0].X*0.5f+0.5f)*vp.m_Width);
	screenScissor.y1 = (GLint)floor((scissor[0].Y*0.5f+0.5f)*vp.m_Height);
	screenScissor.x2 = (GLint)ceil((scissor[1].X*0.5f+0.5f)*vp.m_Width);
	screenScissor.y2 = (GLint)ceil((scissor[1].Y*0.5f+0.5f)*vp.m_Height);

	if (screenScissor.x1 < screenScissor.x2 && screenScissor.y1 < screenScissor.y2)
	{
		glEnable(GL_SCISSOR_TEST);
		glScissor(screenScissor.x1, screenScissor.y1, screenScissor.x2 - screenScissor.x1, screenScissor.y2 - screenScissor.y1);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		glFrontFace(GL_CW);

		// Render sky, terrain and models
		m->skyManager.RenderSky();
		ogl_WarnIfError();
		RenderPatches(context, &m_ViewCamera.GetFrustum());
		ogl_WarnIfError();
		RenderModels(context, &m_ViewCamera.GetFrustum());
		ogl_WarnIfError();
		RenderTransparentModels(context, TRANSPARENT_BLEND, &m_ViewCamera.GetFrustum());
		ogl_WarnIfError();

		glFrontFace(GL_CCW);

		glDisable(GL_SCISSOR_TEST);

		// Copy the image to a texture
		pglActiveTextureARB(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, wm.m_ReflectionTexture);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
				screenScissor.x1, screenScissor.y1,
				screenScissor.x1, screenScissor.y1,
				screenScissor.x2 - screenScissor.x1, screenScissor.y2 - screenScissor.y1);
	}

	// Reset old camera
	m_ViewCamera = normalCamera;
	m->SetOpenGLCamera(m_ViewCamera);

	return screenScissor;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// RenderRefractions: render the water refractions to the refraction texture
SScreenRect CRenderer::RenderRefractions(const CShaderDefines& context, const CBoundingBoxAligned &scissor)
{
	PROFILE3_GPU("water refractions");

	WaterManager& wm = m->waterManager;

	// Remember old camera
	CCamera normalCamera = m_ViewCamera;

	// Temporarily change the camera to make it render to a view port the size of the
	// water texture, stretch the image according to our aspect ratio so it covers
	// the whole screen despite being rendered into a square, and cover slightly more
	// of the view so we can see wavy refractions of slightly off-screen objects.
	m_ViewCamera.UpdateFrustum(scissor);
	m_ViewCamera.ClipFrustum(CVector4D(0, -1, 0, wm.m_WaterHeight));

	SViewPort vp;
	vp.m_Height = wm.m_RefractionTextureSize;
	vp.m_Width = wm.m_RefractionTextureSize;
	vp.m_X = 0;
	vp.m_Y = 0;
	m_ViewCamera.SetViewPort(vp);
	m_ViewCamera.SetProjection(normalCamera.GetNearPlane(), normalCamera.GetFarPlane(), normalCamera.GetFOV()*1.05f); // Slightly higher than view FOV
	CMatrix3D scaleMat;
	scaleMat.SetScaling(m_Height/float(std::max(1, m_Width)), 1.0f, 1.0f);
	m_ViewCamera.m_ProjMat = scaleMat * m_ViewCamera.m_ProjMat;

	m->SetOpenGLCamera(m_ViewCamera);

	CVector4D camPlane(0, -1, 0, wm.m_WaterHeight);
	SetObliqueFrustumClipping(camPlane);

	// Save the model-view-projection matrix so the shaders can use it for projective texturing
	wm.m_RefractionMatrix = m_ViewCamera.GetViewProjection();

	SScreenRect screenScissor;
	screenScissor.x1 = (GLint)floor((scissor[0].X*0.5f+0.5f)*vp.m_Width);
	screenScissor.y1 = (GLint)floor((scissor[0].Y*0.5f+0.5f)*vp.m_Height);
	screenScissor.x2 = (GLint)ceil((scissor[1].X*0.5f+0.5f)*vp.m_Width);
	screenScissor.y2 = (GLint)ceil((scissor[1].Y*0.5f+0.5f)*vp.m_Height);
	if (screenScissor.x1 < screenScissor.x2 && screenScissor.y1 < screenScissor.y2)
	{
		glEnable(GL_SCISSOR_TEST);
		glScissor(screenScissor.x1, screenScissor.y1, screenScissor.x2 - screenScissor.x1, screenScissor.y2 - screenScissor.y1);

		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);		// a neutral gray to blend in with shores
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		// Render terrain and models
		RenderPatches(context, &m_ViewCamera.GetFrustum());
		ogl_WarnIfError();
		RenderModels(context, &m_ViewCamera.GetFrustum());
		ogl_WarnIfError();
		RenderTransparentModels(context, TRANSPARENT_BLEND, &m_ViewCamera.GetFrustum());
		ogl_WarnIfError();

		glDisable(GL_SCISSOR_TEST);

		// Copy the image to a texture
		pglActiveTextureARB(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, wm.m_RefractionTexture);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
				screenScissor.x1, screenScissor.y1,
				screenScissor.x1, screenScissor.y1,
				screenScissor.x2 - screenScissor.x1, screenScissor.y2 - screenScissor.y1);
	}

	// Reset old camera
	m_ViewCamera = normalCamera;
	m->SetOpenGLCamera(m_ViewCamera);

	return screenScissor;
}


void CRenderer::RenderSilhouettes(const CShaderDefines& context)
{
	PROFILE3_GPU("silhouettes");

	CShaderDefines contextOccluder = context;
	contextOccluder.Add("MODE_SILHOUETTEOCCLUDER", "1");

	CShaderDefines contextDisplay = context;
	contextDisplay.Add("MODE_SILHOUETTEDISPLAY", "1");

	// Render silhouettes of units hidden behind terrain or occluders.
	// To avoid breaking the standard rendering of alpha-blended objects, this
	// has to be done in a separate pass.
	// First we render all occluders into depth, then render all units with
	// inverted depth test so any behind an occluder will get drawn in a constant
	// colour.

	float silhouetteAlpha = 0.75f;

	// Silhouette blending requires an almost-universally-supported extension;
	// fall back to non-blended if unavailable
	if (!ogl_HaveExtension("GL_EXT_blend_color"))
		silhouetteAlpha = 1.f;

	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glColorMask(0, 0, 0, 0);

	// Render occluders:

	{
		PROFILE("render patches");

		// To prevent units displaying silhouettes when parts of their model
		// protrude into the ground, only occlude with the back faces of the
		// terrain (so silhouettes will still display when behind hills)
		glCullFace(GL_FRONT);
		m->terrainRenderer.RenderPatches();
		glCullFace(GL_BACK);
	}

	{
		PROFILE("render model occluders");
		m->CallModelRenderers(contextOccluder, MODELFLAG_SILHOUETTE_OCCLUDER);
	}

	{
		PROFILE("render transparent occluders");
		m->CallTranspModelRenderers(contextOccluder, MODELFLAG_SILHOUETTE_OCCLUDER);
	}

	glDepthFunc(GL_GEQUAL);
	glColorMask(1, 1, 1, 1);

	// Render more efficiently if alpha == 1
	if (silhouetteAlpha == 1.f)
	{
		// Ideally we'd render objects back-to-front so nearer silhouettes would
		// appear on top, but sorting has non-zero cost. So we'll keep the depth
		// write enabled, to do the opposite - far objects will consistently appear
		// on top.
		glDepthMask(0);
	}
	else
	{
		// Since we can't sort, we'll use the stencil buffer to ensure we only draw
		// a pixel once (using the colour of whatever model happens to be drawn first).
		glEnable(GL_BLEND);
		glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
		pglBlendColorEXT(0, 0, 0, silhouetteAlpha);

		glEnable(GL_STENCIL_TEST);
		glStencilFunc(GL_NOTEQUAL, 1, (GLuint)-1);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	}

	// TODO: For performance, we probably ought to do a quick raycasting check
	// to see which units are likely blocked by occluders and not bother
	// rendering any of the others

	{
		PROFILE("render models");
		m->CallModelRenderers(contextDisplay, MODELFLAG_SILHOUETTE_DISPLAY);
		// (This won't render transparent objects with SILHOUETTE_DISPLAY - will
		// we have any units that need that?)
	}

	// Restore state
	glDepthFunc(GL_LEQUAL);
	if (silhouetteAlpha == 1.f)
	{
		glDepthMask(1);
	}
	else
	{
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		pglBlendColorEXT(0, 0, 0, 0);
		glDisable(GL_STENCIL_TEST);
	}
}

void CRenderer::RenderParticles()
{
	// Only supported in shader modes
	if (GetRenderPath() != RP_SHADER)
		return;

	PROFILE3_GPU("particles");

	m->particleRenderer.RenderParticles();

#if !CONFIG2_GLES
	if (m_ModelRenderMode == EDGED_FACES)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		glDisable(GL_TEXTURE_2D);
		glColor3f(0.0f, 0.5f, 0.0f);

		m->particleRenderer.RenderParticles(true);

		CShaderTechniquePtr shaderTech = g_Renderer.GetShaderManager().LoadEffect("gui_solid");
		shaderTech->BeginPass();
		CShaderProgramPtr shader = shaderTech->GetShader();
		shader->Uniform("color", 0.0f, 1.0f, 0.0f, 1.0f);
		shader->Uniform("transform", m_ViewCamera.GetViewProjection());

		m->particleRenderer.RenderBounds(shader);

		shaderTech->EndPass();

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// RenderSubmissions: force rendering of any batched objects
void CRenderer::RenderSubmissions()
{
	PROFILE3("render submissions");
	
	GetScene().GetLOSTexture().InterpolateLOS();
	
	if (m_Options.m_Postproc)
		m->postprocManager.CaptureRenderOutput();

	CShaderDefines context = m->globalContext;

	ogl_WarnIfError();

	// Set the camera
	m->SetOpenGLCamera(m_ViewCamera);

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

	if (m_Caps.m_Shadows && m_Options.m_Shadows && GetRenderPath() == RP_SHADER)
	{
		RenderShadowMap(context);
	}

	{
		PROFILE3_GPU("clear buffers");
		glClearColor(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	ogl_WarnIfError();

	CBoundingBoxAligned waterScissor;
	if (m_WaterManager->m_RenderWater)
	{
		waterScissor = m->terrainRenderer.ScissorWater(m_ViewCamera.GetViewProjection());
		if (waterScissor.GetVolume() > 0 && m_WaterManager->WillRenderFancyWater())
		{
			PROFILE3_GPU("water scissor");
			SScreenRect dirty = { 0, 0, 0, 0 };
			if (m_Options.m_WaterRefraction && m_Options.m_WaterReflection)
			{
				SScreenRect reflectionScissor = RenderReflections(context, waterScissor);
				SScreenRect refractionScissor = RenderRefractions(context, waterScissor);
				dirty.x1 = std::min(reflectionScissor.x1, refractionScissor.x1);
				dirty.y1 = std::min(reflectionScissor.y1, refractionScissor.y1);
				dirty.x2 = std::max(reflectionScissor.x2, refractionScissor.x2);
				dirty.y2 = std::max(reflectionScissor.y2, refractionScissor.y2);
 			}
			else if (m_Options.m_WaterRefraction)
			{
				SScreenRect refractionScissor = RenderRefractions(context, waterScissor);
				dirty.x1 = refractionScissor.x1;
				dirty.y1 = refractionScissor.y1;
				dirty.x2 = refractionScissor.x2;
				dirty.y2 = refractionScissor.y2;
			}
            else if (m_Options.m_WaterReflection)
			{
				SScreenRect reflectionScissor = RenderReflections(context, waterScissor);
				dirty.x1 = reflectionScissor.x1;
				dirty.y1 = reflectionScissor.y1;
				dirty.x2 = reflectionScissor.x2;
				dirty.y2 = reflectionScissor.y2;
			}
			if (dirty.x1 < dirty.x2 && dirty.y1 < dirty.y2)
			{
				glEnable(GL_SCISSOR_TEST);
				glScissor(dirty.x1, dirty.y1, dirty.x2 - dirty.x1, dirty.y2 - dirty.y1);
				glClearColor(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
				glDisable(GL_SCISSOR_TEST);
			}
		}
	}

	if (m_Options.m_ShowSky)
	{
		m->skyManager.RenderSky();
	}

	// render submitted patches and models
	RenderPatches(context);
	ogl_WarnIfError();

	// render debug-related terrain overlays
	ITerrainOverlay::RenderOverlaysBeforeWater();
	ogl_WarnIfError();

	// render other debug-related overlays before water (so they can be seen when underwater)
	m->overlayRenderer.RenderOverlaysBeforeWater();
	ogl_WarnIfError();

	RenderModels(context);
	ogl_WarnIfError();

	// render water
	if (m_WaterManager->m_RenderWater && g_Game && waterScissor.GetVolume() > 0)
	{
		// render transparent stuff, but only the solid parts that can occlude block water
		RenderTransparentModels(context, TRANSPARENT_OPAQUE);
		ogl_WarnIfError();

		m->terrainRenderer.RenderWater(context, &m->shadow);
		ogl_WarnIfError();

		// render transparent stuff again, but only the blended parts that overlap water
		RenderTransparentModels(context, TRANSPARENT_BLEND);
		ogl_WarnIfError();
	}
	else
	{
		// render transparent stuff, so it can overlap models/terrain
		RenderTransparentModels(context, TRANSPARENT);
		ogl_WarnIfError();
	}

	// render debug-related terrain overlays
	ITerrainOverlay::RenderOverlaysAfterWater();
	ogl_WarnIfError();

	// render some other overlays after water (so they can be displayed on top of water)
	m->overlayRenderer.RenderOverlaysAfterWater();
	ogl_WarnIfError();

	// particles are transparent so render after water
	if (m_Options.m_Particles)
	{
		RenderParticles();
		ogl_WarnIfError();
	}
	
	if (m_Options.m_Postproc)
	{
		m->postprocManager.ApplyPostproc();
		m->postprocManager.ReleaseRenderOutput();
	}

	if (m_Options.m_Silhouettes)
	{
		RenderSilhouettes(context);
	}

#if !CONFIG2_GLES
	// Clean up texture blend mode so particles and other things render OK 
	// (really this should be cleaned up by whoever set it)
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#endif

	// render debug lines
	if (m_DisplayFrustum)
	{
		DisplayFrustum();
		m->shadow.RenderDebugBounds();
		m->shadow.RenderDebugTexture();
		ogl_WarnIfError();
	}

	// render overlays that should appear on top of all other objects
	m->overlayRenderer.RenderForegroundOverlays(m_ViewCamera);
	ogl_WarnIfError();
	
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EndFrame: signal frame end
void CRenderer::EndFrame()
{
	PROFILE3("end frame");

	// empty lists
	m->terrainRenderer.EndFrame();
	m->overlayRenderer.EndFrame();
	m->particleRenderer.EndFrame();

	// Finish model renderers
	m->Model.NormalSkinned->EndFrame();
	m->Model.TranspSkinned->EndFrame();
	if (m->Model.NormalUnskinned != m->Model.NormalSkinned)
		m->Model.NormalUnskinned->EndFrame();
	if (m->Model.TranspUnskinned != m->Model.TranspSkinned)
		m->Model.TranspUnskinned->EndFrame();

	ogl_tex_bind(0, 0);

	{
		PROFILE3("error check");
		if (glGetError())
		{
			ONCE(LOGERROR(L"CRenderer::EndFrame: GL errors occurred"));
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// DisplayFrustum: debug displays
//  - white: cull camera frustum
//  - red: bounds of shadow casting objects
void CRenderer::DisplayFrustum()
{
#if CONFIG2_GLES
#warning TODO: implement CRenderer::DisplayFrustum for GLES
#else
	glDepthMask(0);
	glDisable(GL_CULL_FACE);
	glDisable(GL_TEXTURE_2D);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4ub(255,255,255,64);
	m_CullCamera.Render(2);
	glDisable(GL_BLEND);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glColor3ub(255,255,255);
	m_CullCamera.Render(2);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glEnable(GL_CULL_FACE);
	glDepthMask(1);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Text overlay rendering
void CRenderer::RenderTextOverlays()
{
	PROFILE3_GPU("text overlays");

	if (m_DisplayTerrainPriorities)
		m->terrainRenderer.RenderPriorities();

	ogl_WarnIfError();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// SetSceneCamera: setup projection and transform of camera and adjust viewport to current view
// The camera always represents the actual camera used to render a scene, not any virtual camera
// used for shadow rendering or reflections.
void CRenderer::SetSceneCamera(const CCamera& viewCamera, const CCamera& cullCamera)
{
	m_ViewCamera = viewCamera;
	m_CullCamera = cullCamera;

	if (m_Caps.m_Shadows && m_Options.m_Shadows && GetRenderPath() == RP_SHADER)
		m->shadow.SetupFrame(m_CullCamera, m_LightEnv->GetSunDir());
}


void CRenderer::SetViewport(const SViewPort &vp)
{
	glViewport((GLint)vp.m_X,(GLint)vp.m_Y,(GLsizei)vp.m_Width,(GLsizei)vp.m_Height);
}

void CRenderer::Submit(CPatch* patch)
{
	m->terrainRenderer.Submit(patch);
}

void CRenderer::Submit(SOverlayLine* overlay)
{
	m->overlayRenderer.Submit(overlay);
}

void CRenderer::Submit(SOverlayTexturedLine* overlay)
{
	m->overlayRenderer.Submit(overlay);
}

void CRenderer::Submit(SOverlaySprite* overlay)
{
	m->overlayRenderer.Submit(overlay);
}

void CRenderer::Submit(SOverlayQuad* overlay)
{
	m->overlayRenderer.Submit(overlay);
}

void CRenderer::Submit(CModelDecal* decal)
{
	m->terrainRenderer.Submit(decal);
}

void CRenderer::Submit(CParticleEmitter* emitter)
{
	m->particleRenderer.Submit(emitter);
}

void CRenderer::SubmitNonRecursive(CModel* model)
{
	if (model->GetFlags() & MODELFLAG_CASTSHADOWS)
	{
		m->shadow.AddShadowedBound(model->GetWorldBounds());
	}

	// Tricky: The call to GetWorldBounds() above can invalidate the position
	model->ValidatePosition();

	bool requiresSkinning = (model->GetModelDef()->GetNumBones() != 0);

	if (model->GetMaterial().UsesAlphaBlending())
	{
		if (requiresSkinning)
			m->Model.TranspSkinned->Submit(model);
		else
			m->Model.TranspUnskinned->Submit(model);
	}
	else
	{
		if (requiresSkinning)
			m->Model.NormalSkinned->Submit(model);
		else
			m->Model.NormalUnskinned->Submit(model);
	}
}


///////////////////////////////////////////////////////////
// Render the given scene
void CRenderer::RenderScene(Scene& scene)
{
	m_CurrentScene = &scene;

	CFrustum frustum = m_CullCamera.GetFrustum();

	scene.EnumerateObjects(frustum, this);

	m->particleManager.RenderSubmit(*this, frustum);

	ogl_WarnIfError();

	RenderSubmissions();

	m_CurrentScene = NULL;
}

Scene& CRenderer::GetScene()
{
	ENSURE(m_CurrentScene);
	return *m_CurrentScene;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BindTexture: bind a GL texture object to current active unit
void CRenderer::BindTexture(int unit, GLuint tex)
{
	pglActiveTextureARB(GL_TEXTURE0+unit);

	glBindTexture(GL_TEXTURE_2D, tex);
#if !CONFIG2_GLES
	if (tex) {
		glEnable(GL_TEXTURE_2D);
	} else {
		glDisable(GL_TEXTURE_2D);
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// LoadAlphaMaps: load the 14 default alpha maps, pack them into one composite texture and
// calculate the coordinate of each alphamap within this packed texture
// NB: A variant of this function is duplicated in TerrainTextureEntry.cpp, for use with the Shader
// renderpath. This copy is kept to load the 'standard' maps for the fixed pipeline and should
// be removed if/when the fixed pipeline goes.
int CRenderer::LoadAlphaMaps()
{
	const wchar_t* const key = L"(alpha map composite)";
	Handle ht = ogl_tex_find(key);
	// alpha map texture had already been created and is still in memory:
	// reuse it, do not load again.
	if(ht > 0)
	{
		m_hCompositeAlphaMap = ht;
		return 0;
	}

	//
	// load all textures and store Handle in array
	//
	Handle textures[NumAlphaMaps] = {0};
	VfsPath path(L"art/textures/terrain/alphamaps/standard");
	const wchar_t* fnames[NumAlphaMaps] = {
		L"blendcircle.png",
		L"blendlshape.png",
		L"blendedge.png",
		L"blendedgecorner.png",
		L"blendedgetwocorners.png",
		L"blendfourcorners.png",
		L"blendtwooppositecorners.png",
		L"blendlshapecorner.png",
		L"blendtwocorners.png",
		L"blendcorner.png",
		L"blendtwoedges.png",
		L"blendthreecorners.png",
		L"blendushape.png",
		L"blendbad.png"
	};
	size_t base = 0;	// texture width/height (see below)
	// for convenience, we require all alpha maps to be of the same BPP
	// (avoids another ogl_tex_get_size call, and doesn't hurt)
	size_t bpp = 0;
	for(size_t i=0;i<NumAlphaMaps;i++)
	{
		// note: these individual textures can be discarded afterwards;
		// we cache the composite.
		textures[i] = ogl_tex_load(g_VFS, path / fnames[i]);
		RETURN_STATUS_IF_ERR(textures[i]);

		// get its size and make sure they are all equal.
		// (the packing algo assumes this)
		size_t this_width = 0, this_height = 0, this_bpp = 0;	// fail-safe
		(void)ogl_tex_get_size(textures[i], &this_width, &this_height, &this_bpp);
		if(this_width != this_height)
			DEBUG_DISPLAY_ERROR(L"Alpha maps are not square");
		// .. first iteration: establish size
		if(i == 0)
		{
			base = this_width;
			bpp  = this_bpp;
		}
		// .. not first: make sure texture size matches
		else if(base != this_width || bpp != this_bpp)
			DEBUG_DISPLAY_ERROR(L"Alpha maps are not identically sized (including pixel depth)");
	}

	//
	// copy each alpha map (tile) into one buffer, arrayed horizontally.
	//
	size_t tile_w = 2+base+2;	// 2 pixel border (avoids bilinear filtering artifacts)
	size_t total_w = round_up_to_pow2(tile_w * NumAlphaMaps);
	size_t total_h = base; ENSURE(is_pow2(total_h));
	shared_ptr<u8> data;
	AllocateAligned(data, total_w*total_h, maxSectorSize);
	// for each tile on row
	for (size_t i = 0; i < NumAlphaMaps; i++)
	{
		// get src of copy
		u8* src = 0;
		(void)ogl_tex_get_data(textures[i], &src);

		size_t srcstep = bpp/8;

		// get destination of copy
		u8* dst = data.get() + (i*tile_w);

		// for each row of image
		for (size_t j = 0; j < base; j++)
		{
			// duplicate first pixel
			*dst++ = *src;
			*dst++ = *src;

			// copy a row
			for (size_t k = 0; k < base; k++)
			{
				*dst++ = *src;
				src += srcstep;
			}

			// duplicate last pixel
			*dst++ = *(src-srcstep);
			*dst++ = *(src-srcstep);

			// advance write pointer for next row
			dst += total_w-tile_w;
		}

		m_AlphaMapCoords[i].u0 = float(i*tile_w+2) / float(total_w);
		m_AlphaMapCoords[i].u1 = float((i+1)*tile_w-2) / float(total_w);
		m_AlphaMapCoords[i].v0 = 0.0f;
		m_AlphaMapCoords[i].v1 = 1.0f;
	}

	for (size_t i = 0; i < NumAlphaMaps; i++)
		(void)ogl_tex_free(textures[i]);

	// upload the composite texture
	Tex t;
	(void)tex_wrap(total_w, total_h, 8, TEX_GREY, data, 0, &t);
	
	/*VfsPath filename("blendtex.png");
	
	DynArray da;
	RETURN_STATUS_IF_ERR(tex_encode(&t, filename.Extension(), &da));

	// write to disk
	//Status ret = INFO::OK;
	{
		shared_ptr<u8> file = DummySharedPtr(da.base);
		const ssize_t bytes_written = g_VFS->CreateFile(filename, file, da.pos);
		if(bytes_written > 0)
			ENSURE(bytes_written == (ssize_t)da.pos);
		//else
		//	ret = (Status)bytes_written;
	}

	(void)da_free(&da);*/
	
	m_hCompositeAlphaMap = ogl_tex_wrap(&t, g_VFS, key);
	(void)ogl_tex_set_filter(m_hCompositeAlphaMap, GL_LINEAR);
	(void)ogl_tex_set_wrap  (m_hCompositeAlphaMap, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	int ret = ogl_tex_upload(m_hCompositeAlphaMap, GL_ALPHA, 0, 0);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// UnloadAlphaMaps: frees the resources allocates by LoadAlphaMaps
void CRenderer::UnloadAlphaMaps()
{
	ogl_tex_free(m_hCompositeAlphaMap);
	m_hCompositeAlphaMap = 0;
}



Status CRenderer::ReloadChangedFileCB(void* param, const VfsPath& path)
{
	CRenderer* renderer = static_cast<CRenderer*>(param);

	// If an alpha map changed, and we already loaded them, then reload them
	if (boost::algorithm::starts_with(path.string(), L"art/textures/terrain/alphamaps/"))
	{
		if (renderer->m_hCompositeAlphaMap)
		{
			renderer->UnloadAlphaMaps();
			renderer->LoadAlphaMaps();
		}
	}

	return INFO::OK;
}

void CRenderer::MakeShadersDirty()
{
	m->ShadersDirty = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Scripting Interface

jsval CRenderer::JSI_GetRenderPath(JSContext*)
{
	return ToJSVal(GetRenderPathName(m_Options.m_RenderPath));
}

void CRenderer::JSI_SetRenderPath(JSContext* ctx, jsval newval)
{
	CStr name;

	if (!ToPrimitive(ctx, newval, name))
		return;

	SetRenderPath(GetRenderPathByName(name));
}

jsval CRenderer::JSI_GetDepthTextureBits(JSContext*)
{
	return ToJSVal(m->shadow.GetDepthTextureBits());
}

void CRenderer::JSI_SetDepthTextureBits(JSContext* ctx, jsval newval)
{
	int depthTextureBits;

	if (!ToPrimitive(ctx, newval, depthTextureBits))
		return;

	m->shadow.SetDepthTextureBits(depthTextureBits);
}

jsval CRenderer::JSI_GetShadows(JSContext*)
{
	return ToJSVal(m_Options.m_Shadows);
}

void CRenderer::JSI_SetShadows(JSContext* ctx, jsval newval)
{
	ToPrimitive(ctx, newval, m_Options.m_Shadows);
	ReloadShaders();
}

jsval CRenderer::JSI_GetShadowAlphaFix(JSContext*)
{
	return ToJSVal(m_Options.m_ShadowAlphaFix);
}

void CRenderer::JSI_SetShadowAlphaFix(JSContext* ctx, jsval newval)
{
	if (!ToPrimitive(ctx, newval, m_Options.m_ShadowAlphaFix))
		return;

	m->shadow.RecreateTexture();
}

jsval CRenderer::JSI_GetShadowPCF(JSContext*)
{
	return ToJSVal(m_Options.m_ShadowPCF);
}

void CRenderer::JSI_SetShadowPCF(JSContext* ctx, jsval newval)
{
	ToPrimitive(ctx, newval, m_Options.m_ShadowPCF);
	ReloadShaders();
}

jsval CRenderer::JSI_GetPreferGLSL(JSContext*)
{
	return ToJSVal(m_Options.m_PreferGLSL);
}

void CRenderer::JSI_SetPreferGLSL(JSContext* ctx, jsval newval)
{
	ToPrimitive(ctx, newval, m_Options.m_PreferGLSL);
	ReloadShaders();
}

jsval CRenderer::JSI_GetSky(JSContext*)
{
	return ToJSVal(m->skyManager.GetSkySet());
}

void CRenderer::JSI_SetSky(JSContext* ctx, jsval newval)
{
	CStrW skySet;
	if (!ToPrimitive<CStrW>(ctx, newval, skySet)) return;
	m->skyManager.SetSkySet(skySet);
}

void CRenderer::ScriptingInit()
{
	AddProperty(L"renderpath", &CRenderer::JSI_GetRenderPath, &CRenderer::JSI_SetRenderPath);
	AddProperty(L"displayFrustum", &CRenderer::m_DisplayFrustum);
	AddProperty(L"shadowZBias", &CRenderer::m_ShadowZBias);
	AddProperty(L"shadowMapSize", &CRenderer::m_ShadowMapSize);
	AddProperty(L"shadows", &CRenderer::JSI_GetShadows, &CRenderer::JSI_SetShadows);
	AddProperty(L"depthTextureBits", &CRenderer::JSI_GetDepthTextureBits, &CRenderer::JSI_SetDepthTextureBits);
	AddProperty(L"shadowAlphaFix", &CRenderer::JSI_GetShadowAlphaFix, &CRenderer::JSI_SetShadowAlphaFix);
	AddProperty(L"shadowPCF", &CRenderer::JSI_GetShadowPCF, &CRenderer::JSI_SetShadowPCF);
	AddProperty(L"preferGLSL", &CRenderer::JSI_GetPreferGLSL, &CRenderer::JSI_SetPreferGLSL);
	AddProperty(L"skipSubmit", &CRenderer::m_SkipSubmit);
	AddProperty(L"skySet", &CRenderer::JSI_GetSky, &CRenderer::JSI_SetSky);

	CJSObject<CRenderer>::ScriptingInit("Renderer");
}


CTextureManager& CRenderer::GetTextureManager()
{
	return m->textureManager;
}

CShaderManager& CRenderer::GetShaderManager()
{
	return m->shaderManager;
}

CParticleManager& CRenderer::GetParticleManager()
{
	return m->particleManager;
}

TerrainRenderer& CRenderer::GetTerrainRenderer()
{
	return m->terrainRenderer;
}

CTimeManager& CRenderer::GetTimeManager()
{
	return m->timeManager;
}

CMaterialManager& CRenderer::GetMaterialManager()
{
	return m->materialManager;
}

CPostprocManager& CRenderer::GetPostprocManager()
{
	return m->postprocManager;
}