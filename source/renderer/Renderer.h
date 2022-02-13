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

#ifndef INCLUDED_RENDERER
#define INCLUDED_RENDERER

#include "graphics/Camera.h"
#include "graphics/ShaderDefines.h"
#include "graphics/ShaderProgramPtr.h"
#include "ps/Singleton.h"
#include "renderer/backend/gl/DeviceCommandContext.h"
#include "renderer/RenderingOptions.h"
#include "renderer/Scene.h"

#include <memory>

class CDebugRenderer;
class CFontManager;
class CPostprocManager;
class CSceneRenderer;
class CShaderManager;
class CTextureManager;
class CTimeManager;

#define g_Renderer CRenderer::GetSingleton()

/**
 * Higher level interface on top of the whole frame rendering. It does know
 * what should be rendered and via which renderer but shouldn't know how to
 * render a particular area, like UI or scene.
 */
class CRenderer : public Singleton<CRenderer>
{
public:
	// stats class - per frame counts of number of draw calls, poly counts etc
	struct Stats
	{
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

	enum class ScreenShotType
	{
		NONE,
		DEFAULT,
		BIG
	};

public:
	CRenderer();
	~CRenderer();

	// open up the renderer: performs any necessary initialisation
	bool Open(int width, int height);

	// resize renderer view
	void Resize(int width, int height);

	// return view width
	int GetWidth() const { return m_Width; }
	// return view height
	int GetHeight() const { return m_Height; }

	void RenderFrame(bool needsPresent);

	// signal frame start
	void BeginFrame();
	// signal frame end
	void EndFrame();

	// trigger a reload of shaders (when parameters they depend on have changed)
	void MakeShadersDirty();

	// set the viewport
	void SetViewport(const SViewPort &);

	// get the last viewport
	SViewPort GetViewport();

	// return stats accumulated for current frame
	Stats& GetStats() { return m_Stats; }

	CTextureManager& GetTextureManager();

	CShaderManager& GetShaderManager();

	CFontManager& GetFontManager();

	CTimeManager& GetTimeManager();

	CPostprocManager& GetPostprocManager();

	CSceneRenderer& GetSceneRenderer();

	CDebugRenderer& GetDebugRenderer();

	/**
	 * Performs a complete frame without presenting to force loading all needed
	 * resources. It's used for the first frame on a game start.
	 * TODO: It might be better to preload resources without a complete frame
	 * rendering.
	 */
	void PreloadResourcesBeforeNextFrame();

	/**
	 * Makes a screenshot on the next RenderFrame according of the given
	 * screenshot type.
	 */
	void MakeScreenShotOnNextFrame(ScreenShotType screenShotType);

	Renderer::Backend::GL::CDeviceCommandContext* GetDeviceCommandContext();

protected:
	friend class CPatchRData;
	friend class CDecalRData;
	friend class HWLightingModelRenderer;
	friend class ShaderModelVertexRenderer;
	friend class InstancingModelRenderer;
	friend class CRenderingOptions;

	bool ShouldRender() const;

	void RenderFrameImpl(const bool renderGUI, const bool renderLogger);
	void RenderScreenShot();
	void RenderBigScreenShot(const bool needsPresent);

	// SetRenderPath: Select the preferred render path.
	// This may only be called before Open(), because the layout of vertex arrays and other
	// data may depend on the chosen render path.
	void SetRenderPath(RenderPath rp);

	void ReloadShaders();

	// Private data that is not needed by inline functions.
	class Internals;
	std::unique_ptr<Internals> m;
	// view width
	int m_Width = 0;
	// view height
	int m_Height = 0;

	SViewPort m_Viewport;

	// per-frame renderer stats
	Stats m_Stats;

	bool m_ShouldPreloadResourcesBeforeNextFrame = false;

	ScreenShotType m_ScreenShotType = ScreenShotType::NONE;
};

#endif // INCLUDED_RENDERER
