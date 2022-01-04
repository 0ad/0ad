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

/*
 * higher level interface on top of OpenGL to render basic objects:
 * terrain, models, sprites, particles etc.
 */

#include "precompiled.h"

#include "Renderer.h"

#include "lib/res/graphics/ogl_tex.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/CStrInternStatic.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "ps/Filesystem.h"
#include "ps/World.h"
#include "ps/Loader.h"
#include "ps/ProfileViewer.h"
#include "graphics/Camera.h"
#include "graphics/FontManager.h"
#include "graphics/ShaderManager.h"
#include "graphics/Terrain.h"
#include "graphics/Texture.h"
#include "graphics/TextureManager.h"
#include "ps/VideoMode.h"
#include "renderer/DebugRenderer.h"
#include "renderer/PostprocManager.h"
#include "renderer/RenderingOptions.h"
#include "renderer/RenderModifiers.h"
#include "renderer/SceneRenderer.h"
#include "renderer/TimeManager.h"
#include "renderer/VertexBufferManager.h"

#include <algorithm>

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

	enum
	{
		Row_DrawCalls = 0,
		Row_TerrainTris,
		Row_WaterTris,
		Row_ModelTris,
		Row_OverlayTris,
		Row_BlendSplats,
		Row_Particles,
		Row_VBReserved,
		Row_VBAllocated,
		Row_TextureMemory,
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
			return "VB reserved";
		sprintf_s(buf, sizeof(buf), "%lu kB", (unsigned long)g_VBMan.GetBytesReserved() / 1024);
		return buf;

	case Row_VBAllocated:
		if (col == 0)
			return "VB allocated";
		sprintf_s(buf, sizeof(buf), "%lu kB", (unsigned long)g_VBMan.GetBytesAllocated() / 1024);
		return buf;

	case Row_TextureMemory:
		if (col == 0)
			return "textures uploaded";
		sprintf_s(buf, sizeof(buf), "%lu kB", (unsigned long)g_Renderer.GetTextureManager().GetBytesUploaded() / 1024);
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
class CRenderer::Internals
{
	NONCOPYABLE(Internals);
public:
	/// true if CRenderer::Open has been called
	bool IsOpen;

	/// true if shaders need to be reloaded
	bool ShadersDirty;

	/// Table to display renderer stats in-game via profile system
	CRendererStatsTable profileTable;

	/// Shader manager
	CShaderManager shaderManager;

	/// Texture manager
	CTextureManager textureManager;

	/// Time manager
	CTimeManager timeManager;

	/// Postprocessing effect manager
	CPostprocManager postprocManager;

	CSceneRenderer sceneRenderer;

	CDebugRenderer debugRenderer;

	CFontManager fontManager;

	Internals() :
		IsOpen(false), ShadersDirty(true), profileTable(g_Renderer.m_Stats), textureManager(g_VFS, false, false)
	{
	}
};

CRenderer::CRenderer()
{
	m = std::make_unique<Internals>();

	g_ProfileViewer.AddRootTable(&m->profileTable);

	m_Width = 0;
	m_Height = 0;

	m_Stats.Reset();
}

CRenderer::~CRenderer()
{
	// We no longer UnloadWaterTextures here -
	// that is the responsibility of the module that asked for
	// them to be loaded (i.e. CGameView).
	m.reset();
}

void CRenderer::EnumCaps()
{
	// assume support for nothing
	m_Caps.m_ARBProgram = false;
	m_Caps.m_ARBProgramShadow = false;
	m_Caps.m_VertexShader = false;
	m_Caps.m_FragmentShader = false;
	m_Caps.m_Shadows = false;
	m_Caps.m_PrettyWater = false;

	// now start querying extensions
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

#if CONFIG2_GLES
	m_Caps.m_PrettyWater = true;
#else
	if (0 == ogl_HaveExtensions(0, "GL_ARB_vertex_shader", "GL_ARB_fragment_shader", "GL_EXT_framebuffer_object", NULL))
		m_Caps.m_PrettyWater = true;
#endif
}

void CRenderer::ReloadShaders()
{
	ENSURE(m->IsOpen);

	m->sceneRenderer.ReloadShaders();
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

	// Validate the currently selected render path
	SetRenderPath(g_RenderingOptions.GetRenderPath());

	if (g_RenderingOptions.GetPostProc())
		m->postprocManager.Initialize();

	m->sceneRenderer.Initialize();

	return true;
}

void CRenderer::Resize(int width, int height)
{
	m_Width = width;
	m_Height = height;

	m->postprocManager.Resize();

	m->sceneRenderer.Resize(width, height);
}

void CRenderer::SetRenderPath(RenderPath rp)
{
	if (!m->IsOpen)
	{
		// Delay until Open() is called.
		return;
	}

	// Renderer has been opened, so validate the selected renderpath
	if (rp == RenderPath::DEFAULT)
	{
		if (m_Caps.m_ARBProgram || (m_Caps.m_VertexShader && m_Caps.m_FragmentShader && g_VideoMode.GetBackend() != CVideoMode::Backend::GL_ARB))
			rp = RenderPath::SHADER;
		else
			rp = RenderPath::FIXED;
	}

	if (rp == RenderPath::SHADER)
	{
		if (!(m_Caps.m_ARBProgram || (m_Caps.m_VertexShader && m_Caps.m_FragmentShader && g_VideoMode.GetBackend() != CVideoMode::Backend::GL_ARB)))
		{
			LOGWARNING("Falling back to fixed function\n");
			rp = RenderPath::FIXED;
		}
	}

	// TODO: remove this once capabilities have been properly extracted and the above checks have been moved elsewhere.
	g_RenderingOptions.m_RenderPath = rp;

	MakeShadersDirty();

	// We might need to regenerate some render data after changing path
	if (g_Game)
		g_Game->GetWorld()->GetTerrain()->MakeDirty(RENDERDATA_UPDATE_COLOR);
}

void CRenderer::BeginFrame()
{
	PROFILE("begin frame");

	// Zero out all the per-frame stats.
	m_Stats.Reset();

	if (m->ShadersDirty)
		ReloadShaders();

	m->sceneRenderer.BeginFrame();
}

void CRenderer::EndFrame()
{
	PROFILE3("end frame");

	m->sceneRenderer.EndFrame();

	ogl_tex_bind(0, 0);
}

void CRenderer::SetViewport(const SViewPort &vp)
{
	m_Viewport = vp;
	glViewport((GLint)vp.m_X,(GLint)vp.m_Y,(GLsizei)vp.m_Width,(GLsizei)vp.m_Height);
}

SViewPort CRenderer::GetViewport()
{
	return m_Viewport;
}

void CRenderer::BindTexture(int unit, GLuint tex)
{
	glActiveTextureARB(GL_TEXTURE0+unit);

	glBindTexture(GL_TEXTURE_2D, tex);
}

void CRenderer::MakeShadersDirty()
{
	m->ShadersDirty = true;
	m->sceneRenderer.MakeShadersDirty();
}

CTextureManager& CRenderer::GetTextureManager()
{
	return m->textureManager;
}

CShaderManager& CRenderer::GetShaderManager()
{
	return m->shaderManager;
}

CTimeManager& CRenderer::GetTimeManager()
{
	return m->timeManager;
}

CPostprocManager& CRenderer::GetPostprocManager()
{
	return m->postprocManager;
}

CSceneRenderer& CRenderer::GetSceneRenderer()
{
	return m->sceneRenderer;
}

CDebugRenderer& CRenderer::GetDebugRenderer()
{
	return m->debugRenderer;
}

CFontManager& CRenderer::GetFontManager()
{
	return m->fontManager;
}
