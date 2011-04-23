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

#include "precompiled.h"

#include <map>
#include <set>
#include <algorithm>

#include <boost/algorithm/string.hpp>

#include "Renderer.h"

#include "lib/bits.h"	// is_pow2
#include "lib/res/graphics/ogl_tex.h"
#include "maths/Matrix3D.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "ps/Filesystem.h"
#include "ps/World.h"
#include "ps/Loader.h"
#include "ps/ProfileViewer.h"
#include "graphics/Camera.h"
#include "graphics/GameView.h"
#include "graphics/LightEnv.h"
#include "graphics/Model.h"
#include "graphics/ModelDef.h"
#include "graphics/ParticleManager.h"
#include "graphics/ShaderManager.h"
#include "graphics/ShaderTechnique.h"
#include "graphics/Terrain.h"
#include "graphics/Texture.h"
#include "graphics/TextureManager.h"
#include "renderer/FixedFunctionModelRenderer.h"
#include "renderer/HWLightingModelRenderer.h"
#include "renderer/InstancingModelRenderer.h"
#include "renderer/ModelRenderer.h"
#include "renderer/OverlayRenderer.h"
#include "renderer/ParticleRenderer.h"
#include "renderer/PlayerRenderer.h"
#include "renderer/RenderModifiers.h"
#include "renderer/ShadowMap.h"
#include "renderer/SkyManager.h"
#include "renderer/TerrainOverlay.h"
#include "renderer/TerrainRenderer.h"
#include "renderer/TransparencyRenderer.h"
#include "renderer/VertexBufferManager.h"
#include "renderer/WaterManager.h"


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
		Row_Counter = 0,
		Row_DrawCalls,
		Row_TerrainTris,
		Row_ModelTris,
		Row_BlendSplats,
		Row_VBReserved,
		Row_VBAllocated,

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
	case Row_Counter:
		if (col == 0)
			return "counter";
		sprintf_s(buf, sizeof(buf), "%lu", (unsigned long)Stats.m_Counter);
		return buf;

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

	case Row_ModelTris:
		if (col == 0)
			return "# model tris";
		sprintf_s(buf, sizeof(buf), "%lu", (unsigned long)Stats.m_ModelTris);
		return buf;

	case Row_BlendSplats:
		if (col == 0)
			return "# blend splats";
		sprintf_s(buf, sizeof(buf), "%lu", (unsigned long)Stats.m_BlendSplats);
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
	TerrainRenderer* terrainRenderer;

	/// Overlay renderer
	OverlayRenderer overlayRenderer;

	/// Particle manager
	CParticleManager particleManager;

	/// Particle renderer
	ParticleRenderer particleRenderer;

	/// Shadow map
	ShadowMap* shadow;

	/// Various model renderers
	struct Models {
		// The following model renderers are aliases for the appropriate real_*
		// model renderers (depending on hardware availability and current settings)
		// and must be used for actual model submission and rendering
		ModelRenderer* Normal;
		ModelRenderer* NormalInstancing;
		ModelRenderer* Player;
		ModelRenderer* PlayerInstancing;
		ModelRenderer* Transp;

		// "Palette" of available ModelRenderers. Do not use these directly for
		// rendering and submission; use the aliases above instead.
		ModelRenderer* pal_NormalFF;
		ModelRenderer* pal_PlayerFF;
		ModelRenderer* pal_TranspFF;
		ModelRenderer* pal_TranspSortAll;

		ModelRenderer* pal_NormalShader;
		ModelRenderer* pal_NormalInstancingShader;
		ModelRenderer* pal_PlayerShader;
		ModelRenderer* pal_PlayerInstancingShader;
		ModelRenderer* pal_TranspShader;

		ModelVertexRendererPtr VertexFF;
		ModelVertexRendererPtr VertexPolygonSort;
		ModelVertexRendererPtr VertexRendererShader;
		ModelVertexRendererPtr VertexInstancingShader;

		// generic RenderModifiers that are supposed to be used directly
		RenderModifierPtr ModWireframe;
		RenderModifierPtr ModSolidColor;
		RenderModifierPtr ModSolidPlayerColor;
		RenderModifierPtr ModTransparentDepthShadow;

		// RenderModifiers that are selected from the palette below
		RenderModifierPtr ModNormal;
		RenderModifierPtr ModNormalInstancing;
		RenderModifierPtr ModPlayer;
		RenderModifierPtr ModPlayerInstancing;
		RenderModifierPtr ModSolid;
		RenderModifierPtr ModSolidInstancing;
		RenderModifierPtr ModSolidPlayer;
		RenderModifierPtr ModSolidPlayerInstancing;
		RenderModifierPtr ModTransparent;

		// Palette of available RenderModifiers
		RenderModifierPtr ModPlainUnlit;
		RenderModifierPtr ModPlayerUnlit;
		RenderModifierPtr ModTransparentUnlit;

		RenderModifierPtr ModShaderSolidColor;
		RenderModifierPtr ModShaderSolidColorInstancing;
		RenderModifierPtr ModShaderSolidPlayerColor;
		RenderModifierPtr ModShaderSolidPlayerColorInstancing;
		RenderModifierPtr ModShaderSolidTex;
		LitRenderModifierPtr ModShaderNormal;
		LitRenderModifierPtr ModShaderNormalInstancing;
		LitRenderModifierPtr ModShaderPlayer;
		LitRenderModifierPtr ModShaderPlayerInstancing;
		LitRenderModifierPtr ModShaderTransparent;
		RenderModifierPtr ModShaderTransparentShadow;
	} Model;


	CRendererInternals() :
		IsOpen(false), ShadersDirty(true), profileTable(g_Renderer.m_Stats), textureManager(g_VFS, false, false)
	{
		terrainRenderer = new TerrainRenderer();
		shadow = new ShadowMap();

		Model.pal_NormalFF = 0;
		Model.pal_PlayerFF = 0;
		Model.pal_TranspFF = 0;
		Model.pal_TranspSortAll = 0;

		Model.pal_NormalShader = 0;
		Model.pal_NormalInstancingShader = 0;
		Model.pal_PlayerShader = 0;
		Model.pal_PlayerInstancingShader = 0;
		Model.pal_TranspShader = 0;

		Model.Normal = 0;
		Model.NormalInstancing = 0;
		Model.Player = 0;
		Model.PlayerInstancing = 0;
		Model.Transp = 0;
	}

	~CRendererInternals()
	{
		delete shadow;
		delete terrainRenderer;
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

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(&proj._11);

		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(&view._11);

		const SViewPort &vp = camera.GetViewPort();
		glViewport((GLint)vp.m_X,(GLint)vp.m_Y,(GLsizei)vp.m_Width,(GLsizei)vp.m_Height);
	}

	/**
	 * Renders all non-transparent models with the given modifiers.
	 */
	void CallModelRenderers(
			const RenderModifierPtr& modNormal, const RenderModifierPtr& modNormalInstancing,
			const RenderModifierPtr& modPlayer, const RenderModifierPtr& modPlayerInstancing,
			int flags)
	{
		Model.Normal->Render(modNormal, flags);
		if (Model.Normal != Model.NormalInstancing)
			Model.NormalInstancing->Render(modNormalInstancing, flags);

		Model.Player->Render(modPlayer, flags);
		if (Model.Player != Model.PlayerInstancing)
			Model.PlayerInstancing->Render(modPlayerInstancing, flags);
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

	m_Width=0;
	m_Height=0;
	m_TerrainRenderMode=SOLID;
	m_ModelRenderMode=SOLID;
	m_ClearColor[0]=m_ClearColor[1]=m_ClearColor[2]=m_ClearColor[3]=0;

	m_SortAllTransparent = false;
	m_DisplayFrustum = false;
	m_DisableCopyShadow = false;
	m_DisplayTerrainPriorities = false;
	m_FastPlayerColor = true;
	m_SkipSubmit = false;

	m_Options.m_NoVBO = false;
	m_Options.m_RenderPath = RP_DEFAULT;
	m_Options.m_FancyWater = false;
	m_Options.m_Shadows = false;
	m_Options.m_ShadowAlphaFix = true;
	m_Options.m_ARBProgramShadow = true;

	m_ShadowZBias = 0.02f;
	m_ShadowMapSize = 0;

	m_LightEnv = NULL;

	m_CurrentScene = NULL;

	m_hCompositeAlphaMap = 0;

	AddLocalProperty(L"fancyWater", &m_Options.m_FancyWater, false);
	AddLocalProperty(L"horizonHeight", &m->skyManager.m_HorizonHeight, false);
	AddLocalProperty(L"waterMurkiness", &m->waterManager.m_Murkiness, false);
	AddLocalProperty(L"waterReflTintStrength", &m->waterManager.m_ReflectionTintStrength, false);
	AddLocalProperty(L"waterRepeatPeriod", &m->waterManager.m_RepeatPeriod, false);
	AddLocalProperty(L"waterShininess", &m->waterManager.m_Shininess, false);
	AddLocalProperty(L"waterSpecularStrength", &m->waterManager.m_SpecularStrength, false);
	AddLocalProperty(L"waterWaviness", &m->waterManager.m_Waviness, false);

	RegisterFileReloadFunc(ReloadChangedFileCB, this);
}

///////////////////////////////////////////////////////////////////////////////////
// CRenderer destructor
CRenderer::~CRenderer()
{
	UnregisterFileReloadFunc(ReloadChangedFileCB, this);

	// model rendering
	delete m->Model.pal_NormalFF;
	delete m->Model.pal_PlayerFF;
	delete m->Model.pal_TranspFF;
	delete m->Model.pal_TranspSortAll;

	delete m->Model.pal_NormalShader;
	delete m->Model.pal_NormalInstancingShader;
	delete m->Model.pal_PlayerShader;
	delete m->Model.pal_PlayerInstancingShader;
	delete m->Model.pal_TranspShader;

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

	if (0 == ogl_HaveExtensions(0, "GL_ARB_shadow", "GL_ARB_depth_texture", "GL_EXT_framebuffer_object", NULL))
	{
		if (ogl_max_tex_units >= 4)
			m_Caps.m_Shadows = true;
	}
}

void CRenderer::ReloadShaders()
{
	debug_assert(m->IsOpen);

	typedef std::map<CStr, CStr> Defines;

	Defines defNull;

	Defines defBasic;
	if (m_Options.m_Shadows)
	{
		defBasic["USE_SHADOW"] = "1";
		if (m_Caps.m_ARBProgramShadow && m_Options.m_ARBProgramShadow)
			defBasic["USE_FP_SHADOW"] = "1";
	}

	if (m_LightEnv)
		defBasic["LIGHTING_MODEL_" + m_LightEnv->GetLightingModel()] = "1";

	Defines defColored = defBasic;
	defColored["USE_OBJECTCOLOR"] = "1";

	Defines defTransparent = defBasic;
	defTransparent["USE_TRANSPARENT"] = "1";

	// TODO: it'd be nicer to load this technique from an XML file or something
	CShaderPass passTransparent0(m->shaderManager.LoadProgram("solid_tex", defNull));
	passTransparent0.AlphaFunc(GL_GREATER, 0.975f);
	passTransparent0.ColorMask(0, 0, 0, 0);
	CShaderPass passTransparent1(m->shaderManager.LoadProgram("model_common", defTransparent));
	passTransparent1.AlphaFunc(GL_GREATER, 0.0f);
	passTransparent1.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	passTransparent1.DepthMask(0);
	CShaderTechnique techTransparent(passTransparent0);
	techTransparent.AddPass(passTransparent1);

	CShaderPass passTransparentShadow(m->shaderManager.LoadProgram("solid_tex", defBasic));
	passTransparentShadow.AlphaFunc(GL_GREATER, 0.4f);
	CShaderTechnique techTransparentShadow(passTransparentShadow);

	m->Model.ModShaderSolidColor = RenderModifierPtr(new ShaderRenderModifier(CShaderTechnique(m->shaderManager.LoadProgram(
			"solid", defNull))));
	m->Model.ModShaderSolidColorInstancing = RenderModifierPtr(new ShaderRenderModifier(CShaderTechnique(m->shaderManager.LoadProgram(
			"solid_instancing", defNull))));

	m->Model.ModShaderSolidPlayerColor = RenderModifierPtr(new ShaderRenderModifier(CShaderTechnique(m->shaderManager.LoadProgram(
			"solid_player", defNull))));
	m->Model.ModShaderSolidPlayerColorInstancing = RenderModifierPtr(new ShaderRenderModifier(CShaderTechnique(m->shaderManager.LoadProgram(
			"solid_player_instancing", defNull))));

	m->Model.ModShaderSolidTex = RenderModifierPtr(new ShaderRenderModifier(CShaderTechnique(m->shaderManager.LoadProgram(
			"solid_tex", defNull))));

	m->Model.ModShaderNormal = LitRenderModifierPtr(new ShaderRenderModifier(CShaderTechnique(m->shaderManager.LoadProgram(
			"model_common", defBasic))));
	m->Model.ModShaderNormalInstancing = LitRenderModifierPtr(new ShaderRenderModifier(CShaderTechnique(m->shaderManager.LoadProgram(
			"model_common_instancing", defBasic))));

	m->Model.ModShaderPlayer = LitRenderModifierPtr(new ShaderRenderModifier(CShaderTechnique(m->shaderManager.LoadProgram(
			"model_common", defColored))));
	m->Model.ModShaderPlayerInstancing = LitRenderModifierPtr(new ShaderRenderModifier(CShaderTechnique(m->shaderManager.LoadProgram(
			"model_common_instancing", defColored))));

	m->Model.ModShaderTransparent = LitRenderModifierPtr(new ShaderRenderModifier(
			techTransparent));
	m->Model.ModShaderTransparentShadow = LitRenderModifierPtr(new ShaderRenderModifier(
			techTransparentShadow));

	m->ShadersDirty = false;
}

bool CRenderer::Open(int width, int height)
{
	m->IsOpen = true;

	// Must query card capabilities before creating renderers that depend
	// on card capabilities.
	EnumCaps();

	// model rendering
	m->Model.VertexFF = ModelVertexRendererPtr(new FixedFunctionModelRenderer);
	m->Model.VertexPolygonSort = ModelVertexRendererPtr(new PolygonSortModelRenderer);
	m->Model.VertexRendererShader = ModelVertexRendererPtr(new ShaderModelRenderer);
	m->Model.VertexInstancingShader = ModelVertexRendererPtr(new InstancingModelRenderer);

	m->Model.pal_NormalFF = new BatchModelRenderer(m->Model.VertexFF);
	m->Model.pal_PlayerFF = new BatchModelRenderer(m->Model.VertexFF);
	m->Model.pal_TranspFF = new SortModelRenderer(m->Model.VertexFF);

	m->Model.pal_TranspSortAll = new SortModelRenderer(m->Model.VertexPolygonSort);

	m->Model.pal_NormalShader = new BatchModelRenderer(m->Model.VertexRendererShader);
	m->Model.pal_NormalInstancingShader = new BatchModelRenderer(m->Model.VertexInstancingShader);
	m->Model.pal_PlayerShader = new BatchModelRenderer(m->Model.VertexRendererShader);
	m->Model.pal_PlayerInstancingShader = new BatchModelRenderer(m->Model.VertexInstancingShader);
	m->Model.pal_TranspShader = new SortModelRenderer(m->Model.VertexRendererShader);

	m->Model.ModWireframe = RenderModifierPtr(new WireframeRenderModifier);
	m->Model.ModPlainUnlit = RenderModifierPtr(new PlainRenderModifier);
	SetFastPlayerColor(true);
	m->Model.ModSolidColor = RenderModifierPtr(new SolidColorRenderModifier);
	m->Model.ModSolidPlayerColor = RenderModifierPtr(new SolidPlayerColorRender);
	m->Model.ModTransparentUnlit = RenderModifierPtr(new TransparentRenderModifier);
	m->Model.ModTransparentDepthShadow = RenderModifierPtr(new TransparentDepthShadowModifier);

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

	return true;
}

// resize renderer view
void CRenderer::Resize(int width,int height)
{
	// need to recreate the shadow map object to resize the shadow texture
	m->shadow->RecreateTexture();

	m_Width = width;
	m_Height = height;
}

//////////////////////////////////////////////////////////////////////////////////////////
// SetOptionBool: set boolean renderer option
void CRenderer::SetOptionBool(enum Option opt,bool value)
{
	switch (opt) {
		case OPT_NOVBO:
			m_Options.m_NoVBO=value;
			break;
		case OPT_SHADOWS:
			m_Options.m_Shadows=value;
			MakeShadersDirty();
			break;
		case OPT_FANCYWATER:
			m_Options.m_FancyWater=value;
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
		case OPT_FANCYWATER:
			return m_Options.m_FancyWater;
		default:
			debug_warn(L"CRenderer::GetOptionBool: unknown option");
			break;
	}

	return false;
}

void CRenderer::SetOptionFloat(enum Option opt, float val)
{
	switch(opt)
	{
	case OPT_LODBIAS:
		m_Options.m_LodBias = val;
		break;
	default:
		debug_warn(L"CRenderer::SetOptionFloat: unknown option");
		break;
	}
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
		if (m_Caps.m_ARBProgram)
			rp = RP_SHADER;
		else
			rp = RP_FIXED;
	}

	if (rp == RP_SHADER)
	{
		if (!m_Caps.m_ARBProgram)
		{
			LOGWARNING(L"Falling back to fixed function\n");
			rp = RP_FIXED;
		}
	}

	m_Options.m_RenderPath = rp;

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
// SetFastPlayerColor
void CRenderer::SetFastPlayerColor(bool fast)
{
	m_FastPlayerColor = fast;

	if (m_FastPlayerColor)
	{
		if (!FastPlayerColorRender::IsAvailable())
		{
			LOGWARNING(L"Falling back to slower player color rendering.");
			m_FastPlayerColor = false;
		}
	}

	if (m_FastPlayerColor)
		m->Model.ModPlayerUnlit = RenderModifierPtr(new FastPlayerColorRender);
	else
		m->Model.ModPlayerUnlit = RenderModifierPtr(new SlowPlayerColorRender);
}

//////////////////////////////////////////////////////////////////////////////////////////
// BeginFrame: signal frame start
void CRenderer::BeginFrame()
{
	PROFILE("begin frame");

	// zero out all the per-frame stats
	m_Stats.Reset();

	// choose model renderers for this frame

	if (m_Options.m_RenderPath == RP_SHADER)
	{
		if (m->ShadersDirty)
			ReloadShaders();

		m->Model.ModShaderNormal->SetShadowMap(m->shadow);
		m->Model.ModShaderNormal->SetLightEnv(m_LightEnv);

		m->Model.ModShaderNormalInstancing->SetShadowMap(m->shadow);
		m->Model.ModShaderNormalInstancing->SetLightEnv(m_LightEnv);

		m->Model.ModShaderPlayer->SetShadowMap(m->shadow);
		m->Model.ModShaderPlayer->SetLightEnv(m_LightEnv);

		m->Model.ModShaderPlayerInstancing->SetShadowMap(m->shadow);
		m->Model.ModShaderPlayerInstancing->SetLightEnv(m_LightEnv);

		m->Model.ModShaderTransparent->SetShadowMap(m->shadow);
		m->Model.ModShaderTransparent->SetLightEnv(m_LightEnv);

		m->Model.ModNormal = m->Model.ModShaderNormal;
		m->Model.ModNormalInstancing = m->Model.ModShaderNormalInstancing;
		m->Model.ModPlayer = m->Model.ModShaderPlayer;
		m->Model.ModPlayerInstancing = m->Model.ModShaderPlayerInstancing;
		m->Model.ModSolid = m->Model.ModShaderSolidColor;
		m->Model.ModSolidInstancing = m->Model.ModShaderSolidColorInstancing;
		m->Model.ModSolidPlayer = m->Model.ModShaderSolidPlayerColor;
		m->Model.ModSolidPlayerInstancing = m->Model.ModShaderSolidPlayerColorInstancing;
		m->Model.ModTransparent = m->Model.ModShaderTransparent;

		m->Model.Normal = m->Model.pal_NormalShader;
		m->Model.NormalInstancing = m->Model.pal_NormalInstancingShader;

		m->Model.Player = m->Model.pal_PlayerShader;
		m->Model.PlayerInstancing = m->Model.pal_PlayerInstancingShader;

		m->Model.Transp = m->Model.pal_TranspShader;
	}
	else
	{
		m->Model.ModNormal = m->Model.ModPlainUnlit;
		m->Model.ModNormalInstancing = m->Model.ModPlainUnlit;
		m->Model.ModPlayer = m->Model.ModPlayerUnlit;
		m->Model.ModPlayerInstancing = m->Model.ModPlayerUnlit;
		m->Model.ModTransparent = m->Model.ModTransparentUnlit;

		m->Model.NormalInstancing = m->Model.pal_NormalFF;
		m->Model.Normal = m->Model.pal_NormalFF;

		m->Model.PlayerInstancing = m->Model.pal_PlayerFF;
		m->Model.Player = m->Model.pal_PlayerFF;

		m->Model.ModSolid = m->Model.ModSolidColor;
		m->Model.ModSolidInstancing = m->Model.ModSolidColor;
		m->Model.ModSolidPlayer = m->Model.ModSolidPlayerColor;
		m->Model.ModSolidPlayerInstancing = m->Model.ModSolidPlayerColor;

		if (m_SortAllTransparent)
			m->Model.Transp = m->Model.pal_TranspSortAll;
		else
			m->Model.Transp = m->Model.pal_TranspFF;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// SetClearColor: set color used to clear screen in BeginFrame()
void CRenderer::SetClearColor(SColor4ub color)
{
	m_ClearColor[0] = float(color.R) / 255.0f;
	m_ClearColor[1] = float(color.G) / 255.0f;
	m_ClearColor[2] = float(color.B) / 255.0f;
	m_ClearColor[3] = float(color.A) / 255.0f;
}

void CRenderer::RenderShadowMap()
{
	PROFILE("render shadow map");

	m->shadow->BeginRender();

	float shadowTransp = m_LightEnv->GetTerrainShadowTransparency();
	glColor3f(shadowTransp, shadowTransp, shadowTransp);

	// Figure out transparent rendering strategy
	RenderModifierPtr transparentShadows;
	if (GetRenderPath() == RP_SHADER)
	{
		transparentShadows = m->Model.ModShaderTransparentShadow;
	}
	else
	{
		transparentShadows = m->Model.ModTransparentDepthShadow;
	}


	// Render all closed models (i.e. models where rendering back faces will produce
	// the correct result)
	glCullFace(GL_FRONT);

	{
		PROFILE("render patches");
		m->terrainRenderer->RenderPatches();
	}

	glCullFace(GL_BACK);

	// Render models that aren't closed
	glDisable(GL_CULL_FACE);

	{
		PROFILE("render models");
		m->CallModelRenderers(m->Model.ModSolid, m->Model.ModSolidInstancing,
				m->Model.ModSolid, m->Model.ModSolidInstancing, MODELFLAG_CASTSHADOWS);
	}

	{
		PROFILE("render transparent models");
		m->Model.Transp->Render(transparentShadows, MODELFLAG_CASTSHADOWS);
	}

	glEnable(GL_CULL_FACE);

	glColor3f(1.0, 1.0, 1.0);

	m->shadow->EndRender();
}

void CRenderer::RenderPatches()
{
	PROFILE("render patches");

	// switch on wireframe if we need it
	if (m_TerrainRenderMode == WIREFRAME)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	// render all the patches, including blend pass
	if (GetRenderPath() == RP_SHADER)
		m->terrainRenderer->RenderTerrainShader((m_Caps.m_Shadows && m_Options.m_Shadows) ? m->shadow : 0);
	else
		m->terrainRenderer->RenderTerrain();


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
		glDisable(GL_TEXTURE_2D);
		glColor3f(0.5f, 0.5f, 1.0f);
		glLineWidth(2.0f);

		// render tiles edges
		m->terrainRenderer->RenderPatches();

		// set color for outline
		glColor3f(0, 0, 1);
		glLineWidth(4.0f);

		// render outline of each patch
		m->terrainRenderer->RenderOutlines();

		// .. and restore the renderstates
		glLineWidth(1.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

void CRenderer::RenderModels()
{
	PROFILE("render models");

	if (m_ModelRenderMode == WIREFRAME)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	m->CallModelRenderers(m->Model.ModNormal, m->Model.ModNormalInstancing,
			m->Model.ModPlayer, m->Model.ModPlayerInstancing, 0);

	if (m_ModelRenderMode == WIREFRAME)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	else if (m_ModelRenderMode == EDGED_FACES)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_TEXTURE_2D);
		glColor3f(1.0f, 1.0f, 0.0f);

		m->CallModelRenderers(m->Model.ModSolid, m->Model.ModSolidInstancing,
				m->Model.ModSolid, m->Model.ModSolidInstancing, 0);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

void CRenderer::RenderTransparentModels()
{
	PROFILE("render transparent models");

	// switch on wireframe if we need it
	if (m_ModelRenderMode == WIREFRAME)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	m->Model.Transp->Render(m->Model.ModTransparent, 0);

	if (m_ModelRenderMode == WIREFRAME)
	{
		// switch wireframe off again
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	else if (m_ModelRenderMode == EDGED_FACES)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_TEXTURE_2D);
		glColor3f(1.0f, 0.0f, 0.0f);

		m->Model.Transp->Render(m->Model.ModSolid, 0);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// GetModelViewProjectionMatrix: save the current OpenGL model-view-projection matrix
CMatrix3D CRenderer::GetModelViewProjectionMatrix()
{
	CMatrix3D proj;
	CMatrix3D view;

	glGetFloatv( GL_PROJECTION_MATRIX, &proj._11 );
    glGetFloatv( GL_MODELVIEW_MATRIX, &view._11 );

	return( proj * view );
}



///////////////////////////////////////////////////////////////////////////////////////////////////
// SetObliqueFrustumClipping: change the near plane to the given clip plane (in world space)
// Based on code from Game Programming Gems 5, from http://www.terathon.com/code/oblique.html
// - cp is a clip plane in camera space (cp.Dot(v) = 0 for any vector v on the plane)
// - sign is 1 or -1, to specify the side to clip on
void CRenderer::SetObliqueFrustumClipping(const CVector4D& cp, int sign)
{
    float matrix[16];
    CVector4D q;

	// First, we'll convert the given clip plane to camera space, then we'll 
	// Get the view matrix and normal matrix (top 3x3 part of view matrix)
	CMatrix3D viewMatrix;
	m_ViewCamera.m_Orientation.GetInverse(viewMatrix);
	CMatrix3D normalMatrix = viewMatrix;
	normalMatrix._14 = 0;
	normalMatrix._24 = 0;
	normalMatrix._34 = 0;
	normalMatrix._44 = 1;
	normalMatrix._41 = 0;
	normalMatrix._42 = 0;
	normalMatrix._43 = 0;

	// Convert the normal to camera space
	CVector4D planeNormal(cp.m_X, cp.m_Y, cp.m_Z, 0);
	planeNormal = normalMatrix.Transform(planeNormal);
	planeNormal.Normalize();

	// Find a point on the plane: we'll take the normal times -D
	float oldD = cp.m_W;
	CVector4D pointOnPlane(-oldD * cp.m_X, -oldD * cp.m_Y, -oldD * cp.m_Z, 1);
	pointOnPlane = viewMatrix.Transform(pointOnPlane);
	float newD = -pointOnPlane.Dot(planeNormal);

	// Now create a clip plane from the new normal and new D
	CVector4D camPlane = planeNormal;
	camPlane.m_W = newD;

    // Grab the current projection matrix from OpenGL
    glGetFloatv(GL_PROJECTION_MATRIX, matrix);
    
    // Calculate the clip-space corner point opposite the clipping plane
    // as (sgn(camPlane.x), sgn(camPlane.y), 1, 1) and
    // transform it into camera space by multiplying it
    // by the inverse of the projection matrix
    
    q.m_X = (sgn(camPlane.m_X) + matrix[8]) / matrix[0];
    q.m_Y = (sgn(camPlane.m_Y) + matrix[9]) / matrix[5];
    q.m_Z = -1.0f;
    q.m_W = (1.0f + matrix[10]) / matrix[14];
    
    // Calculate the scaled plane vector
    CVector4D c = camPlane * (sign * 2.0f / camPlane.Dot(q));
    
    // Replace the third row of the projection matrix
    matrix[2] = c.m_X;
    matrix[6] = c.m_Y;
    matrix[10] = c.m_Z + 1.0f;
    matrix[14] = c.m_W;

    // Load it back into OpenGL
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(matrix);

	glMatrixMode(GL_MODELVIEW);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// RenderReflections: render the water reflections to the reflection texture
void CRenderer::RenderReflections()
{
	PROFILE("render reflections");

	WaterManager& wm = m->waterManager;

	// Remember old camera
	CCamera normalCamera = m_ViewCamera;

	// Temporarily change the camera to one that is reflected.
	// Also, for texturing purposes, make it render to a view port the size of the
	// water texture, stretch the image according to our aspect ratio so it covers
	// the whole screen despite being rendered into a square, and cover slightly more
	// of the view so we can see wavy reflections of slightly off-screen objects.
	m_ViewCamera.m_Orientation.Translate(0, -wm.m_WaterHeight, 0);
	m_ViewCamera.m_Orientation.Scale(1, -1, 1);
	m_ViewCamera.m_Orientation.Translate(0, wm.m_WaterHeight, 0);
	SViewPort vp;
	vp.m_Height = wm.m_ReflectionTextureSize;
	vp.m_Width = wm.m_ReflectionTextureSize;
	vp.m_X = 0;
	vp.m_Y = 0;
	m_ViewCamera.SetViewPort(vp);
	m_ViewCamera.SetProjection(CGameView::defaultNear, CGameView::defaultFar, CGameView::defaultFOV*1.05f); // Slightly higher than view FOV
	CMatrix3D scaleMat;
	scaleMat.SetScaling(m_Height/float(std::max(1, m_Width)), 1.0f, 1.0f);
	m_ViewCamera.m_ProjMat = scaleMat * m_ViewCamera.m_ProjMat;

	m->SetOpenGLCamera(m_ViewCamera);

	CVector4D camPlane(0, 1, 0, -wm.m_WaterHeight);
	SetObliqueFrustumClipping(camPlane, -1);

	// Save the model-view-projection matrix so the shaders can use it for projective texturing
	wm.m_ReflectionMatrix = GetModelViewProjectionMatrix();

	// Disable backface culling so trees render properly (it might also be possible to flip
	// the culling direction here, but this seems to lead to problems)
	glDisable(GL_CULL_FACE);

	// Make the depth buffer work backwards; there seems to be some oddness with 
	// oblique frustum clipping and the "sign" parameter here
	glClearDepth(0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glDepthFunc(GL_GEQUAL);

	// Render sky, terrain and models
	m->skyManager.RenderSky();
	ogl_WarnIfError();
	RenderPatches();
	ogl_WarnIfError();
	RenderModels();
	ogl_WarnIfError();
	RenderTransparentModels();
	ogl_WarnIfError();

	// Copy the image to a texture
	pglActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, wm.m_ReflectionTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
		(GLsizei)wm.m_ReflectionTextureSize, (GLsizei)wm.m_ReflectionTextureSize);

	//Reset old camera and re-enable backface culling
	m_ViewCamera = normalCamera;
	m->SetOpenGLCamera(m_ViewCamera);

	glEnable(GL_CULL_FACE);
	//glClearDepth(1);
	//glClear(GL_DEPTH_BUFFER_BIT);
	//glDepthFunc(GL_LEQUAL);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// RenderRefractions: render the water refractions to the refraction texture
void CRenderer::RenderRefractions()
{
	PROFILE("render refractions");

	WaterManager& wm = m->waterManager;

	// Remember old camera
	CCamera normalCamera = m_ViewCamera;

	// Temporarily change the camera to make it render to a view port the size of the
	// water texture, stretch the image according to our aspect ratio so it covers
	// the whole screen despite being rendered into a square, and cover slightly more
	// of the view so we can see wavy refractions of slightly off-screen objects.
	SViewPort vp;
	vp.m_Height = wm.m_RefractionTextureSize;
	vp.m_Width = wm.m_RefractionTextureSize;
	vp.m_X = 0;
	vp.m_Y = 0;
	m_ViewCamera.SetViewPort(vp);
	m_ViewCamera.SetProjection(CGameView::defaultNear, CGameView::defaultFar, CGameView::defaultFOV*1.05f); // Slightly higher than view FOV
	CMatrix3D scaleMat;
	scaleMat.SetScaling(m_Height/float(std::max(1, m_Width)), 1.0f, 1.0f);
	m_ViewCamera.m_ProjMat = scaleMat * m_ViewCamera.m_ProjMat;
	m->SetOpenGLCamera(m_ViewCamera);

	CVector4D camPlane(0, 1, 0, -wm.m_WaterHeight);
	SetObliqueFrustumClipping(camPlane, -1);

	// Save the model-view-projection matrix so the shaders can use it for projective texturing
	wm.m_RefractionMatrix = GetModelViewProjectionMatrix();

	// Make the depth buffer work backwards; there seems to be some oddness with 
	// oblique frustum clipping and the "sign" parameter here
	glClearDepth(0);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);	// a neutral gray to blend in with shores
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glDepthFunc(GL_GEQUAL);

	// Render terrain and models
	RenderPatches();
	ogl_WarnIfError();
	RenderModels();
	ogl_WarnIfError();
	RenderTransparentModels();
	ogl_WarnIfError();

	// Copy the image to a texture
	pglActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, wm.m_RefractionTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
		(GLsizei)wm.m_RefractionTextureSize, (GLsizei)wm.m_RefractionTextureSize);

	//Reset old camera and re-enable backface culling
	m_ViewCamera = normalCamera;
	m->SetOpenGLCamera(m_ViewCamera);

	glEnable(GL_CULL_FACE);
	glClearDepth(1);
	glDepthFunc(GL_LEQUAL);
}


void CRenderer::RenderSilhouettes()
{
	PROFILE("render silhouettes");

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
		m->terrainRenderer->RenderPatches();
		glCullFace(GL_BACK);
	}

	{
		PROFILE("render model occluders");
		m->CallModelRenderers(m->Model.ModSolid, m->Model.ModSolidInstancing,
				m->Model.ModSolid, m->Model.ModSolidInstancing, MODELFLAG_SILHOUETTE_OCCLUDER);
	}

	{
		PROFILE("render transparent occluders");
		if (GetRenderPath() == RP_SHADER)
		{
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, 0.4f);
			m->Model.Transp->Render(m->Model.ModShaderSolidTex, MODELFLAG_SILHOUETTE_OCCLUDER);
			glDisable(GL_ALPHA_TEST);
		}
		else
		{
			// Reuse the depth shadow modifier to get alpha-tested rendering
			m->Model.Transp->Render(m->Model.ModTransparentDepthShadow, MODELFLAG_SILHOUETTE_OCCLUDER);
		}
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
		m->CallModelRenderers(m->Model.ModSolidPlayer, m->Model.ModSolidPlayerInstancing,
				m->Model.ModSolidPlayer, m->Model.ModSolidPlayerInstancing, MODELFLAG_SILHOUETTE_DISPLAY);
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

	PROFILE("render particles");

	m->particleRenderer.RenderParticles();

	if (m_ModelRenderMode == EDGED_FACES)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		glDisable(GL_TEXTURE_2D);
		glColor3f(0.0f, 0.5f, 0.0f);

		m->particleRenderer.RenderParticles(true);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// RenderSubmissions: force rendering of any batched objects
void CRenderer::RenderSubmissions()
{
	PROFILE("render submissions");

	ogl_WarnIfError();

	// Set the camera
	m->SetOpenGLCamera(m_ViewCamera);

	// Prepare model renderers
	PROFILE_START("prepare models");
	m->Model.Normal->PrepareModels();
	m->Model.Player->PrepareModels();
	if (m->Model.Normal != m->Model.NormalInstancing)
		m->Model.NormalInstancing->PrepareModels();
	if (m->Model.Player != m->Model.PlayerInstancing)
		m->Model.PlayerInstancing->PrepareModels();
	m->Model.Transp->PrepareModels();
	PROFILE_END("prepare models");

	PROFILE_START("prepare terrain");
	m->terrainRenderer->PrepareForRendering();
	PROFILE_END("prepare terrain");

	PROFILE_START("prepare overlays");
	m->overlayRenderer.PrepareForRendering();
	PROFILE_END("prepare overlays");

	PROFILE_START("prepare particles");
	m->particleRenderer.PrepareForRendering();
	PROFILE_END("prepare particles");

	if (m_Caps.m_Shadows && m_Options.m_Shadows && GetRenderPath() == RP_SHADER)
	{
		RenderShadowMap();
	}

	// clear buffers
	PROFILE_START("clear buffers");
	glClearColor(m_ClearColor[0],m_ClearColor[1],m_ClearColor[2],m_ClearColor[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	PROFILE_END("clear buffers");

	ogl_WarnIfError();

	if (m_WaterManager->m_RenderWater && m_WaterManager->WillRenderFancyWater())
	{
		// render reflected and refracted scenes, then re-clear the screen
		RenderReflections();
		RenderRefractions();
		glClearColor(m_ClearColor[0],m_ClearColor[1],m_ClearColor[2],m_ClearColor[3]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	// render submitted patches and models
	RenderPatches();
	ogl_WarnIfError();

	if (g_Game)
	{
//		g_Game->GetWorld()->GetTerritoryManager()->RenderTerritories(); // TODO: implement in new sim system
		ogl_WarnIfError();
	}

	// render debug-related terrain overlays
	TerrainOverlay::RenderOverlays();
	ogl_WarnIfError();

	// render other debug-related overlays before water (so they can be displayed when underwater)
	PROFILE_START("render overlays");
	m->overlayRenderer.RenderOverlays();
	PROFILE_END("render overlays");
	ogl_WarnIfError();

	RenderModels();
	ogl_WarnIfError();

	// render transparent stuff, so it can overlap models/terrain
	RenderTransparentModels();
	ogl_WarnIfError();

	// render water
	if (m_WaterManager->m_RenderWater && g_Game)
	{
		m->terrainRenderer->RenderWater();
		ogl_WarnIfError();
		
		// render transparent stuff again, so it can overlap the water
		RenderTransparentModels();
		ogl_WarnIfError();
		
		// TODO: Maybe think of a better way to deal with transparent objects;
		// they can appear both under and above water (seaweed vs. trees), but doing
		// 2 renders causes (a) inefficiency and (b) darker over-water objects (e.g.
		// trees) than usual because the transparent bits get overwritten twice.
		// This doesn't look particularly bad, but it is noticeable if you try
		// turning the water off. On the other hand every user will have water
		// on all the time, so it might not be worth worrying about.
	}

	// particles are transparent so render after water
	RenderParticles();
	ogl_WarnIfError();

	RenderSilhouettes();

	// Clean up texture blend mode so particles and other things render OK 
	// (really this should be cleaned up by whoever set it)
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	// render debug lines
	if (m_DisplayFrustum)
	{
		DisplayFrustum();
		m->shadow->RenderDebugDisplay();
		ogl_WarnIfError();
	}

	// render overlays that should appear on top of all other objects
	PROFILE_START("render fg overlays");
	m->overlayRenderer.RenderForegroundOverlays(m_ViewCamera);
	PROFILE_END("render fg overlays");
	ogl_WarnIfError();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EndFrame: signal frame end
void CRenderer::EndFrame()
{
	PROFILE("end frame");

	// empty lists
	m->terrainRenderer->EndFrame();
	m->overlayRenderer.EndFrame();
	m->particleRenderer.EndFrame();

	// Finish model renderers
	m->Model.Normal->EndFrame();
	m->Model.Player->EndFrame();
	if (m->Model.Normal != m->Model.NormalInstancing)
		m->Model.NormalInstancing->EndFrame();
	if (m->Model.Player != m->Model.PlayerInstancing)
		m->Model.PlayerInstancing->EndFrame();
	m->Model.Transp->EndFrame();

	ogl_tex_bind(0, 0);

	if (glGetError())
	{
		ONCE(LOGERROR(L"CRenderer::EndFrame: GL errors occurred"));
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// DisplayFrustum: debug displays
//  - white: cull camera frustum
//  - red: bounds of shadow casting objects
void CRenderer::DisplayFrustum()
{
	glDepthMask(0);
	glDisable(GL_CULL_FACE);

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
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Text overlay rendering
void CRenderer::RenderTextOverlays()
{
	PROFILE("render text overlays");

	if (m_DisplayTerrainPriorities)
		m->terrainRenderer->RenderPriorities();

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
		m->shadow->SetupFrame(m_CullCamera, m_LightEnv->GetSunDir());
}


void CRenderer::SetViewport(const SViewPort &vp)
{
	glViewport((GLint)vp.m_X,(GLint)vp.m_Y,(GLsizei)vp.m_Width,(GLsizei)vp.m_Height);
}

void CRenderer::Submit(CPatch* patch)
{
	m->terrainRenderer->Submit(patch);
}

void CRenderer::Submit(SOverlayLine* overlay)
{
	m->overlayRenderer.Submit(overlay);
}

void CRenderer::Submit(SOverlaySprite* overlay)
{
	m->overlayRenderer.Submit(overlay);
}

void CRenderer::Submit(CModelDecal* decal)
{
	m->terrainRenderer->Submit(decal);
}

void CRenderer::Submit(CParticleEmitter* emitter)
{
	m->particleRenderer.Submit(emitter);
}

void CRenderer::SubmitNonRecursive(CModel* model)
{
	if (model->GetFlags() & MODELFLAG_CASTSHADOWS) {
//		PROFILE( "updating shadow bounds" );
		m->shadow->AddShadowedBound(model->GetBounds());
	}

	// Tricky: The call to GetBounds() above can invalidate the position
	model->ValidatePosition();

	bool canUseInstancing = false;

	if (model->GetModelDef()->GetNumBones() == 0)
		canUseInstancing = true;

	if (model->GetMaterial().IsPlayer())
	{
		if (canUseInstancing)
			m->Model.PlayerInstancing->Submit(model);
		else
			m->Model.Player->Submit(model);
	}
	else if (model->GetMaterial().UsesAlpha())
	{
		m->Model.Transp->Submit(model);
	}
	else
	{
		if (canUseInstancing)
			m->Model.NormalInstancing->Submit(model);
		else
			m->Model.Normal->Submit(model);
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
	debug_assert(m_CurrentScene);
	return *m_CurrentScene;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BindTexture: bind a GL texture object to current active unit
void CRenderer::BindTexture(int unit,GLuint tex)
{
	pglActiveTextureARB(GL_TEXTURE0+unit);

	glBindTexture(GL_TEXTURE_2D,tex);
	if (tex) {
		glEnable(GL_TEXTURE_2D);
	} else {
		glDisable(GL_TEXTURE_2D);
	}
}

static inline void CopyTriple(unsigned char* dst,const unsigned char* src)
{
	dst[0]=src[0];
	dst[1]=src[1];
	dst[2]=src[2];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// LoadAlphaMaps: load the 14 default alpha maps, pack them into one composite texture and
// calculate the coordinate of each alphamap within this packed texture
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
		RETURN_ERR(textures[i]);

		// get its size and make sure they are all equal.
		// (the packing algo assumes this)
		size_t this_width = 0, this_bpp = 0;	// fail-safe
		(void)ogl_tex_get_size(textures[i], &this_width, 0, &this_bpp);
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
	size_t total_h = base; debug_assert(is_pow2(total_h));
	shared_ptr<u8> data = io_Allocate(total_w*total_h*3);
	// for each tile on row
	for(size_t i=0;i<NumAlphaMaps;i++)
	{
		// get src of copy
		u8* src = 0;
		(void)ogl_tex_get_data(textures[i], &src);

		size_t srcstep=bpp/8;

		// get destination of copy
		u8* dst=data.get()+3*(i*tile_w);

		// for each row of image
		for (size_t j=0;j<base;j++) {
			// duplicate first pixel
			CopyTriple(dst,src);
			dst+=3;
			CopyTriple(dst,src);
			dst+=3;

			// copy a row
			for (size_t k=0;k<base;k++) {
				CopyTriple(dst,src);
				dst+=3;
				src+=srcstep;
			}
			// duplicate last pixel
			CopyTriple(dst,(src-srcstep));
			dst+=3;
			CopyTriple(dst,(src-srcstep));
			dst+=3;

			// advance write pointer for next row
			dst+=3*(total_w-tile_w);
		}

		m_AlphaMapCoords[i].u0=float(i*tile_w+2)/float(total_w);
		m_AlphaMapCoords[i].u1=float((i+1)*tile_w-2)/float(total_w);
		m_AlphaMapCoords[i].v0=0.0f;
		m_AlphaMapCoords[i].v1=1.0f;
	}

	for (size_t i=0;i<NumAlphaMaps;i++)
		(void)ogl_tex_free(textures[i]);

	// upload the composite texture
	Tex t;
	(void)tex_wrap(total_w, total_h, 24, 0, data, 0, &t);
	m_hCompositeAlphaMap = ogl_tex_wrap(&t, g_VFS, key);
	(void)ogl_tex_set_filter(m_hCompositeAlphaMap, GL_LINEAR);
	(void)ogl_tex_set_wrap  (m_hCompositeAlphaMap, GL_CLAMP_TO_EDGE);
	int ret = ogl_tex_upload(m_hCompositeAlphaMap, 0, 0, GL_INTENSITY);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// UnloadAlphaMaps: frees the resources allocates by LoadAlphaMaps
void CRenderer::UnloadAlphaMaps()
{
	ogl_tex_free(m_hCompositeAlphaMap);
	m_hCompositeAlphaMap = 0;
}



LibError CRenderer::ReloadChangedFileCB(void* param, const VfsPath& path)
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

jsval CRenderer::JSI_GetFastPlayerColor(JSContext*)
{
	return ToJSVal(m_FastPlayerColor);
}

void CRenderer::JSI_SetFastPlayerColor(JSContext* ctx, jsval newval)
{
	bool fast;

	if (!ToPrimitive(ctx, newval, fast))
		return;

	SetFastPlayerColor(fast);
}

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
	return ToJSVal(m->shadow->GetDepthTextureBits());
}

void CRenderer::JSI_SetDepthTextureBits(JSContext* ctx, jsval newval)
{
	int depthTextureBits;

	if (!ToPrimitive(ctx, newval, depthTextureBits))
		return;

	m->shadow->SetDepthTextureBits(depthTextureBits);
}

jsval CRenderer::JSI_GetShadows(JSContext*)
{
	return ToJSVal(m_Options.m_Shadows);
}

void CRenderer::JSI_SetShadows(JSContext* ctx, jsval newval)
{
	if (!ToPrimitive(ctx, newval, m_Options.m_Shadows))
		return;

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

	m->shadow->RecreateTexture();
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
	AddProperty(L"fastPlayerColor", &CRenderer::JSI_GetFastPlayerColor, &CRenderer::JSI_SetFastPlayerColor);
	AddProperty(L"renderpath", &CRenderer::JSI_GetRenderPath, &CRenderer::JSI_SetRenderPath);
	AddProperty(L"sortAllTransparent", &CRenderer::m_SortAllTransparent);
	AddProperty(L"displayFrustum", &CRenderer::m_DisplayFrustum);
	AddProperty(L"shadowZBias", &CRenderer::m_ShadowZBias);
	AddProperty(L"shadowMapSize", &CRenderer::m_ShadowMapSize);
	AddProperty(L"disableCopyShadow", &CRenderer::m_DisableCopyShadow);
	AddProperty(L"shadows", &CRenderer::JSI_GetShadows, &CRenderer::JSI_SetShadows);
	AddProperty(L"depthTextureBits", &CRenderer::JSI_GetDepthTextureBits, &CRenderer::JSI_SetDepthTextureBits);
	AddProperty(L"shadowAlphaFix", &CRenderer::JSI_GetShadowAlphaFix, &CRenderer::JSI_SetShadowAlphaFix);
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
